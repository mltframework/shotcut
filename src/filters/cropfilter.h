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

#ifndef CROPFILTER_H
#define CROPFILTER_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class CropFilter;
}

class CropFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit CropFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~CropFilter();
    
private slots:
    void on_preset_selected(void* p);
    void on_preset_saveClicked();
    void on_centerCheckBox_toggled(bool checked);
    
    void on_biasSpinner_valueChanged(int arg1);
    
    void on_topSpinner_valueChanged(int arg1);
    
    void on_bottomSpinner_valueChanged(int arg1);
    
    void on_leftSpinner_valueChanged(int arg1);
    
    void on_rightSpinner_valueChanged(int arg1);
    
private:
    Ui::CropFilter *ui;
    Mlt::Filter m_filter;
};

#endif // CROPFILTER_H
