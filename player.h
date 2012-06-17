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

#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QtGui/QIcon>
#include <QtCore/QSettings>
#include "mltcontroller.h"

class ScrubBar;
class QSpinBox;
class QLabel;
class TimeSpinBox;
class AudioSignal;
class QSlider;

namespace Ui {
    class Player;
}

class Player : public QWidget
{
    Q_OBJECT
public:
    explicit Player(QWidget *parent = 0);

    void setIn(unsigned);
    void setOut(unsigned);
    unsigned position() const {
        return m_position;
    }

signals:
    void endOfStream();
    void seeked();
    void showStatusMessage(QString);
    void audioSamplesSignal(const QVector<int16_t>&, const int&, const int&, const int&);

public slots:
    void play(double speed = 1.0);
    void pause();
    void stop();
    void togglePlayPaused();
    void seek(int position);
    void onProducerOpened();
    void onShowFrame(Mlt::QFrame frame);
    void onVolumeChanged(int);

protected:
    void resizeEvent(QResizeEvent* event);

private:
    void readSettings();
    void changeDeinterlacer(bool checked, const char* method);
    void changeInterpolation(bool checked, const char* method);
    void showAudio(Mlt::Frame* frame);

    Ui::Player* ui;
    QSettings m_settings;
    ScrubBar* m_scrubber;
    TimeSpinBox* m_positionSpinner;
    QLabel* m_durationLabel;
    unsigned m_position;
    QIcon m_playIcon;
    QIcon m_pauseIcon;
    QSlider* m_volumeSlider;
    AudioSignal* m_audioSignal;
    int m_seekPosition;

private slots:
    void onInChanged(int in);
    void onOutChanged(int out);
    void on_actionSkipNext_triggered();
    void on_actionSkipPrevious_triggered();
    void on_actionRewind_triggered();
    void on_actionFastForward_triggered();
    void onVideoWidgetContextMenu(const QPoint& pos);
#ifdef Q_WS_X11
    void on_actionOpenGL_triggered(bool checked);
#endif
    void on_actionProgressive_triggered(bool checked);
    void on_actionOneField_triggered(bool checked);
    void on_actionLinearBlend_triggered(bool checked);
    void on_actionYadifTemporal_triggered(bool checked);
    void on_actionYadifSpatial_triggered(bool checked);
    void on_actionNearest_triggered(bool checked);
    void on_actionBilinear_triggered(bool checked);
    void on_actionBicubic_triggered(bool checked);
    void on_actionHyper_triggered(bool checked);
};

#endif // PLAYER_H
