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

#define LOGTAG "repair"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QMenu>
#  include <QtGui/QToolBar>
#  include <QtGui/QFileDialog>
#include <newsflash/warnpop.h>
#include "repair.h"
#include "../debug.h"
#include "../settings.h"
#include "../repairer.h"
#include "../archive.h"
#include "../utility.h"

namespace gui 
{

Repair::Repair(app::Repairer& repairer) : model_(repairer), numRepairs_(0)
{
    ui_.setupUi(this);
    ui_.tableList->setModel(model_.getRecoveryList());
    ui_.tableData->setModel(model_.getRecoveryData());
    ui_.progressList->setVisible(false);
    ui_.progressList->setMinimum(0);
    ui_.progressList->setMaximum(100);    
    ui_.lblStatus->clear();    

    QObject::connect(&model_, SIGNAL(recoveryStart(const app::Archive&)),
        this, SLOT(recoveryStart(const app::Archive&)));
    QObject::connect(&model_, SIGNAL(recoveryReady(const app::Archive&)),
        this, SLOT(recoveryReady(const app::Archive&)));

    QObject::connect(&model_, SIGNAL(scanProgress(const QString&, int)),
        this, SLOT(scanProgress(const QString&, int)));
    QObject::connect(&model_, SIGNAL(repairProgress(const QString&, int)),
        this, SLOT(repairProgress(const QString&, int)));

    QObject::connect(ui_.tableList->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this, SLOT(tableList_selectionChanged()));

    tableList_selectionChanged();

    DEBUG("Repair UI created");
}

Repair::~Repair()
{
    DEBUG("Repair UI deleted");
}

void Repair::addActions(QToolBar& bar)
{
    bar.addAction(ui_.actionAutoRepair);
    bar.addSeparator();
    bar.addAction(ui_.actionAdd);

    bar.addSeparator();        
    bar.addAction(ui_.actionTop);
    bar.addAction(ui_.actionMoveUp);
    bar.addAction(ui_.actionMoveDown);
    bar.addAction(ui_.actionBottom);
    bar.addSeparator();                
    bar.addAction(ui_.actionDel);    
    bar.addSeparator();
    bar.addAction(ui_.actionClear);

}

void Repair::addActions(QMenu& menu)
{
    menu.addAction(ui_.actionAutoRepair);
    menu.addSeparator();
}

void Repair::activate(QWidget*)
{
    numRepairs_ = 0;
    setWindowTitle("Repairs");
}

void Repair::deactivate()
{
    numRepairs_ = 0;
    setWindowTitle("Repairs");
}

void Repair::refresh(bool isActive)
{
    if (!isActive && numRepairs_)
        setWindowTitle(QString("Repairs (%1)").arg(numRepairs_));    
}

void Repair::loadState(app::Settings& settings)
{
    app::loadTableLayout("repair", ui_.tableList, settings);
    app::loadTableLayout("repair", ui_.tableData, settings);

    const auto writeLogs = settings.get("repair", "write_log_files", true);
    const auto purgePars = settings.get("repair", "purge_recovery_files_on_success", false);
    ui_.chkWriteLogs->setChecked(writeLogs);
    ui_.chkPurgePars->setChecked(purgePars);

    model_.setPurgePars(purgePars);
    model_.setWriteLogs(writeLogs);
}

void Repair::saveState(app::Settings& settings)
{
    app::saveTableLayout("repair", ui_.tableList, settings);
    app::saveTableLayout("repair", ui_.tableData, settings);

    const auto writeLogs = ui_.chkWriteLogs->isChecked();
    const auto purgePars = ui_.chkPurgePars->isChecked();
    settings.set("repair", "write_log_files", writeLogs);
    settings.set("repair", "purge_recovery_files_on_success", purgePars);
}


void Repair::on_tableList_customContextMenuRequested(QPoint)
{
    QMenu menu(this);
    menu.addAction(ui_.actionTop);
    menu.addAction(ui_.actionMoveUp);
    menu.addAction(ui_.actionMoveDown);
    menu.addAction(ui_.actionBottom);
    menu.addSeparator();
    menu.addAction(ui_.actionOpenLog);
    menu.exec(QCursor::pos());
}

void Repair::on_actionAdd_triggered()
{
    const auto& file = QFileDialog::getOpenFileName(this,
        tr("Select Recovery File"), QString(), "(Recovery Files) *.par2");
    if (file.isEmpty())
        return;

    QFileInfo info(file);
    auto base = info.completeBaseName();
    auto name = info.fileName();
    auto path = info.filePath().remove(name);

    app::Archive arc;
    arc.state = app::Archive::Status::Queued;
    arc.desc  = name;
    arc.file  = name;
    arc.path  = path;
    model_.addRecovery(arc);
}

void Repair::on_actionDel_triggered()
{
    auto indices = ui_.tableList->selectionModel()->selectedRows();

    model_.kill(indices);
}

void Repair::on_actionMoveUp_triggered()
{
    moveTasks(TaskDirection::Up);
}

void Repair::on_actionMoveDown_triggered()
{
    moveTasks(TaskDirection::Down);
}

void Repair::on_actionTop_triggered()
{
    moveTasks(TaskDirection::Top);
}

void Repair::on_actionBottom_triggered()
{
    moveTasks(TaskDirection::Bottom);
}

void Repair::on_actionClear_triggered()
{
    model_.killComplete();
}

void Repair::on_actionOpenLog_triggered()
{
    auto indices = ui_.tableList->selectionModel()->selectedRows();

    model_.openLog(indices);
}

void Repair::on_chkWriteLogs_stateChanged(int)
{
    const auto value = ui_.chkWriteLogs->isChecked();

    model_.setWriteLogs(value);
}

void Repair::on_chkPurgePars_stateChanged(int)
{
    const auto value = ui_.chkPurgePars->isChecked();

    model_.setPurgePars(value);
}

void Repair::tableList_selectionChanged()
{
    auto indices = ui_.tableList->selectionModel()->selectedRows();

    ui_.actionTop->setEnabled(false);
    ui_.actionMoveUp->setEnabled(false);
    ui_.actionMoveDown->setEnabled(false);
    ui_.actionBottom->setEnabled(false);
    ui_.actionDel->setEnabled(false);
    ui_.actionClear->setEnabled(false);
    ui_.actionOpenLog->setEnabled(false);
    if (indices.isEmpty())
        return;

    qSort(indices);
    const auto first = indices.first().row();
    const auto last  = indices.last().row();
    const auto numRows = (int)model_.numRepairs();    

    ui_.actionTop->setEnabled(first > 0);
    ui_.actionMoveUp->setEnabled(first > 0);
    ui_.actionMoveDown->setEnabled(last < numRows - 1);
    ui_.actionBottom->setEnabled(last < numRows - 1);
    ui_.actionDel->setEnabled(true);
    ui_.actionOpenLog->setEnabled(true);
}

void Repair::recoveryStart(const app::Archive& rec)
{
    ui_.progressList->setVisible(true);
    ui_.lblStatus->setVisible(true);
    if (ui_.chkWriteLogs->isChecked())
        ui_.actionOpenLog->setChecked(true);

    numRepairs_++;
}

void Repair::recoveryReady(const app::Archive& rec)
{
    ui_.progressList->setVisible(false);
    ui_.lblStatus->setVisible(false);
    ui_.actionClear->setEnabled(true);
}

void Repair::scanProgress(const QString& file, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(QString("Scanning ... %1").arg(file));
    ui_.tableData->scrollToBottom();
}

void Repair::repairProgress(const QString& step, int done)
{
    ui_.progressList->setValue(done);
    ui_.lblStatus->setText(step);
}

void Repair::moveTasks(TaskDirection dir)
{
    auto indices = ui_.tableList->selectionModel()->selectedRows();

    switch (dir)
    {
        case TaskDirection::Up:
            model_.moveUp(indices);
            break;
        case TaskDirection::Down:
            model_.moveDown(indices);
            break;
        case TaskDirection::Top:
            model_.moveToTop(indices);
            break;
        case TaskDirection::Bottom:
            model_.moveToBottom(indices);
            break;
    }


    QItemSelection selection;
    for (int i=0; i<indices.size(); ++i)
        selection.select(indices[i], indices[i]);

    auto* model = ui_.tableList->selectionModel();
    model->setCurrentIndex(selection.indexes().first(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    model->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

} // gui