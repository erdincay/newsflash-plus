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

#define LOGTAG "tasks"

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtGui/QIcon>
#include <newsflash/warnpop.h>

#include "tasklist.h"
#include "engine.h"
#include "debug.h"
#include "format.h"

namespace app
{

using states  = newsflash::ui::task::states;
using flags   = newsflash::ui::task::flags;
using bitflag = newsflash::bitflag<newsflash::ui::task::flags>;

const char* str(states s)
{
    switch (s)
    {
        case states::queued: return "Queued";
        case states::waiting: return "Waiting";
        case states::active: return "Active";
        case states::paused: return "Paused";
        case states::finalize: return "Finalize";
        case states::complete: return "Complete";
        case states::error: return "Error";
    }
    return "???";
}

const char* str(bitflag f)
{
    if (f.test(flags::dmca))
        return "DMCA";
    else if (f.test(flags::damaged))
        return "Damaged";    
    else if (f.test(flags::unavailable))
        return "Unavailable";
    else if (f.test(flags::error))
        return "NNTP Error";

    return "Complete";
}


tasklist::tasklist()
{}

tasklist::~tasklist()
{}

QVariant tasklist::data(const QModelIndex& index, int role) const 
{
    const auto col = index.column();
    const auto row = index.row();
    const auto& ui = tasks_[row];

    static const auto infinity = QChar(0x221E);

    if (role == Qt::DisplayRole)
    {
        switch ((columns)col)
        {
            case columns::status:
                if (ui.state == states::complete)
                    return str(ui.errors);
                return str(ui.state);

            case columns::done:
                return QString("%1%").arg(ui.completion, 0, 'f', 2);

            case columns::time:
                return format(runtime{ui.runtime});

            case columns::eta:
                if (ui.etatime == newsflash::ui::WHO_KNOWS)
                    return QString("  %1  ").arg(infinity);
                return format(runtime{ui.etatime});

            case columns::size:
                if (ui.size == 0)
                    return QString("n/a");
                return format(size{ui.size});

            case columns::desc:
                return from_utf8(ui.desc);

            case columns::sentinel: break;
        }
    }
    if (role == Qt::DecorationRole)
    {
        if (col != (int)columns::status)
            return QVariant();

        switch (ui.state)
        {
            case states::queued: 
                return QIcon(":/resource/16x16_ico_png/ico_task_queued.png");
            case states::waiting: 
                return QIcon(":/resource/16x16_ico_png/ico_task_waiting.png");
            case states::active: 
                return QIcon(":/resource/16x16_ico_png/ico_task_running.png");
            case states::paused: 
                return QIcon(":/resource/16x16_ico_png/ico_task_paused.png");   
            case states::finalize:
                return QIcon(":/resource/16x16_ico_png/ico_task_finalize.png");                             
            case states::complete: 
                if (ui.errors.any_bit())
                    return QIcon(":/resource/16x16_ico_png/ico_damaged.png");

                return QIcon(":/resource/16x16_ico_png/ico_task_complete.png");                

            case states::error: 
                return QIcon(":/resource/16x16_ico_png/ico_task_error.png");                
        }
    }
    return QVariant();
}

QVariant tasklist::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (columns(section))
    {
        case columns::status:
            return "Status";
        case columns::done:
            return "Done";
        case columns::time:
            return "Time";
        case columns::eta:
            return "ETA";
        case columns::size:
            return "Size";
        case columns::desc:
            return "Description";
        default:
            Q_ASSERT(!"incorrect column");
            break;
    }    
    return QVariant();
}

int tasklist::rowCount(const QModelIndex&) const 
{
    return (int)tasks_.size();
}

int tasklist::columnCount(const QModelIndex&) const
{
    return (int)columns::sentinel;
}

void tasklist::refresh(bool remove_complete, bool group_similar)
{
    //DEBUG("refresh tasklist");

    const auto cur_size = tasks_.size();

    g_engine->update_task_list(tasks_);
    if (tasks_.size() != cur_size)
    {
        reset();        
    }
    else
    {
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(tasks_.size(), (int)columns::sentinel);
        emit dataChanged(first, last);
    }
}

void tasklist::pause(QModelIndexList& list)
{
    manage_tasks(list, action::pause);
}

void tasklist::resume(QModelIndexList& list)
{
    manage_tasks(list, action::resume);
}

void tasklist::kill(QModelIndexList& list)
{
    qSort(list);

    int removed = 0;

    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row() - removed;
        beginRemoveRows(QModelIndex(), row, row);
        g_engine->kill_task(row);
        g_engine->update_task_list(tasks_);
        endRemoveRows();
        ++removed;
    }
}

void tasklist::move_up(QModelIndexList& list)
{
    manage_tasks(list, action::move_up);
}

void tasklist::move_down(QModelIndexList& list)
{
    manage_tasks(list, action::move_down);
}

void tasklist::manage_tasks(QModelIndexList& list, tasklist::action a)
{
    int min_index = std::numeric_limits<int>::max();
    int max_index = std::numeric_limits<int>::min();    
    for (int i=0; i<list.size(); ++i)
    {
        const auto row = list[i].row();
        if (row > max_index)
            max_index = row;
        if (row < min_index)
            min_index = row;        

        switch (a)
        {
            case action::pause:
                g_engine->pause_task(row);
                break;

            case action::resume:
                g_engine->resume_task(row);
                break;

            case action::move_up:
                g_engine->move_task_up(row);
                break;

            case action::move_down:
                g_engine->move_task_down(row);
                break;
        }
    }
    Q_ASSERT(min_index >= 0);
    Q_ASSERT(max_index < tasks_.size());    

    g_engine->update_task_list(tasks_);

    auto first = QAbstractTableModel::index(min_index, 0);
    auto last  = QAbstractTableModel::index(max_index, (int)columns::sentinel);
    emit dataChanged(first, last);    
}

} // app
