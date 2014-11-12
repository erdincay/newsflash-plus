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

#pragma once

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <QAbstractListModel>
#  include <QDateTime>
#  include <QString>
#  include <QObject>
#include <newsflash/warnpop.h>

#include <memory>
#include <vector>
#include "account.h"

namespace app
{
    class settings;

    class accounts : public QAbstractListModel
    {
    public:
        accounts();
       ~accounts();

        // suggest a new account object
        account suggest() const;

        // get account at index
        const 
        account& get(std::size_t index) const;

        account& get(std::size_t index);
        
        const 
        account* get_fill_account() const;

        const 
        account* get_main_account() const;

        // delete the account at index 
        void del(std::size_t index);

        // insert or modify an account.
        // if account already exists it's modified otherwise it's inserted
        void set(const account& acc);

        void set_main_account(quint32 id);

        void set_fill_account(quint32 id);

        // persist accounts into datastore
        bool savestate(settings& store) const;

        // retrieve accounts from datastore
        void loadstate(settings& store); 

        std::size_t num_accounts() const
        { return accounts_.size(); }

        // AbstractListModel impl
        virtual int rowCount(const QModelIndex&) const override;

        // AbstractListMode impl
        virtual QVariant data(const QModelIndex&, int role) const override;

    private:
        std::vector<account> accounts_;
        quint32 main_account_;
        quint32 fill_account_;
    };

    extern accounts* g_accounts;

} // app
   