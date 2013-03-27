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

#ifndef MOVITSHARPENFILTER_H
#define MOVITSHARPENFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class MovitSharpenFilter;
}

class MovitSharpenFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit MovitSharpenFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~MovitSharpenFilter();
    
private slots:
    void on_preset_selected(void* p);
    
    void on_preset_saveClicked();
    
    void on_matrixSizeSpinner_valueChanged(int arg1);
    
    void on_circleRadiusSpinner_valueChanged(double arg1);
    
    void on_gaussianRadiusSpinner_valueChanged(double arg1);
    
    void on_correlationSpinner_valueChanged(double arg1);
    
    void on_correlationSlider_valueChanged(int value);
    
    void on_noiseLevelSpinner_valueChanged(double arg1);
    
    void on_noiseLevelSlider_valueChanged(int value);
    
    void on_defaultMatrixSizeButton_clicked();
    
    void on_defaultCircleRadiusButton_clicked();
    
    void on_defaultGaussianRadiusButton_clicked();
    
    void on_defaultCorrelationButton_clicked();
    
    void on_defaultNoiseLevelButton_clicked();
    
private:
    Ui::MovitSharpenFilter *ui;
    Mlt::Filter m_filter;
    int m_defaultMatrixSize;
    double m_defaultCircleRadius;
    double m_defaultGaussianRadius;
    double m_defaultCorrelation;
    double m_defaultNoiseLevel;
};

#endif // MOVITSHARPENFILTER_H
