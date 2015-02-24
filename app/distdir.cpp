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

#include <newsflash/warnpush.h>
#  include <QCoreApplication>
#  include <QDir>
#include <newsflash/warnpop.h>

#include <cstring>
#include "distdir.h"
#include "format.h"

namespace app
{

QString distdir::path()
{
    return QCoreApplication::applicationDirPath();
}

QString distdir::path(const QString& path)
{
    const auto base = QCoreApplication::applicationDirPath();
    const auto native = QDir::toNativeSeparators(base + "/" + path + "/");
    return QDir::cleanPath(native);
}

QString distdir::file(const QString& file)
{
    const auto base   = QCoreApplication::applicationDirPath();
    const auto native = QDir::toNativeSeparators(base + "/" + file);
    return QDir::cleanPath(native);
}

QString distdir::executable() 
{
    return QCoreApplication::applicationFilePath();
}

} // app