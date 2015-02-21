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
#  include "ui_nzbfile.h"
#include <newsflash/warnpop.h>
#include <memory>

#include "mainwidget.h"
#include "../nzbfile.h"

namespace gui
{
    // nzbfile widget displays the contents of a single NZB file
    class NZBFile : public MainWidget
    {
        Q_OBJECT

    public:
        NZBFile();
       ~NZBFile();

        virtual void addActions(QMenu& menu) override;
        virtual void addActions(QToolBar& bar) override;
        virtual void loadState(app::Settings& s) override;
        virtual void saveState(app::Settings& s) override;

        virtual info getInformation() const override
        { 
            return {"nzb.html", false, false}; 
        }

        // open and display the contents of the given NZB file.
        void open(const QString& nzbfile);

        // open and display the contents of the given in memory buffer.
        void open(const QByteArray& nzb, const QString& desc);

    private slots:
        void on_actionOpen_triggered();
        void on_actionDownload_triggered();
        void on_actionBrowse_triggered();
        void on_actionClear_triggered();
        void on_actionDelete_triggered();
        void on_actionSettings_triggered();
        void on_tableView_customContextMenuRequested(QPoint);
        void on_chkFilenamesOnly_clicked();
        void downloadToPrevious();
    private:
        void downloadSelected(const QString& folder);

    private:
        Ui::NZB ui_;

    private:
        app::NZBFile model_;
    };

} // gui
