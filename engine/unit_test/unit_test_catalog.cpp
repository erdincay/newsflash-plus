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

#include <newsflash/config.h>

#include <boost/test/minimal.hpp>
#include <chrono>
#include "../filemap.h"
#include "../filebuf.h"
#include "../catalog.h"
#include "unit_test_common.h"

struct string {
    std::string s;

    string(const char* p) : s(p)
    {}
};

bool operator==(const char* p, const string& s)
{
    return std::strncmp(p, s.s.c_str(), s.s.size()) == 0;
}

std::size_t length(const string& s)
{
    return s.s.size();
}


#define S(x) string(x)
#define L(x) length(string(x))

void unit_test_create_new()
{
    using catalog = newsflash::catalog<newsflash::filebuf>;

    delete_file("file");

    // create a new database
    {
        catalog db;
        db.open("file");

        BOOST_REQUIRE(db.first_article_number() == 0);
        BOOST_REQUIRE(db.last_article_number() == 0);
        BOOST_REQUIRE(db.num_articles() == 0);

        catalog::article a;
        a.ptr_author  = "John Doe";
        a.len_author  = std::strlen(a.ptr_author);
        a.ptr_subject = "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)";
        a.len_subject = std::strlen(a.ptr_subject);
        a.state.set(catalog::flags::is_broken);
        a.state.set(catalog::flags::is_binary);
        db.insert(a);

        BOOST_REQUIRE(db.num_articles() == 1);

        a.ptr_author  = "Mickey mouse";
        a.len_author  = std::strlen(a.ptr_author);
        a.ptr_subject = "Mickey and Goofy in Disneyland";
        a.len_subject = std::strlen(a.ptr_subject);
        a.state.clear();
        a.state.set(catalog::flags::is_downloaded);
        db.insert(a);

        BOOST_REQUIRE(db.num_articles() == 2);

        db.flush();
    }

    // open an existing
    {
        catalog db;
        db.open("file");

        BOOST_REQUIRE(db.num_articles() == 2);        

        auto a = db.lookup(0);
        BOOST_REQUIRE(a.state.test(catalog::flags::is_broken));
        BOOST_REQUIRE(a.state.test(catalog::flags::is_binary));
        BOOST_REQUIRE(a.ptr_author == S("John Doe"));
        BOOST_REQUIRE(a.len_author == L("John Doe"));
        BOOST_REQUIRE(a.ptr_subject == S("[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)"));
        BOOST_REQUIRE(a.len_subject == L("[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)"));

        a = db.lookup(a.next());
        BOOST_REQUIRE(a.state.test(catalog::flags::is_downloaded));
        BOOST_REQUIRE(a.ptr_author == S("Mickey mouse"));
        BOOST_REQUIRE(a.len_author == L("Mickey mouse"));
        BOOST_REQUIRE(a.ptr_subject == S("Mickey and Goofy in Disneyland"));
        BOOST_REQUIRE(a.len_subject == L("Mickey and Goofy in Disneyland"));
    }

    delete_file("file");
}

void unit_test_performance()
{
    delete_file("file");

    // write
    {
        using catalog = newsflash::catalog<newsflash::filebuf>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        catalog::article a;
        a.ptr_author = "John Doe";
        a.len_author = std::strlen(a.ptr_author);
        a.ptr_subject = "[#scnzb@efnet][529762] Automata.2014.BRrip.x264.Ac3-MiLLENiUM [1/4] - \"Automata.2014.BRrip.x264.Ac3-MiLLENiUM.mkv\" yEnc (1/1513)";
        a.len_subject = std::strlen(a.ptr_subject);

        for (std::size_t i=0; i<1000000; ++i)
        {
            db.insert(a);
        }

        db.flush();

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Writing time spent (ms): " << ms.count() << std::endl;


    }

    // read with filebuf
    {
        using catalog = newsflash::catalog<newsflash::filebuf>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        catalog::article a;
        catalog::key_t key = 0;
        for (std::size_t i=0; i<1000000; ++i)
        {
            a = db.lookup(key);
            key = a.next();
        }

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Reading with filebuf Time spent (ms): " << ms.count() << std::endl;
    }

    // read with filemap
    {
        using catalog = newsflash::catalog<newsflash::filemap>;

        catalog db;
        db.open("file");

        const auto beg = std::chrono::steady_clock::now();

        catalog::article a;
        catalog::key_t key = 0;
        for (std::size_t i=0; i<1000000; ++i)
        {
            a = db.lookup(key);
            key = a.next();
        }

        const auto end = std::chrono::steady_clock::now();
        const auto diff = end - beg;
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Reading with filemap Time spent (ms): " << ms.count() << std::endl;        
    }

}

int test_main(int, char*[])
{
    //unit_test_create_new();

    unit_test_performance();

    return 0;
}