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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QDialog>
#  include "ui_dlgcommand.h"
#include <newsflash/warnpop.h>
#include "../commands.h"

namespace gui
{
    class DlgCommand : public QDialog
    {
        Q_OBJECT

    public:
        DlgCommand(QWidget* parent, app::Commands::Command& command);
       ~DlgCommand();

    private slots:
        void on_btnBrowseExec_clicked();
        void on_btnBrowseFile_clicked();
        void on_btnCancel_clicked();
        void on_btnAccept_clicked();
        void on_btnDefault_clicked();

    private:
        Ui::DlgCommand ui_;
    private:
        app::Commands::Command& command_;
    };
} // gui