/*
 * Copyright (c) 2015 Meltytech, LLC
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

#ifndef TRACKPROPERTIESWIDGET_H
#define TRACKPROPERTIESWIDGET_H

#include <MltProducer.h>
#include <QWidget>

namespace Ui {
class TrackPropertiesWidget;
}
namespace Mlt {
class Transition;
}

class TrackPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrackPropertiesWidget(Mlt::Producer &track, QWidget *parent = 0);
    ~TrackPropertiesWidget();

private slots:
    void on_blendModeCombo_currentIndexChanged(int index);
    void onModeChanged(QString &mode);

private:
    Mlt::Transition *getTransition(const QString &name);

    Ui::TrackPropertiesWidget *ui;
    Mlt::Producer m_track;
};

#endif // TRACKPROPERTIESWIDGET_H
