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
#include <newsflash/warnpop.h>

#include <newsflash/engine/ui/task.h>
#include <deque>

namespace app
{
    class tasklist : public QAbstractTableModel
    {
    public:
        tasklist();
       ~tasklist();

        // QAbstractTableModel
        virtual QVariant data(const QModelIndex& index, int role) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        virtual int rowCount(const QModelIndex&) const override;
        virtual int columnCount(const QModelIndex&) const override;

        void refresh(bool remove_complete, bool group_similar);

        void pause(QModelIndexList& list);

        void resume(QModelIndexList& list);

        void kill(QModelIndexList& list);

        void move_up(QModelIndexList& list);

        void move_down(QModelIndexList& list);

        const 
        newsflash::ui::task& getItem(std::size_t i) const 
        { return tasks_[i]; }

    private:
        enum class action {
            pause, resume, move_up, move_down
        };

        void manage_tasks(QModelIndexList& list, action a);

    private:
        enum class columns { status, done, time, eta, size, desc, sentinel };

    private:
        std::deque<newsflash::ui::task> tasks_;
    };

} // app
