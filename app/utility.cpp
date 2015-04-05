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
#  include <QtGui/QTableView>
#  include <QtGui/QCheckBox>
#  include <QtGui/QLineEdit>
#  include <QtGui/QComboBox>
#  include <QtGui/QHeaderView>
#  include <QtGui/QPixmap>
#  include <QtGui/QImage>
#  include <QtGui/QIcon>
#  include <QAbstractTableModel>
#  include <QDir>
#include <newsflash/warnpop.h>
#include "utility.h"
#include "settings.h"

namespace app
{

QString joinPath(const QString& lhs, const QString& rhs)
{
    const auto p = lhs + "/" + rhs;
    return QDir::toNativeSeparators(QDir::cleanPath(p));
}

QPixmap toGrayScale(const QPixmap& p)
{
    QImage img = p.toImage();
    const int width  = img.width();
    const int height = img.height();
    for (int i=0; i<width; ++i)
    {
        for (int j=0; j<height; ++j)
        {
            const auto pix = img.pixel(i, j);
            const auto val = qGray(pix);
            img.setPixel(i, j, qRgba(val, val, val, (pix >> 24 & 0xff)));
        }
    }
    return QPixmap::fromImage(img);
}

QPixmap toGrayScale(const QString& pixmap)
{
    QPixmap pix(pixmap);
    if (pix.isNull())
        return {};

    return toGrayScale(pix);
}

void saveTableLayout(const QString& key, const QTableView* view, Settings& settings)
{
    const auto model    = view->model();
    const auto objName  = view->objectName();    
    const auto colCount = model->columnCount();

    // the last column is expected to have "stretching" enabled.
    // so can't explicitly set it's width. (will mess up the rendering) 

    for (int i=0; i<colCount - 1; ++i)
    {
        const auto name  = QString("%1_column_%2").arg(objName).arg(i);
        const auto width = view->columnWidth(i);
        settings.set(key, name, width);
    }

    if (view->isSortingEnabled())
    {
        const QHeaderView* header = view->horizontalHeader();
        const auto sortColumn = header->sortIndicatorSection();
        const auto sortOrder  = header->sortIndicatorOrder();
        settings.set(key, "sort_column", sortColumn);
        settings.set(key, "sort_order", sortOrder);
    }    
}

void loadTableLayout(const QString& key, QTableView* view, const Settings& settings)
{
    const auto model    = view->model();
    const auto objName  = view->objectName();    
    const auto colCount = model->columnCount();

    for (int i=0; i<colCount-1; ++i)
    {
        const auto name  = QString("%1_column_%2").arg(objName).arg(i);
        const auto width = settings.get(key, name, 
            view->columnWidth(i));
        view->setColumnWidth(i, width);
    }

    if (view->isSortingEnabled())
    {
        const QHeaderView* header = view->horizontalHeader();
        const auto column = settings.get(key, "sort_column",
            (int)header->sortIndicatorSection());
        const auto order = settings.get(key, "sort_order",
            (int)header->sortIndicatorOrder());

        view->sortByColumn(column,(Qt::SortOrder)order);
    }

}

void loadState(const QString& key, QCheckBox* chk, Settings& settings)
{
    const auto check = settings.get(key, chk->objectName(), 
        chk->isChecked());
    chk->setChecked(check);
}

void loadState(const QString& key, QLineEdit* edt, Settings& settings)
{
    const auto text = settings.get(key, edt->objectName(),
        edt->text());
    edt->setText(text);
    edt->setCursorPosition(0);
}

void loadState(const QString& key, QComboBox* cmb, Settings& settings)
{
    const auto text = settings.get(key, cmb->objectName(), 
        cmb->currentText());
    const auto index = cmb->findText(text);
    if (index != -1)
        cmb->setCurrentIndex(index);
}

void saveState(const QString& key, const QCheckBox* chk, Settings& settings)
{
    settings.set(key, chk->objectName(), chk->isChecked());
}

void saveState(const QString& key, const QLineEdit* edt, Settings& settings)
{
    settings.set(key, edt->objectName(), edt->text());
}

void saveState(const QString& key, const QComboBox* cmb, Settings& settings)
{
    settings.set(key, cmb->objectName(), cmb->currentText());
}

} // app
