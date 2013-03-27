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

#ifndef FREI0RSHARPNESSFILTER_H
#define FREI0RSHARPNESSFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class Frei0rSharpnessFilter;
}

class Frei0rSharpnessFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit Frei0rSharpnessFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~Frei0rSharpnessFilter();
    
private slots:
    void on_amountSpinner_valueChanged(double arg1);
    
    void on_sizeSpinner_valueChanged(double arg1);
    
    void on_defaultAmountButton_clicked();
    
    void on_defaultSizeButton_clicked();
    
    void on_amountSlider_valueChanged(int value);
    
    void on_sizeSlider_valueChanged(int value);
    
private:
    Ui::Frei0rSharpnessFilter *ui;
    Mlt::Filter m_filter;
    double m_defaultAmount;
    double m_defaultSize;
};

#endif // FREI0RSHARPNESSFILTER_H
