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

#include <boost/crc.hpp>
#include "yenc_single_decoder.h"
#include "yenc.h"
#include "bodyiter.h"

namespace newsflash
{

yenc_single_decoder::yenc_single_decoder()
{}

yenc_single_decoder::~yenc_single_decoder()
{}

void yenc_single_decoder::decode(const void* data, std::size_t len)
{
    nntp::bodyiter beg((const char*)data);
    nntp::bodyiter end((const char*)data + len);

    const auto header = yenc::parse_header(beg, end);
    if (!header.first)
        throw decoder::exception("yenc: broken or missing header");

    if (on_info)
    {
        decoder::info info;
        info.name = header.second.name;
        info.size = header.second.size;
        on_info(info);
    }

    std::vector<char> buff;
    buff.reserve(header.second.size);

    yenc::decode(beg, end, std::back_inserter(buff));
    
    const auto footer = yenc::parse_end(beg, end);
    if (!footer.first)
        throw decoder::exception("yenc: broken or missing footer");

    on_write(&buff[0], buff.size(), 0);

    if (footer.second.crc32)
    {
        boost::crc_32_type crc;
        crc.process_bytes(&buff[0], buff.size());
        if (footer.second.crc32 != crc.checksum())
        {
            if (on_error)
                on_error("yenc: crc32 mismatch");
        }
    }
    if (footer.second.size != header.second.size)
    {
        if (on_error)
            on_error("yenc: size mismatch between binary size in footer and header");
    }

    if (footer.second.size != buff.size())
    {
        if (on_error)
            on_error("yenc: size mismatch between footer and actual binary size");
    }
}

void yenc_single_decoder::finish()
{}

void yenc_single_decoder::cancel()
{}

} // newsflash