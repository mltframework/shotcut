/*
 * Copyright (c) 2016-2021 Meltytech, LLC
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

#ifndef COUNTPRODUCERWIDGET_H
#define COUNTPRODUCERWIDGET_H

#include "abstractproducerwidget.h"

#include <QWidget>

namespace Ui {
class CountProducerWidget;
}

class CountProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit CountProducerWidget(QWidget *parent = 0);
    ~CountProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);

signals:
    void producerChanged(Mlt::Producer *);
    void producerReopened(bool play);

private slots:
    void on_directionCombo_activated(int index);
    void on_styleCombo_activated(int index);
    void on_soundCombo_activated(int index);
    void on_backgroundCombo_activated(int index);
    void on_dropCheckBox_clicked(bool checked);
    void on_durationSpinBox_editingFinished();
    void on_preset_selected(void *p);
    void on_preset_saveClicked();

private:
    QString detail() const;
    QString currentDirection() const;
    QString currentStyle() const;
    QString currentSound() const;
    QString currentBackground() const;
    Ui::CountProducerWidget *ui;
};

#endif // COUNTPRODUCERWIDGET_H
