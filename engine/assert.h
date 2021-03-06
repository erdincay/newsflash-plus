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
#include <sstream>
#include <string>

namespace debug 
{
    // check if running in debugger
    bool has_debugger();

    NORETURN 
    void do_assert(const char* expression, const char* file, const char* func, int line);

    NORETURN 
    void do_break();

} // debug

// What is an assert and what is an ASSERT?
// assert is the standard C utility which by default
// is elided when a release build is made (_NDEBUG is defined).
// This can be sufficient for cases where the assert is 
// only to quickly remind the programmer of having done something
// silly. In other words it's a development time utility. 
// However when checking for conditions whose violation could 
// have severe consequences for the process (e.g undefined behaviour)
// we have ASSERT that is *always* compiled in.
//
// To throw or to dump core?
// Throwing an exception might seem like a good idea but it has 
// few drawbacks. It's might be difficult to know which part of the
// system has failed, hence doing operations such as saving current
// state might cause further assertion failures. Also Keep in mind
// that to throw an exception causes the stack to be unwound
// which will a) potentially execute loads of code b) unwind the callstack 
// c) alter the state of the program making it harder to do post-mortem
// diagnosis based on the core-dump.
#define ASSERT(expr) \
    (expr) \
    ? ((void)0) \
    : (debug::has_debugger() ? \
        debug::do_break() : \
        debug::do_assert(#expr, __FILE__, __PRETTY_FUNCTION__, __LINE__))

#ifdef _NDEBUG
#  define CHECK(func, retval) \
    do { \
        const auto ret = func; \
        assert(ret == retval); \
    } while (0)
#else
#  define CHECK(func, retval) \
    do { \
        func; \
    } while(0)
#endif
