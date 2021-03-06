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

#include <system_error>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdint>
#include "action.h"

namespace newsflash
{
    class cmdlist;
    class throttle;

    class connection
    {
        struct state;
    public:
        enum class error {
            resolve,
            authentication_rejected,
            no_permission,
            timeout,
            network,
            pipeline_reset
        };
        class exception : public std::exception
        {
        public:
            exception(connection::error err, std::string what) 
                : error_(err), what_(std::move(what))
            {}

            const char* what() const NOTHROW // noexcept
            { return what_.c_str(); }

            connection::error error() const NOTHROW // noexcept
            { return error_; }

        private:
            connection::error error_;
            std::string what_;
        };

        // perform host resolution
        class resolve : public action
        {
        public:
            resolve(std::shared_ptr<state> s) : state_(s)
            {}

            virtual void xperform() override;

            virtual std::string describe() const override;            
        private:
            std::shared_ptr<state> state_;
        };

        // perform socket connect 
        class connect : public action
        {
        public:
            connect(std::shared_ptr<state> s);

            virtual void xperform() override;

            virtual std::string describe() const override;            
        private:
            std::shared_ptr<state> state_;
        };

        // perform nntp init
        class initialize : public action
        {
        public:
            initialize(std::shared_ptr<state> s);

            virtual void xperform() override;

            virtual std::string describe() const override;
        private:
            std::shared_ptr<state> state_;
        };

        // execute cmdlist 
        class execute : public action
        {
        public:
            execute(std::shared_ptr<state> s, std::shared_ptr<cmdlist> cmds, std::size_t tid);

            virtual void xperform() override;

            virtual std::string describe() const override;

            std::shared_ptr<cmdlist> get_cmdlist() const
            { return cmds_; }

            std::size_t get_tid() const 
            { return tid_; }

            // get the total number of data bytes transferred (downloaded)
            // from the server. this includes some protocol data.
            std::size_t get_bytes_transferred() const
            { return bytes_; }

            // get the total number of content bytes transferred (downloaded)
            // from the server. this is only the payload data.
            std::size_t get_content_transferred() const 
            { return content_; }

        private:
            std::shared_ptr<state> state_;
            std::shared_ptr<cmdlist> cmds_;
            std::size_t tid_;
            std::size_t bytes_;
            std::size_t content_;
        };

        class disconnect : public action
        {
        public:
            disconnect(std::shared_ptr<state> s);

            virtual void xperform() override;

            virtual std::string describe() const override;
            
        private:
            std::shared_ptr<state> state_;
        };

        class ping : public action
        {
        public:
            ping(std::shared_ptr<state> s);

            virtual void xperform() override;

            virtual std::string describe() const override;

        private:
            std::shared_ptr<state> state_;
        };

        struct spec {
            std::string username;
            std::string password;
            std::string hostname;
            std::uint16_t hostport;         
            bool use_ssl;
            bool enable_pipelining;
            bool enable_compression;
            throttle* pthrottle;
        };

        // begin connecting to the given host specification.
        std::unique_ptr<action> connect(spec s);

        // perform disconnect from the host. 
        std::unique_ptr<action> disconnect();

        // perform ping
        std::unique_ptr<action> ping();

        // complete the given action. returns a new action if any.
        std::unique_ptr<action> complete(std::unique_ptr<action> a);

        // execute the given cmdlist
        std::unique_ptr<action> execute(std::shared_ptr<cmdlist> cmd, std::size_t tid);

        // cancel a pending operation in the connection such as connecting
        // or executing a cmdlist. note that if the operation is excute
        // cancelling the connection does not cancel the cmdlist!
        void cancel();

        // some getters
        const std::string& username() const;

        const std::string& password() const;

        std::uint32_t current_speed_bps() const;

        std::uint64_t num_bytes_transferred() const;
    private:
        std::shared_ptr<state> state_;

    };

    


} // newsflash


