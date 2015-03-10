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

#include <newsflash/config.h>
#include <newsflash/warnpush.h>
#  include <QtAlgorithms>
#  include <QModelIndex>
#include <newsflash/warnpop.h>
#include <algorithm>

class QAbstractTableModel;
class QTableView;

namespace app
{

class Settings;

inline
void sortAscending(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qLess<QModelIndex>());
}

inline
void sortDescending(QModelIndexList& list)
{
    qSort(list.begin(), list.end(), qGreater<QModelIndex>());
}

void saveTableLayout(const QString& key, const QTableView* view, Settings& settings);
void loadTableLayout(const QString& key, QTableView* view, const Settings& settings);

template<typename Class, typename Member>
struct CompareLess {
    typedef Member (Class::*MemPtr);

    CompareLess(MemPtr p) : ptr_(p)
    {}

    bool operator()(const Class& lhs, const Class& rhs) const 
    {
        return lhs.*ptr_ < rhs.*ptr_;
    }
private:
    MemPtr ptr_;
}; 

template<typename Class, typename Member>
struct CompareGreater {
    typedef Member (Class::*MemPtr);

    CompareGreater(MemPtr p) : ptr_(p)
    {}

    bool operator()(const Class& lhs, const Class& rhs) const 
    {
        return lhs.*ptr_ > rhs.*ptr_;
    }
private:
    MemPtr ptr_;
};

template<typename Class, typename Member>
CompareLess<Class, Member> less(Member Class::*p)
{
    return CompareLess<Class, Member>(p);
}

template<typename Class, typename Member>
CompareGreater<Class, Member> greater(Member Class::*p)
{
    return CompareGreater<Class, Member>(p);
}

template<typename Container, typename MemPtr>
void sort(Container& container, Qt::SortOrder order, MemPtr p)
{
    if (order == Qt::AscendingOrder)
        std::sort(std::begin(container), std::end(container), less(p));
    else std::sort(std::begin(container), std::end(container), greater(p));
}

} // app