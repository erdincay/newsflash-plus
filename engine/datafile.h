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
#include <string>
#include <atomic>
#include <vector>
#include <cassert>
#include "bigfile.h"
#include "action.h"
#include "filesys.h"

namespace newsflash
{
    class datafile 
    {
    public:
        class write : public action
        {
        public:
            // note that there's a little hack here and the offset is offset by +1
            // so that we're using 0 for indicating that offset is not being used at all.
            write(std::size_t offset, 
                std::vector<char> data, std::shared_ptr<datafile> file) : offset_(offset),
                data_(std::move(data)), file_(file)
            {
            #ifdef NEWSFLASH_DEBUG
                file_->num_writes_++;
            #endif
                set_affinity(affinity::single_thread);
            }

           ~write()
            {
            #ifdef NEWSFLASH_DEBUG
                file_->num_writes_--;
            #endif
            }
            virtual std::string describe() const override
            {
                return "Datafile::Write";
            }

            virtual void xperform() override
            {
                if (file_->discard_)
                    return;

                // see the comment in the constructor about the offset value
                if (offset_)
                    file_->big_.seek(offset_-1);

                file_->big_.write(&data_[0], data_.size());
            }
            std::size_t get_write_size() const 
            { return data_.size(); }

        private:
            std::size_t offset_;
            std::vector<char> data_;
            std::shared_ptr<datafile> file_;
        };

        // open datafile to an existing file
        datafile(std::string filepath, std::string filename, std::string dataname, bool bin) : discard_(false),
            filepath_(std::move(filepath)), filename_(std::move(filename)), dataname_(std::move(dataname)), binary_(bin), finalsize_(0)
        {
            big_.open(fs::joinpath(filepath_, filename_));

        #ifdef NEWSFLASH_DEBUG
            num_writes_ = 0;
        #endif
        }


        // create a new data file possibly overwriting existing files.
        datafile(std::string path, std::string binaryname, std::size_t size, bool bin, bool overwrite) : 
            discard_(false), binary_(bin), finalsize_(0)
        {
            std::string file;
            std::string name;
            binaryname = fs::remove_illegal_filename_chars(binaryname);
            // try to open the file at the given location under different
            // filenames unless ovewrite flag is set.
            for (int i=0; i<10; ++i)
            {
                name = fs::filename(i, binaryname);
                file = fs::joinpath(path, name);
                if (!overwrite && bigfile::exists(file))
                    continue;

                // todo: there's a race condition between the call to exists and the open below.

                big_.open(file, bigfile::o_create | bigfile::o_truncate);
                if (size)
                {
                    const auto error = bigfile::resize(file, size);
                    if (error)
                        throw std::system_error(error, "error resizing file: " + file);
                }
                break;
            }
            if (!big_.is_open())
                throw std::runtime_error("unable to create file at: " + path);

            filepath_ = path;
            filename_ = name;
            dataname_ = binaryname;
        #ifdef NEWSFLASH_DEBUG
            num_writes_ = 0;
        #endif
        }

       ~datafile()
        {
            close();
        }

        void close()
        {
            if (!big_.is_open())
                return;

        #ifdef NEWSFLASH_DEBUG
            assert(num_writes_ == 0);
        #endif
            finalsize_ = big_.size();

            big_.close();
            if (discard_)
                bigfile::erase(fs::joinpath(filepath_, filename_));
        }

        void discard_on_close()
        { discard_ = true; }

        std::uint64_t size() const 
        { return finalsize_; }
                    
        const std::string& filename() const
        { return filename_; }

        const std::string& filepath() const 
        { return filepath_; }

        const std::string& binary_name() const 
        { return dataname_; }

        bool is_binary() const 
        { return binary_; }

        bool is_open() const
        { return big_.is_open(); }

    private:
        friend class write;

        bigfile big_;
        std::atomic<bool> discard_;
        std::string filepath_;
        std::string filename_;
        std::string dataname_;
        bool binary_;
    #ifdef NEWSFLASH_DEBUG
        std::atomic<std::size_t> num_writes_;
    #endif
        std::uint64_t finalsize_;
    };

} // newsflash
