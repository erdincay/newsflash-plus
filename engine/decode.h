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
#include <stdexcept>
#include <string>
#include <vector>

#include "action.h"
#include "buffer.h"
#include "encoding.h"
#include "bitflag.h"

namespace newsflash
{
    // implement data decoding action. I.e. read input NNTP data and extract and decode
    // any ascii armored binary
    class decode : public action
    {
    public:
        // gets throw when decoding has encountered
        // an unrecoverable error and to continue is impossible
        class exception : public std::exception
        {
        public:
            exception(std::string what) : what_(std::move(what))
            {}

            const char* what() const noexcept
            { return what_.c_str(); }

        private:
            std::string what_;
        };       

        // a soft error. the binary might be unsable
        enum class error {
            crc_mismatch, size_mismatch
        };

        decode(buffer&& data);
       ~decode();

        // get text content buffer (if any)
        std::vector<char>&& get_text_data() &&
        { return std::move(text_); }

        // get the binary content (if any)
        std::vector<char>&& get_binary_data() &&
        { return std::move(binary_); }

        const 
        std::vector<char>& get_text_data() const &
        { return text_; }

        const 
        std::vector<char>& get_binary_data() const &
        { return binary_; }


        // if binary is part of a multipart binary then 
        // it might have a specific offset in the final output binary.
        // the caller should use this value only with encodings that
        // support such a scheme. (i.e. yenc multi)
        std::size_t get_binary_offset() const
        { return binary_offset_; }

        // get the binary size. note that in case of a multipart binary
        // that supports this, the value reported is the *whole* binary
        // size, not the size of the current part.
        std::size_t get_binary_size() const 
        { return binary_size_; }

        // get the binary file name
        std::string get_binary_name() const 
        { return binary_name_; }

        // get error flags set after the decoding.
        bitflag<error> get_errors() const 
        { return errors_; }

        // get the identified encoding. see comments in get_binary_offset
        encoding get_encoding() const
        { return encoding_; }

    private:
        virtual void xperform() override;

    private:
        std::size_t decode_yenc_single(const char* data, std::size_t len);
        std::size_t decode_yenc_multi(const char* data, std::size_t len);
        std::size_t decode_uuencode_single(const char* data, std::size_t len);
        std::size_t decode_uuencode_multi(const char* data, std::size_t len);

    private:
        buffer data_;
    private:
        std::vector<char> text_;
        std::vector<char> binary_;
        std::size_t binary_offset_;
        std::size_t binary_size_;
        std::string binary_name_;
    private:
        encoding encoding_;
    private:
        bitflag<error> errors_;
    };

} // newsflash