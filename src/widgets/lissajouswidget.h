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

#ifndef LISSAJOUSWIDGET_H
#define LISSAJOUSWIDGET_H

#include "abstractproducerwidget.h"

#include <QWidget>

namespace Mlt {
class Producer;
}

namespace Ui {
class LissajousWidget;
}

class LissajousWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit LissajousWidget(QWidget *parent = 0);
    ~LissajousWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void producerChanged(Mlt::Producer *);

private slots:
    void on_xratioDial_valueChanged(int value);
    void on_xratioSpinner_valueChanged(double arg1);
    void on_yratioDial_valueChanged(int value);
    void on_yratioSpinner_valueChanged(double arg1);
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

private:
    Ui::LissajousWidget *ui;
};

#endif // LISSAJOUSWIDGET_H
