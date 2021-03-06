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
#  include <QThread>
#include <newsflash/warnpop.h>
#include <memory>
#include <vector>
#include <mutex>
#include "nzbparse.h"

class QIODevice;

namespace app
{
    // this is a class in app scope because MOC doesn't support nested classes...
    // nzbthread provides one-shot background thread for parsing NZB content 
    // from an already opened IO object.
    class NZBThread : public QThread
    {
        Q_OBJECT

    public:
        // create a new one-shot thread object with the 
        // given IO object. The IO should be already opened.
        NZBThread(std::unique_ptr<QIODevice> io);
       ~NZBThread();

        // get the result of the parse operation.
        // this should be called once the complete() signal
        // has been emitted indicating completion.
        NZBError result(std::vector<NZBContent>& data);

    signals:
        // complete signal is emitted once the data from the IO object
        // has been read and parsed. The result + the parsed nzb data
        // can then be retrieved with a call to result()
        void complete();

    private:
        virtual void run() override;

    private:
        std::unique_ptr<QIODevice> io_;
        std::vector<NZBContent> data_;
        std::mutex mutex_;
        NZBError error_;
    };

} // app
