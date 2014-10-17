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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include "ui_coresettings.h"
#include <newsflash/warnpop.h>
#include <memory>
#include "mainmodule.h"
#include "settings.h"

namespace gui
{
    // have to be namespace scope class because MOC doesnt support
    // shitty signals and slots for nested classes... 
    class coresettings  : public settings
    {
        Q_OBJECT
    public:
        coresettings();
       ~coresettings();

        virtual bool validate() const override;
    private slots:
        void on_btnBrowseLog_clicked();
        void on_btnBrowseDownloads_clicked();

    private:
        Ui::CoreSettings ui_;
    private:
        friend class coremodule;
    };

    // this is a dumping ground for a bunch of stuff that doesn't really belong
    // anywhere else such as engine configuration, feedback configuration etc.
    class coremodule : public mainmodule
    {
    public:
        coremodule();
       ~coremodule();

        virtual void loadstate(app::settings& s) override;
        virtual bool savestate(app::settings& s) override;
        virtual void first_launch() override;

        virtual gui::settings* get_settings(app::settings& s) override;
        virtual void apply_settings(settings* gui, app::settings& backend) override;
        virtual void free_settings(settings* s) override;

    private:
    };
} // gui