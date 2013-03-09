/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#ifndef MOVITBLURFILTER_H
#define MOVITBLURFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class MovitBlurFilter;
}

class MovitBlurFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit MovitBlurFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~MovitBlurFilter();

private slots:
    void on_doubleSpinBox_valueChanged(double arg1);
    
    void on_pushButton_clicked();
    
private:
    Ui::MovitBlurFilter *ui;
    Mlt::Filter m_filter;
    double m_radiusDefault;
};

#endif // MOVITBLURFILTER_H
