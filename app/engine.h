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

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QObject>
#  include <QString>
#  include <QMetaType>
#include "newsflash/warnpop.h"

#include <memory>
#include <vector>
#include <string>

#include "engine/ui/connection.h"
#include "engine/ui/task.h"
#include "engine/engine.h"
#include "platform.h"
#include "format.h"
#include "accounts.h"

class QEvent;

namespace app
{
    class Settings;
    struct NZBContent;
    struct Download;

    struct FileInfo;
    struct FilePackInfo;
    struct NewsGroupInfo;
    struct HeaderInfo;

    // manager class around newsflash engine + engine state
    // translate between native c++ and Qt types and events.
    class Engine : public QObject
    {
        Q_OBJECT

    public:
        Engine();
       ~Engine();

        QString resolveDownloadPath(const QString& basePath) const;

        void testAccount(const Accounts::Account& acc);

        // set/modify account in the engine.
        void setAccount(const Accounts::Account& acc);

        // delete an account.
        void delAccount(const Accounts::Account& acc);

        // set the current fill account.
        void setFillAccount(quint32 id);

        bool downloadNzbContents(const Download& download, const QByteArray& nzb);
        bool downloadNzbContents(const Download& download, std::vector<NZBContent> nzb);

        // retrieve a newgroup listing from the specified account.
        // returns a task id that can be used to manage the task.
        quint32 retrieveNewsgroupListing(quint32 acc);

        quint32 retrieveHeaders(quint32 acc, const QString& path, const QString& name);

        void loadState(Settings& s);
        void saveState(Settings& s);

        void loadSession();
        void saveSession();

        void connect(bool on_off);

        // refresh the engine state such as download speed, available disk spaces etc.
        void refresh();

        // begin engine shutdown. returns true if complete immediately
        // otherwise false in which case the a signal is emitted
        // once all pending actions have been processed in the engine.
        bool shutdown();

        // refresh the list of UI task states.
        void refreshTaskList(std::deque<newsflash::ui::task>& list)
        {
            engine_->update(list);
        }

        // refresh the list of UI connection states
        void refreshConnList(std::deque<newsflash::ui::connection>& list)
        {
            engine_->update(list);

            totalspeed_ = 0;

            // at any given moment the current engine combined download
            // speed should equal that of of all it's connections.
            // however this is the only place where we are able to get that
            // information. so in order to be able to call get_download_speed()
            // reliable a call to update_conn_list has to be made first.
            for (const auto& ui : list)
                totalspeed_ += ui.bps;
        }

        // get the free disk space at the default download location.
        quint64 getFreeDiskSpace() const
        {
            return diskspace_;
        }

        // get the free disk space on the disk where the downloadPath points to.
        quint64 getFreeDiskSpace(const QString& downloadPath) const
        {
            const auto& location   = resolveDownloadPath(downloadPath);
            const auto& mountPoint = app::resolveMountPoint(location);
            const auto  freeSpace  = app::getFreeDiskSpace(mountPoint);
            return freeSpace;
        }

        // get current total download speed (of all connections)
        quint32 getDownloadSpeed() const
        {
            return totalspeed_;
        }

        // get the number of bytes downloaded from all servers/accounts.
        quint64 getBytesDownloaded() const
        {
            return engine_->get_bytes_downloaded();
        }

        // get the bytes currently queued in engine for downloading.
        quint64 getBytesQueued() const
        {
            return engine_->get_bytes_queued();
        }

        // get the number of bytes currently ready.
        // bytes ready is always a fraction of bytesQueued
        // so if bytesQueued is zero then bytesReady is also zero.
        quint64 getBytesReady() const
        {
            return engine_->get_bytes_ready();
        }

        // get the number of bytes written to the disk
        quint64 getBytesWritten() const
        {
            return engine_->get_bytes_written();
        }

        const QString& getLogfilesPath() const
        {
            return logifiles_;
        }

        QString getEngineLogfile() const
        {
            return fromUtf8(engine_->get_logfile());
        }

        void setLogfilesPath(const QString& path)
        {
            logifiles_ = path;
        }

        const QString& getDownloadPath() const
        {
            return downloads_;
        }

        void setDownloadPath(const QString& path)
        {
            downloads_  = path;
            mountpoint_ = resolveMountPoint(downloads_);
        }

        bool getOverwriteExistingFiles() const
        {
            return engine_->get_overwrite_existing_files();
        }

        bool getDiscardTextContent() const
        {
            return engine_->get_discard_text_content();
        }

        bool getPreferSecure() const
        {
            return engine_->get_prefer_secure();
        }

        bool isStarted() const
        {
            return engine_->is_started();
        }

        bool getConnect() const
        {
            return connect_;
        }

        bool getThrottle() const
        {
            return engine_->get_throttle();
        }

        unsigned getThrottleValue() const
        {
            return engine_->get_throttle_value();
        }

        bool getCheckLowDisk() const
        { return checkLowDisk_; }

        void setCheckLowDisk(bool on_off)
        { checkLowDisk_ = on_off; }

        void setOverwriteExistingFiles(bool on_off)
        {
            engine_->set_overwrite_existing_files(on_off);
        }

        void setDiscardTextContent(bool on_off)
        {
            engine_->set_discard_text_content(on_off);
        }

        void setPreferSecure(bool on_off)
        {
            engine_->set_prefer_secure(on_off);
        }

        void setGroupSimilar(bool on_off)
        {
            engine_->set_group_items(on_off);
        }

        void setThrottle(bool on_off)
        {
            engine_->set_throttle(on_off);
        }

        void setThrottleValue(unsigned val)
        {
            engine_->set_throttle_value(val);
        }

        void killConnection(std::size_t index)
        {
            engine_->kill_connection(index);
        }

        void cloneConnection(std::size_t index)
        {
            engine_->clone_connection(index);
        }

        void killAction(quint32 id)
        {
            engine_->kill_action(id);
        }

        void killTask(std::size_t index)
        {
            const auto action = engine_->get_action_id(index);

            engine_->kill_task(index);

            emit actionKilled(action);
        }

        void pauseTask(std::size_t index)
        {
            engine_->pause_task(index);
        }

        void resumeTask(std::size_t index)
        {
            engine_->resume_task(index);
        }

        void moveTaskUp(std::size_t index)
        {
            engine_->move_task_up(index);
        }

        void moveTaskDown(std::size_t index)
        {
            engine_->move_task_down(index);
        }

    signals:
        void shutdownComplete();
        void newDownloadQueued(const Download& download);
        void newHeaderDataAvailable(const QString& group, const QString& file);
        void newHeaderInfoAvailable(const QString& group, quint64 numLocal, quint64 numRemote);
        void fileCompleted(const app::FileInfo& file);
        void packCompleted(const app::FilePackInfo& pack);
        void listCompleted(quint32 account, const QList<app::NewsGroupInfo>& list);
        void updateCompleted(const app::HeaderInfo& headers);
        void allCompleted();
        void numPendingTasks(std::size_t num);
        void quotaUpdate(std::size_t bytes, std::size_t account);
        void actionKilled(quint32 action);
        void testAccountComplete(bool success);
        void testAccountLogMsg(const QString& msg);

    private:
        virtual bool eventFilter(QObject* object, QEvent* event) override;
        virtual void timerEvent(QTimerEvent* event) override;

    private:
        void start();

    private:
        void onError(const newsflash::ui::error& e);
        void onTaskComplete(const newsflash::ui::task& t);
        void onFileComplete(const newsflash::ui::file& f);
        void onBatchComplete(const newsflash::ui::batch& b);
        void onListComplete(const newsflash::ui::listing& l);
        void onUpdateComplete(const newsflash::ui::update& u);
        void onHeaderDataAvailable(const std::string& group,
            const std::string& file);
        void onHeaderInfoAvailable(const std::string& group,
            std::uint64_t numLocal, std::uint64_t numRemote);
        void onAllComplete();
        void onQuota(std::size_t bytes, std::size_t account);
        void onConnectionTestComplete(bool success);
        void onConnectionTestLogMsg(const std::string& msg);

    private:
        QString logifiles_;
        QString downloads_;
        QString mountpoint_;
        quint64 diskspace_;
        quint32 totalspeed_;
        int ticktimer_;
        std::unique_ptr<newsflash::engine> engine_;
        bool connect_;
        bool shutdown_;
        bool checkLowDisk_;
    };

    extern Engine* g_engine;

} // app
