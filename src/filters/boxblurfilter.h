/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOXBLURFILTER_H
#define BOXBLURFILTER_H

#include <QWidget>
#include <MltFilter.h>

namespace Ui {
class BoxblurFilter;
}

class BoxblurFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit BoxblurFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~BoxblurFilter();
    
private slots:
    void on_widthSpinBox_valueChanged(int arg1);
    
    void on_heightSpinBox_valueChanged(int arg1);
    
    void on_defaultWidthButton_clicked();
    
    void on_defaultHeightButton_clicked();
    
private:
    Ui::BoxblurFilter *ui;
    Mlt::Filter m_filter;
    int m_defaultWidth;
    int m_defaultHeight;
};

#endif // BOXBLURFILTER_H
