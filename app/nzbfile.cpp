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

#define LOGTAG "nzb"

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QFile>
#  include <QBuffer>
#  include <QAbstractListModel>
#include <newsflash/warnpop.h>

#include <newsflash/engine/nntp.h>

#include "nzbfile.h"
#include "nzbthread.h"
#include "debug.h"
#include "format.h"
#include "eventlog.h"
#include "filetype.h"
#include "engine.h"

namespace app
{

nzbfile::nzbfile() : show_filename_only_(false)
{
    DEBUG("nzbfile created");
}

nzbfile::~nzbfile()
{
    if (thread_.get())
    {
        DEBUG("Waiting nzb parser thread....");
        thread_->wait();
        thread_.reset();
    }

    DEBUG("nzbfile destroyed");    
}

bool nzbfile::load(const QString& file)
{
    std::unique_ptr<QFile> io(new QFile(file));
    if (!io->open(QIODevice::ReadOnly))
    {
        ERROR(str("Failed to open file _1", *io.get()));
        return false;
    }

    if (thread_.get())
    {
        DEBUG("Waiting existing parsing thread...");
        thread_->wait();
        thread_.reset();
    }
    file_ = file;
    thread_.reset(new nzbthread(std::move(io)));

    QObject::connect(thread_.get(), SIGNAL(complete()),
        this, SLOT(parse_complete()), Qt::QueuedConnection);

    thread_->start();
    return true;
}

bool nzbfile::load(const QByteArray& bytes, const QString& desc)
{
    if (thread_.get())
    {
        DEBUG("Waiting existing parsing thread...");
        thread_->wait();
        thread_.reset();
    }
    buffer_ = bytes;
    file_   = desc;    

    std::unique_ptr<QBuffer> io(new QBuffer);
    io->setBuffer(&buffer_);

    thread_.reset(new nzbthread(std::move(io)));

    QObject::connect(thread_.get(), SIGNAL(complete()),
        this, SLOT(parse_complete()), Qt::QueuedConnection);

    thread_->start();
    return true;
}

void nzbfile::clear()
{
    data_.clear();
    QAbstractTableModel::reset();
}

void nzbfile::kill(QModelIndexList& list)
{
    qSort(list);

    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;
        beginRemoveRows(QModelIndex(), row, row);

        auto it = data_.begin();
        it += row;
        data_.erase(it);

        endRemoveRows();

        ++removed;
    }
}

void nzbfile::download(const QModelIndexList& list, quint32 account, const QString& folder, const QString& desc)
{
    std::vector<const nzbcontent*> selected;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        selected.push_back(&data_[row]);
    }

    g_engine->download_nzb_contents(account, folder, desc, selected);

}

void nzbfile::set_show_filenames_only(bool on_off)
{
    if (on_off == show_filename_only_)
        return;
    show_filename_only_ = on_off;

    auto first = QAbstractTableModel::index(0, 0);
    auto last  = QAbstractTableModel::index(data_.size(), 3);
    emit dataChanged(first, last);
}


int nzbfile::rowCount(const QModelIndex&) const
{
    return (int)data_.size();
}

int nzbfile::columnCount(const QModelIndex&) const
{
    return 3;
}

void nzbfile::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    if (column == 0)
    {
        std::sort(std::begin(data_), std::end(data_),
            [&](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.type < rhs.type;
                return lhs.type > rhs.type;
            });
    }
    else if (column == 1)
    {
        std::sort(std::begin(data_), std::end(data_),
            [=](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.bytes < rhs.bytes;
                return lhs.bytes > rhs.bytes;
            });
    }
    else if (column == 2)
    {
        std::sort(std::begin(data_), std::end(data_),
            [&](const nzbcontent& lhs, const nzbcontent& rhs) {
                if (order == Qt::AscendingOrder)
                    return lhs.subject < rhs.subject;
                return lhs.subject > rhs.subject;
            });
    }
    emit layoutChanged();
}

QVariant nzbfile::data(const QModelIndex& index, int role) const
{
    const auto& item = data_[index.row()];                

    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
            return str(item.type);
        else if (index.column() == 1)
            return str(app::size { item.bytes });
        else if (index.column() == 2)
        {
            if (show_filename_only_)
            {
                const auto& utf8 = to_utf8(item.subject);
                const auto& name = nntp::find_filename(utf8);
                if (name.size() > 5)
                    return from_utf8(name);
            }
            return item.subject;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (index.column() == 0)
            return item.icon;

    }
    return QVariant();
}

QVariant nzbfile::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (section == 0)
        return "Type";
    else if (section == 1)
        return "Size";
    else if (section == 2)
        return "File";

    return QVariant();
}

void nzbfile::parse_complete()
{
    DEBUG(str("parse_complete _1", file_));

    const auto result = thread_->result(data_);

    switch (result)
    {
        case nzberror::none:
            DEBUG("NZB parse succesful!");
            break;
        case nzberror::xml:
            ERROR(str("XML error while parsing NZB file _1", file_));;
            break;

        case nzberror::nzb:
            ERROR(str("Error in NZB file content _1", file_));
            break;

        case nzberror::io:
            ERROR(str("I/O error while parsing NZB file _1",file_));
            break;

        case nzberror::other:
            ERROR(str("Unknown error while parsing NZB file _1", file_));
            break;
    }

    for (auto& item : data_)
    {
        const auto utf8 = app::to_utf8(item.subject);
        auto name = nntp::find_filename(utf8);
        if (name.size() < 5)
            name = utf8;
        if (!name.empty())
        {
            item.type = find_filetype(app::from_utf8(name));
            item.icon = find_fileicon(item.type);
        }
    }

    reset();

    on_ready(result == nzberror::none);
}


} // app