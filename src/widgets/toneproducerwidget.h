/*
 * Copyright (c) 2015-2020 Meltytech, LLC
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

#ifndef TONEPRODUCERWIDGET_H
#define TONEPRODUCERWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
class ToneProducerWidget;
}

class ToneProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit ToneProducerWidget(QWidget *parent = 0);
    ~ToneProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void modified();

private slots:
    void on_frequencySpinBox_valueChanged(int);
    void on_levelSpinBox_valueChanged(int);
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

private:
    QString detail() const;
    Ui::ToneProducerWidget *ui;
};

#endif // TONEPRODUCERWIDGET_H
