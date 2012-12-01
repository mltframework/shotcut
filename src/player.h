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
class QAction;
class QActionGroup;

class Player : public QWidget
{
    Q_OBJECT
public:
    explicit Player(QWidget *parent = 0);
    void connectTransport(const TransportControllable*);
    void setIn(unsigned);
    void setOut(unsigned);
    void setMarkers(const QList<int>&);
    unsigned position() const {
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

public slots:
    void play(double speed = 1.0);
    void pause();
    void stop();
    void togglePlayPaused();
    void seek(int position);
    void onProducerOpened();
    void onProducerModified();
    void onShowFrame(Mlt::QFrame frame);
    void onVolumeChanged(int);
    void onCaptureStateChanged(bool);
    void resetProfile();
    void rewind();
    void fastForward();

protected:
    void resizeEvent(QResizeEvent* event);

private:
    void addProfile(QWidget* widget, const QString& desc, const QString& name);
    void setupActions(QWidget* widget);
    void retranslateUi(QWidget* widget);
    void readSettings();
    void changeDeinterlacer(bool checked, const char* method);
    void changeInterpolation(bool checked, const char* method);
    void showAudio(Mlt::Frame* frame);

    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionSkipNext;
    QAction *actionSkipPrevious;
    QAction *actionProgressive;
    QAction *actionOneField;
    QAction *actionLinearBlend;
    QAction *actionYadifTemporal;
    QAction *actionYadifSpatial;
    QAction *actionNearest;
    QAction *actionBilinear;
    QAction *actionBicubic;
    QAction *actionHyper;
    QAction *actionRewind;
    QAction *actionFastForward;
    QAction *actionRealtime;
#ifdef Q_WS_X11
    QAction *actionOpenGL;
#endif
    QActionGroup *externalGroup;
    QActionGroup *profileGroup;
    QAction *actionJack;

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

private slots:
    void updateSelection();
    void onInChanged(int in);
    void onOutChanged(int out);
    void on_actionSkipNext_triggered();
    void on_actionSkipPrevious_triggered();
    void onVideoWidgetContextMenu(const QPoint& pos);
#ifdef Q_WS_X11
    void on_actionOpenGL_triggered(bool checked);
#endif
    void on_actionRealtime_triggered(bool checked);
    void on_actionProgressive_triggered(bool checked);
    void on_actionOneField_triggered(bool checked);
    void on_actionLinearBlend_triggered(bool checked);
    void on_actionYadifTemporal_triggered(bool checked);
    void on_actionYadifSpatial_triggered(bool checked);
    void on_actionNearest_triggered(bool checked);
    void on_actionBilinear_triggered(bool checked);
    void on_actionBicubic_triggered(bool checked);
    void on_actionHyper_triggered(bool checked);
    void on_actionJack_triggered(bool checked);
    void onExternalTriggered(QAction*);
    void onProfileTriggered(QAction*);
    void onVolumeButtonToggled(bool checked);
    void onMuteButtonToggled(bool checked);
};

#endif // PLAYER_H
