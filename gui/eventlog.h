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

#pragma once

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include "ui_eventlog.h"
#include "newsflash/warnpop.h"
#include "mainwidget.h"

namespace app {
    struct Event;
} // app

namespace gui
{
    // eventlog GUI
    class EventLog : public MainWidget
    {
        Q_OBJECT

    public:
        EventLog();
       ~EventLog();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void activate(QWidget*);
        info getInformation() const override;

    private slots:
        void on_actionClearLog_triggered();
        void on_listLog_customContextMenuRequested(QPoint pos);
        void newEvent(const app::Event& event);

    private:
        Ui::Eventlog ui_;
    private:
        std::size_t numEvents_;
    };

} // gui


