/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#ifndef TIMELINEPROPERTIESWIDGET_H
#define TIMELINEPROPERTIESWIDGET_H

#include <QWidget>
#include <MltService.h>

namespace Ui {
class TimelinePropertiesWidget;
}

class TimelinePropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimelinePropertiesWidget(Mlt::Service& service, QWidget *parent = 0);
    ~TimelinePropertiesWidget();

signals:
    void editProfile();

private:
    Ui::TimelinePropertiesWidget *ui;
    Mlt::Service m_service;
};

#endif //TIMELINEPROPERTIESWIDGET_H
