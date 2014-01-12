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

// $Id: yenc.h,v 1.18 2010/02/22 22:14:07 svaisane Exp $

#pragma once

#include <boost/spirit/include/classic.hpp>
#include <string>
#include <iterator>
#include <vector>
#include <cassert>

// yEnc decoder/encoder implementation.
// http://www.yenc.org/

namespace yenc
{
    // yEnc header precedes the yEnc encoded data.
    // In case of a multipart data the part field will 
    // be present and have a non-zero value after successful parsing.
    struct header {
        size_t part;            // the number of the part when the yenc content is a part of a multipart binary. (not available in single part binaries)
        size_t line;            // typical line length.
        size_t size;            // the size of the original binary (in bytes)
        size_t total;           // the total number of parts in a multipart binary. (only available in yEnc 1.2)
        std::string name;       // the filename of the encoded binary
    };

    // yEnc part header. 
    // ypart specifies the starting and ending points (in bytes) of the block
    // in the original file    
    struct part {
        size_t begin; // starting point
        size_t end;   // ending point
    };

    // yEnd end line follows the yEnc encoded data.
    // In case of a multipart data the part field will
    // be present and have a non-zero value after succesful parsing. 
    struct end {
        size_t size;
        size_t part; 
        size_t crc32;
        size_t pcrc32;
    };
    

    // parse yEnc header. returns {true, header} if succesful, otherwise {false, header} and
    // contents of header are unspecified.
    template<typename InputIterator>
    std::pair<bool, header> parse_header(InputIterator& beg, InputIterator end)
    {
        using namespace boost::spirit::classic;
        
        yenc::header header {0, 0, 0, 0, ""};

        chset<> name_p(chset<>(anychar_p) - '\r' - '\n');
        const auto ret = parse(beg, end,
           (
            str_p("=ybegin") >>
            !(str_p("part=")  >> uint_p[assign(header.part)])    >> // not included in single part headers
            !(str_p("total=") >> uint_p[assign(header.total)])   >> // only in yEnc 1.2
            ((str_p("line=")  >> uint_p[assign(header.line)]) | (str_p("size=")  >> uint_p[assign(header.size)])) >>
            ((str_p("line=")  >> uint_p[assign(header.line)]) | (str_p("size=")  >> uint_p[assign(header.size)])) >>
            (str_p("name=")  >> (*name_p)[assign(header.name)])   >>
            !eol_p
            ), ch_p(' '));
        
        beg = ret.stop;

        return {ret.hit, header};
    }

    // parse yEnc part. returns { true, part } if succesful, otherwise { false, part  } and
    // the contents of the part are unspecified.
    template<typename InputIterator>
    std::pair<bool, part> parse_part(InputIterator& beg, InputIterator end)
    {
        using namespace boost::spirit::classic;

        yenc::part part {0, 0};

        const auto ret = parse(beg, end, 
          (
           str_p("=ypart")  >>
           (str_p("begin=") >> uint_p[assign(part.begin)]) >>
           (str_p("end=")   >> uint_p[assign(part.end)]) >>
           !eol_p
           ), ch_p(' '));

        beg = ret.stop;

        return {ret.hit, part};
    }


    template<typename InputIterator>
    std::pair<bool, yenc::end> parse_end(InputIterator& beg, InputIterator end)
    {
        using namespace boost::spirit::classic;
        
        yenc::end yend {0, 0, 0, 0};

        const auto ret = parse(beg, end, 
          (                                                
           str_p("=yend") >>
           (str_p("size=") >> uint_p[assign(yend.size)]) >>
           !(str_p("part=") >> uint_p[assign(yend.part)]) >>
           !(str_p("pcrc32=") >> hex_p[assign(yend.pcrc32)]) >>
           !(str_p("crc32=") >> hex_p[assign(yend.crc32)]) >> 
           !eol_p
          ), space_p);

        beg = ret.stop;

        return {ret.hit, yend};
    }

    // Decode yEnc encoded data back into its original binary representation.
    // This function always returns end.
    template<typename InputIterator, typename OutputIterator>
    std::pair<bool, InputIterator> decode(InputIterator beg, InputIterator end, OutputIterator dest)
    {
        // todo: some error checking perhaps?
        while (beg != end)
        {
            unsigned char c = *beg;
            if( c == '\r' || c == '\n') 
            {
                ++beg;
                continue;
            }
            else if (c == '=')
            {
                InputIterator tmp(beg);
                if (++beg == end)
                {
                    *dest++ = c-42;
                    return { true, end };
                }
                else if (*beg == 'y')
                    return { true, tmp};

                c  = *beg;
                c -= 64;
            }
            *dest++ = c - 42;
            ++beg;
        }
        return {true, end};
    }

    template<typename InputIterator>
    std::pair<bool, InputIterator> decode(InputIterator beg, InputIterator end, std::vector<char>& dest)
    {
        // yEnc enlarges the data when its encoded, so the output should always 
        // be less than the yEnc input
        if (dest.empty())
            dest.resize(end - beg);

        size_t bytes = 0;

        while (beg != end)
        {
            unsigned char c = *beg;
            if (c == '\r' || c == '\n')
            {
                ++beg;
                continue;
            }
            else if (c == '=')
            {
                if (++beg == end)
                {
                    dest[bytes++] = c - 42;
                    assert(bytes < dest.size());
                    dest.resize(bytes);
                    return { true, end };
                }
                if (*beg == 'y')
                {
                    assert(bytes < dest.size());
                    dest.resize(bytes);
                    return --beg;
                }
                c  = *beg;
                c -= 64;
            }
            dest[bytes++] = c - 42;
            ++beg;
        }
        assert(bytes < dest.size());
        dest.resize(bytes);

        return { true, end };
    }


    // Encode data into yEnc. Line should be the preferred
    // line length after wich a new line (\r\n) is written into the output stream.
    template<typename InputIterator, typename OutputIterator>
    void encode(InputIterator beg, InputIterator end, OutputIterator dest, const int line = 128)
    {
        int output = 0;
        while (beg != end)
        {
            int val = (*beg + 42) % 256;
            // these are characters that may not appear in 
            // yEnc encoded content without being "escaped".
            if (val == 0x00 || val == 0x0A || val == 0x0D || val == 0x3D || val == 0x09)
            {
                ++output;
                *dest++ = '=';
                val = (val + 64) % 256;
            }
            *dest++ = static_cast<char>(val);
            ++output;
            if (output >= line)
            {
                *dest++ = '\r';
                *dest++ = '\n';
                output = 0;
            }
            ++beg;
        }
    }

} // yEnc
