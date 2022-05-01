/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#ifndef ISINGWIDGET_H
#define ISINGWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
class IsingWidget;
}

class IsingWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit IsingWidget(QWidget *parent = 0);
    ~IsingWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void producerChanged(Mlt::Producer *);

private slots:
    void on_tempDial_valueChanged(int value);
    void on_tempSpinner_valueChanged(double arg1);
    void on_borderGrowthDial_valueChanged(int value);
    void on_borderGrowthSpinner_valueChanged(double arg1);
    void on_spontGrowthDial_valueChanged(int value);
    void on_spontGrowthSpinner_valueChanged(double arg1);
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

private:
    Ui::IsingWidget *ui;
};

#endif // ISINGWIDGET_H
