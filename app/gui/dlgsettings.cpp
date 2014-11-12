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

#include <newsflash/config.h>

#include "dlgsettings.h"
#include "settings.h"

namespace gui
{

DlgSettings::DlgSettings(QWidget* parent) : QDialog(parent)
{
    ui_.setupUi(this);
}

DlgSettings::~DlgSettings()
{
    for (int i=0; i<ui_.tab->count(); ++i)
    {
        QWidget* w = ui_.tab->widget(i);
        w->setParent(nullptr);
    }
    ui_.tab->clear();
}

void DlgSettings::attach(settings* tab)
{
    Q_ASSERT(!tab->parent());

    // apparently this reparents the object.
    // GAY QT shite again
    //ui_.tab->addTab(tab, tab->windowTitle());
    stash_.insert({tab->windowTitle(), tab});
}

void DlgSettings::organize()
{
    const char* tab_layout[] = {
        "Settings"
    };

    int index = 0;
    for (auto it = std::begin(tab_layout); it != std::end(tab_layout); ++it, ++index)
    {
        auto p = stash_.find(*it);
        if (p == std::end(stash_)) {
            Q_ASSERT(!"incorrect tab name in the tab layout table");
            continue;
        }
        settings* widget = p->second;
        ui_.tab->insertTab(index, widget, widget->windowTitle());
        stash_.erase(p);
    }

    // append the rest
    for (auto p = std::begin(stash_); p != std::end(stash_); ++p)
    {
        settings* widget = p->second;
        ui_.tab->addTab(widget, widget->windowTitle());
    }
}

void DlgSettings::show(const QString& title)
{
    const auto count = ui_.tab->count();
    for (int i=0; i<count; ++i)
    {
        QWidget* tab = ui_.tab->widget(i);
        if (tab->windowTitle() == title)
        {
            ui_.tab->setCurrentIndex(i);
            return;
        }
    }
}

void DlgSettings::on_btnAccept_clicked()
{
    const auto count = ui_.tab->count();
    for (int i=0; i<count; ++i)
    {
        auto* ptr = ui_.tab->widget(i);
        auto* tab = static_cast<settings*>(ptr);
        if (!tab->validate())
        {
            ui_.tab->setCurrentIndex(i);
            return;
        }
    }

    for (int i=0; i<count; ++i)
    {
        auto* ptr = ui_.tab->widget(i);
        auto* tab = static_cast<settings*>(ptr);
        tab->accept();
    }

    accept();
}

void DlgSettings::on_btnCancel_clicked()
{
    const auto count = ui_.tab->count();
    for (int i=0; i<count; ++i)
    {
        auto* ptr = ui_.tab->widget(i);
        auto* tab = static_cast<settings*>(ptr);
        tab->cancel();
    }
    reject();
}

} // gui