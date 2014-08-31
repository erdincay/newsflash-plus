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
#if defined(WINDOWS_OS)
#  include "windows.h"
#elif defined(LINUX_OS)
#  include "linux.h"
#endif

#include <cstdint>

namespace newsflash
{
    using ipv4addr_t = std::uint32_t;
    using ipv4port_t = std::uint16_t;

    struct kb {
        kb(std::uint64_t bytes) : value(bytes) 
        {}
        std::uint64_t value;
    };

    struct mb {
        mb(std::uint64_t bytes) : value(bytes) 
        {}
        std::uint64_t value;
    };

    struct gb {
        gb(std::uint64_t bytes) 
        {}
        std::uint64_t value;
    };

    struct ipv4 {
        ipv4(std::uint32_t ip) : addr(ip)
        {}
        std::uint32_t addr;
    };

} // newsflash