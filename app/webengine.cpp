// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
// 
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define LOGTAG "web"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QtNetwork/QNetworkAccessManager>
#  include <QThread>
#include <newsflash/warnpop.h>
#include "webengine.h"
#include "webquery.h"
#include "debug.h"

// Notes about the implementation...
// Qt 4.5.3 QNetworkAccessManager has a problem with network down situations.
// If a request has already been succesfully made before "connection" is established and
// the network goes down during/before a subsequent request there is no way to get a 
// timeout signal from the network access manager. However on "first" request timeout works
// as expected.
//
// I tried to workaround this problem by using a timer per request but that turns out to create
// more problems (more Qt bugs). One problem is that calling QNetworkReply::abort() on timer 
// sometimes leads to an assert in Qt code. I.e. if the first request was aborted and later 
// QNetworkAccessManager timeouts the same request an assert is fired. Simply deleting (deleteLater())
// the request doesn't work either because then it still seems to be in Qt queues and on exiting the 
// application application gets stuck in pthread_cond_wait when destruction QApplication.

// site for testing.
// http://httpbin.org/

namespace app
{

WebEngine::WebEngine() : m_timeout(DefaultTimeout)
{
    DEBUG("WebEngine created");

    QObject::connect(&m_qnam, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(finished(QNetworkReply*)));
    QObject::connect(&m_timer, SIGNAL(timeout()),
        this, SLOT(heartbeat()));
}

WebEngine::~WebEngine()
{
    if (!m_live.empty())
    {
        m_timer.stop();
        m_timer.blockSignals(true);
        m_qnam.blockSignals(true);
        DEBUG("WebEngine has %1 pending queries...", m_live.size());

        for (auto& query : m_live)
            query->abort();
    }

    DEBUG("WebEngine deleted");
}

WebQuery* WebEngine::submit(WebQuery query)
{
    std::unique_ptr<WebQuery> q(new WebQuery(std::move(query)));

    WebQuery* ret = q.get();

    m_live.push_back(std::move(q));

    m_timer.setInterval(1000);
    m_timer.start();
    if (m_live.size() == 1)
        heartbeat();

    return ret;
}

void WebEngine::setTimeout(std::size_t seconds)
{
    m_timeout = seconds;
}

void WebEngine::finished(QNetworkReply* reply)
{
    DEBUG("Finished reply handler!");
    DEBUG("Current thread %1", QThread::currentThreadId());

    auto it = std::find_if(std::begin(m_live), std::end(m_live),
        [=](const std::unique_ptr<WebQuery>& q) {
            return q->isOwner(reply);
        });
    if (it != std::end(m_live))
    {
        std::unique_ptr<WebQuery> query = std::move(*it);

        m_live.erase(it);

        query->receive(*reply);    
    }
    else
    {
        auto it = std::find_if(std::begin(m_dead), std::end(m_dead),
            [=](const std::unique_ptr<WebQuery>& q) {
                return q->isOwner(reply);
            });
        ENDCHECK(m_dead, it);

        std::unique_ptr<WebQuery>& query = *it;

        query->receive(*reply);
    }
}

void WebEngine::timedout(QNetworkReply* reply)
{
    DEBUG("Timedout reply handler!");

    auto it = std::find_if(std::begin(m_dead), std::end(m_dead),
        [=](const std::unique_ptr<WebQuery>& q) {
            return q->isOwner(reply);
        });
    ENDCHECK(m_dead, it);

    std::unique_ptr<WebQuery>& query = *it;

    query->receive(*reply);
    reply->deleteLater();
}

void WebEngine::heartbeat()
{
    // first see if there's a new query to be submitted.
    auto it = std::find_if(std::begin(m_live), std::end(m_live),
        [](const std::unique_ptr<WebQuery>& query) {
            return !query->isActive() && !query->isAborted();
        });
    if (it != std::end(m_live))
    {
        auto& query = *it;
        query->submit(m_qnam);
    }

    // remove queries that were aborted before being submitted.
    auto end = std::remove_if(std::begin(m_live), std::end(m_live),
        [&](const std::unique_ptr<WebQuery>& query) {
            return query->isAborted();
        });
    m_live.erase(end, std::end(m_live));


    // important: if we manually timeout() a webquery the finished signal for the
    // query will be emitted. This is exactly what we want since basically this 
    // means that the query completed/finished but with *errors*. 
    // However we must be careful since the same callback is also used for 
    // succesful queries and in those cases it deletes the query object
    // not to mess up our iteration here!

    // tick live queries and abort the ones that have timed out.
    for (auto it = std::begin(m_live); it != std::end(m_live); )
    {
        auto& query = *it;
        // if not yet active (i.e. submitted), skip it.
        if (!query->isActive())
        {
            ++it;
            continue;
        }

        // update the tick, returns true if still within timeout treshold.
        if (query->tick(m_timeout))
        {
            ++it;
            continue;
        }

        // timeout the query.
        // see the comments above about the signal handler.
        //query->timeout();
        m_dead.push_back(std::move(*it));
        it = m_live.erase(it);
    }

    if (!m_dead.empty())
    {
        DEBUG("Deleting timedout queries");

        // doesn't f***N disconnect!
        //QObject::disconnect(&m_qnam, SIGNAL(finished(QNetworkReply*)));
        //QObject::connect(&m_qnam, SIGNAL(finished(QNetworkReply*)),
        //    this, SLOT(timedout(QNetworkReply*)));

        for (auto& carcass : m_dead)
        {
            auto& query = carcass;
            query->timeout();
        }

        // QObject::disconnect(&m_qnam, SIGNAL(finished(QNetworkReply*)));
        // QObject::connect(&m_qnam, SIGNAL(finished(QNetworkReply*)),
        //     this, SLOT(finished(QNetworkReply*)));

        m_dead.clear();
    }

    if (m_live.empty())
    {
        m_timer.stop();
        emit allFinished();
    }
}

WebEngine* g_web;

} // app
