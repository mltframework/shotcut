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

#ifndef FREI0RGLOWFILTER_H
#define FREI0RGLOWFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class Frei0rGlowFilter;
}

class Frei0rGlowFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit Frei0rGlowFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~Frei0rGlowFilter();
    
private slots:
    void on_spinBox_valueChanged(int arg1);
    
    void on_pushButton_clicked();
    
private:
    Ui::Frei0rGlowFilter *ui;
    Mlt::Filter m_filter;
    int m_defaultBlur;
};

#endif // FREI0RGLOWFILTER_H
