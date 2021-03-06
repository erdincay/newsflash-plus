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

#include <newsflash/config.h>

#include <newsflash/warnpush.h>
#  include <boost/filesystem/path.hpp>
#  include <boost/filesystem/operations.hpp>
#include <newsflash/warnpop.h>
#include <sstream>
#include <algorithm>
#include "filesys.h"

namespace bfs = boost::filesystem;

namespace fs
{

#if defined(WINDOWS_OS)
bool is_illegal_filename_char(int c)
{
    switch (c)
    {
        case '\\':
        case '/':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
        case '^': // FAT limitation
        return true;
    }
    return false;
}
bool is_illegal_filepath_char(int c)
{
    return is_illegal_filename_char(c);
}

#elif defined(LINUX_OS)
bool is_illegal_filename_char(int c)
{
    switch (c)
    {
        case '/':
        case '"':
        case '<':
        case '>':
        case '|':
        case '!':
        return true;
    }
    return false;
}
bool is_illegal_filepath_char(int c)
{
    if (c == '/' || c == '\\')
        return false;

    return is_illegal_filename_char(c);
}

#endif


std::string remove_illegal_filename_chars(std::string name)
{
    const auto it = std::remove_if(name.begin(), name.end(), 
        is_illegal_filename_char);
    name.erase(it, name.end());
    return name;
}

std::string remove_illegal_filepath_chars(std::string name)
{
    const auto it = std::remove_if(name.begin(), name.end(),
        is_illegal_filepath_char);
    name.erase(it, name.end());
    return name;
}

std::string joinpath(const std::string& path, const std::string& str)
{
    if (str.empty())
        return path;

    bfs::path p(path);
    p /= str;
    return p.string();
}

std::string filename(int attempt, const std::string& name)
{
    if (attempt == 0)
        return name;

    std::stringstream ss;
    ss << "(" << attempt+1 << ") " << name;
    return ss.str();
}

std::string createpath(const std::string& path)
{
    if (path.empty())
        return path;
    else if (path == "." || path == "./")
        return path;

    const bfs::path p(path);
    bfs::create_directories(p);
    return p.string();
}

bool exists(const std::string& path)
{
    bfs::path p(path);
    return bfs::exists(p);    
}

} // fs