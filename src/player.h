/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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
#include <QSize>
#include "sharedframe.h"

class DockToolBar;
class ScrubBar;
class QSpinBox;
class QLabel;
class TimeSpinBox;
class QFrame;
class QSlider;
class QAction;
class QActionGroup;
class QScrollBar;
class QToolButton;
class QTabBar;
class QHBoxLayout;
class QPushButton;
class TransportControllable;
class QLabel;
class QPushButton;
class QMenu;
class NewProjectFolder;
class StatusLabelWidget;

class Player : public QWidget
{
    Q_OBJECT
public:
    typedef enum {
        SourceTabIndex = 0,
        ProjectTabIndex
    } TabIndex;

    explicit Player(QWidget *parent = 0);
    void connectTransport(const TransportControllable *);
    void setIn(int);
    void setOut(int);
    void setMarkers(const QList<int> &);
    QSize videoSize() const;
    int position() const
    {
        return m_position;
    }
    NewProjectFolder *projectWidget() const
    {
        return m_projectWidget;
    }
    void moveVideoToScreen(int screen = -1);
    void setPauseAfterOpen(bool pause);
    TabIndex tabIndex() const;

signals:
    void endOfStream();
    void showStatusMessage(QString);
    void inChanged(int delta);
    void outChanged(int delta);
    void played(double speed);
    void paused(int position);
    void stopped();
    void seeked(int position);
    void rewound(bool forceChangeDirection);
    void fastForwarded(bool forceChangeDirection);
    void previousSought(int currentPosition);
    void previousSought();
    void nextSought(int currentPosition);
    void nextSought();
    void zoomChanged(float zoom);
    void gridChanged(int grid);
    void scrolledHorizontally(int x);
    void scrolledVertically(int y);
    void tabIndexChanged(int index);
    void trimIn();
    void trimOut();
    void loopChanged(int start, int end);
    void toggleVuiRequested();

public slots:
    void play(double speed = 1.0);
    void pause(int position = -1);
    void stop();
    void seek(int position);
    void reset();
    void onProducerOpened(bool play = true);
    void onDurationChanged();
    void onFrameDisplayed(const SharedFrame &frame);
    void onVolumeChanged(int);
    void onCaptureStateChanged(bool);
    void rewind(bool forceChangeDirection = true);
    void fastForward(bool forceChangeDirection = true);
    void showPaused();
    void showPlaying();
    void switchToTab(TabIndex index);
    void enableTab(TabIndex index, bool enabled = true);
    void onTabBarClicked(int index);
    void setStatusLabel(const QString &text, int timeoutSeconds, QAction *action,
                        QPalette::ColorRole role = QPalette::ToolTipBase);
    void showIdleStatus();
    void focusPositionSpinner() const;
    void onMuteButtonToggled(bool checked);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupActions();
    void adjustScrollBars(float horizontal, float vertical);
    double setVolume(int volume);
    void setLoopRange(int start, int end);
    void layoutToolbars();

    ScrubBar *m_scrubber;
    TimeSpinBox *m_positionSpinner;
    QLabel *m_durationLabel;
    QLabel *m_inPointLabel;
    QLabel *m_selectedLabel;
    int m_position;
    int m_playPosition;
    QIcon m_playIcon;
    QIcon m_loopIcon;
    QIcon m_pauseIcon;
    QIcon m_stopIcon;
    QFrame *m_volumePopup;
    QSlider *m_volumeSlider;
    QPushButton *m_muteButton;
    int m_previousIn;
    int m_previousOut;
    double m_savedVolume;
    int m_duration;
    bool m_isSeekable;
    QScrollBar *m_horizontalScroll;
    QScrollBar *m_verticalScroll;
    QToolButton *m_zoomButton;
    QToolButton *m_gridButton;
    QActionGroup *m_gridActionGroup;
    QAction *m_gridDefaultAction;
    QToolButton *m_volumeButton;
    float m_zoomToggleFactor;
    QTabBar *m_tabs;
    bool m_pauseAfterOpen;
    int m_monitorScreen;
    QWidget *m_videoWidget;
    QHBoxLayout *m_videoLayout;
    QWidget *m_videoScrollWidget;
    const TransportControllable *m_currentTransport;
    StatusLabelWidget *m_statusLabel;
    QMenu *m_zoomMenu;
    QMenu *m_mainMenu;
    NewProjectFolder *m_projectWidget;
    int m_loopStart;
    int m_loopEnd;
    DockToolBar *m_currentDurationToolBar;
    DockToolBar *m_controlsToolBar;
    DockToolBar *m_optionsToolBar;
    DockToolBar *m_inSelectedToolBar;
    QHBoxLayout *m_toolRow1;
    QHBoxLayout *m_toolRow2;

private slots:
    void updateSelection();
    void onInChanged(int in);
    void onOutChanged(int out);
    void onVolumeTriggered();
    void setZoom(float factor, const QIcon &icon);
    void onZoomTriggered();
    void toggleZoom(bool checked);
    void onGridToggled();
    void toggleGrid(bool checked);
    void onStatusFinished();
    void onOffsetChanged(const QPoint &offset);
};

#endif // PLAYER_H
