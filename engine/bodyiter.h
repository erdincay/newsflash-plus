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

#include <iterator>
#include <cstddef>

namespace nntp
{
    // iterate over NNTP body and collapse leading double dots
    // to single dots. 
    class bodyiter : public
        std::iterator<std::forward_iterator_tag, char>
    {
    public:
        bodyiter(const char* ptr) : ptr_(ptr), pos_(0)
        {}


        char operator * () const
        {
            return ptr_[pos_];
        }

        const char* operator -> () const
        {
            return &ptr_[pos_];
        }

        // postfix
        bodyiter operator++(int)
        {
            bodyiter tmp(*this);
            forward();
            return *this;
        }
        bodyiter& operator++()
        {
            forward();
            return *this;
        }
        bool operator==(const bodyiter& other) const
        {
            return ptr_ + pos_ == other.ptr_ + other.pos_;
        }
        bool operator!=(const bodyiter& other) const
        {
            return !(other == *this);
        }
    private:
        bool double_dot() const
        {
            if (pos_ > 3)
            {
                if (ptr_[pos_] == '.' && ptr_[pos_-1] == '.' && ptr_[pos_-2] == '\n' && ptr_[pos_-3] == '\r')
                    return true;
            }
            else if (pos_ == 1)
            {
                if (ptr_[pos_] == '.' && ptr_[pos_-1] == '.')
                    return true;
            }

            return false;
        }

        void forward()
        {
            pos_++;
            if (double_dot())
                pos_++;
        }

    private:
        const char* ptr_;
        std::size_t pos_;
    };

} // nntp
