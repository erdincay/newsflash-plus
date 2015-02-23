// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#define LOGTAG "home"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtNetwork/QNetworkRequest>
#  include <QtNetwork/QNetworkReply>
#  include <QUrl>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <newsflash/keygen/keygen.h>
#include "telephone.h"
#include "debug.h"
#include "eventlog.h"
#include "platform.h"
#include "format.h"
#include "netman.h"
#include "version.h"

namespace app
{

Telephone::Telephone()
{
    DEBUG("Current platform %1", getPlatformName());

    net_ = g_net->getSubmissionContext();
}

Telephone::~Telephone()
{}

void Telephone::callhome()
{
    DEBUG("Calling home...");
    
    const auto& platform    = getPlatformName();
    const auto& fingerprint = keygen::generate_fingerprint();

    QUrl url;
    url.setUrl("http://ensisoft.com/callhome.php");    
    url.addQueryItem("version", NEWSFLASH_VERSION);
    url.addQueryItem("platform", platform);    
    url.addQueryItem("fingerprint", fingerprint);

    g_net->submit(std::bind(&Telephone::on_finished, this, 
        std::placeholders::_1), net_, url);
}

void Telephone::on_finished(QNetworkReply& reply)
{
    const auto err = reply.error();
    if (err != QNetworkReply::NoError)
    {
        ERROR(str("Checking for new version failed _1", str(err)));
        emit completed(false, "");
        return;
    }

    QByteArray response = reply.readAll();
    QString latest  = QString::fromUtf8(response.constData(), response.size());
    QString current = NEWSFLASH_VERSION;

    // todo: error code checking coming from the site

    latest.remove(QRegExp("\\n"));
    latest.remove(QRegExp("\\r"));

    const bool newVersion = checkVersionUpdate(NEWSFLASH_VERSION, latest);
    DEBUG("Latest available version %1", latest);
    DEBUG("Have new version? %1", newVersion);

    emit completed(newVersion, latest);
}

} // app
