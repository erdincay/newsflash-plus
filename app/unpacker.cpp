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

#define LOGTAG "extract"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#  include <QDir>
#  include <QFileInfo>
#  include <QRegExp>
#include <newsflash/warnpop.h>
#include <limits>
#include <algorithm>
#include "unpacker.h"
#include "debug.h"
#include "eventlog.h"
#include "archive.h"
#include "archiver.h"
#include "utility.h"
#include "platform.h"

namespace app
{

class Unpacker::UnpackList : public QAbstractTableModel
{
public:
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal)
            return {};

        if (role != Qt::DisplayRole)
            return {};

        switch ((Columns)section)
        {
            case Columns::Status: return "Status";
            case Columns::Name:   return "Name";
            case Columns::Error:  return "Message";
            case Columns::Path:   return "Location";
            case Columns::LAST:   Q_ASSERT(0); 
        }

        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        const auto& arc = list_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::Status: return toString(arc.state);
                case Columns::Error:  return arc.message;
                case Columns::Name:   return arc.file;
                case Columns::Path:   return arc.path;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }

        if (role == Qt::DecorationRole)
        {
            if ((Columns)col == Columns::Status)
                return toIcon(arc.state);
        }
        return {};
    }

    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)list_.size();
    }

    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }    

    void addUnpack(const Archive& arc)
    {
        const auto rowPos = (int)list_.size();
        beginInsertRows(QModelIndex(), rowPos, rowPos);
        list_.push_back(arc);
        endInsertRows();
    }

    void refresh(std::size_t index)
    {
        auto first = QAbstractTableModel::index(index, 0);
        auto last  = QAbstractTableModel::index(index, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    static const std::size_t NoSuchUnpack; // = std::numeric_limits<std::size_t>::max();

    std::size_t findNextPending() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Queued;
            });
        if (it == std::end(list_))
            return NoSuchUnpack;

        return std::distance(std::begin(list_), it);
    }

    std::size_t findActive() const 
    {
        auto it = std::find_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return arc.state == Archive::Status::Active;
            });
        if (it == std::end(list_))
            return NoSuchUnpack;

        return std::distance(std::begin(list_), it);
    }

    Archive& getArchive(std::size_t index) 
    { 
        BOUNDSCHECK(list_, index);
        return list_[index];
    }

    void moveItems(QModelIndexList& list, int direction, int distance)
    {
        auto minIndex = std::numeric_limits<int>::max();
        auto maxIndex = std::numeric_limits<int>::min();

        for (int i=0; i<list.size(); ++i)
        {
            const auto rowBase = list[i].row();
            for (int x=0; x<distance; ++x)
            {
                const auto row = rowBase + x * direction;

                BOUNDSCHECK(list_, row);
                BOUNDSCHECK(list_, row + direction);

                std::swap(list_[row], list_[row + direction]);
                minIndex = std::min(minIndex, row + direction);
                maxIndex = std::max(maxIndex, row + direction);
            }
            list[i] = QAbstractTableModel::index(rowBase + direction * distance, 0);
        }
        auto first = QAbstractTableModel::index(minIndex, 0);
        auto last  = QAbstractTableModel::index(maxIndex, (int)Columns::LAST);
        emit dataChanged(first, last);
    }

    void killItems(QModelIndexList& killList)
    {
        std::deque<Archive> survivors;
        std::size_t killIndex = 0;

        sortAscending(killList);

        for (std::size_t i=0; i<list_.size(); ++i)
        {
            if (killIndex < killList.size())
            {
                if (killList[killIndex].row() == (int)i)
                {
                    ++killIndex;
                    continue;
                }
            }
            survivors.push_back(list_[i]);
        }
        list_ = std::move(survivors);

        QAbstractTableModel::reset();
    }

    void killComplete()
    {
        auto it = std::remove_if(std::begin(list_), std::end(list_),
            [](const Archive& arc) {
                return (arc.state == Archive::Status::Success) ||
                       (arc.state == Archive::Status::Error) ||
                       (arc.state == Archive::Status::Failed);
            });
        list_.erase(it, std::end(list_));

        QAbstractTableModel::reset();
    }

    std::size_t numUnpacks() const 
    { return list_.size(); }

private:
    enum class Columns {
        Status, Error, Path, Name, LAST
    };
private:
    friend class RepairEngine;
    std::deque<Archive> list_;
};

const std::size_t Unpacker::UnpackList::NoSuchUnpack = std::numeric_limits<std::size_t>::max();

class Unpacker::UnpackData : public QAbstractTableModel
{
public:
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal)
            return {};

        if (role != Qt::DisplayRole)
            return {};

        switch ((Columns)section)
        {
            case Columns::File:   return "File";
            case Columns::LAST:   Q_ASSERT(0);
        }

        return {};
    }

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();

        const auto& data = files_[row];
        if (role == Qt::DisplayRole)
        {
            switch ((Columns)col)
            {
                case Columns::File:   return data;
                case Columns::LAST:   Q_ASSERT(0);
            }
        }
        if (role == Qt::DecorationRole)
        {
            const static QIcon ico("icons:ico_bullet_green.png");
            return ico;
        }
        return {};
    }

    virtual int rowCount(const QModelIndex&) const override
    {
        return (int)files_.size();
    }

    virtual int columnCount(const QModelIndex&) const override
    {
        return (int)Columns::LAST;
    }    

    void update(const app::Archive& arc, const QString& file)
    {
        Q_ASSERT(file.isEmpty() == false);

        const auto curSize = (int)files_.size();

        QAbstractTableModel::beginInsertRows(QModelIndex(), curSize, curSize);
        files_.push_back(file);
        QAbstractTableModel::endInsertRows();
    }

    void clear()
    {
        files_.clear();
        reset();
    }

private:
    enum class Columns {
        File, LAST
    };

private:
    friend class Unpacker;
    std::vector<QString> files_;
};


Unpacker::Unpacker(std::unique_ptr<Archiver> archiver) : engine_(std::move(archiver))
{
    DEBUG("Unpacker created");

    list_.reset(new UnpackList);    
    data_.reset(new UnpackData);

    engine_->onExtract = std::bind(&UnpackData::update, data_.get(),
        std::placeholders::_1, std::placeholders::_2);

    engine_->onProgress = [=](const app::Archive& arc, const QString& target, int done) {
        emit unpackProgress(target, done);
    };

    engine_->onReady = [=](const app::Archive& arc) {
        DEBUG("Archive %1 unpack ready with status %2", arc.file, toString(arc.state));

        if (arc.state == Archive::Status::Success)
        {
            NOTE("Archive %1 succesfully unpacked.", arc.desc);
            INFO("Archive %1 succesfully unpacked.", arc.desc);
        }
        else if (arc.state == Archive::Status::Failed)
        {
            WARN("Archive %1 unpack failed. %2", arc.desc, arc.message);
        }
        else if (arc.state == Archive::Status::Error)
        {
            ERROR("Archive %1 unpack error. %2", arc.desc, arc.message);
        }

        emit unpackReady(arc);

        const auto index = list_->findActive();
        if (index != UnpackList::NoSuchUnpack)
        {
            auto& active = list_->getArchive(index);
            active = arc;
            list_->refresh(index);
        }

        startNextUnpack();
    };

    enabled_    = true;
    cleanup_    = false;
    overwrite_  = false;
    keepBroken_ = true;
    writeLog_   = true;
}

Unpacker::~Unpacker()
{
    DEBUG("Unpacker deleted");
}

QAbstractTableModel* Unpacker::getUnpackList()
{ return list_.get(); }

QAbstractTableModel* Unpacker::getUnpackData()
{ return data_.get(); }

void Unpacker::addUnpack(const Archive& arc)
{
    list_->addUnpack(arc);

    emit unpackEnqueue(arc);

    startNextUnpack();
}

void Unpacker::stopUnpack()
{
    engine_->stop();
}

void Unpacker::moveUp(QModelIndexList& list)
{
    sortAscending(list);

    list_->moveItems(list, -1, 1);
}

void Unpacker::moveDown(QModelIndexList& list)
{
    sortDescending(list);

    list_->moveItems(list, 1, 1);
}

void Unpacker::moveToTop(QModelIndexList& list)
{
    sortAscending(list);

    const auto dist = list.front().row();

    list_->moveItems(list, -1, dist);
}

void Unpacker::moveToBottom(QModelIndexList& list)
{
    sortDescending(list);

    const auto numRows = (int)list_->numUnpacks();
    const auto lastRow = list.first().row();
    const auto dist    = numRows - lastRow - 1;

    list_->moveItems(list, 1, dist);
}

void Unpacker::kill(QModelIndexList& list)
{
    for (const auto& index : list)
    {
        const auto row  = index.row();
        const auto& arc = list_->getArchive(row);
        if (arc.state == Archive::Status::Active)
        {
            engine_->stop();
            data_->clear();
        }
    }
    list_->killItems(list);
}

void Unpacker::killComplete()
{
    list_->killComplete();
}

void Unpacker::openLog(QModelIndexList& list)
{
    for (const auto& i : list)
    {
        const auto& arc = list_->getArchive(i.row());
        if (arc.state == Archive::Status::Queued)
            continue;
        QFileInfo info(arc.path + "/extract.log");
        if (info.exists())
            app::openFile(info.absoluteFilePath());
    }
}

void Unpacker::startNextUnpack()
{
    if (engine_->isRunning())
        return;
    if (!enabled_)
        return;

    const auto index = list_->findNextPending();
    if (index == UnpackList::NoSuchUnpack)
    {
        DEBUG("Unpacking all done!");
        return;
    }

    Archiver::Settings settings;
    settings.keepBroken        = keepBroken_;
    settings.purgeOnSuccess    = cleanup_;
    settings.overWriteExisting = overwrite_;
    settings.writeLog          = writeLog_;

    auto& unpack = list_->getArchive(index);
    unpack.state = Archive::Status::Active;

    // note the stupid QProcess can invoke the signals synchronously while
    // QProcess::start() is in the *callstack* when the fucking .exe is not found...
    emit unpackStart(unpack);
    DEBUG("Start unpack for %1", unpack.file);

    engine_->extract(unpack, settings);

    data_->clear();
    list_->refresh(index);
}

QStringList Unpacker::findUnpackVolumes(const QStringList& fileEntries)
{
    return engine_->findArchives(fileEntries);
}

const Archive& Unpacker::getUnpack(const QModelIndex& index) const 
{
    return list_->getArchive(index.row());
}

std::size_t Unpacker::numUnpacks() const 
{
    return list_->numUnpacks();
}

} // app