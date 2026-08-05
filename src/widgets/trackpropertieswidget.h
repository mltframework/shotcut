/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

class QTimer;

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
    explicit TrackPropertiesWidget(Mlt::Producer &track,
                                   bool showBlend = true,
                                   bool showDucking = true,
                                   QWidget *parent = 0);
    ~TrackPropertiesWidget();

private slots:
    void on_blendModeCombo_currentIndexChanged(int index);
    void onModeChanged(QString &mode);
    void on_duckThresholdSpinBox_valueChanged(double value);
    void on_duckAttenuationSpinBox_valueChanged(double value);
    void on_duckFadeInSpinBox_valueChanged(double value);
    void on_duckFadeOutSpinBox_valueChanged(double value);
    void onDuckThresholdChanged(double value);
    void onDuckAttenuationChanged(double value);
    void onDuckFadeInChanged(double value);
    void onDuckFadeOutChanged(double value);
    void refreshDuckStatus();

private:
    Mlt::Transition *getTransition(const QString &name);
    void setDuckingVisible(bool visible);
    void updateDuckStatus(double value);

    Ui::TrackPropertiesWidget *ui;
    Mlt::Producer m_track;
    QTimer *m_duckStatusTimer;
};

#endif // TRACKPROPERTIESWIDGET_H
