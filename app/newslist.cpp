// Copyright (c) 2014 Sami Väisänen, Ensisoft 
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

#define LOGTAG "news"

#include <newsflash/warnpush.h>
#  include <QtGui/QFont>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QStringList>
#  include <QFile>
#  include <QTextStream>
#  include <QFileInfo>
#include <newsflash/warnpop.h>
#include <algorithm>

#include "newslist.h"
#include "settings.h"
#include "eventlog.h"
#include "debug.h"
#include "format.h"
#include "engine.h"
#include "accounts.h"
#include "platform.h"

namespace app
{

const int CurrentFileVersion = 1;

NewsList::NewsList() : size_(0)
{
    DEBUG("NewsList created");

    QObject::connect(g_engine, SIGNAL(listingCompleted(quint32, const QList<app::NewsGroup>&)),
        this, SLOT(listingCompleted(quint32, const QList<app::NewsGroup>&)));
}

NewsList::~NewsList()
{
    DEBUG("NewsList deleted");
}


QVariant NewsList::headerData(int section, Qt::Orientation orietantation, int role) const 
{
    if (orietantation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch ((NewsList::Columns)section)
        {
            case Columns::subscribed: return "";
            case Columns::messages:   return "Messages";
            case Columns::name:       return "Name";
            default:
                Q_ASSERT(!"missing column case");
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon gray(toGrayScale(QPixmap("icons:ico_star.png")));

        if ((Columns)section == Columns::subscribed)
            return gray;

    }
    return QVariant();
}

QVariant NewsList::data(const QModelIndex& index, int role) const
{
    const auto col = index.column();
    const auto row = index.row();
    if (role == Qt::DisplayRole)
    {
        const auto& group = groups_[row];
        switch ((NewsList::Columns)col)
        {
            case Columns::name:
               return group.name;

            case Columns::messages:
                return format(app::volume{group.size});

            default:
                Q_ASSERT(!"missing column case");
                break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col == 0)
            return QIcon("icons:ico_news.png");
        else if ((Columns)col == Columns::subscribed)
        {
            const auto& group = groups_[row];
            if (group.flags & Flags::Subscribed)
                return QIcon("icons:ico_star.png");
        }
    }

    return QVariant();
}

void NewsList::sort(int column, Qt::SortOrder order) 
{
    if (order == Qt::AscendingOrder)
        DEBUG("Sort in Ascending Order");
    else DEBUG("Sort in Descending Order");

    const auto beg = std::begin(groups_);
    const auto end = std::begin(groups_) + size_;

#define SORT(x) \
    std::sort(beg, end, \
        [&](const group& lhs, const group& rhs) { \
            if (order == Qt::AscendingOrder) \
                return lhs.x < rhs.x; \
            return lhs.x > rhs.x;  \
        });


    emit layoutAboutToBeChanged();

    switch ((Columns)column)
    {
        case Columns::messages:   SORT(size); break;
        case Columns::subscribed: SORT(flags); break;
        case Columns::name:       SORT(name); break;
        default:
        Q_ASSERT("wut");
    }

    emit layoutChanged();
}

int NewsList::rowCount(const QModelIndex&) const 
{
    return (int)size_;
}

int NewsList::columnCount(const QModelIndex&) const
{
    return (int)Columns::last;
}


void NewsList::clear()
{
    groups_.clear();
    size_ = 0;
    reset();
}

void NewsList::loadListing(const QString& file, quint32 accountId)
{
    QFile io(file);
    if (!io.open(QIODevice::ReadOnly))
    {
        ERROR(str("Failed to read listing file _1", io));
        return;
    }

    const auto& account  = g_accounts->findAccount(accountId);
    const auto& newslist = account.subscriptions;

    QTextStream stream(&io);
    stream.setCodec("UTF-8");

    const quint32 curVersion = stream.readLine().toUInt();
    const quint32 numGroups  = stream.readLine().toUInt();

    quint32 curGroup   = 0;
    while (!stream.atEnd())
    {
        const auto& line = stream.readLine();
        const auto& toks = line.split("\t");
        group g;
        g.name  = toks[0];
        g.size  = toks[1].toULongLong();
        g.flags = 0;
        for (int i=0; i<newslist.size(); ++i)
        {
            if (newslist[i] == g.name)
                g.flags |= Flags::Subscribed;
        }

        groups_.push_back(g);
        curGroup++;

        if (!(curGroup % 100))
            emit progressUpdated(accountId, numGroups, curGroup);
    }

    size_ = groups_.size();

    reset();

    emit loadComplete(accountId);
}

void NewsList::makeListing(const QString& file, quint32 account)
{
    auto it = pending_.find(account);
    if (it != std::end(pending_))
        return;

    auto batchid = g_engine->retrieveNewsgroupListing(account);

    operation op;
    op.file    = file;
    op.account = account;
    op.batchId  = batchid;
    pending_.insert(std::make_pair(account, op));

    emit progressUpdated(account, 0, 0);
}

void NewsList::subscribe(QModelIndexList& list, quint32 accountId)
{
    int minIndex = std::numeric_limits<int>::max();
    int maxIndex = std::numeric_limits<int>::min();

    qSort(list);
    for (int i=0; i<list.size(); ++i)
    {
        const int row = list[0].row();
        if (row < minIndex)
            minIndex = row;
        if (row > maxIndex)
            maxIndex = row;

        auto& group = groups_[row];
        group.flags |= Flags::Subscribed;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::last);
    emit dataChanged(first, last);

    setAccountSubscriptions(accountId);
}

void NewsList::unsubscribe(QModelIndexList& list, quint32 accountId)
{
    int minIndex = std::numeric_limits<int>::max();
    int maxIndex = std::numeric_limits<int>::min();

    qSort(list);
    for (int i=0; i<list.size(); ++i)
    {
        const int row = list[0].row();
        if (row < minIndex)
            minIndex = row;
        if (row > maxIndex)
            maxIndex = row;

        auto& group = groups_[row];
        group.flags &= ~Flags::Subscribed;
    }

    auto first = QAbstractTableModel::index(minIndex, 0);
    auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::last);
    emit dataChanged(first, last);

    setAccountSubscriptions(accountId);    
}

void NewsList::filter(const QString& str, bool subscribed)
{
    const auto oldSize = size_;

    if (str.isEmpty())
    {
        if (subscribed)
        {
            auto end = std::stable_partition(std::begin(groups_), std::end(groups_),
                [=](const group& g) {
                    return g.flags & Flags::Subscribed;
                });
            size_ = std::distance(std::begin(groups_), end);
        }
        else
        {
            size_ = groups_.size();
        }
    }
    else
    {
        if (subscribed)
        {
            auto end = std::stable_partition(std::begin(groups_), std::end(groups_),
                [=](const group& g) {
                    if (!(g.flags & Flags::Subscribed))
                        return false;
                    return g.name.indexOf(str) != -1;
                });
            size_ = std::distance(std::begin(groups_), end);
        }
        else
        {
            auto end = std::stable_partition(std::begin(groups_), std::end(groups_),
                [=](const group& g) {
                    return g.name.indexOf(str) != -1;
                });
            size_ = std::distance(std::begin(groups_), end);
        }
    }

    if (oldSize != size_)
    {
        reset();
    }
    else
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(size_, (int)Columns::last);
        emit dataChanged(first, last);
    }
}

void NewsList::listingCompleted(quint32 acc, const QList<app::NewsGroup>& list)
{
    auto it = pending_.find(acc);
    Q_ASSERT(it != std::end(pending_));

    auto op = it->second;

    QFile io(op.file);
    if (!io.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ERROR(str("Unable to write listing file _1", io));
        return;
    }

    QTextStream stream(&io);
    stream.setCodec("UTF-8");
    stream << CurrentFileVersion << "\n";
    stream << list.size() << "\n";

    for (int i=0; i<list.size(); ++i)
    {
        const auto& group = list[i];
        stream << group.name << "\t" << group.size << "\n";
    }

    io.flush();
    io.close();

    pending_.erase(it);

    emit makeComplete(acc);
}

void NewsList::setAccountSubscriptions(quint32 accountId)
{
    QStringList list;
    for (const auto& group : groups_)
    {
        if (group.flags & Flags::Subscribed)
            list << group.name;
    }

    auto& account = g_accounts->findAccount(accountId);

    account.subscriptions = list;
}

} // app