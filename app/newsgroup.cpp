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

#include "newsflash/warnpush.h"
#  include <QtGui/QIcon>
#  include <QtGui/QFont>
#  include <QtGui/QBrush>
#  include <QFile>
#  include <QFileInfo>
#  include <QDir>
#  include <QCoreApplication>
#include "newsflash/warnpop.h"

#include <limits>

#include "engine/nntp.h"
#include "engine/utf8.h"
#include "engine/iso_8859_15.h"
#include "stringlib/string.h"
#include "newsgroup.h"
#include "eventlog.h"
#include "debug.h"
#include "platform.h"
#include "engine.h"
#include "utility.h"
#include "format.h"
#include "types.h"
#include "fileinfo.h"
#include "filetype.h"
#include "nzbparse.h"
#include "download.h"

namespace app
{

// Starting with RFC-3977 the default encoding used should be UTF-8
// so we're going to assume that the subject lines are UTF-8 encoded.
QString toString(const str::string_view& str, bool utf8_enabled)
{
    if (utf8_enabled)
        return QString::fromUtf8(str.c_str(), str.size());
    return QString::fromLatin1(str.c_str(), str.size());
}

class NewsGroup::VolumeList : public QAbstractTableModel
{
public:
    VolumeList(std::deque<Block>& blocks,
        std::deque<catalog>& catalogs) : blocks_(blocks), catalogs_(catalogs)
    {}
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        if (orientation != Qt::Horizontal)
            return {};

        switch ((Columns)section)
        {
            case Columns::State:  return "Status";
            case Columns::Usage:  return "Used";
            case Columns::Ages:   return "Ages";
            case Columns::File:   return "File";
            case Columns::LAST:   Q_ASSERT(0); break;
        }
        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row   = index.row();
        const auto col   = (Columns)index.column();
        const auto& item = blocks_[row];

        if (role == Qt::DisplayRole)
        {
            switch (col)
            {
                case Columns::State: return toString(item.state);
                case Columns::File:  return QFileInfo(item.file).baseName();

                case Columns::Usage:
                    if (item.state == State::Loaded)
                    {
                        const auto& db = catalogs_[item.index];
                        const auto maxItems = (double)newsflash::CATALOG_SIZE;
                        const auto numItems = (double)db.size();
                        const auto used     = (numItems / maxItems) * 100.0;
                        return QString("%1%").arg(used, 0, 'f', 2);
                    }
                    break;

                case Columns::Ages:
                    if (item.state == State::Loaded)
                    {
                        const auto& db = catalogs_[item.index];
                        const auto first = QDateTime::fromTime_t(db.first_date());
                        const auto last  = QDateTime::fromTime_t(db.last_date());
                        const auto now   = QDateTime::currentDateTime();
                        return QString("%1 - %2 d").arg(last.daysTo(now)).arg(first.daysTo(now));
                    }
                    break;

                case Columns::LAST:  Q_ASSERT(0); break;
            }
        }
        else if (role == Qt::DecorationRole)
        {
            if (col == Columns::State)
            {
                if (item.state == State::Loaded)
                    return QIcon("icons:ico_bullet_green.png");
                return QIcon("icons:ico_bullet_grey.png");
            }
        }
        else if (role == Qt::FontRole)
        {
            if (item.purge) {
                QFont font;
                font.setStrikeOut(true);
                return font;
            }
        }

        return {};
    }

    virtual int rowCount(const QModelIndex& index) const override
    {
        return (int)catalogs_.size();
    }
    virtual int columnCount(const QModelIndex& index) const override
    {
        return (int)Columns::LAST;
    }

    void updateBlock(std::size_t index)
    {
        BOUNDSCHECK(catalogs_, index);
        auto first = QAbstractTableModel::index(index, 0);
        auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    void hardReset()
    {
        QAbstractTableModel::beginResetModel();
        QAbstractTableModel::reset();
        QAbstractTableModel::endResetModel();
    }

private:
    QString toString(State s) const
    {
        switch (s)
        {
            case State::Loaded:   return "Loaded";
            case State::UnLoaded: return "Available";
        }
        Q_ASSERT(0);
        return "";
    }

    enum class Columns {
        State,
        Usage,
        Ages,
        File,
        LAST
    };

private:
    std::deque<Block>& blocks_;
    std::deque<catalog>& catalogs_;
};

NewsGroup::NewsGroup() : task_(0)
{
    DEBUG("NewsGroup created");

    QObject::connect(g_engine, SIGNAL(newHeaderDataAvailable(const QString&, const QString&)),
        this, SLOT(newHeaderDataAvailable(const QString&, const QString&)));
    QObject::connect(g_engine, SIGNAL(newHeaderInfoAvailable(const QString&, quint64, quint64)),
        this, SLOT(newHeaderInfoAvailable(const QString&, quint64, quint64)));
    QObject::connect(g_engine, SIGNAL(updateCompleted(const app::HeaderInfo&)),
        this, SLOT(updateCompleted(const app::HeaderInfo&)));
    QObject::connect(g_engine, SIGNAL(actionKilled(quint32)),
        this, SLOT(actionKilled(quint32)));

    // loader callback
    index_.on_load = [this] (std::size_t key, std::size_t idx) {
        auto& c = catalogs_[key];
        return c.load(catalog::index_t{idx});
    };
    index_.on_filter = [this](const Article& article) {
        if (!show_these_filetypes_.test(article.type()))
            return false;

        const auto bits = article.bits();
        if (!show_these_fileflags_.test(FileFlag::broken) &&
            bits.test(FileFlag::broken))
            return false;
        if (!show_these_fileflags_.test(FileFlag::deleted) &&
            bits.test(FileFlag::deleted))
            return false;

        const auto date = article.pubdate();
        if (date < min_show_pubdate_ || date > max_show_pubdate_)
            return false;

        const auto size = article.bytes();
        if (size < min_show_file_size_ || size > max_show_file_size_)
            return false;

        if (string_matcher_utf8_.empty())
            return true;

        const auto& subject = article.subject();
        if (article.is_utf8_enabled() && string_matcher_case_sensitive_)
            return string_matcher_utf8_.search(subject.c_str(), subject.size());

        const auto& wide = toString(subject, article.is_utf8_enabled());
        if (string_matcher_case_sensitive_)
            return (bool)wide.contains(match_string_wide_, Qt::CaseSensitive);

        return (bool)wide.contains(match_string_wide_, Qt::CaseInsensitive);

    };
    numSelected_ = 0;
    show_these_filetypes_.set_from_value(~0);
    show_these_fileflags_.set_from_value(~0);
    min_show_file_size_ = std::numeric_limits<std::uint64_t>::min();
    max_show_file_size_ = std::numeric_limits<std::uint64_t>::max();
    min_show_pubdate_   = std::numeric_limits<std::time_t>::min();
    max_show_pubdate_   = std::numeric_limits<std::time_t>::max();
}

NewsGroup::~NewsGroup()
{
    if (task_)
    {
        g_engine->killAction(task_);
    }

    for (auto& cat : catalogs_)
    {
        cat.close();
    }

    for (const auto& b : blocks_)
    {
        if (!b.purge)
            continue;
        QFile file(b.file);
        if (!file.remove())
        {
            ERROR("Failed to purge %1", b.file);
        }
        else
        {
            INFO("Purged %1", b.file);
        }
    }

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
            case Columns::BrokenFlag:
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return "";

            case Columns::Type:    return "File";
            case Columns::Age:     return "Age";
            case Columns::Size:    return "Size";
            case Columns::Subject: return "Subject";
            case Columns::LAST:    break;
        }
    }
    else if (role == Qt::DecorationRole)
    {
        static const QIcon icoBinary(toGrayScale("icons:ico_flag_binary.png"));
        static const QIcon icoDownload(toGrayScale("icons:ico_flag_download.png"));
        static const QIcon icoBroken(toGrayScale("icons:ico_flag_broken.png"));
        static const QIcon icoBookmark(toGrayScale("icons:ico_flag_bookmark.png"));

        switch ((Columns)section)
        {
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
            case Columns::Type:         return "Sort by file type";
            case Columns::BrokenFlag:   return "Sort by broken status";
            case Columns::DownloadFlag: return "Sort by download status";
            case Columns::BookmarkFlag: return "Sort by bookmark status";
            case Columns::Age:          return "Sort by age";
            case Columns::Size:         return "Sort by size";
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
            case Columns::BrokenFlag:
            case Columns::DownloadFlag:
            case Columns::BookmarkFlag:
                return {};

            case Columns::Type:
                switch (item.type())
                {
                    case FileType::none:     return "n/a";
                    case FileType::audio:    return toString(app::FileType::Audio);
                    case FileType::video:    return toString(app::FileType::Video);
                    case FileType::image:    return toString(app::FileType::Image);
                    case FileType::text:     return toString(app::FileType::Text);
                    case FileType::archive:  return toString(app::FileType::Archive);
                    case FileType::parity:   return toString(app::FileType::Parity);
                    case FileType::document: return toString(app::FileType::Document);
                    case FileType::other:    return toString(app::FileType::Other);
                    default: Q_ASSERT(!"unknown file type");
                }
                break;

            case Columns::Age:
                if (item.pubdate() == 0)
                    return "???";
                return toString(age { QDateTime::fromTime_t(item.pubdate()) });

            case Columns::Size:
                return toString(size { item.bytes() });

            case Columns::Subject:
                return toString(item.subject(), item.is_utf8_enabled());

            case Columns::LAST: Q_ASSERT(false);
        }
    }
    else if (role == Qt::DecorationRole)
    {
        switch (col)
        {
            case Columns::BrokenFlag:
                if (item.test(FileFlag::broken))
                    return QIcon("icons:ico_flag_broken.png");
                break;

            case Columns::DownloadFlag:
                if (item.test(FileFlag::downloaded))
                    return QIcon("icons:ico_flag_download.png");
                break;

            case Columns::BookmarkFlag:
                if (item.test(FileFlag::bookmarked))
                    return QIcon("icons:ico_flag_bookmark.png");
                break;

            case Columns::Type:
                switch (item.type())
                {
                    case FileType::none:     return findFileIcon(app::FileType::Text);
                    case FileType::audio:    return findFileIcon(app::FileType::Audio);
                    case FileType::video:    return findFileIcon(app::FileType::Video);
                    case FileType::image:    return findFileIcon(app::FileType::Image);
                    case FileType::text:     return findFileIcon(app::FileType::Text);
                    case FileType::archive:  return findFileIcon(app::FileType::Archive);
                    case FileType::parity:   return findFileIcon(app::FileType::Parity);
                    case FileType::document: return findFileIcon(app::FileType::Document);
                    case FileType::other:    return findFileIcon(app::FileType::Other);
                    default: Q_ASSERT(!"unknown file type");
                }
                break;


            case Columns::Age:
            case Columns::Size:
            case Columns::Subject:
                break;
            case Columns::LAST: Q_ASSERT(0);
        }
    }
    else if (role == Qt::FontRole)
    {
        if (item.is_deleted())
        {
            QFont font;
            font.setStrikeOut(true);
            return font;
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

void NewsGroup::sort(int column, Qt::SortOrder order)
{
    DEBUG("Sorting to %1 order by column %2", order, column);

    emit layoutAboutToBeChanged();

    const auto o = (order == Qt::AscendingOrder) ?
        index::sortdir::ascending :
        index::sortdir::descending;

    DEBUG("Index has %1 articles", index_.size());

    switch ((Columns)column)
    {
        case Columns::Type:
            index_.sort(index::sorting::sort_by_type, o);
            break;

        case Columns::BrokenFlag:
            index_.sort(index::sorting::sort_by_broken, o);
            break;

        case Columns::DownloadFlag:
            index_.sort(index::sorting::sort_by_downloaded, o);
            break;

        case Columns::BookmarkFlag:
            index_.sort(index::sorting::sort_by_bookmarked, o);
            break;

        case Columns::Age:
            index_.sort(index::sorting::sort_by_date, o);
            break;

        case Columns::Subject:
            index_.sort(index::sorting::sort_by_subject, o);
            break;

        case Columns::Size:
            index_.sort(index::sorting::sort_by_size, o);
            break;

        case Columns::LAST: Q_ASSERT(0); break;
    }

    emit layoutChanged();

    DEBUG("Sorting done!");
}

bool NewsGroup::init(QString path, QString name)
{
    DEBUG("Init data for %1 from %2", name, path);
    path_ = path;
    name_ = name;

    const auto& dataPath = joinPath(path_, name_);

    // data is in .vol files with the file number indicating data ordering.
    // i.e. the bigger the index in the datafile name the newer the data.
    QDir dir;
    dir.setPath(dataPath);
    dir.setSorting(QDir::Name | QDir::Reversed);
    dir.setNameFilters(QStringList("vol*.dat"));
    QStringList files = dir.entryList();
    if (files.isEmpty())
        return false;

    for (const auto& file : files)
    {
        Block block;
        block.prevOffset = 0;
        block.prevSize   = 0;
        block.file       = joinPath(dataPath, file);
        block.state      = State::UnLoaded;
        block.index      = catalogs_.size();
        block.purge      = false;
        blocks_.push_back(block);
        catalogs_.emplace_back();
    }

    return true;
}

void NewsGroup::load(std::size_t blockIndex)
{
    DEBUG("Load block %1", blockIndex);

    BOUNDSCHECK(blocks_, blockIndex);
    auto& block = blocks_[blockIndex];
    if (block.state == State::Loaded)
        return;

    const auto& path = joinPath(path_, name_);
    const auto& file = joinPath(path, block.file);
    DEBUG("Loading headers from %1", file);

    onLoadBegin(blockIndex, blocks_.size());

    loadData(block, true);

    block.state = State::Loaded;

    onLoadComplete(blockIndex, blocks_.size());

    if (volumeList_)
        volumeList_->updateBlock(blockIndex);

    DEBUG("Loaded block %1", blockIndex);
}

void NewsGroup::load()
{
    // find first available block that isn't loaded yet.
    auto it = std::find_if(std::begin(blocks_), std::end(blocks_),
        [](const Block& block) {
            return block.state != State::Loaded;
        });
    if (it == std::end(blocks_))
        return;

    auto index = std::distance(std::begin(blocks_), it);
    load(index);
}

void NewsGroup::purge(std::size_t blockIndex)
{
    BOUNDSCHECK(blocks_, blockIndex);

    auto& block = blocks_[blockIndex];
    block.purge = !block.purge;

    if (volumeList_)
        volumeList_->updateBlock(blockIndex);
}


void NewsGroup::refresh(quint32 account, QString path, QString name)
{
    Q_ASSERT(task_ == 0);

    DEBUG("Refresh data for %1 in %2", name, path);
    path_ = path;
    name_ = name;
    task_ = g_engine->retrieveHeaders(account, path, name);
}

void NewsGroup::stop()
{
    Q_ASSERT(task_ != 0);

    g_engine->killAction(task_);

    task_ = 0;
}

void NewsGroup::killSelected(const QModelIndexList& list)
{
    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::min();
    for (const auto& i : list)
    {
        const auto row = i.row();
        minRow = std::min(row, minRow);
        maxRow = std::max(row, maxRow);

        auto article  = index_[row];
        auto deletion = article.is_deleted();
        article.set_bits(FileFlag::deleted, !deletion);
        //article.save();
    }
    if (!showDeleted())
    {
        applyFilter();
    }
    else
    {
        const auto first = QAbstractTableModel::index(minRow, 0);
        const auto last  = QAbstractTableModel::index(maxRow, (int)Columns::LAST);
        emit dataChanged(first, last);
    }
}

void NewsGroup::scanSelected(QModelIndexList& list)
{
    const auto numArticles = index_.size();

    for (std::size_t i=0; i<numArticles; ++i)
    {
        if (index_.is_selected(i))
        {
            list.append(QAbstractTableModel::index(i, 0));
            if (list.size() == (int)numSelected_)
                break;
        }
    }
}

void NewsGroup::select(const QModelIndexList& list, bool val)
{
    numSelected_ = 0;

    for (const auto& i : list)
        index_.select(i.row(), val);

    if (val)
        numSelected_ = list.size();
}

void NewsGroup::bookmark(const QModelIndexList& list)
{
    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::min();
    for (const auto& i : list)
    {
        const auto row = i.row();
        minRow = std::min(row, minRow);
        maxRow = std::max(row, maxRow);
        auto article  = index_[row];
        auto bookmark = article.is_bookmarked();
        article.set_bits(FileFlag::bookmarked, !bookmark);
    }

    const auto first = QAbstractTableModel::index(minRow, 0);
    const auto last  = QAbstractTableModel::index(maxRow, (int)Columns::LAST);
    emit dataChanged(first, last);
}

void NewsGroup::download(const QModelIndexList& list, quint32 acc, const QString& folder)
{
    std::vector<NZBContent> pack;
    std::vector<std::string> subjects;

    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::min();

    for (const auto& i : list)
    {
        const auto row = i.row();
        minRow = std::min(row, minRow);
        maxRow = std::max(row, maxRow);

        auto article = index_[row];
        NZBContent nzb;
        nzb.bytes   = article.bytes();
        nzb.subject = toString(article.subject(), article.is_utf8_enabled());
        nzb.poster  = toString(article.author(), article.is_utf8_enabled());
        nzb.groups.push_back(narrow(name_));

        subjects.push_back(article.subject_as_string());

        std::vector<std::uint64_t> segments;
        segments.push_back(article.number());

        if (article.has_parts())
        {
            const auto numSegments = article.num_parts_total();
            const auto baseSegment = article.number();
            const auto idbKey = article.idbkey();
            for (unsigned i=0; i<numSegments + 1; ++i)
            {
                const auto index = idbKey + i;

                // if the idlist is not yet open we must open it.
                // also if're doing updates to the newsgroup its possible
                // that the idlist is growing, in which case the item's idbkey
                // could be out of it's current bounds. if that is the case
                // we also need to reload the idlist. if the idb index
                // is still out of bounds then we have a bug. god bless.

                if (!idlist_.is_open() || index >= idlist_.size())
                {
                    const auto& path = joinPath(path_, name_);
                    const auto& file = joinPath(path, name_ + ".idb");
                    idlist_.open(narrow(file));
                }

                const std::int16_t segment = idlist_[index];
                if (segment == 0)
                    continue;
                const auto number = baseSegment + segment;
                segments.push_back(number);
            }
        }
        std::sort(std::begin(segments), std::end(segments));
        for (const auto seg : segments)
            nzb.segments.push_back(std::to_string(seg));

        pack.push_back(nzb);
        article.set_bits(FileFlag::downloaded, true);
        //article.save();
    }

    bool priority = false;

    QString desc;
    QString path;
    QString name;

    if (subjects.size() == 1)
    {
        const std::string& filename = nntp::find_filename(subjects[0]);
        if (filename.size() > 5)
            desc = fromLatin(filename);
        else desc = fromLatin(subjects[0]);

        priority = true;
    }
    else
    {
        name = app::suggestName(subjects);
        if (name.isEmpty())
        {
            desc = toString("%1 file(s) from %2", list.size(), name_);
        }
        else
        {
            path = cleanPath(name);
            desc = name;
        }
    }

    // want something like alt.binaries.foobar/some-file-batch-name
    path = joinPath(name_, path);

    DEBUG("Suggest batch name '%1' and folder '%2'", name, path);

    Download download;
    download.type     = findMediaType(name_);
    download.source   = MediaSource::Headers;
    download.account  = acc;
    download.basepath = folder;
    download.folder   = path;
    download.desc     = desc;
    download.priority = priority;
    g_engine->downloadNzbContents(download, std::move(pack));

    const auto first = QAbstractTableModel::index(minRow, 0);
    const auto last  = QAbstractTableModel::index(maxRow, (int)Columns::LAST);
    emit dataChanged(first, last);
}

quint64 NewsGroup::sumDataSizes(const QModelIndexList& list) const
{
    quint64 ret = 0;
    for (const auto& i : list)
    {
        const auto row = i.row();
        const auto& article = index_[row];
        ret += article.bytes();
    }
    return ret;
}

QString NewsGroup::suggestName(const QModelIndexList& list) const
{
    QString ret;

    std::vector<std::string> subjectLines;

    for (const auto& i : list)
    {
        const auto row = i.row();
        const auto& article = index_[row];
        subjectLines.push_back(article.subject_as_string());
    }
    if (subjectLines.size() == 1)
    {
        const std::string& filename = nntp::find_filename(subjectLines[0]);
        if (filename.size() > 5)
            ret = fromLatin(filename);
        else ret = fromLatin(subjectLines[0]);
    }
    else
    {
        ret = app::suggestName(subjectLines);
    }
    return ret;
}

std::size_t NewsGroup::numItems() const
{
    return index_.real_size();
}

std::size_t NewsGroup::numShown() const
{
    return index_.size();
}

std::size_t NewsGroup::numBlocksAvail() const
{
    return catalogs_.size();
}

std::size_t NewsGroup::numBlocksLoaded() const
{
    const auto num = std::count_if(std::begin(blocks_), std::end(blocks_),
        [](const Block& block) {
            return block.state == State::Loaded;
        });

    return num;
}

NewsGroup::Article NewsGroup::getArticle(std::size_t i) const
{
    return index_[i];
}

QAbstractTableModel* NewsGroup::getVolumeList()
{
    if (!volumeList_)
        volumeList_.reset(new VolumeList(blocks_, catalogs_));

    return volumeList_.get();
}

void NewsGroup::deleteData(quint32 account, QString path, QString group)
{
    const auto& dataDir = joinPath(path, group);

    DEBUG("Delete NewsGroup data %1", dataDir);

    if (!removeDirectory(dataDir))
    {
        ERROR("Failed to remove data directory %1", dataDir);
        return;
    }

    INFO("Deleted data directory %1", dataDir);
}

void NewsGroup::newHeaderDataAvailable(const QString& group, const QString& file)
{
    DEBUG("New headers available in %1", file);

    // see if this data file is of interest to us. if it isn't just ignore the signal
    if (file.indexOf(joinPath(path_, name_)) != 0)
        return;

    auto it = std::find_if(std::begin(blocks_), std::end(blocks_),
        [&](const Block& block) {
            return block.file == file;
        });
    if (it == std::end(blocks_))
    {
        // the data is organized into files that are lexiographically comparable
        // i.e. bigger numbers means newer data. If we're reciving new headers
        // in a datafile that is newer than the current we're always going
        // load it up on the ui. otherwise it's up to the user to specifically
        // request to load older data blocks.
        bool newData = true;
        if (!blocks_.empty())
            newData = file > blocks_[0].file;

        Block block;
        block.prevSize   = 0;
        block.prevOffset = 0;
        block.state      = newData ? State::Loaded : State::UnLoaded;
        block.file       = file;
        block.index      = catalogs_.size();
        block.purge      = false;
        catalogs_.emplace_back();

#ifdef NEWSFLASH_DEBUG
        ASSERT(std::is_sorted(std::begin(blocks_), std::end(blocks_),
            [&](const Block& lhs, const Block& rhs) {
                return lhs.file > rhs.file;
            }));

        for (const auto& block : blocks_)
        {
            DEBUG("Block list: %1", block.file);
        }
#endif

        // descending order, since the larger the index in the filename
        // the newer the content.
        it = std::lower_bound(std::begin(blocks_), std::end(blocks_), block,
            [&](const Block& lhs, const Block& rhs) {
                return lhs.file > rhs.file;
            });

        it = blocks_.insert(it, block);

        if (volumeList_)
            volumeList_->hardReset();
    }

    auto& block = *it;
    if (block.state != State::Loaded)
        return;

    loadData(block, false);

    if (volumeList_)
    {
        auto index = std::distance(std::begin(blocks_), it);
        volumeList_->updateBlock(index);
    }

#ifdef NEWSFLASH_DEBUG
    ASSERT(std::is_sorted(std::begin(blocks_), std::end(blocks_),
        [&](const Block& lhs, const Block& rhs) {
            return lhs.file > rhs.file;
        }));

    for (const auto& block : blocks_)
    {
        DEBUG("Block list: %1", block.file);
    }
#endif

    DEBUG("Loaded new headers from %1", file);
}

void NewsGroup::newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote)
{
    // nothing to do for now
}

void NewsGroup::updateCompleted(const app::HeaderInfo& info)
{
    if (info.groupName != name_ || info.groupPath != path_)
        return;

    task_ = 0;
}

void NewsGroup::actionKilled(quint32 action)
{
    //DEBUG("Action killed %1, my action %2", action, task_);

    if (action == task_)
    {
        DEBUG("Newsgroup update killed...");
        task_ = 0;
        onKilled();
    }
}

void NewsGroup::loadData(Block& block, bool guiLoad)
{
    // resetting the model will make it forget the current selection.
    // however using beginInsertRows and endInsertRows would need to be
    // done for each row because the insertions into the index are not
    // necessarily consecutive. This was tried and turns out that it's
    // much slower than just resetting the model.
    // however this means that we must maintain stuff like the current
    // selection manually. this is easily accomplished by having a
    // selection bit for each item in the index. When the GUI makes
    // a new selection we store the information per each selected
    // item in the selected bit. When model is reset we can then
    // scan the index for the items that were selected before.
    // and create a new Selection list for the GUI.
    // see scanSelected and select and the GUI code for modelReset

    auto& db = catalogs_[block.index];
    db.open(narrow(block.file));
    if (db.size() == block.prevSize)
        return;

    QAbstractTableModel::beginResetModel();

    DEBUG("Block %1 is at offset %2", block.file, block.prevOffset);
    DEBUG("Index has %1 articles. Loading more...", index_.size());

    std::size_t curItem  = 0;
    std::size_t numItems = db.size();

    // load all the articles, starting at the latest offset that we know off.
    auto beg = db.begin(block.prevOffset);
    auto end = db.end();
    for (; beg != end; ++beg, ++curItem)
    {
        const auto& article  = *beg;

        index_.insert(article, block.index, article.index());

        if (guiLoad)
        {
            onLoadProgress(curItem, numItems);
        }
    }

    DEBUG("Load done. Index now has %1 articles", index_.size());

    // save the database and the current offset
    // when new data is added to the same database
    // we reload the db and then use the current latest
    // index to start loading the objects.
    block.prevOffset = beg.offset();
    block.prevSize   = db.size();
    DEBUG("Block %1 is at new offset %2", block.file, block.prevOffset);

    QAbstractTableModel::reset();
    QAbstractTableModel::endResetModel();
}

} // app
