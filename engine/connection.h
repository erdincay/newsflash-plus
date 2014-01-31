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

#include <boost/noncopyable.hpp>
#include <mutex>
#include <thread>
#include <cstdint>
#include "command.h"
#include "msgqueue.h"
#include "protocol.h"
#include "event.h"

namespace newsflash
{
    class socket;

    // connection represents a high level connection to some
    // newsserver host. 
    class connection : boost::noncopyable
    {
    public:
        enum class state {
            connecting, idle, active, error
        };
        enum class error {
            none,        // no error 
            resolve,     // failed to resolve host address
            refused,     // host refused the connection attempt
            forbidden,   // connection was made but user credentials were refused
            protocol,    // nntp protocol level error occurred
            socket,      // tcp socket error occurred
            ssl,         // ssl error occurred
            timeout,     // no data was received from host within timeout
            interrupted, // pending operation was interrupted 
            unknown      // something else.
        };

        // server argument to connect to
        struct server {
            std::string addr;
            std::string user;
            std::string pass;
            int port;
            bool ssl;
        };

        // create and start a new connection to the given host.
        // cmdqueue is the queue of input commands to start  executing 
        // as soon as the connection is ready.
        // resqueue is the queue of responses generated by the in commands.
        connection(const std::string& logfile, const server& host, cmdqueue& in, resqueue& out);

       ~connection();

        // current execution status pack
        struct status {
            connection::state state;       
            connection::error error;      
            uint64_t bytes; 
            uint64_t speed;
        };

        // get the current status of the connection.
        status get_status();

        // get a level triggered waithandle for waiting on state
        // changes in the connection. the signal is set when a new
        // state is reached and remains signaled untill get_status()
        // is called to implicitly reset the signal
        waithandle wait() const;

    private:
        void main(const server& host, const std::string& logfile);
        bool connect(const server& host);
        void execute();

        void auth(std::string& user, std::string& pass);
        void send(const void* data, size_t len);
        size_t read(void* buff, size_t len);

        void goto_state(state s, error e);

        mutable std::mutex mutex_;
        // state that is procted by the mutex
        state    state_;
        error    error_;
        uint64_t bytes_;
        uint64_t speed_;
        // end state

        cmdqueue& commands_;
        resqueue& responses_;

        std::string username_;
        std::string password_;
        std::thread thread_;        
        std::unique_ptr<socket> socket_;
        protocol proto_;
        event shutdown_;
        event status_;
    };

} // newsflash


