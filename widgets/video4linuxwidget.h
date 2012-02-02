/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef VIDEO4LINUXWIDGET_H
#define VIDEO4LINUXWIDGET_H

#include <QWidget>
#include "abstractproducerwidget.h"

namespace Ui {
    class Video4LinuxWidget;
}

class Video4LinuxWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit Video4LinuxWidget(QWidget *parent = 0);
    ~Video4LinuxWidget();

    // AbstractProducerWidget overrides
    QString producerName() const
        { return "video4linux2"; }
    QString URL() const;
    void load(Mlt::Properties&);

private:
    Ui::Video4LinuxWidget *ui;
};

#endif // VIDEO4LINUXWIDGET_H
