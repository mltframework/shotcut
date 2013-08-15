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

#ifndef MOVITCOLORFILTER_H
#define MOVITCOLORFILTER_H

#include <QWidget>
#include <MltFilter.h>

namespace Ui {
class MovitColorFilter;
}

class MovitColorFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit MovitColorFilter(Mlt::Filter filter, bool setDefaults = false, QWidget *parent = 0);
    ~MovitColorFilter();
    
private slots:
    void on_liftWheel_colorChanged(const QColor &color);
    void on_gammaWheel_colorChanged(const QColor &color);
    void on_gainWheel_colorChanged(const QColor &color);
    void on_preset_selected(void* p);
    void on_preset_saveClicked();

private:
    Ui::MovitColorFilter *ui;
    Mlt::Filter m_filter;
    QColor m_liftDefault;
    QColor m_gammaDefault;
    QColor m_gainDefault;
};

#endif // MOVITCOLORFILTER_H
