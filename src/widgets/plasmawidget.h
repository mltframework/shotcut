/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#ifndef PLASMAWIDGET_H
#define PLASMAWIDGET_H

#include <QWidget>
#include <abstractproducerwidget.h>

namespace Ui {
class PlasmaWidget;
}

class PlasmaWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit PlasmaWidget(QWidget *parent = 0);
    ~PlasmaWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void producerChanged(Mlt::Producer *);

private slots:
    void on_speed1Dial_valueChanged(int value);
    void on_speed1Spinner_valueChanged(double arg1);
    void on_speed2Dial_valueChanged(int value);
    void on_speed2Spinner_valueChanged(double arg1);
    void on_speed3Dial_valueChanged(int value);
    void on_speed3Spinner_valueChanged(double arg1);
    void on_speed4Dial_valueChanged(int value);
    void on_speed4Spinner_valueChanged(double arg1);
    void on_move1Dial_valueChanged(int value);
    void on_move1Spinner_valueChanged(double arg1);
    void on_move2Dial_valueChanged(int value);
    void on_move2Spinner_valueChanged(double arg1);
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

private:
    Ui::PlasmaWidget *ui;
};

#endif // PLASMAWIDGET_H
