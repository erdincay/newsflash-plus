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

#define LOGTAG "gui"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QToolBar>
#  include <QtGui/QMenu>
#  include <QtGui/QSplitter>
#  include <QtGui/QMouseEvent>
#include <newsflash/warnpop.h>

#include "downloads.h"
#include "../debug.h"
#include "../format.h"
#include "../eventlog.h"
#include "../settings.h"
#include "../engine.h"

namespace gui
{

downloads::downloads() : panels_y_pos_(0)
{
    ui_.setupUi(this);
    ui_.tableConns->setModel(&conns_);
    ui_.tableTasks->setModel(&tasks_);
    ui_.splitter->installEventFilter(this);

    DEBUG("Created downloads UI");
}


downloads::~downloads()
{
    DEBUG("Destroyed downloads UI");
}

void downloads::add_actions(QMenu& menu)
{}

void downloads::add_actions(QToolBar& bar)
{
    bar.addAction(ui_.actionConnect);
    bar.addAction(ui_.actionPreferSSL);
    bar.addAction(ui_.actionThrottle);
    bar.addSeparator();
    bar.addAction(ui_.actionTaskPause);
    bar.addAction(ui_.actionTaskResume);
    bar.addAction(ui_.actionTaskMoveUp);
    bar.addAction(ui_.actionTaskMoveDown);
    bar.addAction(ui_.actionTaskMoveTop);
    bar.addAction(ui_.actionTaskMoveBottom);
    bar.addSeparator();    
    bar.addAction(ui_.actionTaskDelete);
    bar.addAction(ui_.actionTaskClear);    
    bar.addSeparator();        

}

void downloads::loadstate(app::settings& s) 
{
    const auto task_list_height = s.get("downloads", "task_list_height", 0);
    if (task_list_height)
         ui_.grpConns->setFixedHeight(task_list_height);

    //const auto enable_throttle = app::g_engine->get_
    const auto prefer_ssl = app::g_engine->get_prefer_secure();
    const auto connect = app::g_engine->get_connect();
    const auto throttle = app::g_engine->get_throttle();
    const auto group_related = s.get("downloads", "group_related", false);
    const auto remove_complete = s.get("downloads", "remove_complete", false);

    ui_.actionConnect->setChecked(connect);
    ui_.actionThrottle->setChecked(throttle);
    ui_.actionPreferSSL->setChecked(prefer_ssl);
    ui_.chkGroupSimilar->setChecked(group_related);
    ui_.chkRemoveComplete->setChecked(remove_complete);
}

bool downloads::savestate(app::settings& s)
{
    s.set("downloads", "task_list_height", ui_.grpConns->height());
    return true;
}

void downloads::refresh()
{
    //DEBUG("Refresh");

    const auto remove_complete = ui_.chkRemoveComplete->isChecked();
    const auto group_similar   = ui_.chkGroupSimilar->isChecked();
    tasks_.refresh(remove_complete, group_similar);
    conns_.refresh();
}

void downloads::on_actionConnect_triggered()
{
    const auto on_off = ui_.actionConnect->isChecked();

    app::g_engine->connect(on_off);
}

void downloads::on_actionPreferSSL_triggered()
{
    const auto on_off = ui_.actionPreferSSL->isChecked();

    app::g_engine->set_prefer_secure(on_off);
}

void downloads::on_actionThrottle_triggered()
{
    const auto on_off = ui_.actionThrottle->isChecked();

    app::g_engine->set_throttle(on_off);
}

bool downloads::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        const auto* mickey = static_cast<QMouseEvent*>(event);
        const auto& point  = mapFromGlobal(mickey->globalPos());
        const auto  ypos   = point.y();
        if (panels_y_pos_)
        {
            auto change = ypos - panels_y_pos_;
            auto height = ui_.grpTasks->height();
            if (height + change < 150)
                return true;

            height = ui_.grpConns->height();
            if (height - change < 100)
                return true;

            ui_.grpConns->setFixedHeight(height - change);
        }
        panels_y_pos_ = ypos;
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        panels_y_pos_ = 0;
        return true;
    }

    return QObject::eventFilter(obj, event);
}

} // gui
