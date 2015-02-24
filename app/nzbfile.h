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
#  include <QAbstractTableModel>
#  include <QObject>
#  include <QString>
#include <newsflash/warnpop.h>
#include <memory>
#include <vector>
#include <functional>
#include "nzbparse.h"

namespace app
{
    class NZBThread;

    class NZBFile : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        NZBFile();
       ~NZBFile();

        std::function<void (bool success)> on_ready;

        // begin loading the NZB contents from the given file
        // returns true if file was succesfully opened and then subsequently
        // emits ready() once the file has been parsed.
        // otherwise returns false and no signal will arrive.
        bool load(const QString& file);

        // load NZB contents from a byte array. 
        bool load(const QByteArray& array, const QString& desc);

        // clear the contents of the model
        void clear();

        // kill the items specified by the index list from the data list.
        void kill(QModelIndexList& list);

        // download the items specified by the index list 
        void download(const QModelIndexList& list, quint32 account, const QString& folder, const QString& desc);


        void set_show_filenames_only(bool on_off);

        // QAbstractTableModel
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;                
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;                

    private slots:
        void parseComplete();

    private:
        std::unique_ptr<NZBThread> thread_;
        std::vector<NZBContent> data_;
    private:
        QString file_;
        QByteArray buffer_;
        bool show_filename_only_;
    };

} // app
