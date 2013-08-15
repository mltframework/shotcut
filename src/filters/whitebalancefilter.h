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

#ifndef WHITEBALANCEFILTER_H
#define WHITEBALANCEFILTER_H

#include <QWidget>
#include <MltFilter.h>

namespace Ui {
class WhiteBalanceFilter;
}

class WhiteBalanceFilter : public QWidget
{
    Q_OBJECT
    
public:
    explicit WhiteBalanceFilter(Mlt::Filter &filter, bool setDefaults = false, QWidget *parent = 0);
    ~WhiteBalanceFilter();
    
private slots:
    void on_colorPicker_colorPicked(const QColor& color);
    void on_colorPicker_disableCurrentFilter(bool disable);

    void on_defaultTemperatureButton_clicked();

    void on_colorTemperatureSpinner_valueChanged(double arg1);

    void on_defaultNeutralButton_clicked();

private:
    Ui::WhiteBalanceFilter *ui;
    Mlt::Filter m_filter;
    bool m_isMovit;
    QString m_defaultColor;
    double m_defaultTemperature;
};

#endif // WHITEBALANCEFILTER_H
