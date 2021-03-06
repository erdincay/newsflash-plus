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

#define LOGTAG "rss"

#include "newsflash/config.h"
#include "newsflash/warnpush.h"
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QMessageBox>
#  include "ui_rss_settings.h"
#include "newsflash/warnpop.h"
#include "rss.h"
#include "mainwindow.h"
#include "nzbfile.h"
#include "common.h"
#include "app/debug.h"
#include "app/format.h"
#include "app/eventlog.h"
#include "app/settings.h"
#include "app/utility.h"
#include "app/historydb.h"

namespace {

// we can keep the RSS settings in the .cpp because
// it doesn't use signals so it doesn't need MOC to be run (which requires a .h file)
class MySettings : public gui::SettingsWidget
{
public:
    MySettings()
    {
        ui_.setupUi(this);
    }
   ~MySettings()
    {}

    virtual bool validate() const override
    {
        const bool enable_nzbs = ui_.grpNZBS->isChecked();
        if (enable_nzbs)
        {
            const auto userid = ui_.edtNZBSUserId->text();
            if (userid.isEmpty())
            {
                ui_.edtNZBSUserId->setFocus();
                return false;
            }
            const auto apikey = ui_.edtNZBSApikey->text();
            if(apikey.isEmpty())
            {
                ui_.edtNZBSApikey->setFocus();
                return false;
            }
        }
        return true;
    }

private:
    Ui::RSSSettings ui_;
private:
    friend class gui::RSS;
};

} // namespace


namespace gui
{

RSS::RSS()
{
    ui_.setupUi(this);
    ui_.tableView->setModel(model_.getModel());
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionStop->setEnabled(false);
    ui_.progressBar->setVisible(false);
    ui_.progressBar->setValue(0);
    ui_.progressBar->setRange(0, 0);

    auto* selection = ui_.tableView->selectionModel();
    QObject::connect(selection, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(rowChanged()));

    QObject::connect(&popup_, SIGNAL(timeout()),
        this, SLOT(popupDetails()));

    ui_.tableView->viewport()->installEventFilter(this);
    ui_.tableView->viewport()->setMouseTracking(true);
    ui_.tableView->setMouseTracking(true);
    //ui_.tableView->setColumnWidth((int)app::RSSReader::columns::locked, 32);

    // when the model has no more actions we hide the progress bar and disable
    // the stop button.
    model_.on_ready = [&]() {
        ui_.progressBar->hide();
        ui_.actionStop->setEnabled(false);
        ui_.actionRefresh->setEnabled(true);
    };

    show_popup_hint_ = false;

    DEBUG("RSS gui created");
}

RSS::~RSS()
{
    DEBUG("RSS gui destroyed");
}

void RSS::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addAction(ui_.actionSave);
    menu.addSeparator();
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
}

void RSS::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionRefresh);
    bar.addSeparator();
    bar.addAction(ui_.actionDownload);
    bar.addSeparator();
    bar.addAction(ui_.actionSettings);
    bar.addSeparator();
    bar.addAction(ui_.actionStop);
}

void RSS::activate(QWidget*)
{
    if (model_.isEmpty())
    {
        const bool refreshing = ui_.progressBar->isVisible();
        if (refreshing)
            return;
        refreshStreams(false);
    }
}

void RSS::saveState(app::Settings& settings)
{
    app::saveState("rss", ui_.chkAdult, settings);
    app::saveState("rss", ui_.chkApps, settings);
    app::saveState("rss", ui_.chkGames, settings);
    app::saveState("rss", ui_.chkMovies, settings);
    app::saveState("rss", ui_.chkMusic, settings);
    app::saveState("rss", ui_.chkTV, settings);

    settings.set("rss", "enable_nzbs", enable_nzbs_);
    settings.set("rss", "enable_womble", enable_womble_);
    settings.set("rss", "nzbs_apikey", nzbs_apikey_);
    settings.set("rss", "nzbs_userid", nzbs_userid_);
    settings.set("rss", "streams", quint64(streams_.value()));

    app::saveTableLayout("rss", ui_.tableView, settings);
}

void RSS::shutdown()
{
    // we're shutting down. cancel all pending requests if any.
    model_.stop();
}

void RSS::loadState(app::Settings& settings)
{
    app::loadState("rss", ui_.chkAdult, settings);
    app::loadState("rss", ui_.chkApps, settings);
    app::loadState("rss", ui_.chkGames, settings);
    app::loadState("rss", ui_.chkMovies, settings);
    app::loadState("rss", ui_.chkMusic, settings);
    app::loadState("rss", ui_.chkTV, settings);

    enable_nzbs_    = settings.get("rss", "enable_nzbs", false);
    enable_womble_  = settings.get("rss", "enable_womble", true);
    nzbs_apikey_    = settings.get("rss", "nzbs_apikey", "");
    nzbs_userid_    = settings.get("rss", "nzbs_userid", "");

    quint64 streams = 0;

    // if the mediatype has become incompatible we simply reset it.
    if (settings.get("version", "mediatype").toInt() != app::MediaTypeVersion)
        streams = ~streams;
    else streams = settings.get("rss", "streams", ~quint64(0));

    streams_.set_from_value(streams);

    model_.enableFeed("womble", enable_womble_);
    model_.enableFeed("nzbs", enable_nzbs_);
    model_.setCredentials("nzbs", nzbs_userid_, nzbs_apikey_);

    app::loadTableLayout("rss", ui_.tableView, settings);
}

MainWidget::info RSS::getInformation() const
{
    return {"rss.html", true};
}

SettingsWidget* RSS::getSettings()
{
    auto* p = new MySettings;
    auto& ui = p->ui_;

    using m = app::MediaType;
    if (streams_.test(m::TvInt)) ui.chkTvInt->setChecked(true);
    if (streams_.test(m::TvSD))  ui.chkTvSD->setChecked(true);
    if (streams_.test(m::TvHD))  ui.chkTvHD->setChecked(true);

    if (streams_.test(m::AppsPC)) ui.chkComputerPC->setChecked(true);
    if (streams_.test(m::AppsISO)) ui.chkComputerISO->setChecked(true);
    if (streams_.test(m::AppsAndroid)) ui.chkComputerAndroid->setChecked(true);
    if (streams_.test(m::AppsMac) || streams_.test(m::AppsIos))
        ui.chkComputerMac->setChecked(true);

    if (streams_.test(m::MusicMp3)) ui.chkMusicMP3->setChecked(true);
    if (streams_.test(m::MusicVideo)) ui.chkMusicVideo->setChecked(true);
    if (streams_.test(m::MusicLossless)) ui.chkMusicLosless->setChecked(true);

    if (streams_.test(m::MoviesInt)) ui.chkMoviesInt->setChecked(true);
    if (streams_.test(m::MoviesSD)) ui.chkMoviesSD->setChecked(true);
    if (streams_.test(m::MoviesHD)) ui.chkMoviesHD->setChecked(true);
    if (streams_.test(m::MoviesWMV)) ui.chkMoviesWMV->setChecked(true);

    if (streams_.test(m::GamesNDS) ||
        streams_.test(m::GamesWii))
        ui.chkConsoleNintendo->setChecked(true);

    if (streams_.test(m::GamesPSP) ||
        streams_.test(m::GamesPS2) ||
        streams_.test(m::GamesPS3) ||
        streams_.test(m::GamesPS4))
        ui.chkConsolePlaystation->setChecked(true);

    if (streams_.test(m::GamesXbox) ||
        streams_.test(m::GamesXbox360))
        ui.chkConsoleXbox->setChecked(true);

    if (streams_.test(m::AdultDVD)) ui.chkXXXDVD->setChecked(true);
    if (streams_.test(m::AdultSD)) ui.chkXXXSD->setChecked(true);
    if (streams_.test(m::AdultHD)) ui.chkXXXHD->setChecked(true);

    ui.grpWomble->setChecked(enable_womble_);
    ui.grpNZBS->setChecked(enable_nzbs_);
    ui.edtNZBSUserId->setText(nzbs_userid_);
    ui.edtNZBSApikey->setText(nzbs_apikey_);

    return p;
}


void RSS::applySettings(SettingsWidget* gui)
{
    auto* mine = dynamic_cast<MySettings*>(gui);
    auto& ui = mine->ui_;

    using m = app::MediaType;

    streams_.clear();

    if (ui.chkTvInt->isChecked()) streams_.set(m::TvInt);
    if (ui.chkTvSD->isChecked()) streams_.set(m::TvSD);
    if (ui.chkTvHD->isChecked()) streams_.set(m::TvHD);

    if (ui.chkComputerPC->isChecked()) streams_.set(m::AppsPC);
    if (ui.chkComputerISO->isChecked()) streams_.set(m::AppsISO);
    if (ui.chkComputerAndroid->isChecked()) streams_.set(m::AppsAndroid);
    if (ui.chkComputerMac->isChecked()) {
        streams_.set(m::AppsMac);
        streams_.set(m::AppsIos);
    }

    if (ui.chkMusicMP3->isChecked()) streams_.set(m::MusicMp3);
    if (ui.chkMusicVideo->isChecked()) streams_.set(m::MusicVideo);
    if (ui.chkMusicLosless->isChecked()) streams_.set(m::MusicLossless);

    if (ui.chkMoviesInt->isChecked()) streams_.set(m::MoviesInt);
    if (ui.chkMoviesSD->isChecked()) streams_.set(m::MoviesSD);
    if (ui.chkMoviesHD->isChecked()) streams_.set(m::MoviesHD);
    if (ui.chkMoviesWMV->isChecked()) streams_.set(m::MoviesWMV);

    if (ui.chkConsoleNintendo->isChecked())
    {
        streams_.set(m::GamesNDS);
        streams_.set(m::GamesWii);
    }
    if (ui.chkConsolePlaystation->isChecked())
    {
        streams_.set(m::GamesPSP);
        streams_.set(m::GamesPS2);
        streams_.set(m::GamesPS3);
        streams_.set(m::GamesPS4);
    }
    if (ui.chkConsoleXbox->isChecked())
    {
        streams_.set(m::GamesXbox);
        streams_.set(m::GamesXbox360);
    }

    if (ui.chkXXXDVD->isChecked()) streams_.set(m::AdultDVD);
    if (ui.chkXXXHD->isChecked()) streams_.set(m::AdultHD);
    if (ui.chkXXXSD->isChecked()) streams_.set(m::AdultSD);

    enable_womble_    = ui.grpWomble->isChecked();
    enable_nzbs_      = ui.grpNZBS->isChecked();
    nzbs_apikey_      = ui.edtNZBSApikey->text();
    nzbs_userid_      = ui.edtNZBSUserId->text();

    model_.enableFeed("womble", enable_womble_);
    model_.enableFeed("nzbs", enable_nzbs_);
    model_.setCredentials("nzbs", nzbs_userid_, nzbs_apikey_);
}

void RSS::freeSettings(SettingsWidget* s)
{
    delete s;
}

Finder* RSS::getFinder()
{
    return this;
}

void RSS::firstLaunch()
{
    show_popup_hint_ = true;
}

bool RSS::isMatch(const QString& str, std::size_t index, bool caseSensitive)
{
    const auto& item = model_.getItem(index);
    if (!caseSensitive)
    {
        auto upper = item.title.toUpper();
        if (upper.indexOf(str) != -1)
            return true;
    }
    else
    {
        if (item.title.indexOf(str) != -1)
            return true;
    }
    return false;
}

bool RSS::isMatch(const QRegExp& regex, std::size_t index)
{
    const auto& item = model_.getItem(index);
    if (regex.indexIn(item.title) != -1)
        return true;
    return false;
}

std::size_t RSS::numItems() const
{
    return model_.numItems();
}

std::size_t RSS::curItem() const
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return 0;
    const auto& first = indices.first();
    return first.row();
}

void RSS::setFound(std::size_t index)
{
    auto* model = ui_.tableView->model();
    auto i = model->index(index, 0);
    ui_.tableView->setCurrentIndex(i);
    ui_.tableView->scrollTo(i);
}

bool RSS::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        popup_.start(1000);
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        if (movie_)
            movie_->hide();
    }
    return QObject::eventFilter(obj, event);
}

void RSS::downloadSelected(const QString& folder)
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    bool actions = false;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto row = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& desc = item.title;

        if (!passDuplicateCheck(this, desc, item.type))
            continue;

        if (!passSpaceCheck(this, desc, folder, item.size, item.size))
            continue;

        const auto acc = selectAccount(this, desc);
        if (acc == 0)
            continue;

        model_.downloadNzbContent(row, acc, folder);
        actions = true;
    }

    if (actions)
    {
        ui_.progressBar->setVisible(true);
        ui_.actionStop->setEnabled(true);
    }
}

void RSS::refreshStreams(bool verbose)
{
    if (!enable_nzbs_ && !enable_womble_)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText(tr("You haven't enabled any RSS feed sites.\n\rYou can enable them in the RSS settings."));
            msg.exec();
        }
        return;
    }

    using m = app::MediaType;
    using s = newsflash::bitflag<app::MediaType, quint64>;

    const auto musicMask   = s(m::MusicMp3) | s(m::MusicLossless) | s(m::MusicVideo) | s(m::MusicOther);
    const auto moviesMask  = s(m::MoviesInt) | s(m::MoviesSD) | s(m::MoviesHD) | s(m::MoviesWMV) | s(m::MoviesOther);
    const auto tvMask      = s(m::TvInt) | s(m::TvHD) | s(m::TvSD) | s(m::TvSport) | s(m::TvOther);
    const auto appsMask    = s(m::AppsPC) | s(m::AppsISO) | s(m::AppsAndroid) | s(m::AppsIos) | s(m::AppsMac) | s(m::AppsOther);
    const auto adultMask   = s(m::AdultDVD) | s(m::AdultHD) | s(m::AdultSD) | s(m::AdultImg) | s(m::AdultOther);
    const auto gamesMask   = s(m::GamesNDS) | s(m::GamesWii) | s(m::GamesXbox) | s(m::GamesXbox360) |
        s(m::GamesPSP) | s(m::GamesPS1) | s(m::GamesPS2) | s(m::GamesPS3) | s(m::GamesPS3) | s(m::GamesPS4) |
        s(m::GamesOther);

    struct feed {
        QCheckBox* chk;
        s mask;
    } selected_feeds[] = {
        {ui_.chkGames, gamesMask},
        {ui_.chkMusic, musicMask},
        {ui_.chkMovies, moviesMask},
        {ui_.chkTV, tvMask},
        {ui_.chkApps, appsMask},
        {ui_.chkAdult, adultMask}
    };

    bool have_selections = false;
    bool have_feeds = false;

    for (const auto& feed : selected_feeds)
    {
        if (!feed.chk->isChecked())
            continue;

        // iterate all the bits (streams) and start RSS refresh operations
        // on all bits that are set.
        auto beg = app::MediaIterator::begin();
        auto end = app::MediaIterator::end();
        for (; beg != end; ++beg)
        {
            const auto m = *beg;
            if (!feed.mask.test(m))
                continue;
            if (!streams_.test(m))
                continue;

            if (model_.refresh(m))
                have_feeds = true;

            have_selections = true;
        }
    }

    if (!have_selections)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("You haven't selected any RSS Media categories.\r\n"
                "Select the sub-categories in RSS settings and main categories in the RSS main window");
            msg.exec();
        }
        return;
    }
    else if (!have_feeds)
    {
        if (verbose)
        {
            QMessageBox msg(this);
            msg.setStandardButtons(QMessageBox::Ok);
            msg.setIcon(QMessageBox::Information);
            msg.setText("There are no feeds available matching the selected categories.");
            msg.exec();
        }
        return;
    }

    ui_.progressBar->setVisible(true);
    ui_.actionDownload->setEnabled(false);
    ui_.actionDownloadTo->setEnabled(false);
    ui_.actionSave->setEnabled(false);
    ui_.actionOpen->setEnabled(false);
    ui_.actionStop->setEnabled(true);
    ui_.actionRefresh->setEnabled(false);
    ui_.actionInformation->setEnabled(false);
}

void RSS::on_actionRefresh_triggered()
{
    refreshStreams(true);
}

void RSS::on_actionDownload_triggered()
{
    downloadSelected("");
}

void RSS::on_actionDownloadTo_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void RSS::on_actionSave_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    for (int i=0; i<indices.size(); ++i)
    {
        const auto& item = model_.getItem(indices[i].row());
        const auto& nzb  = g_win->selectNzbSaveFile(item.title + ".nzb");
        if (nzb.isEmpty())
            continue;
        model_.downloadNzbFile(indices[i].row(), nzb);
        ui_.progressBar->setVisible(true);
        ui_.actionStop->setEnabled(true);
    }
}

void RSS::on_actionOpen_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    static auto callback = [=](const QByteArray& bytes, const QString& desc) {
        auto* view = new NZBFile();
        view->setProperty("parent-object", QVariant::fromValue(static_cast<QObject*>(this)));
        g_win->attach(view, false);
        view->open(bytes, desc);
    };

    for (int i=0; i<indices.size(); ++i)
    {
        const auto  row  = indices[i].row();
        const auto& item = model_.getItem(row);
        const auto& desc = item.title;
        model_.downloadNzbFile(row, std::bind(callback,
            std::placeholders::_1, desc));
    }

    ui_.progressBar->setVisible(true);
    ui_.actionStop->setEnabled(true);
}

void RSS::on_actionSettings_triggered()
{
    emit showSettings(this);
}

void RSS::on_actionStop_triggered()
{
    model_.stop();
    ui_.progressBar->hide();
    ui_.actionStop->setEnabled(false);
    ui_.actionRefresh->setEnabled(true);
}

void RSS::on_actionBrowse_triggered()
{
    const auto& folder = g_win->selectDownloadFolder();
    if (folder.isEmpty())
        return;

    downloadSelected(folder);
}

void RSS::on_actionInformation_triggered()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    const auto& first = indices[0];
    const auto& item  = model_.getItem(first);

    if (isMovie(item.type))
    {
        const auto& title = app::findMovieTitle(item.title);
        if (title.isEmpty())
            return;
        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
        }
        movie_->lookupMovie(title);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;
        // the same dialog + api can be used for TV series.
        if (!movie_)
            movie_.reset(new DlgMovie(this));

        movie_->lookupSeries(title);
    }

    if (show_popup_hint_)
    {
        QMessageBox msg(this);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setIcon(QMessageBox::Information),
        msg.setWindowTitle("Hint");
        msg.setText("Did you know that you can also open this dialog by letting the mouse hover over the selected item.");
        msg.exec();
        show_popup_hint_ = false;
    }
}

void RSS::on_tableView_customContextMenuRequested(QPoint point)
{
    QMenu sub("Download to");
    sub.setIcon(QIcon("icons:ico_download.png"));

    const auto& recents = g_win->getRecentPaths();
    for (const auto& recent : recents)
    {
        QAction* action = sub.addAction(QIcon("icons:ico_folder.png"), recent);
        QObject::connect(action, SIGNAL(triggered(bool)),
            this, SLOT(downloadToPrevious()));
    }

    sub.addSeparator();
    sub.addAction(ui_.actionBrowse);
    sub.setEnabled(ui_.actionDownload->isEnabled());

    QMenu menu;
    menu.addAction(ui_.actionRefresh);
    menu.addSeparator();
    menu.addAction(ui_.actionDownload);
    menu.addMenu(&sub);
    menu.addSeparator();
    menu.addAction(ui_.actionSave);
    menu.addSeparator();
    menu.addAction(ui_.actionOpen);
    menu.addSeparator();
    menu.addAction(ui_.actionInformation);
    menu.addSeparator();
    menu.addAction(ui_.actionStop);
    menu.addSeparator();
    menu.addAction(ui_.actionSettings);
    menu.exec(QCursor::pos());
}

void RSS::on_tableView_doubleClicked(const QModelIndex&)
{
    on_actionDownload_triggered();
}

void RSS::rowChanged()
{
    const auto& indices = ui_.tableView->selectionModel()->selectedRows();
    if (indices.isEmpty())
        return;

    ui_.actionDownload->setEnabled(true);
    ui_.actionSave->setEnabled(true);
    ui_.actionOpen->setEnabled(true);
    ui_.actionInformation->setEnabled(true);

    for (const auto& i : indices)
    {
        const auto& item = model_.getItem(i);
        if (!(isMovie(item.type) || isTelevision(item.type)))
        {
            ui_.actionInformation->setEnabled(false);
            break;
        }
    }
}

void RSS::downloadToPrevious()
{
    const auto* action = qobject_cast<const QAction*>(sender());

    // use the directory from the action title
    const auto folder = action->text();

    downloadSelected(folder);
}

void RSS::popupDetails()
{
    //DEBUG("Popup event!");

    popup_.stop();
    if (movie_ && movie_->isVisible())
        return;

    if (!ui_.tableView->underMouse())
        return;

    const QPoint global = QCursor::pos();
    const QPoint local  = ui_.tableView->viewport()->mapFromGlobal(global);

    const auto& all = ui_.tableView->selectionModel()->selectedRows();
    const auto& sel = ui_.tableView->indexAt(local);
    int i=0;
    for (; i<all.size(); ++i)
    {
        if (all[i].row() == sel.row())
            break;
    }
    if (i == all.size())
        return;

    const auto& item  = model_.getItem(sel.row());
    if (isMovie(item.type))
    {
        const auto& title = app::findMovieTitle(item.title);
        if (title.isEmpty())
            return;
        if (!movie_)
        {
            movie_.reset(new DlgMovie(this));
        }
        movie_->lookupMovie(title);
    }
    else if (isTelevision(item.type))
    {
        const auto& title = app::findTVSeriesTitle(item.title);
        if (title.isEmpty())
            return;

        if (!movie_)
            movie_.reset(new DlgMovie(this));

        movie_->lookupSeries(title);
    }

}

} // gui

