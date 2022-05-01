/*
 * Copyright (c) 2015-2017 Meltytech, LLC
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

#ifndef AVFOUNDATIONPRODUCERWIDGET_H
#define AVFOUNDATIONPRODUCERWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
class AvfoundationProducerWidget;
}

class AvfoundationProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit AvfoundationProducerWidget(QWidget *parent = 0);
    ~AvfoundationProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &profile);
    void setProducer(Mlt::Producer *producer);

signals:
    void producerChanged(Mlt::Producer *);

private slots:
    void on_videoCombo_activated(int index);
    void on_audioCombo_activated(int index);

private:
    Ui::AvfoundationProducerWidget *ui;
};

#endif // AVFOUNDATIONPRODUCERWIDGET_H
