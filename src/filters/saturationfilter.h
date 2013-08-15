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

#ifndef SATURATIONFILTER_H
#define SATURATIONFILTER_H

#include <QWidget>
#include <MltFilter.h>

namespace Ui {
class SaturationFilter;
}

class SaturationFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit SaturationFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~SaturationFilter();
    
private slots:
    void on_doubleSpinBox_valueChanged(double arg1);
    
    void on_horizontalSlider_valueChanged(int value);
    
    void on_pushButton_clicked();
    
private:
    Ui::SaturationFilter *ui;
    Mlt::Filter m_filter;
    bool m_isMovit;
};

#endif // SATURATIONFILTER_H
