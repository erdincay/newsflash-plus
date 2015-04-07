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

#define LOGTAG "news"

#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "newsgroup.h"
#include "eventlog.h"
#include "debug.h"
#include "platform.h"
#include "engine.h"
#include "utility.h"
#include "format.h"
#include "types.h"

namespace app
{

NewsGroup::NewsGroup()
{
    DEBUG("NewsGroup created");

    QObject::connect(g_engine, SIGNAL(newHeadersAvailable(const QString&)),
        this, SLOT(newHeadersAvailable(const QString&)));
}

NewsGroup::~NewsGroup()
{
    DEBUG("Newsgroup deleted");
}

QVariant NewsGroup::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {

        switch ((Columns)section)
        {
            // fallthrough intentional for these cases.
            case Columns::BinaryFlag:
            case Columns::RecentFlag:
            case Columns::BrokenFlag:          
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return ""; 

            case Columns::Age:     return "Age";
            case Columns::Size:    return "Size";
            case Columns::Author:  return "Author";
            case Columns::Subject: return "Subject";
            case Columns::LAST:    break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon icoBinary(toGrayScale("icons:ico_flag_binary.png"));
        static const QIcon icoNew(toGrayScale("icons:ico_flag_new.png"));
        static const QIcon icoDownload(toGrayScale("icons:ico_flag_download.png"));
        static const QIcon icoBroken(toGrayScale("icons:ico_flag_broken.png"));
        static const QIcon icoBookmark(toGrayScale("icons:ico_flag_bookmark.png"));

        switch ((Columns)section)
        {
            case Columns::BinaryFlag:   return icoBinary;
            case Columns::RecentFlag:   return icoNew;
            case Columns::BrokenFlag:   return icoBroken;            
            case Columns::DownloadFlag: return icoDownload;
            case Columns::BookmarkFlag: return icoBookmark;
            default: break;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        switch ((Columns)section)
        {
            case Columns::BinaryFlag:   return "Sort by binary status";
            case Columns::RecentFlag:   return "Sort by new status";
            case Columns::BrokenFlag:   return "Sort by broken status";            
            case Columns::DownloadFlag: return "Sort by download status";
            case Columns::BookmarkFlag: return "Sort by bookmark status";
            case Columns::Age:          return "Sort by age";
            case Columns::Size:         return "Sort by size";
            case Columns::Author:       return "Sort by author";
            case Columns::Subject:      return "Sort by subject";
            case Columns::LAST: break;
        }
    }
    return QVariant();
}

QVariant NewsGroup::data(const QModelIndex& index, int role) const
{
    const auto row   = (std::size_t)index.row();
    const auto col   = (Columns)index.column();
    const auto& item = index_[row];    

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            // fall-through intended.            
            case Columns::BinaryFlag:
            case Columns::RecentFlag:
            case Columns::BrokenFlag:
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return {}; 

            case Columns::Age:     return toString(age { QDateTime::fromTime_t(item.pubdate) });
            case Columns::Size:    return toString(size { item.bytes });
            case Columns::Author:  return QString::fromStdString(item.author);
            case Columns::Subject: return QString::fromStdString(item.subject);
            case Columns::LAST:    Q_ASSERT(false);
        }
    }
    if (role == Qt::DecorationRole)
    {
        using f = newsflash::article::flags;
        switch (col)
        {
            case Columns::BinaryFlag:
                if (item.bits.test(f::binary)) 
                    return QIcon("icons:ico_flag_binary.png");
                else return QIcon("icons:ico_flag_text.png");
            case Columns::RecentFlag:
                break; // todo:

            case Columns::BrokenFlag:
                if (item.bits.test(f::broken))
                    return QIcon("icons:ico_flag_broken.png");

            case Columns::DownloadFlag:
                if (item.bits.test(f::downloaded))
                    return QIcon("icons:ico_flag_downloaded.png");

            case Columns::BookmarkFlag:
                if (item.bits.test(f::bookmarked))
                    return QIcon("icons:ico_flag_bookmark.png");

            case Columns::Age:
            case Columns::Size:
            case Columns::Author:
            case Columns::Subject:
                break;
            case Columns::LAST: Q_ASSERT(0);
        }
    }
    return {};
}

int NewsGroup::rowCount(const QModelIndex&) const 
{
    return (int)index_.size();
}

int NewsGroup::columnCount(const QModelIndex&) const 
{
    return (int)Columns::LAST;
}

bool NewsGroup::load(quint32 account, QString path, QString name)
{
    // data is in .vol files
    QDir dir;
    dir.setPath(path + "/" + name);
    dir.setSorting(QDir::Name | QDir::Reversed);
    dir.setNameFilters(QStringList("vol*.dat"));
    QStringList files = dir.entryList();
    if (files.isEmpty())
    {
        g_engine->retrieveHeaders(account, path, name);
        return false;
    }
    else
    {
        const auto p = joinPath(path, name);
        const auto f = joinPath(p, files[0]);
        DEBUG("Loading headers from %1", f);
        newHeadersAvailable(f);
    }

    return true;
}

void NewsGroup::newHeadersAvailable(const QString& file)
{
    DEBUG("New headers available in %1", file);

    const auto& n = narrow(file);

    if (!index_.on_load)
    {
        index_.on_load = [this] (std::size_t key, std::size_t idx) {
            return filedbs_[key].lookup(filedb::index_t{idx});
        };
    }

    auto it = std::find_if(std::begin(filedbs_), std::end(filedbs_),
        [&](const filedb& db) {
            return db.filename() == n;
        });

    std::size_t index  = 0;
    std::size_t offset = 0;

    if (it == std::end(filedbs_))
    {
        index = filedbs_.size();
        filedbs_.emplace_back();
        offsets_.emplace_back();
        it = --filedbs_.end();
    }
    else
    {
        const auto dist = std::distance(std::begin(filedbs_), it);
        index  = dist;
        offset = offsets_[dist];
    }

    DEBUG("%1 is at offset %2", file, offset);    
    DEBUG("Index has %1 articles. Loading more...", index_.size());

    filedb& db = *it;
    db.open(n);

    // load all the articles, starting at the latest offset that we know off.
    auto beg = db.begin(offset);
    auto end = db.end();
    for (; beg != end; ++beg)
    {
        const auto& article = *beg;
        index_.insert(article, index, article.index);
    }

    DEBUG("Load done. Index now has %1 articles", index_.size());

    // save the database and the current offset
    // when new data is added to the same database
    // we reload the db and then use the current latest
    // index to start loading the objects.
    offset = beg.offset();
    offsets_[index] = offset;

    DEBUG("%1 is at new offst %2", file, offset);

    QAbstractTableModel::reset();    
}

} // app
