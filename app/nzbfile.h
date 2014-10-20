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
    class nzbthread;

    class nzbfile : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        nzbfile();
       ~nzbfile();

        std::function<void ()> on_ready;

        // begin loading the NZB contents from the given file
        // returns true if file was succesfully opened and then subsequently
        // emits ready() once the file has been parsed.
        // otherwise returns false and no signal will arrive.
        bool load(const QString& file);

        bool load(const QByteArray& array, const QString& desc);

        // clear the contents of the model
        void clear();

        // QAbstractTableModel
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;                
        virtual void sort(int column, Qt::SortOrder order) override;
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;                

    private slots:
        void parse_complete();

    private:
        std::unique_ptr<nzbthread> thread_;
        std::vector<nzbcontent> data_;
    private:
        QString file_;
        QByteArray buffer_;
    };

} // app
