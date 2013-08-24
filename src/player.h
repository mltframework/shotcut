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
#include <QIcon>
#include <QSettings>
#include "mltcontroller.h"

class ScrubBar;
class QSpinBox;
class QLabel;
class TimeSpinBox;
class AudioSignal;
class QSlider;
class QAction;

class Player : public QWidget
{
    Q_OBJECT
public:
    explicit Player(QWidget *parent = 0);
    void connectTransport(const TransportControllable*);
    void setIn(int);
    void setOut(int);
    void setMarkers(const QList<int>&);
    int position() const {
        return m_position;
    }

signals:
    void endOfStream();
    void seeked();
    void showStatusMessage(QString);
    void audioLevels(const QVector<double>&);
    void inChanged(int);
    void outChanged(int);
    void played(double speed);
    void paused();
    void stopped();
    void seeked(int position);
    void rewound();
    void fastForwarded();
    void previousSought(int currentPosition);
    void nextSought(int currentPosition);
    void profileChanged();

public slots:
    void play(double speed = 1.0);
    void pause();
    void stop();
    void togglePlayPaused();
    void seek(int position);
    void onProducerOpened();
    void onMeltedUnitOpened();
    void onProducerModified();
    void onShowFrame(int position, double fps, int in, int out, int length, bool isPlaying);
    void onShowFrame(Mlt::QFrame frame);
    void onVolumeChanged(int);
    void onCaptureStateChanged(bool);
    void resetProfile();
    void rewind();
    void fastForward();
    void showPaused();

protected:
    void resizeEvent(QResizeEvent* event);

private:
    void setupActions(QWidget* widget);
    void retranslateUi(QWidget* widget);
    void showAudio(Mlt::Frame* frame);

    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionSkipNext;
    QAction *actionSkipPrevious;
    QAction *actionRewind;
    QAction *actionFastForward;

    QSettings m_settings;
    ScrubBar* m_scrubber;
    TimeSpinBox* m_positionSpinner;
    QLabel* m_durationLabel;
    QLabel* m_inPointLabel;
    QLabel* m_selectedLabel;
    int m_position;
    QIcon m_playIcon;
    QIcon m_pauseIcon;
    QSlider* m_volumeSlider;
    AudioSignal* m_audioSignal;
    int m_seekPosition;
    int m_previousIn;
    int m_previousOut;
    double m_savedVolume;
    int m_duration;
    bool m_isSeekable;
    int m_isMeltedPlaying;

private slots:
    void updateSelection();
    void onInChanged(int in);
    void onOutChanged(int out);
    void on_actionSkipNext_triggered();
    void on_actionSkipPrevious_triggered();
    void onVolumeButtonToggled(bool checked);
    void onMuteButtonToggled(bool checked);
};

#endif // PLAYER_H
