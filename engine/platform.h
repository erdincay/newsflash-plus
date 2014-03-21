// Copyright (c) 2013 Sami Väisänen, Ensisoft 
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

#include <string>
#include <iosfwd>
#include "types.h"

// this file contains an assortment of platform specific global functions
// and associated types.

namespace engine
{
    // get a platform provided human readable error string.
    std::string get_error_string(int code);

    struct localtime {
        size_t millis;
        size_t seconds;        
        size_t minutes;
        size_t hours;
    };

    localtime get_localtime();

    unsigned long get_thread_identity();

    // doesn't work with libstd++
    //std::ofstream open_fstream(const std::string& filename);

    std::ofstream& open_fstream(const std::string& filename, std::ofstream& stream);

    void throw_system_error(int code, std::string what);

} // engine

