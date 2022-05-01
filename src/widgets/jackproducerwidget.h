/*
 * Copyright (c) 2012-2017 Meltytech, LLC
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

#ifndef JACKPRODUCERWIDGET_H
#define JACKPRODUCERWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
class JackProducerWidget;
}

class JackProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit JackProducerWidget(QWidget *parent = 0);
    ~JackProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &profile);

private:
    Ui::JackProducerWidget *ui;
};

#endif // JACKPRODUCERWIDGET_H
