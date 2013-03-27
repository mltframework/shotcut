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

#ifndef FREI0RCOLORADJWIDGET_H
#define FREI0RCOLORADJWIDGET_H

#include <QWidget>
#include <mlt++/MltFilter.h>

namespace Ui {
class Frei0rColoradjWidget;
}

class Frei0rColoradjWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit Frei0rColoradjWidget(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~Frei0rColoradjWidget();
    
private slots:
    void on_preset_selected(void* p);
    void on_preset_saveClicked();
    void on_wheel_colorChanged(const QColor &color);

    void on_modeComboBox_currentIndexChanged(int index);
    
    void on_keepLumaCheckBox_toggled(bool checked);
    
private:
    Ui::Frei0rColoradjWidget *ui;
    Mlt::Filter m_filter;
    double m_defaultAction;
    bool m_defaultLuma;
    QColor m_defaultRGB;
};

#endif // FREI0RCOLORADJWIDGET_H
