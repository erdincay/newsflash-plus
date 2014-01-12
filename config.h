// Copyright (c) 2010-2013 Sami Väisänen, Ensisoft 
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

// software configuration options
// msvc for windows, (g++) or clang for linux

#ifdef _MSC_VER
  #define WINDOWS_OS
  #define __MSVC__
  #ifdef _M_AMD64
    #define X86_64
  #endif

  // minimum supported version, windows Server 2003 and Windows XP
  #define _WIN32_WINNT 0x0501 

  // exclude some stuff we don't need
  #define WIN32_LEAN_AND_MEAN 

  #define __PRETTY_FUNCTION__ __FUNCSIG__

  // disable the silly min,max macros 
  #define NOMINMAX 

  // disable strcpy etc. warnings
  #define _CRT_SECURE_NO_WARNINGS
  #define _SCL_SECURE_NO_WARNINGS 

  // on windows use UTF8 in the engine api to pass strings
  // and then use Unicode win32 API
  #define UNICODE

  // msvc2013 doesn't support noexcept. 
  // todo: is this smart, the semantics are not *exactly* the same...
  #define  NOTHROW throw() 
#elif defined(__GNUG__)
  #define LINUX_OS
  #ifdef __LP64__
    #define X86_64
  #endif
  #define NOTHROW noexcept
#elif defined(__clang__)
  #define __CLANG__
  #define LINUX_OS
  #ifdef __LP64__
    #define X86_64
  #endif
  #define NOTHROW noexcept
#endif

#ifndef _NDEBUG
  // if we're in debug build enable more copius logging information
  #define NEWSFLASH_SCOPE_LOGGING
#endif


