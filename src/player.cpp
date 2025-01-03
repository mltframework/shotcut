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

#include "player.h"

#include "actions.h"
#include "scrubbar.h"
#include "mainwindow.h"
#include "dialogs/durationdialog.h"
#include "widgets/statuslabelwidget.h"
#include "widgets/timespinbox.h"
#include "widgets/audioscale.h"
#include "settings.h"
#include "widgets/newprojectfolder.h"
#include "proxymanager.h"
#include "widgets/docktoolbar.h"
#include <Logger.h>

#include <QtWidgets>
#include <limits>

#define VOLUME_KNEE (88)
#define SEEK_INACTIVE (-1)
#define VOLUME_SLIDER_HEIGHT (300)

class NoWheelTabBar : public QTabBar
{
    void wheelEvent(QWheelEvent *event)
    {
        event->ignore();
    };
};

QString blankTime()
{
    switch (Settings.timeFormat()) {
    case mlt_time_frames:
        return "--------";
    case mlt_time_clock:
        return "--:--:--.---";
    case mlt_time_smpte_df:
        if (MLT.profile().fps() == 30000.0 / 1001.0 || MLT.profile().fps() == 60000.0 / 1001.0)
            return "--:--:--;--";
    // Fallthrough on purpose
    default:
        return "--:--:--:--";
    }
}

Player::Player(QWidget *parent)
    : QWidget(parent)
    , m_position(0)
    , m_playPosition(std::numeric_limits<int>::max())
    , m_previousIn(-1)
    , m_previousOut(-1)
    , m_duration(0)
    , m_isSeekable(false)
    , m_zoomToggleFactor(Settings.playerZoom() == 0.0f ? 1.0f : Settings.playerZoom())
    , m_pauseAfterOpen(false)
    , m_monitorScreen(-1)
    , m_currentTransport(nullptr)
    , m_loopStart(-1)
    , m_loopEnd(-1)
{
    setObjectName("Player");
    Mlt::Controller::singleton();
    setupActions();

    // Create a layout.
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setObjectName("playerLayout");
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(4);

    // Add tab bar to indicate/select what is playing: clip, playlist, timeline.
    m_tabs = new NoWheelTabBar;
    m_tabs->setShape(QTabBar::RoundedSouth);
    m_tabs->setUsesScrollButtons(false);
    m_tabs->addTab(tr("Source"));
    m_tabs->addTab(tr("Project"));
    m_tabs->setTabEnabled(SourceTabIndex, false);
    m_tabs->setTabEnabled(ProjectTabIndex, false);
    QHBoxLayout *tabLayout = new QHBoxLayout;
    tabLayout->setSpacing(8);
    tabLayout->addWidget(m_tabs);
    connect(m_tabs, &QTabBar::tabBarClicked, this, &Player::onTabBarClicked);

    // Add status bar.
    m_statusLabel = new StatusLabelWidget();
    connect(m_statusLabel, &StatusLabelWidget::statusCleared, this, &Player::onStatusFinished);
    tabLayout->addWidget(m_statusLabel);
    tabLayout->addStretch(1);

    // Add the layouts for managing video view, scroll bars, and audio controls.
    m_videoLayout = new QHBoxLayout;
    m_videoLayout->setSpacing(4);
    m_videoLayout->setContentsMargins(0, 0, 0, 0);
    vlayout->addLayout(m_videoLayout, 1);
    m_videoScrollWidget = new QWidget;
    m_videoLayout->addWidget(m_videoScrollWidget, 10);
    m_videoLayout->addStretch();
    QGridLayout *glayout = new QGridLayout(m_videoScrollWidget);
    glayout->setSpacing(0);
    glayout->setContentsMargins(0, 0, 0, 0);

    // Add the video widgets.
    m_videoWidget = qobject_cast<QWidget *>(MLT.videoWidget());
    Q_ASSERT(m_videoWidget);
    m_videoWidget->setMinimumSize(QSize(1, 1));
    glayout->addWidget(m_videoWidget, 0, 0);
    m_verticalScroll = new QScrollBar(Qt::Vertical);
    glayout->addWidget(m_verticalScroll, 0, 1);
    m_verticalScroll->hide();
    m_horizontalScroll = new QScrollBar(Qt::Horizontal);
    glayout->addWidget(m_horizontalScroll, 1, 0);
    m_horizontalScroll->hide();

    // Add the new project widget.
    m_projectWidget = new NewProjectFolder(this);
    vlayout->addWidget(m_projectWidget, 10);
    vlayout->addStretch();

    // Add the volume and signal level meter
    m_volumePopup = new QFrame(this, Qt::Popup);
    QVBoxLayout *volumeLayoutV = new QVBoxLayout(m_volumePopup);
    volumeLayoutV->setContentsMargins(0, 0, 0, 0);
    volumeLayoutV->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    QBoxLayout *volumeLayoutH = new QHBoxLayout;
    volumeLayoutH->setSpacing(0);
    volumeLayoutH->setContentsMargins(0, 0, 0, 0);
    volumeLayoutH->addWidget(new AudioScale);
    m_volumeSlider = new QSlider(Qt::Vertical);
    m_volumeSlider->setFocusPolicy(Qt::NoFocus);
    m_volumeSlider->setMinimumHeight(VOLUME_SLIDER_HEIGHT);
    m_volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    volumeLayoutH->addWidget(m_volumeSlider);
    volumeLayoutV->addLayout(volumeLayoutH);
    m_volumeSlider->setRange(0, 99);
    m_volumeSlider->setValue(Settings.playerVolume());
    setVolume(m_volumeSlider->value());
    m_savedVolume = MLT.volume();
    m_volumeSlider->setToolTip(tr("Adjust the audio volume"));
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)));
    connect(m_volumeSlider, &QAbstractSlider::sliderReleased, m_volumePopup, &QWidget::hide);

    // Add mute-volume buttons layout
#ifdef Q_OS_MAC
    if (Settings.theme() == "system")
        volumeLayoutH = new QVBoxLayout;
    else
#endif
        volumeLayoutH = new QHBoxLayout;
    volumeLayoutH->setContentsMargins(0, 0, 0, 0);
    volumeLayoutH->setSpacing(0);
    volumeLayoutV->addLayout(volumeLayoutH);

    // Add mute button
    m_muteButton = new QPushButton(this);
    m_muteButton->setFocusPolicy(Qt::NoFocus);
    m_muteButton->setObjectName(QString::fromUtf8("muteButton"));
    m_muteButton->setIcon(QIcon::fromTheme("audio-volume-muted",
                                           QIcon(":/icons/oxygen/32x32/status/audio-volume-muted.png")));
    m_muteButton->setToolTip(tr("Silence the audio"));
    m_muteButton->setCheckable(true);
    m_muteButton->setChecked(Settings.playerMuted());
    volumeLayoutH->addWidget(m_muteButton);
    connect(m_muteButton, SIGNAL(clicked(bool)), this, SLOT(onMuteButtonToggled(bool)));

    // Add the scrub bar.
    m_scrubber = new ScrubBar(this);
    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("scrubBar");
    m_scrubber->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    vlayout->addWidget(m_scrubber);

    // Make a toolbar for the current and total duration times
    m_currentDurationToolBar = new DockToolBar(tr("Current/Total Times"), this);
    m_currentDurationToolBar->setAreaHint(Qt::BottomToolBarArea);
    m_currentDurationToolBar->setContentsMargins(0, 0, 0, 0);
    m_currentDurationToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_positionSpinner = new TimeSpinBox(this);
    m_positionSpinner->setToolTip(tr("Current position"));
    m_positionSpinner->setEnabled(false);
    m_positionSpinner->setKeyboardTracking(false);
    m_currentDurationToolBar->addWidget(m_positionSpinner);
    m_currentDurationToolBar->addWidget(new QLabel(" / "));
    m_durationLabel = new QLabel(this);
    m_durationLabel->setToolTip(tr("Total Duration"));
    m_durationLabel->setText(blankTime());
    QFontMetrics fm(m_durationLabel->font());
    m_durationLabel->setFixedWidth(fm.boundingRect("00:00:00:.000").width() + 2);
    m_durationLabel->setFixedHeight(m_positionSpinner->sizeHint().height());
    m_currentDurationToolBar->addWidget(m_durationLabel);

    // Make toolbar for transport controls.
    m_controlsToolBar = new DockToolBar(tr("Player Controls"), this);
    m_controlsToolBar->setAreaHint(Qt::BottomToolBarArea);
    m_controlsToolBar->setContentsMargins(0, 0, 0, 0);
    m_controlsToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_controlsToolBar->addAction(Actions["playerSkipPreviousAction"]);
    m_controlsToolBar->addAction(Actions["playerRewindAction"]);
    m_controlsToolBar->addAction(Actions["playerPlayPauseAction"]);
    m_controlsToolBar->addAction(Actions["playerFastForwardAction"]);
    m_controlsToolBar->addAction(Actions["playerSkipNextAction"]);

    // Make a toolbar for player options
    m_optionsToolBar = new DockToolBar(tr("Player Options"), this);
    m_optionsToolBar->setAreaHint(Qt::BottomToolBarArea);
    m_optionsToolBar->setContentsMargins(0, 0, 0, 0);
    m_optionsToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    // Pause button
    QToolButton *loopButton = new QToolButton;
    QMenu *loopMenu = new QMenu(this);
    loopMenu->addAction(Actions["playerLoopRangeAllAction"]);
    loopMenu->addAction(Actions["playerLoopRangeMarkerAction"]);
    loopMenu->addAction(Actions["playerLoopRangeSelectionAction"]);
    loopMenu->addAction(Actions["playerLoopRangeAroundAction"]);
    loopButton->setMenu(loopMenu);
    loopButton->setPopupMode(QToolButton::MenuButtonPopup);
    loopButton->setDefaultAction(Actions["playerLoopAction"]);
    m_optionsToolBar->addWidget(loopButton);
    // Zoom button
    m_zoomButton = new QToolButton;
    m_zoomMenu = new QMenu(this);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-fit-best", QIcon(":/icons/oxygen/32x32/actions/zoom-fit-best")),
        tr("Zoom Fit"), this, SLOT(onZoomTriggered()))->setData(0.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-out", QIcon(":/icons/oxygen/32x32/actions/zoom-out")),
        tr("Zoom 10%"), this, SLOT(onZoomTriggered()))->setData(0.1f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-out", QIcon(":/icons/oxygen/32x32/actions/zoom-out")),
        tr("Zoom 25%"), this, SLOT(onZoomTriggered()))->setData(0.25f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-out", QIcon(":/icons/oxygen/32x32/actions/zoom-out")),
        tr("Zoom 50%"), this, SLOT(onZoomTriggered()))->setData(0.5f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-original", QIcon(":/icons/oxygen/32x32/actions/zoom-original")),
        tr("Zoom 100%"), this, SLOT(onZoomTriggered()))->setData(1.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 200%"), this, SLOT(onZoomTriggered()))->setData(2.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 300%"), this, SLOT(onZoomTriggered()))->setData(3.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 400%"), this, SLOT(onZoomTriggered()))->setData(4.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 500%"), this, SLOT(onZoomTriggered()))->setData(5.0f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 750%"), this, SLOT(onZoomTriggered()))->setData(7.5f);
    m_zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 1000%"), this, SLOT(onZoomTriggered()))->setData(10.0f);
    connect(m_zoomButton, SIGNAL(toggled(bool)), SLOT(toggleZoom(bool)));
    m_zoomButton->setMenu(m_zoomMenu);
    m_zoomButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_zoomButton->setCheckable(true);
    m_zoomButton->setToolTip(tr("Toggle zoom"));
    m_optionsToolBar->addWidget(m_zoomButton);
    toggleZoom(false);
    // Add grid display button to toolbar.
    m_gridButton = new QToolButton;
    QMenu *gridMenu = new QMenu(this);
    m_gridActionGroup = new QActionGroup(this);
    QAction *action = gridMenu->addAction(tr("2x2 Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(2);
    m_gridDefaultAction = action;
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("3x3 Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(3);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("4x4 Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(4);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("16x16 Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(16);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("20 Pixel Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(10020);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("10 Pixel Grid"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(10010);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("80/90% Safe Areas"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(8090);
    m_gridActionGroup->addAction(action);
    action = gridMenu->addAction(tr("EBU R95 Safe Areas"), this, SLOT(onGridToggled()));
    action->setCheckable(true);
    action->setData(95);
    m_gridActionGroup->addAction(action);
    gridMenu->addSeparator();
    action = gridMenu->addAction(tr("Snapping"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, SIGNAL(toggled(bool)), MLT.videoWidget(), SLOT(setSnapToGrid(bool)));
    connect(m_gridButton, SIGNAL(toggled(bool)), SLOT(toggleGrid(bool)));
    m_gridButton->setMenu(gridMenu);
    m_gridButton->setIcon(QIcon::fromTheme("view-grid",
                                           QIcon(":/icons/oxygen/32x32/actions/view-grid")));
    m_gridButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_gridButton->setCheckable(true);
    m_gridButton->setToolTip(tr("Toggle grid display on the player"));
    m_optionsToolBar->addWidget(m_gridButton);
    // Add volume control to toolbar.
    m_volumeButton = new QToolButton;
    m_volumeButton->setObjectName(QString::fromUtf8("volumeButton"));
    m_volumeButton->setIcon(QIcon::fromTheme("player-volume",
                                             QIcon(":/icons/oxygen/32x32/actions/player-volume.png")));
    m_volumeButton->setText(tr("Volume"));
    m_volumeButton->setToolTip(tr("Show the volume control"));
    connect(m_volumeButton, SIGNAL(clicked()), this, SLOT(onVolumeTriggered()));
    m_optionsToolBar->addWidget(m_volumeButton);

    // Make a toolbar for in-point and selected duration
    m_inSelectedToolBar = new DockToolBar(tr("Player Options"), this);
    m_inSelectedToolBar->setAreaHint(Qt::BottomToolBarArea);
    m_inSelectedToolBar->setContentsMargins(0, 0, 0, 0);
    m_inPointLabel = new QLabel(this);
    m_inPointLabel->setText(blankTime());
    m_inPointLabel->setToolTip(tr("In Point"));
    m_inPointLabel->setFixedWidth(fm.boundingRect("00:00:00.000").width() + 2);
    m_inPointLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_inPointLabel->setFixedHeight(m_positionSpinner->sizeHint().height());
    m_inSelectedToolBar->addWidget(m_inPointLabel);
    m_inSelectedToolBar->addWidget(new QLabel(" / "));
    m_selectedLabel = new QLabel(this);
    m_selectedLabel->setText(blankTime());
    m_selectedLabel->setToolTip(tr("Selected Duration"));
    m_selectedLabel->setFixedWidth(fm.boundingRect("00:00:00.000").width() + 2);
    m_selectedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_selectedLabel->setFixedHeight(m_positionSpinner->sizeHint().height());
    m_inSelectedToolBar->addWidget(m_selectedLabel);

    // Create two rows for the toolbars.
    // The toolbars will be layed out in one or two rows depending on the width.
    m_toolRow1 = new QHBoxLayout();
    vlayout->addLayout(m_toolRow1);
    m_toolRow2 = new QHBoxLayout();
    vlayout->addLayout(m_toolRow2);
    vlayout->addLayout(tabLayout);
    layoutToolbars();

    onMuteButtonToggled(Settings.playerMuted());

    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame &)), this,
            SLOT(onFrameDisplayed(const SharedFrame &)));
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(seek(int)));
    connect(m_scrubber, SIGNAL(paused(int)), this, SLOT(pause(int)));
    connect(m_scrubber, SIGNAL(inChanged(int)), this, SLOT(onInChanged(int)));
    connect(m_scrubber, SIGNAL(outChanged(int)), this, SLOT(onOutChanged(int)));
    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    connect(this, SIGNAL(endOfStream()), this, SLOT(pause()));
    connect(this, SIGNAL(gridChanged(int)), MLT.videoWidget(), SLOT(setGrid(int)));
    connect(this, SIGNAL(zoomChanged(float)), MLT.videoWidget(), SLOT(setZoom(float)));
    connect(m_horizontalScroll, SIGNAL(valueChanged(int)), MLT.videoWidget(), SLOT(setOffsetX(int)));
    connect(m_verticalScroll, SIGNAL(valueChanged(int)), MLT.videoWidget(), SLOT(setOffsetY(int)));
    connect(MLT.videoWidget(), SIGNAL(offsetChanged(const QPoint &)),
            SLOT(onOffsetChanged(const QPoint &)));

    connect(&Settings, &ShotcutSettings::timeFormatChanged, this, [&]() {
        updateSelection();
        if (MLT.isSeekable()) {
            onDurationChanged();
        }
    });

    setFocusPolicy(Qt::StrongFocus);
}

void Player::connectTransport(const TransportControllable *receiver)
{
    if (receiver == m_currentTransport) return;
    if (m_currentTransport)
        disconnect(m_currentTransport);
    m_currentTransport = receiver;
    connect(this, SIGNAL(played(double)), receiver, SLOT(play(double)));
    connect(this, SIGNAL(paused(int)), receiver, SLOT(pause(int)));
    connect(this, SIGNAL(stopped()), receiver, SLOT(stop()));
    connect(this, SIGNAL(seeked(int)), receiver, SLOT(seek(int)));
    connect(this, SIGNAL(rewound(bool)), receiver, SLOT(rewind(bool)));
    connect(this, SIGNAL(fastForwarded(bool)), receiver, SLOT(fastForward(bool)));
    connect(this, SIGNAL(previousSought(int)), receiver, SLOT(previous(int)));
    connect(this, SIGNAL(nextSought(int)), receiver, SLOT(next(int)));
}

void Player::setupActions()
{
    QIcon icon;
    QAction *action;

    m_playIcon = QIcon::fromTheme("media-playback-start",
                                  QIcon(":/icons/oxygen/32x32/actions/media-playback-start.png"));
    m_pauseIcon = QIcon::fromTheme("media-playback-pause",
                                   QIcon(":/icons/oxygen/32x32/actions/media-playback-pause.png"));
    m_stopIcon = QIcon::fromTheme("media-playback-stop",
                                  QIcon(":/icons/oxygen/32x32/actions/media-playback-stop.png"));

    action = new QAction(tr("Play/Pause"), this);
    action->setShortcut(QKeySequence(Qt::Key_Space));
    action->setIcon(m_playIcon);
    action->setDisabled(true);
    action->setToolTip(tr("Toggle play or pause"));
    connect(action, &QAction::triggered, this, [&]() {
        if (Actions["playerPlayPauseAction"]->icon().cacheKey() == m_playIcon.cacheKey())
            play();
        else if (m_isSeekable)
            pause();
        else
            stop();
    });
    Actions.add("playerPlayPauseAction", action);

    action = new QAction(tr("Loop"), this);
    action->setShortcut(QKeySequence(Qt::Key_Backslash));
    action->setIcon(QIcon::fromTheme("media-playback-loop",
                                     QIcon(":/icons/oxygen/32x32/actions/media-playback-loop.png")));
    action->setCheckable(true);
    action->setToolTip(tr("Toggle player looping"));
    connect(action, &QAction::toggled, this, [&]() {
        setLoopRange(m_loopStart, m_loopEnd);
    });
    Actions.add("playerLoopAction", action);

    action = new QAction(tr("Loop All"), this);
    action->setToolTip(tr("Loop back to the beginning when the end is reached"));
    connect(action, &QAction::triggered, this, [&]() {
        Actions["playerLoopAction"]->setChecked(true);
        setLoopRange(0, m_duration);
    });
    Actions.add("playerLoopRangeAllAction", action);

    action = new QAction(tr("Loop Marker"), this);
    action->setToolTip(tr("Loop around the marker under the cursor in the timeline"));
    connect(action, &QAction::triggered, this, [&]() {
        int start, end;
        MAIN.getMarkerRange(m_position, &start, &end);
        if (start >= 0) {
            Actions["playerLoopAction"]->setChecked(true);
            setLoopRange(start, end);
        }
    });
    Actions.add("playerLoopRangeMarkerAction", action);

    action = new QAction(tr("Loop Selection"), this);
    action->setToolTip(tr("Loop around the selected clips"));
    connect(action, &QAction::triggered, this, [&]() {
        int start, end;
        MAIN.getSelectionRange(&start, &end);
        if (start >= 0) {
            Actions["playerLoopAction"]->setChecked(true);
            setLoopRange(start, end);
        } else {
            emit showStatusMessage(tr("Nothing selected"));
        }
    });
    Actions.add("playerLoopRangeSelectionAction", action);

    action = new QAction(tr("Loop Around Cursor"), this);
    action->setToolTip(tr("Loop around the current cursor position"));
    connect(action, &QAction::triggered, this, [&]() {
        Actions["playerLoopAction"]->setChecked(true);
        // Set the range one second before and after the cursor
        int fps = qRound(MLT.profile().fps());
        if (m_duration <= fps * 2) {
            setLoopRange(0, m_duration);
        } else {
            int start = position() - fps;
            int end = position() + fps;
            if (start < 0) {
                end -= start;
                start = 0;
            }
            if (end >= m_duration) {
                start -= end - m_duration;
                end = m_duration;
            }
            setLoopRange(start, end);
        }
    });
    Actions.add("playerLoopRangeAroundAction", action);

    action = new QAction(tr("Skip Next"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
    icon = QIcon::fromTheme("media-skip-forward",
                            QIcon(":/icons/oxygen/32x32/actions/media-skip-forward.png"));
    action->setIcon(icon);
    action->setDisabled(true);
    action->setToolTip(tr("Skip to the next point"));
    connect(action, &QAction::triggered, this, [&]() {
        if (m_scrubber->markers().size() > 0) {
            foreach (int x, m_scrubber->markers()) {
                if (x > m_position) {
                    emit seeked(x);
                    return;
                }
            }
            emit seeked(m_duration - 1);
        } else {
            emit nextSought(m_position);
            emit nextSought();
        }
    });
    Actions.add("playerSkipNextAction", action);

    action = new QAction(tr("Skip Previous"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
    icon = QIcon::fromTheme("media-skip-backward",
                            QIcon(":/icons/oxygen/32x32/actions/media-skip-backward.png"));
    action->setIcon(icon);
    action->setDisabled(true);
    action->setToolTip(tr("Skip to the previous point"));
    connect(action, &QAction::triggered, this, [&]() {
        if (m_scrubber->markers().size() > 0) {
            QList<int> markers = m_scrubber->markers();
            int n = markers.count();
            while (n--) {
                if (markers[n] < m_position) {
                    emit seeked(markers[n]);
                    return;
                }
            }
            emit seeked(0);
        } else {
            emit previousSought(m_position);
            emit previousSought();
        }
    });
    Actions.add("playerSkipPreviousAction", action);

    action = new QAction(tr("Rewind"), this);
    action->setProperty(Actions.hardKeyProperty, "J");
    icon = QIcon::fromTheme("media-seek-backward",
                            QIcon(":/icons/oxygen/32x32/actions/media-seek-backward.png"));
    action->setIcon(icon);
    action->setDisabled(true);
    action->setToolTip(tr("Play quickly backwards"));
    connect(action, &QAction::triggered, this, &Player::rewind);
    Actions.add("playerRewindAction", action);

    action = new QAction(tr("Fast Forward"), this);
    action->setProperty(Actions.hardKeyProperty, "L");
    icon = QIcon::fromTheme("media-seek-forward",
                            QIcon(":/icons/oxygen/32x32/actions/media-seek-forward.png"));
    action->setIcon(icon);
    action->setDisabled(true);
    action->setToolTip(tr("Play quickly forwards"));
    connect(action, &QAction::triggered, this, &Player::fastForward);
    Actions.add("playerFastForwardAction", action);

    action = new QAction(tr("Seek Start"), this);
    action->setShortcut(QKeySequence(Qt::Key_Home));
    connect(action, &QAction::triggered, this, [&]() {
        seek(0);
    });
    Actions.add("playerSeekStartAction", action);

    action = new QAction(tr("Seek End"), this);
    action->setShortcut(QKeySequence(Qt::Key_End));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer()) {
            pause(MLT.producer()->get_length());
            seek(MLT.producer()->get_length());
        }
    });
    Actions.add("playerSeekEndAction", action);

    action = new QAction(tr("Next Frame"), this);
    action->setProperty(Actions.hardKeyProperty, "K+L");
    action->setShortcut(QKeySequence(Qt::Key_Right));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer()) {
            pause(position() + 1);
            seek(position() + 1);
        }
    });
    Actions.add("playerNextFrameAction", action);

    action = new QAction(tr("Previous Frame"), this);
    action->setProperty(Actions.hardKeyProperty, "K+J");
    action->setShortcut(QKeySequence(Qt::Key_Left));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer()) {
            pause(position() - 1);
            seek(position() - 1);
        }
    });
    Actions.add("playerPreviousFrameAction", action);

    action = new QAction(tr("Forward One Second"), this);
    action->setShortcut(QKeySequence(Qt::Key_PageDown));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() + qRound(MLT.profile().fps()));
    });
    Actions.add("playerForwardOneSecondAction", action);

    action = new QAction(tr("Backward One Second"), this);
    action->setShortcut(QKeySequence(Qt::Key_PageUp));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() - qRound(MLT.profile().fps()));
    });
    Actions.add("playerBackwardOneSecondAction", action);

    action = new QAction(tr("Forward Two Seconds"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_PageDown));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() + 2 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerForwardTwoSecondsAction", action);

    action = new QAction(tr("Backward Two Seconds"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_PageUp));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() - 2 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerBackwardTwoAction", action);

    action = new QAction(tr("Forward Five Seconds"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() + 5 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerForwardFiveSecondsAction", action);

    action = new QAction(tr("Backward Five Seconds"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageUp));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() - 5 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerBackwardFiveSecondsAction", action);

    action = new QAction(tr("Forward Ten Seconds"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_PageDown));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() + 10 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerForwardTenSecondsAction", action);

    action = new QAction(tr("Backward Ten Seconds"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_PageUp));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() - 10 * qRound(MLT.profile().fps()));
    });
    Actions.add("playerBackwardTenSecondsAction", action);

    action = new QAction(tr("Forward Jump"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_PageDown));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() + qRound(MLT.profile().fps() * Settings.playerJumpSeconds()));
    });
    Actions.add("playerForwardJumpAction", action);

    action = new QAction(tr("Backward Jump"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_PageUp));
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.producer())
            seek(position() - qRound(MLT.profile().fps() * Settings.playerJumpSeconds()));
    });
    Actions.add("playerBackwardJumpAction", action);

    action = new QAction(tr("Set Jump Time"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    connect(action, &QAction::triggered, this, [&]() {
        DurationDialog dialog(this);
        dialog.setDuration(qRound(MLT.profile().fps() * Settings.playerJumpSeconds()));
        if (dialog.exec() == QDialog::Accepted) {
            Settings.setPlayerJumpSeconds((double)dialog.duration() / MLT.profile().fps());
        }
    });
    Actions.add("playerSetJumpAction", action);

    action = new QAction(tr("Trim Clip In"), this);
    action->setShortcut(QKeySequence(Qt::Key_I));
    connect(action, &QAction::triggered, this, [&]() {
        if (tabIndex() == Player::SourceTabIndex && MLT.isSeekableClip()) {
            setIn(position());
            int delta = position() - MLT.producer()->get_in();
            emit inChanged(delta);
        } else if (tabIndex() == Player::ProjectTabIndex) {
            emit trimIn();
        }
    });
    Actions.add("playerSetInAction", action);

    action = new QAction(tr("Trim Clip Out"), this);
    action->setShortcut(QKeySequence(Qt::Key_O));
    connect(action, &QAction::triggered, this, [&]() {
        if (tabIndex() == Player::SourceTabIndex && MLT.isSeekableClip()) {
            setOut(position());
            int delta = position() - MLT.producer()->get_out();
            emit outChanged(delta);
        } else if (tabIndex() == Player::ProjectTabIndex) {
            emit trimOut();
        }
    });
    Actions.add("playerSetOutAction", action);

    action = new QAction(tr("Set Time Position"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(action, &QAction::triggered, this, [&]() {
        m_positionSpinner->setFocus(Qt::ShortcutFocusReason);
    });
    Actions.add("playerSetPositionAction", action);

    action = new QAction(tr("Switch Source/Project"), this);
    action->setShortcut(QKeySequence(Qt::Key_P));
    addAction(action);
    connect(action, &QAction::triggered, this, [&]() {
        if (MLT.isPlaylist()) {
            if (MAIN.isMultitrackValid())
                onTabBarClicked(Player::ProjectTabIndex);
            else if (MLT.savedProducer())
                onTabBarClicked(Player::SourceTabIndex);
        } else if (MLT.isMultitrack()) {
            if (MLT.savedProducer())
                onTabBarClicked(Player::SourceTabIndex);
            // TODO else open clip under playhead of current track if available
        } else {
            if (MAIN.isMultitrackValid() || (MAIN.playlist() && MAIN.playlist()->count() > 0))
                onTabBarClicked(Player::ProjectTabIndex);
        }
    });
    Actions.add("playerSwitchSourceProgramAction", action);

    action = new QAction(tr("Pause"), this);
    action->setProperty(Actions.hardKeyProperty, "K");
    action->setIcon(m_pauseIcon);
    action->setToolTip(tr("Pause playback"));
    connect(action, &QAction::triggered, this, &Player::pause);
    Actions.add("playerPauseAction", action, tr("Player"));

    action = new QAction(tr("Focus Player"), this);
    action->setProperty(Actions.hardKeyProperty, "Shift+Esc");
    connect(action, &QAction::triggered, this, [&]() {
        setFocus();
    });
    Actions.add("playerFocus", action, tr("Player"));
}

void Player::setIn(int pos)
{
    LOG_DEBUG() << "in" << pos << "out" << m_previousOut;
    // Changing out must come before in because mlt_playlist will automatically swap them if out < in
    if (pos >= 0 && pos > m_previousOut) {
        onOutChanged(m_duration - 1);
        m_scrubber->setOutPoint(m_duration - 1);
    }
    m_scrubber->setInPoint(pos);
}

void Player::setOut(int pos)
{
    LOG_DEBUG() << "in" << m_previousIn << "out" << pos;
    // Changing in must come before out because mlt_playlist will automatically swap them if out < in
    if (pos >= 0 && pos < m_previousIn) {
        onInChanged(0);
        m_scrubber->setInPoint(0);
    }
    m_scrubber->setOutPoint(pos);
}

void Player::setMarkers(const QList<int> &markers)
{
    m_scrubber->setMarkers(markers);
}

QSize Player::videoSize() const
{
    return m_videoWidget->size();
}

void Player::resizeEvent(QResizeEvent *)
{
    MLT.onWindowResize();
    if (Settings.playerZoom() > 0.0f) {
        float horizontal = float(m_horizontalScroll->value()) / m_horizontalScroll->maximum();
        float vertical = float(m_verticalScroll->value()) / m_verticalScroll->maximum();
        adjustScrollBars(horizontal, vertical);
    } else {
        m_horizontalScroll->hide();
        m_verticalScroll->hide();
    }
    layoutToolbars();
}

bool Player::event(QEvent *event)
{
    bool result = QWidget::event(event);
    if (event->type() == QEvent::PaletteChange) {
        m_videoScrollWidget->hide();
        m_videoScrollWidget->show();
    }
    return result;
}

void Player::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void Player::play(double speed)
{
    // Start from beginning if trying to start at the end.
    if (m_position >= m_duration - 1 && !MLT.isMultitrack()) {
        emit seeked(m_previousIn);
        m_position = m_previousIn;
    }
    emit played(speed);
    if (m_isSeekable) {
        Actions["playerPlayPauseAction"]->setIcon(m_pauseIcon);
    } else {
        Actions["playerPlayPauseAction"]->setIcon(m_stopIcon);
    }
    m_playPosition = m_position;
}

void Player::pause(int position)
{
    emit paused(position);
    showPaused();
}

void Player::stop()
{
    emit stopped();
    Actions["playerPlayPauseAction"]->setIcon(m_playIcon);
}

void Player::seek(int position)
{
    if (m_isSeekable) {
        if (position >= 0) {
            emit seeked(qMin(position, MLT.isMultitrack() ? m_duration : m_duration - 1));
        }
    }
    if (Settings.playerPauseAfterSeek()) {
        Actions["playerPlayPauseAction"]->setIcon(m_playIcon);
        m_playPosition = std::numeric_limits<int>::max();
    }
}

void Player::reset()
{
    m_scrubber->setMarkers(QList<int>());
    m_inPointLabel->setText(blankTime());
    m_selectedLabel->setText(blankTime());
    m_durationLabel->setText(blankTime());
    m_scrubber->setDisabled(true);
    m_scrubber->setScale(1);
    m_positionSpinner->setValue(0);
    m_positionSpinner->setDisabled(true);
    Actions["playerPlayPauseAction"]->setDisabled(true);
    Actions["playerSkipPreviousAction"]->setDisabled(true);
    Actions["playerSkipNextAction"]->setDisabled(true);
    Actions["playerRewindAction"]->setDisabled(true);
    Actions["playerFastForwardAction"]->setDisabled(true);
    m_videoWidget->hide();
    m_projectWidget->show();
    m_previousIn = m_previousOut = -1;
}

void Player::onProducerOpened(bool play)
{
    m_projectWidget->hide();
    m_videoWidget->show();
    m_duration = MLT.producer()->get_length();
    setLoopRange(0, m_duration - 1);
    m_isSeekable = MLT.isSeekable();
    MLT.producer()->set("ignore_points", 1);
    m_scrubber->setFramerate(MLT.profile().fps());
    m_scrubber->setScale(m_duration);
    if (!MLT.isPlaylist())
        m_scrubber->setMarkers(QList<int>());
    m_inPointLabel->setText(blankTime());
    m_selectedLabel->setText(blankTime());
    if (m_isSeekable) {
        m_durationLabel->setText(QString(MLT.producer()->get_length_time(Settings.timeFormat())));
        MLT.producer()->get_length_time(mlt_time_clock);
        m_previousIn = MLT.isClip() ? MLT.producer()->get_in() : -1;
        m_scrubber->setEnabled(true);
        m_scrubber->setInPoint(m_previousIn);
        m_previousOut = MLT.isClip() ? MLT.producer()->get_out() : -1;
        m_scrubber->setOutPoint(m_previousOut);
    } else {
        m_durationLabel->setText(tr("Not Seekable"));
        m_scrubber->setDisabled(true);
        // cause scrubber redraw
        m_scrubber->setScale(m_duration);
    }
    if (MLT.isMultitrack()) {
        Actions["playerLoopRangeMarkerAction"]->setEnabled(true);
    } else {
        Actions["playerLoopRangeMarkerAction"]->setEnabled(false);
    }
    m_positionSpinner->setEnabled(m_isSeekable);
    setVolume(m_volumeSlider->value());
    m_savedVolume = MLT.volume();
    onMuteButtonToggled(Settings.playerMuted());
    toggleZoom(Settings.playerZoom() > 0.0f);

    Actions["playerPlayPauseAction"]->setEnabled(true);
    Actions["playerSkipPreviousAction"]->setEnabled(m_isSeekable);
    Actions["playerSkipNextAction"]->setEnabled(m_isSeekable);
    Actions["playerRewindAction"]->setEnabled(m_isSeekable);
    Actions["playerFastForwardAction"]->setEnabled(m_isSeekable);

    connectTransport(MLT.transportControl());

    if (play || (MLT.isClip() && !MLT.isClosedClip())) {
        if (m_pauseAfterOpen) {
            m_pauseAfterOpen = false;
            QTimer::singleShot(500, this, [ = ]() {
                if (MLT.producer())
                    pause(MLT.producer()->position());
            });
            if (MLT.isClip()) {
                pause();
            } else {
                MLT.producer()->seek(0);
            }
        } else {
            // Closing the previous producer might call pause() milliseconds before
            // calling play() here. Delays while purging the consumer on pause can
            // interfere with the play() call. So, we delay play a little to let
            // pause purging to complete.
            if (!MLT.consumer()->is_stopped()) {
                // This seek purges the consumer to prevent latent end-of-stream detection.
                seek(0);
            }
            QTimer::singleShot(500, this, SLOT(play()));
        }
    } else {
        pause(0);
    }
}

void Player::onDurationChanged()
{
    m_duration = MLT.producer()->get_length();
    setLoopRange(0, m_duration - 1);
    m_isSeekable = MLT.isSeekable();
    m_scrubber->setScale(m_duration);
    m_scrubber->setMarkers(QList<int>());
    m_durationLabel->setText(QString(MLT.producer()->get_length_time(Settings.timeFormat())));
    MLT.producer()->get_length_time(mlt_time_clock);
    if (MLT.producer()->get_speed() == 0)
        seek(m_position);
    else if (m_position >= m_duration)
        pause(m_duration - 1);
}

void Player::onFrameDisplayed(const SharedFrame &frame)
{
    if (!MLT.producer() || !MLT.producer()->is_valid())
        return;

    if (MLT.producer()->get_length() != m_duration) {
        // This can happen if the profile changes. Reload the properties from the producer.
        onProducerOpened(false);
    }
    int position = frame.get_position();
    bool loop = position >= (m_loopEnd - 1) && Actions["playerLoopAction"]->isChecked();
    if (position > MLT.producer()->get_length()) {
        position = MLT.producer()->get_length();
    }
    if (position <= m_duration) {
        m_position = position;
        m_positionSpinner->blockSignals(true);
        m_positionSpinner->setValue(position);
        m_positionSpinner->blockSignals(false);
        m_scrubber->onSeek(position);
        if (m_playPosition < m_previousOut && m_position >= m_previousOut && !loop) {
            pause(m_previousOut);
        }
    }
    if (loop) {
        MLT.producer()->seek(m_loopStart);
        MLT.consumer()->purge();
    } else if (position >= m_duration - 1) {
        emit endOfStream();
    }
}

void Player::updateSelection()
{
    if (MLT.producer() && MLT.producer()->get_in() > 0) {
        m_inPointLabel->setText(QString(MLT.producer()->get_time("in", Settings.timeFormat())));
        m_selectedLabel->setText(MLT.producer()->frames_to_time(MLT.producer()->get_playtime(),
                                                                Settings.timeFormat()));
    } else {
        m_inPointLabel->setText(blankTime());
        if (MLT.isClip() && MLT.producer()->get_out() < m_duration - 1) {
            m_selectedLabel->setText(MLT.producer()->frames_to_time(MLT.producer()->get_playtime(),
                                                                    Settings.timeFormat()));
        } else if (!MLT.producer() || MLT.producer()->get_in() == 0) {
            m_selectedLabel->setText(blankTime());
        }
    }
}

void Player::onInChanged(int in)
{
    if (in != m_previousIn && in >= 0) {
        int delta = in - MLT.producer()->get_in();
        MLT.setIn(in);
        emit inChanged(delta);
    }
    m_previousIn = in;
    updateSelection();
}

void Player::onOutChanged(int out)
{
    if (out != m_previousOut && out >= 0) {
        int delta = out - MLT.producer()->get_out();
        MLT.setOut(out);
        emit outChanged(delta);
    }
    m_previousOut = out;
    m_playPosition = m_previousOut; // prevent O key from pausing
    updateSelection();
}

void Player::rewind(bool forceChangeDirection)
{
    if (m_isSeekable)
        emit rewound(forceChangeDirection);
}

void Player::fastForward(bool forceChangeDirection)
{
    if (m_isSeekable) {
        emit fastForwarded(forceChangeDirection);
        m_playPosition = m_position;
    } else {
        play();
    }
}

void Player::showPaused()
{
    Actions["playerPlayPauseAction"]->setIcon(m_playIcon);
}

void Player::showPlaying()
{
    Actions["playerPlayPauseAction"]->setIcon(m_pauseIcon);
}

void Player::switchToTab(TabIndex index)
{
    m_tabs->setCurrentIndex(index);
    emit tabIndexChanged(index);
}

void Player::enableTab(TabIndex index, bool enabled)
{
    m_tabs->setTabEnabled(index, enabled);
}

void Player::onTabBarClicked(int index)
{
    // Do nothing if requested tab is already selected.
    if (m_tabs->currentIndex() == index)
        return;

    switch (index) {
    case SourceTabIndex:
        if (MLT.savedProducer() && MLT.savedProducer()->is_valid() && MLT.producer()
                && MLT.producer()->get_producer() != MLT.savedProducer()->get_producer()) {
            m_pauseAfterOpen = true;
            MAIN.open(new Mlt::Producer(MLT.savedProducer()));
        }
        break;
    case ProjectTabIndex:
        if (MAIN.isMultitrackValid()) {
            if (!MLT.isMultitrack())
                MAIN.seekTimeline(MAIN.multitrack()->position());
        } else {
            if (!MLT.isPlaylist() && MAIN.playlist())
                MAIN.seekPlaylist(MAIN.playlist()->position());
        }
        break;
    }
}

void Player::setStatusLabel(const QString &text, int timeoutSeconds, QAction *action,
                            QPalette::ColorRole role)
{
    m_statusLabel->setWidth(m_scrubber->width() - m_tabs->width());
    m_statusLabel->showText(text, timeoutSeconds, action, role);
}

void Player::onStatusFinished()
{
    showIdleStatus();
}

void Player::onOffsetChanged(const QPoint &offset)
{
    if (!offset.isNull() && m_horizontalScroll->isVisible()) {
        m_horizontalScroll->setValue(offset.x());
        m_verticalScroll->setValue(offset.y());
    }
}

void Player::adjustScrollBars(float horizontal, float vertical)
{
    if (MLT.profile().width() * m_zoomToggleFactor > m_videoWidget->width()) {
        m_horizontalScroll->setPageStep(m_videoWidget->width());
        m_horizontalScroll->setMaximum(MLT.profile().width() * m_zoomToggleFactor
                                       - m_horizontalScroll->pageStep());
        m_horizontalScroll->setValue(qRound(horizontal * m_horizontalScroll->maximum()));
        emit m_horizontalScroll->valueChanged(m_horizontalScroll->value());
        m_horizontalScroll->show();
    } else {
        int max = MLT.profile().width() * m_zoomToggleFactor - m_videoWidget->width();
        emit m_horizontalScroll->valueChanged(qRound(0.5 * max));
        m_horizontalScroll->hide();
    }

    if (MLT.profile().height() * m_zoomToggleFactor > m_videoWidget->height()) {
        m_verticalScroll->setPageStep(m_videoWidget->height());
        m_verticalScroll->setMaximum(MLT.profile().height() * m_zoomToggleFactor
                                     - m_verticalScroll->pageStep());
        m_verticalScroll->setValue(qRound(vertical * m_verticalScroll->maximum()));
        emit m_verticalScroll->valueChanged(m_verticalScroll->value());
        m_verticalScroll->show();
    } else {
        int max = MLT.profile().height() * m_zoomToggleFactor - m_videoWidget->height();
        emit m_verticalScroll->valueChanged(qRound(0.5 * max));
        m_verticalScroll->hide();
    }
}

double Player::setVolume(int volume)
{
    const double gain = double(volume) / VOLUME_KNEE;
    MLT.setVolume(gain);
    return gain;
}

void Player::setLoopRange(int start, int end)
{
    m_loopStart = start;
    m_loopEnd = end;
    if (Actions["playerLoopAction"]->isChecked()) {
        m_scrubber->setLoopRange(m_loopStart, m_loopEnd);
        emit loopChanged(m_loopStart, m_loopEnd);
    } else {
        m_scrubber->setLoopRange(-1, -1);
        emit loopChanged(-1, -1);
    }
}

void Player::layoutToolbars()
{
    int totalWidth = m_currentDurationToolBar->sizeHint().width() +
                     m_controlsToolBar->sizeHint().width() + m_optionsToolBar->sizeHint().width() +
                     m_inSelectedToolBar->sizeHint().width() + 20;
    bool twoRowsInUse = m_toolRow2->count() > 0;

    if (m_toolRow1->count() > 0) {
        if (totalWidth <= this->width() && !twoRowsInUse) {
            // Everything still fits in one toolbar. Nothing to change.
            return;
        } else if (totalWidth > this->width() && twoRowsInUse) {
            // Two toolbars still needed. Nothing to change.
            return;
        }
    }

    // Remove all the widgets from the tool bar area
    QLayoutItem *child;
    while ((child = m_toolRow1->takeAt(0)) != nullptr) {
        QWidget *widget = child->widget();
        if (widget->objectName().startsWith("spacer")) {
            delete widget;
        }
        delete child;
    }
    while ((child = m_toolRow2->takeAt(0)) != nullptr) {
        QWidget *widget = child->widget();
        if (widget->objectName().startsWith("spacer")) {
            delete widget;
        }
        delete child;
    }

    QWidget *spacer;
    if (totalWidth <= this->width()) {
        // Use one row
        m_toolRow1->addWidget(m_currentDurationToolBar);
        spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        spacer->setObjectName("spacerLeft");
        m_toolRow1->addWidget(spacer);
        m_toolRow1->addWidget(m_controlsToolBar);
        m_toolRow1->addWidget(m_optionsToolBar);
        spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        spacer->setObjectName("spacerRight");
        m_toolRow1->addWidget(spacer);
        m_toolRow1->addWidget(m_inSelectedToolBar);
    } else {
        // Use two rows
        spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        spacer->setObjectName("spacerLeft");
        m_toolRow1->addWidget(spacer);
        m_toolRow1->addWidget(m_controlsToolBar);
        m_toolRow1->addWidget(m_optionsToolBar);
        spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        spacer->setObjectName("spacerRight");
        m_toolRow1->addWidget(spacer);
        m_toolRow2->addWidget(m_currentDurationToolBar);
        spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        spacer->setObjectName("spacerMiddle");
        m_toolRow2->addWidget(spacer);
        m_toolRow2->addWidget(m_inSelectedToolBar);
    }
}

void Player::showIdleStatus()
{
    if (Settings.proxyEnabled() && Settings.playerPreviewScale() > 0) {
        setStatusLabel(tr("Proxy and preview scaling are ON at %1p").arg(ProxyManager::resolution()), -1,
                       nullptr, QPalette::AlternateBase);
    } else if (Settings.proxyEnabled()) {
        setStatusLabel(tr("Proxy is ON at %1p").arg(ProxyManager::resolution()), -1, nullptr,
                       QPalette::AlternateBase);
    } else if (Settings.playerPreviewScale() > 0) {
        setStatusLabel(tr("Preview scaling is ON at %1p").arg(Settings.playerPreviewScale()), -1, nullptr,
                       QPalette::AlternateBase);
    } else {
        setStatusLabel("", -1, nullptr);
    }
}

void Player::focusPositionSpinner() const
{
    m_positionSpinner->setFocus(Qt::ShortcutFocusReason);
}

void Player::moveVideoToScreen(int screen)
{
    if (screen == m_monitorScreen) return;
    if (screen == -2) {
        // -2 = embedded
        if (!m_videoScrollWidget->isFullScreen()) return;
        m_videoScrollWidget->showNormal();
        m_videoLayout->insertWidget(0, m_videoScrollWidget, 10);
    } else if (QGuiApplication::screens().size() > 1) {
        // -1 = find first screen the app is not using
        for (int i = 0; screen == -1 && i < QGuiApplication::screens().size(); i++) {
            if (QGuiApplication::screens().at(i) != this->screen())
                screen = i;
        }
        m_videoScrollWidget->showNormal();
        m_videoScrollWidget->setParent(nullptr);
        m_videoScrollWidget->move(QGuiApplication::screens().at(screen)->geometry().topLeft());
        m_videoScrollWidget->showFullScreen();
    }
    m_monitorScreen = screen;
    QCoreApplication::processEvents();
}

void Player::setPauseAfterOpen(bool pause)
{
    m_pauseAfterOpen = pause;
}

Player::TabIndex Player::tabIndex() const
{
    return TabIndex(m_tabs->currentIndex());
}

//----------------------------------------------------------------------------
// IEC standard dB scaling -- as borrowed from meterbridge (c) Steve Harris

static inline float IEC_dB ( float fScale )
{
    float dB = 0.0f;

    if (fScale < 0.025f)        // IEC_Scale(-60.0f)
        dB = (fScale / 0.0025f) - 70.0f;
    else if (fScale < 0.075f)   // IEC_Scale(-50.0f)
        dB = (fScale - 0.025f) / 0.005f - 60.0f;
    else if (fScale < 0.15f)    // IEC_Scale(-40.0f)
        dB = (fScale - 0.075f) / 0.0075f - 50.0f;
    else if (fScale < 0.3f)     // IEC_Scale(-30.0f)
        dB = (fScale - 0.15f) / 0.015f - 40.0f;
    else if (fScale < 0.5f)     // IEC_Scale(-20.0f)
        dB = (fScale - 0.3f) / 0.02f - 30.0f;
    else /* if (fScale < 1.0f)  // IED_Scale(0.0f)) */
        dB = (fScale - 0.5f) / 0.025f - 20.0f;

    return (dB > -0.001f && dB < 0.001f ? 0.0f : dB);
}

void Player::onVolumeChanged(int volume)
{
    const double gain = setVolume(volume);
    emit showStatusMessage(QStringLiteral("%L1 dB").arg(IEC_dB(gain)));
    Settings.setPlayerVolume(volume);
    Settings.setPlayerMuted(false);
    m_muteButton->setChecked(false);
    m_volumeButton->setIcon(QIcon::fromTheme("player-volume",
                                             QIcon(":/icons/oxygen/32x32/actions/player-volume.png")));
    m_muteButton->setIcon(QIcon::fromTheme("audio-volume-muted",
                                           QIcon(":/icons/oxygen/32x32/status/audio-volume-muted.png")));
    m_muteButton->setToolTip(tr("Mute"));
}

void Player::onCaptureStateChanged(bool active)
{
    Actions["playerPlayPauseAction"]->setDisabled(active);
}

void Player::onVolumeTriggered()
{
    // We must show first to realizes the volume popup geometry.
    m_volumePopup->show();
    int x = (m_volumeButton->width() - m_volumePopup->width()) / 2;
    int y = m_volumeButton->height() - m_volumePopup->height();
    m_volumePopup->move(m_volumeButton->mapToGlobal(QPoint(x, y)));
    m_volumeButton->hide();
    m_volumeButton->show();
}

void Player::onMuteButtonToggled(bool checked)
{
    m_muteButton->setChecked(checked);
    if (checked) {
        m_savedVolume = MLT.volume();
        MLT.setVolume(0);
        m_volumeButton->setIcon(QIcon::fromTheme("audio-volume-muted",
                                                 QIcon(":/icons/oxygen/32x32/status/audio-volume-muted.png")));
        m_muteButton->setIcon(QIcon::fromTheme("audio-volume-high",
                                               QIcon(":/icons/oxygen/32x32/status/audio-volume-high.png")));
        m_muteButton->setToolTip(tr("Unmute"));
    } else {
        MLT.setVolume(m_savedVolume);
        m_volumeButton->setIcon(QIcon::fromTheme("player-volume",
                                                 QIcon(":/icons/oxygen/32x32/actions/player-volume.png")));
        m_muteButton->setIcon(QIcon::fromTheme("audio-volume-muted",
                                               QIcon(":/icons/oxygen/32x32/status/audio-volume-muted.png")));
        m_muteButton->setToolTip(tr("Mute"));
    }
    Settings.setPlayerMuted(checked);
    m_volumePopup->hide();
}

void Player::setZoom(float factor, const QIcon &icon)
{
    emit zoomChanged(factor);
    Settings.setPlayerZoom(factor);
    if (factor == 0.0f) {
        m_zoomButton->setIcon(icon);
        m_zoomButton->setChecked(false);
        m_horizontalScroll->hide();
        m_verticalScroll->hide();
    } else {
        m_zoomToggleFactor = factor;
        adjustScrollBars(0.5f, 0.5f);
        m_zoomButton->setIcon(icon);
        m_zoomButton->setChecked(true);
    }
}

void Player::onZoomTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    setZoom(action->data().toFloat(), action->icon());
}

void Player::toggleZoom(bool checked)
{
    foreach (QAction *a, m_zoomMenu->actions()) {
        if ((!checked || m_zoomToggleFactor == 0.0f) && a->data().toFloat() == 0.0f) {
            setZoom(0.0f, a->icon());
            break;
        } else if (a->data().toFloat() == m_zoomToggleFactor) {
            setZoom(m_zoomToggleFactor, a->icon());
            break;
        }
    }
}

void Player::onGridToggled()
{
    m_gridButton->setChecked(true);
    m_gridDefaultAction = qobject_cast<QAction *>(sender());
    emit gridChanged(m_gridDefaultAction->data().toInt());
}

void Player::toggleGrid(bool checked)
{
    QAction *action = m_gridActionGroup->checkedAction();
    if (!checked) {
        if (action)
            action->setChecked(false);
        emit gridChanged(0);
    } else {
        if (!action)
            m_gridDefaultAction->trigger();
    }
}
