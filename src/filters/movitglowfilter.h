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

#ifndef MOVITGLOWFILTER_H
#define MOVITGLOWFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class MovitGlowFilter;
}

class MovitGlowFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit MovitGlowFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~MovitGlowFilter();
    
private slots:
    void on_radiusSpinBox_valueChanged(double arg1);
    
    void on_blurMixSpinBox_valueChanged(double arg1);
    
    void on_highlightCutoffSpinBox_valueChanged(double arg1);
    
    void on_radiusDefaultButton_clicked();
    
    void on_blurMixDefaultButton_clicked();
    
    void on_cutoffDefaultButton_clicked();
    
    void on_preset_selected(void* p);
    
    void on_preset_saveClicked();

private:
    Ui::MovitGlowFilter *ui;
    Mlt::Filter m_filter;
    double m_radiusDefault;
    double m_blurMixDefault;
    double m_cutoffDefault;
};

#endif // MOVITGLOWFILTER_H
