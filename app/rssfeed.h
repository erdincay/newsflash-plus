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
#  include <QString>
#  include <QUrl>
#include <newsflash/warnpop.h>

#include <vector>
#include "media.h"

class QIODevice;
class QUrl;

namespace app
{
    // RSS feed processing interface
    class rssfeed
    {
    public:
        struct params {
            QString user;
            QString key;
            int feedsize;
        };
        rssfeed() : enabled_(true)
        {}

        virtual ~rssfeed() = default;

        // parse the RSS feed coming from the IO device and store the 
        // parsed media items into the vector.
        virtual bool parse(QIODevice& io, std::vector<mediaitem>& rss) = 0;

        // prepare available URLs to retrieve the RSS feed for the matching media type.
        virtual void prepare(media m, std::vector<QUrl>& urls) = 0;

        // get site URL
        virtual QString site() const = 0;

        // get site name
        virtual QString name() const = 0;

        virtual void set_params(const params& p) {}

        virtual void enable(bool on_off)
        { enabled_ = on_off; }

        virtual bool is_enabled() const 
        { return enabled_; }
    protected:
        bool enabled_;
    private:
    };

} // app