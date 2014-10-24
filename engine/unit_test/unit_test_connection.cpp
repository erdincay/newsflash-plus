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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include <thread>
#include <deque>
#include <string>
#include <fstream>
#include <iterator>
#include "../connection.h"
#include "../action.h"
#include "../sockets.h"
#include "../socketapi.h"
#include "../cmdlist.h"
#include "../session.h"
#include "../buffer.h"

namespace nf = newsflash;

#define REQUIRE_EXCEPTION(x) \
    try { \
        x; \
        BOOST_FAIL("exception was expected"); \
    } catch(const std::exception& e) {}


void test_connect()
{
    std::unique_ptr<nf::action> act;

    // resolve fails
    {
        nf::connection::spec s;
        s.hostname = "foobar";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::connection conn;

        act = conn.connect(s);
        act->perform();
        BOOST_REQUIRE(act->has_exception());
    }

    // connect fails
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1818;
        s.use_ssl  = false;

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->perform();
        BOOST_REQUIRE(!act->has_exception());

        // connect
        act = conn.complete(std::move(act));
        act->perform();
        BOOST_REQUIRE(act->has_exception());
    }

    // nntp init fails
    // {
    //     //auto sp = open_server();

    //     nf::connection::server serv;
    //     serv.hostname = "localhost";
    //     serv.port     = 1919;
    //     serv.use_ssl  = false;
    //     conn.connect(serv);
    //     act->perform(); // resolve
    //     conn.complete(std::move(act));

    //     act->perform(); // connect
    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::initializing);
    //     act->perform(); // initialize
    //     BOOST_REQUIRE(act->has_exception());
    //     conn.complete(std::move(act));
    //     BOOST_REQUIRE(conn.get_state() == state::error);
    //     BOOST_REQUIRE(conn.get_error() == error::other);

    //     LOG_D("end");        
    //     LOG_FLUSH();
    // }

    // authentication fails
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "fail";
        s.password = "fail";

        nf::connection conn;

        // resolve
        act = conn.connect(s); 
        act->perform(); 
        act = conn.complete(std::move(act));

        // connect
        act->perform(); 
        act = conn.complete(std::move(act));

        // initialize
        act->perform(); 
        BOOST_REQUIRE(act->has_exception());
    }

    // disconnect
    {
        // nf::connection::spec s;
        // s.hostname = "localhost";
        // s.hostport     = 1919;
        // s.use_ssl  = false;
        // s.username = "pass";
        // s.password = "pass";

        // nf::connection conn;

        // conn.connect();
        // act->perform(); // resolve
        // conn.complete(std::move(act));

        // conn.disconnect();
        // act->perform(); // connect
        // conn.complete(std::move(act));

        // BOOST_REQUIRE(conn.get_state() == state::disconnected);
        // BOOST_REQUIRE(conn.get_error() == error::none);
    }

    // succesful connect
    {
        nf::connection::spec s;
        s.hostname = "localhost";
        s.hostport = 1919;
        s.use_ssl  = false;
        s.username = "pass";
        s.password = "pass";

        nf::connection conn;

        // resolve
        act = conn.connect(s);
        act->perform();
        act = conn.complete(std::move(act)); // resolve

        // connect
        act->perform(); 
        act = conn.complete(std::move(act)); 

        // initialize
        act->perform();
        BOOST_REQUIRE(!act->has_exception());

    }

    // discard the connection object while connecting
    {
        // nf::connection::spec s;
        // s.account = 1;
        // s.id      = 1;
        // s.hostname = "localhost";
        // s.port     = 1919;
        // s.use_ssl  = false;
        // s.username = "pass";
        // s.password = "pass";

        // std::unique_ptr<nf::connection> conn(new nf::connection(s));
        // conn->on_action = [&](std::unique_ptr<nf::action> a) {
        //     act = std::move(a);
        // };

        // conn->connect();
        // act->perform(); 
        // conn->complete(std::move(act)); // resolve.

        // // spawn a new thread that executes the action the background
        // // meanwhile the main thread destroys the object.
        // std::thread thread([&]() {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //     act->perform();
        // });
        
        // conn->disconnect();
        // conn.reset();
        // thread.join();
    }
}

void test_execute()
{
    // std::unique_ptr<nf::action> act;

    // nf::connection conn(1, 1);
    // conn.on_action = [&](std::unique_ptr<nf::action> a) {
    //     act = std::move(a);
    // };

    // using state = nf::connection::state;
    // using error = nf::connection::error;

    // nf::connection::server serv;
    // serv.hostname = "localhost";
    // serv.port = 1919;
    // serv.use_ssl = false;
    // serv.username = "pass";
    // serv.password = "pass";
    // conn.connect(serv);

    // act->perform(); // resolve
    // conn.complete(std::move(act));

    // act->perform(); // connect
    // conn.complete(std::move(act));

    // act->perform(); // initialize
    // conn.complete(std::move(act));

    // BOOST_REQUIRE(conn.get_state() == state::connected);
    // BOOST_REQUIRE(conn.get_error() == error::none);

    // class cmdlist : public nf::cmdlist 
    // {
    // public:
    //     cmdlist() : buffers_(0), content_len_(0)
    //     {}

    //     virtual bool is_done(nf::cmdlist::step step) const  override
    //     {
    //         if (step == cmdlist::step::configure)
    //             return true;

    //         return buffers_ == 2;
    //     }
    //     virtual bool is_good(nf::cmdlist::step step) const override
    //     {
    //         return true;
    //     }

    //     virtual void submit(nf::cmdlist::step step, nf::session& session) override
    //     {
    //         if (step == cmdlist::step::configure)
    //             return;

    //         // see server.cpp
    //         session.retrieve_article("1");
    //         session.retrieve_article("4");
    //     }

    //     virtual void receive(nf::cmdlist::step, nf::buffer&& buffer) override
    //     {
    //         if (buffers_ == 0)
    //         {
    //             BOOST_REQUIRE(buffer.content_status() == nf::buffer::status::unavailable);
    //         }
    //         else if (buffers_ == 1)
    //         {
    //             // note the dot doublings in the content
    //             std::string content = 
    //                 "here is some textual content\r\n"
    //                 "first line.\r\n"
    //                 ".. second line starts with a dot\r\n"
    //                 "... two dots\r\n"
    //                 "foo . bar\r\n"
    //                 "....\r\n"
    //                 "end\r\n"
    //                 ".\r\n";
    //             BOOST_REQUIRE(buffer.content_status() == nf::buffer::status::success);
    //             BOOST_REQUIRE(buffer.content_length() == content.size());
    //             BOOST_REQUIRE(!std::strncmp(buffer.content(), content.c_str(),
    //                 content.size()));
    //             content_len_ = content.size();
    //         }
    //         buffers_++;
    //     }

    //     virtual void next(nf::cmdlist::step) 
    //     {}

    //     virtual std::size_t account() const override
    //     { return 0; }

    //     virtual std::size_t task() const override
    //     { return 0; }
    // public:
    //     int buffers_;
    //     int content_len_;
    // };

    // cmdlist test;

    // conn.execute("test", 123, &test);
    // BOOST_REQUIRE(conn.get_state() == state::active);

    // auto ui = conn.get_ui_state();
    // BOOST_REQUIRE(ui.desc == "test");
    // BOOST_REQUIRE(ui.task == 123);

    // act->perform(); // execute cmdlist
    // conn.complete(std::move(act));

    // BOOST_REQUIRE(conn.get_state() == state::connected);
    // BOOST_REQUIRE(conn.get_error() == error::none);

    // ui = conn.get_ui_state();
    // BOOST_REQUIRE(ui.desc == "");
    // BOOST_REQUIRE(ui.task == 0);
    // BOOST_REQUIRE(ui.down >= test.content_len_);
}

void test_cancel_execute()
{
    // todo:
}

int test_main(int argc, char* argv[])
{
    test_connect();
    test_execute();
    test_cancel_execute();

    return 0;
}
