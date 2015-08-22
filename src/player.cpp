/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "player.h"
#include "scrubbar.h"
#include "mainwindow.h"
#include "widgets/timespinbox.h"
#include "widgets/audioscale.h"
#include "settings.h"
#include <QtWidgets>

#define VOLUME_KNEE (88)
#define SEEK_INACTIVE (-1)
#define VOLUME_SLIDER_HEIGHT (300)

Player::Player(QWidget *parent)
    : QWidget(parent)
    , m_position(0)
    , m_isMeltedPlaying(-1)
    , m_zoomToggleFactor(Settings.playerZoom() == 0.0f? 1.0f : Settings.playerZoom())
    , m_pauseAfterOpen(false)
    , m_monitorScreen(-1)
    , m_currentTransport(0)
{
    setObjectName("Player");
    Mlt::Controller::singleton();
    setupActions(this);
    m_playIcon = actionPlay->icon();
    m_pauseIcon = actionPause->icon();

    // Create a layout.
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setObjectName("playerLayout");
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(4);

    // Add tab bar to indicate/select what is playing: clip, playlist, timeline.
    m_tabs = new QTabBar;
    m_tabs->setShape(QTabBar::RoundedSouth);
    m_tabs->addTab(tr("Source"));
    m_tabs->addTab(tr("Program"));
    m_tabs->setTabEnabled(ProgramTabIndex, false);
    QHBoxLayout* tabLayout = new QHBoxLayout;
    tabLayout->addWidget(m_tabs);
    tabLayout->addStretch();
    connect(m_tabs, SIGNAL(tabBarClicked(int)), SLOT(onTabBarClicked(int)));

    // Add the layouts for managing video view, scroll bars, and audio controls.
    m_videoLayout = new QHBoxLayout;
    m_videoLayout->setSpacing(4);
    m_videoLayout->setContentsMargins(0, 0, 0, 0);
    vlayout->addLayout(m_videoLayout, 10);
    vlayout->addStretch();
    m_videoScrollWidget = new QWidget;
    m_videoLayout->addWidget(m_videoScrollWidget, 10);
    m_videoLayout->addStretch();
    QGridLayout* glayout = new QGridLayout(m_videoScrollWidget);
    glayout->setSpacing(0);
    glayout->setContentsMargins(0, 0, 0, 0);

    // Add the video widgets.
    m_videoWidget = QWidget::createWindowContainer(qobject_cast<QWindow*>(MLT.videoWidget()));
    m_videoWidget->setMinimumSize(QSize(320, 180));
    glayout->addWidget(m_videoWidget, 0, 0);
    m_verticalScroll = new QScrollBar(Qt::Vertical);
    glayout->addWidget(m_verticalScroll, 0, 1);
    m_verticalScroll->hide();
    m_horizontalScroll = new QScrollBar(Qt::Horizontal);
    glayout->addWidget(m_horizontalScroll, 1, 0);
    m_horizontalScroll->hide();
#ifdef Q_OS_WIN
    // Workaround video not showing on Windows with Qt 5.5 upgrade.
    m_videoWidget->hide();
#endif

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
    m_muteButton->setIcon(QIcon::fromTheme("dialog-cancel", QIcon(":/icons/oxygen/16x16/actions/dialog-cancel.png")));
    m_muteButton->setToolTip(tr("Silence the audio"));
    m_muteButton->setCheckable(true);
    m_muteButton->setChecked(Settings.playerMuted());
    onMuteButtonToggled(Settings.playerMuted());
    volumeLayoutH->addWidget(m_muteButton);
    connect(m_muteButton, SIGNAL(toggled(bool)), this, SLOT(onMuteButtonToggled(bool)));

    // This hack realizes the volume popup geometry for on_actionVolume_triggered().
    m_volumePopup->show();
    m_volumePopup->hide();

    // Add the scrub bar.
    m_scrubber = new ScrubBar(this);
    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setObjectName("scrubBar");
    m_scrubber->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    vlayout->addWidget(m_scrubber);

    // Add toolbar for transport controls.
    QToolBar* toolbar = new QToolBar(tr("Transport Controls"), this);
    int s = style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolbar->setIconSize(QSize(s, s));
    toolbar->setContentsMargins(0, 0, 0, 0);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_positionSpinner = new TimeSpinBox(this);
    m_positionSpinner->setToolTip(tr("Current position"));
    m_positionSpinner->setEnabled(false);
    m_positionSpinner->setKeyboardTracking(false);
    m_durationLabel = new QLabel(this);
    m_durationLabel->setToolTip(tr("Total Duration"));
    m_durationLabel->setText(" / 00:00:00:00");
    m_durationLabel->setFixedWidth(m_positionSpinner->width());
    m_inPointLabel = new QLabel(this);
    m_inPointLabel->setText("--:--:--:--");
    m_inPointLabel->setToolTip(tr("In Point"));
    m_inPointLabel->setFixedWidth(m_inPointLabel->width());
    m_selectedLabel = new QLabel(this);
    m_selectedLabel->setText("--:--:--:--");
    m_selectedLabel->setToolTip(tr("Selected Duration"));
    m_selectedLabel->setFixedWidth(m_selectedLabel->width());
    toolbar->addWidget(m_positionSpinner);
    toolbar->addWidget(m_durationLabel);
    toolbar->addWidget(spacer);
    toolbar->addAction(actionSkipPrevious);
    toolbar->addAction(actionRewind);
    toolbar->addAction(actionPlay);
    toolbar->addAction(actionFastForward);
    toolbar->addAction(actionSkipNext);

    // Add zoom button to toolbar.
    m_zoomButton = new QToolButton;
    QMenu* zoomMenu = new QMenu(this);
    m_zoomFitAction = zoomMenu->addAction(
        QIcon::fromTheme("zoom-fit-best", QIcon(":/icons/oxygen/32x32/actions/zoom-fit-best")),
        tr("Zoom Fit"), this, SLOT(zoomFit()));
    m_zoomOriginalAction = zoomMenu->addAction(
        QIcon::fromTheme("zoom-original", QIcon(":/icons/oxygen/32x32/actions/zoom-original")),
        tr("Zoom 100%"), this, SLOT(zoomOriginal()));
    m_zoomOutAction = zoomMenu->addAction(
        QIcon::fromTheme("zoom-out", QIcon(":/icons/oxygen/32x32/actions/zoom-out")),
        tr("Zoom 50%"), this, SLOT(zoomOut()));
    m_zoomInAction = zoomMenu->addAction(
        QIcon::fromTheme("zoom-in", QIcon(":/icons/oxygen/32x32/actions/zoom-in")),
        tr("Zoom 200%"), this, SLOT(zoomIn()));
    connect(m_zoomButton, SIGNAL(toggled(bool)), SLOT(toggleZoom(bool)));
    m_zoomButton->setMenu(zoomMenu);
    m_zoomButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_zoomButton->setCheckable(true);
    m_zoomButton->setToolTip(tr("Toggle zoom"));
    toolbar->addWidget(m_zoomButton);
    toolbar->addAction(actionVolume);
    m_volumeWidget = toolbar->widgetForAction(actionVolume);

    // Add in-point and selected duration labels to toolbar.
    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    toolbar->addWidget(m_inPointLabel);
    toolbar->addWidget(m_selectedLabel);
    vlayout->addWidget(toolbar);
    vlayout->addLayout(tabLayout);

    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onFrameDisplayed(const SharedFrame&)));
    connect(actionPlay, SIGNAL(triggered()), this, SLOT(togglePlayPaused()));
    connect(actionPause, SIGNAL(triggered()), this, SLOT(pause()));
    connect(actionFastForward, SIGNAL(triggered()), this, SLOT(fastForward()));
    connect(actionRewind, SIGNAL(triggered()), this, SLOT(rewind()));
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(seek(int)));
    connect(m_scrubber, SIGNAL(inChanged(int)), this, SLOT(onInChanged(int)));
    connect(m_scrubber, SIGNAL(outChanged(int)), this, SLOT(onOutChanged(int)));
    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    connect(m_positionSpinner, SIGNAL(editingFinished()), this, SLOT(setFocus()));
    connect(this, SIGNAL(endOfStream()), this, SLOT(pause()));
    connect(this, SIGNAL(zoomChanged(float)), MLT.videoWidget(), SLOT(setZoom(float)));
    connect(m_horizontalScroll, SIGNAL(valueChanged(int)), MLT.videoWidget(), SLOT(setOffsetX(int)));
    connect(m_verticalScroll, SIGNAL(valueChanged(int)), MLT.videoWidget(), SLOT(setOffsetY(int)));
    setFocusPolicy(Qt::StrongFocus);
}

void Player::connectTransport(const TransportControllable* receiver)
{
    if (receiver == m_currentTransport) return;
    if (m_currentTransport)
        disconnect(m_currentTransport);
    m_currentTransport = receiver;
    connect(this, SIGNAL(played(double)), receiver, SLOT(play(double)));
    connect(this, SIGNAL(paused()), receiver, SLOT(pause()));
    connect(this, SIGNAL(stopped()), receiver, SLOT(stop()));
    connect(this, SIGNAL(seeked(int)), receiver, SLOT(seek(int)));
    connect(this, SIGNAL(rewound()), receiver, SLOT(rewind()));
    connect(this, SIGNAL(fastForwarded()), receiver, SLOT(fastForward()));
    connect(this, SIGNAL(previousSought(int)), receiver, SLOT(previous(int)));
    connect(this, SIGNAL(nextSought(int)), receiver, SLOT(next(int)));
    connect(this, SIGNAL(inChanged(int)), receiver, SLOT(setIn(int)));
    connect(this, SIGNAL(outChanged(int)), receiver, SLOT(setOut(int)));
}

void Player::setupActions(QWidget* widget)
{
    actionPlay = new QAction(widget);
    actionPlay->setObjectName(QString::fromUtf8("actionPlay"));
    actionPlay->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/oxygen/32x32/actions/media-playback-start.png")));
    actionPlay->setDisabled(true);
    actionPause = new QAction(widget);
    actionPause->setObjectName(QString::fromUtf8("actionPause"));
    actionPause->setIcon(QIcon::fromTheme("media-playback-pause", QIcon(":/icons/oxygen/32x32/actions/media-playback-pause.png")));
    actionPause->setDisabled(true);
    actionSkipNext = new QAction(widget);
    actionSkipNext->setObjectName(QString::fromUtf8("actionSkipNext"));
    actionSkipNext->setIcon(QIcon::fromTheme("media-skip-forward", QIcon(":/icons/oxygen/32x32/actions/media-skip-forward.png")));
    actionSkipNext->setDisabled(true);
    actionSkipPrevious = new QAction(widget);
    actionSkipPrevious->setObjectName(QString::fromUtf8("actionSkipPrevious"));
    actionSkipPrevious->setIcon(QIcon::fromTheme("media-skip-backward", QIcon(":/icons/oxygen/32x32/actions/media-skip-backward.png")));
    actionSkipPrevious->setDisabled(true);
    actionRewind = new QAction(widget);
    actionRewind->setObjectName(QString::fromUtf8("actionRewind"));
    actionRewind->setIcon(QIcon::fromTheme("media-seek-backward", QIcon(":/icons/oxygen/32x32/actions/media-seek-backward.png")));
    actionRewind->setDisabled(true);
    actionFastForward = new QAction(widget);
    actionFastForward->setObjectName(QString::fromUtf8("actionFastForward"));
    actionFastForward->setIcon(QIcon::fromTheme("media-seek-forward", QIcon(":/icons/oxygen/32x32/actions/media-seek-forward.png")));
    actionFastForward->setDisabled(true);
    actionVolume = new QAction(widget);
    actionVolume->setObjectName(QString::fromUtf8("actionVolume"));
    actionVolume->setIcon(QIcon::fromTheme("player-volume", QIcon(":/icons/oxygen/16x16/actions/player-volume.png")));
    retranslateUi(widget);
    QMetaObject::connectSlotsByName(widget);
}

void Player::retranslateUi(QWidget* widget)
{
    Q_UNUSED(widget)
    actionPlay->setText(tr("Play"));
#ifndef QT_NO_TOOLTIP
    actionPlay->setToolTip(tr("Start playback (L)"));
#endif // QT_NO_TOOLTIP
    actionPlay->setShortcut(QString("Space"));
    actionPause->setText(tr("Pause"));
#ifndef QT_NO_TOOLTIP
    actionPause->setToolTip(tr("Pause playback (K)"));
#endif // QT_NO_TOOLTIP
    actionSkipNext->setText(tr("Skip Next"));
#ifndef QT_NO_TOOLTIP
    actionSkipNext->setToolTip(tr("Skip to the next point (Alt+Right)"));
#endif // QT_NO_TOOLTIP
    actionSkipNext->setShortcut(QString("Alt+Right"));
    actionSkipPrevious->setText(tr("Skip Previous"));
#ifndef QT_NO_TOOLTIP
    actionSkipPrevious->setToolTip(tr("Skip to the previous point (Alt+Left)"));
#endif // QT_NO_TOOLTIP
    actionSkipPrevious->setShortcut(QString("Alt+Left"));
    actionRewind->setText(tr("Rewind"));
#ifndef QT_NO_TOOLTIP
    actionRewind->setToolTip(tr("Play quickly backwards (J)"));
#endif // QT_NO_TOOLTIP
    actionFastForward->setText(tr("Fast Forward"));
#ifndef QT_NO_TOOLTIP
    actionFastForward->setToolTip(tr("Play quickly forwards (L)"));
#endif // QT_NO_TOOLTIP
    actionVolume->setText(tr("Volume"));
#ifndef QT_NO_TOOLTIP
    actionVolume->setToolTip(tr("Show the volume control"));
#endif
}

void Player::setIn(int pos)
{
    m_scrubber->setInPoint(pos);
    if (pos >= 0 && pos > m_previousOut)
        setOut(m_duration - 1);
}

void Player::setOut(int pos)
{
    m_scrubber->setOutPoint(pos);
    if (pos >= 0 && pos < m_previousIn)
        setIn(0);
}

void Player::setMarkers(const QList<int> &markers)
{
    m_scrubber->setMarkers(markers);
}

QSize Player::videoSize() const
{
    return m_videoWidget->size();
}

void Player::resizeEvent(QResizeEvent*)
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
}

void Player::play(double speed)
{
    emit played(speed);
    if (m_isSeekable) {
        actionPlay->setIcon(m_pauseIcon);
        actionPlay->setText(tr("Pause"));
        actionPlay->setToolTip(tr("Pause playback (K)"));
    }
    else {
        actionPlay->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/icons/oxygen/32x32/actions/media-playback-stop.png")));
        actionPlay->setText(tr("Stop"));
        actionPlay->setToolTip(tr("Stop playback (K)"));
    }
}

void Player::pause()
{
    emit paused();
    showPaused();
}

void Player::stop()
{
    emit stopped();
    actionPlay->setIcon(m_playIcon);
    actionPlay->setText(tr("Play"));
    actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::togglePlayPaused()
{
    if (actionPlay->icon().cacheKey() == m_playIcon.cacheKey())
        play();
    else if (m_isSeekable)
        pause();
    else
        stop();
}

void Player::seek(int position)
{
    if (m_isSeekable) {
        if (position >= 0) {
            emit seeked(qMin(position, m_duration - 1));
        }
    }
    // Seek implies pause.
    actionPlay->setIcon(m_playIcon);
    actionPlay->setText(tr("Play"));
    actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::onProducerOpened(bool play)
{
#ifdef Q_OS_WIN
    // Workaround video not showing on Windows with Qt 5.5 upgrade.
    m_videoWidget->show();
#endif
    m_duration = MLT.producer()->get_length();
    m_isSeekable = MLT.isSeekable();
    MLT.producer()->set("ignore_points", 1);
    m_scrubber->setFramerate(MLT.profile().fps());
    m_scrubber->setScale(m_duration);
    if (!MLT.isPlaylist())
        m_scrubber->setMarkers(QList<int>());
    m_inPointLabel->setText("--:--:--:-- / ");
    m_selectedLabel->setText("--:--:--:--");
    if (m_isSeekable) {
        m_durationLabel->setText(QString(MLT.producer()->get_length_time()).prepend(" / "));
        m_previousIn = MLT.isClip()? MLT.producer()->get_in() : -1;
        m_scrubber->setEnabled(true);
        m_scrubber->setInPoint(m_previousIn);
        m_previousOut = MLT.isClip()? MLT.producer()->get_out() : -1;
        m_scrubber->setOutPoint(m_previousOut);
    }
    else {
        m_durationLabel->setText(tr("Live").prepend(" / "));
        m_scrubber->setDisabled(true);
        // cause scrubber redraw
        m_scrubber->setScale(m_duration);
    }
    m_positionSpinner->setEnabled(m_isSeekable);
    setVolume(m_volumeSlider->value());
    m_savedVolume = MLT.volume();
    onMuteButtonToggled(Settings.playerMuted());
    toggleZoom(Settings.playerZoom() > 0.0f);

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);

    if (!MLT.profile().is_explicit())
        emit profileChanged();
    connectTransport(MLT.transportControl());

    // Closing the previous producer might call pause() milliseconds before
    // calling play() here. Delays while purging the consumer on pause can
    // interfere with the play() call. So, we delay play a little to let
    // pause purging to complete.
    if (play) {
        if (m_pauseAfterOpen) {
            m_pauseAfterOpen = false;
            QTimer::singleShot(500, this, SLOT(postProducerOpened()));
        } else {
            if (MLT.consumer()->is_stopped()) {
                this->play();
            } else {
                // This seek purges the consumer to prevent latent end-of-stream detection.
                seek(0);
                QTimer::singleShot(500, this, SLOT(play()));
            }
        }
    }
}

void Player::postProducerOpened()
{
    seek(MLT.producer()->position());
}

void Player::onMeltedUnitOpened()
{
    m_isMeltedPlaying = -1; // unknown
    m_duration = MLT.producer()->get_length();
    m_isSeekable = true;
    MLT.producer()->set("ignore_points", 1);
    m_scrubber->setFramerate(MLT.profile().fps());
    m_scrubber->setScale(m_duration);
    m_scrubber->setMarkers(QList<int>());
    m_inPointLabel->setText("--:--:--:-- / ");
    m_selectedLabel->setText("--:--:--:--");
    m_durationLabel->setText(QString(MLT.producer()->get_length_time()).prepend(" / "));
    m_previousIn = MLT.producer()->get_in();
    m_scrubber->setEnabled(true);
    m_scrubber->setInPoint(m_previousIn);
    m_previousOut = MLT.producer()->get_out();
    m_scrubber->setOutPoint(m_previousOut);
    m_positionSpinner->setEnabled(m_isSeekable);
    setVolume(m_volumeSlider->value());
    m_savedVolume = MLT.volume();
    onMuteButtonToggled(Settings.playerMuted());
    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);
    setIn(-1);
    setOut(-1);
    setFocus();
}

void Player::onProducerModified()
{
    m_duration = MLT.producer()->get_length();
    m_isSeekable = MLT.isSeekable();
    m_scrubber->setScale(m_duration);
    m_scrubber->setMarkers(QList<int>());
    m_durationLabel->setText(QString(MLT.producer()->get_length_time()).prepend(" / "));
    if (MLT.producer()->get_speed() == 0)
        seek(m_position);
    else if (m_position >= m_duration)
        seek(m_duration - 1);
}

void Player::onShowFrame(int position, double fps, int in, int out, int length, bool isPlaying)
{
    m_duration = length;
    MLT.producer()->set("length", length);
    m_durationLabel->setText(QString(MLT.producer()->get_length_time()).prepend(" / "));
    m_scrubber->setFramerate(fps);
    m_scrubber->setScale(m_duration);
    if (position < m_duration) {
        m_position = position;
        m_positionSpinner->blockSignals(true);
        m_positionSpinner->setValue(position);
        m_positionSpinner->blockSignals(false);
        m_scrubber->onSeek(position);
    }
    if (m_isMeltedPlaying == -1 || isPlaying != m_isMeltedPlaying) {
        m_isMeltedPlaying = isPlaying;
        if (isPlaying) {
            actionPlay->setIcon(m_pauseIcon);
            actionPlay->setText(tr("Pause"));
            actionPlay->setToolTip(tr("Pause playback (K)"));
        }
        else {
            actionPlay->setIcon(m_playIcon);
            actionPlay->setText(tr("Play"));
            actionPlay->setToolTip(tr("Start playback (L)"));
        }
    }
    m_previousIn = in;
    m_previousOut = out;
    m_scrubber->blockSignals(true);
    setIn(in);
    setOut(out);
    m_scrubber->blockSignals(false);
}

void Player::onFrameDisplayed(const SharedFrame& frame)
{
    int position = frame.get_position();
    if (position < m_duration) {
        m_position = position;
        m_positionSpinner->blockSignals(true);
        m_positionSpinner->setValue(position);
        m_positionSpinner->blockSignals(false);
        m_scrubber->onSeek(position);
    }
    if (position >= m_duration)
        emit endOfStream();
}

void Player::updateSelection()
{
    if (MLT.producer() && MLT.producer()->get_in() > 0) {
        m_inPointLabel->setText(QString(MLT.producer()->get_time("in")).append(" / "));
        m_selectedLabel->setText(MLT.producer()->frames_to_time(MLT.producer()->get_playtime()));
    } else {
        m_inPointLabel->setText("--:--:--:-- / ");
        if (MLT.producer() && MLT.isClip() &&
                MLT.producer()->get_out() < m_duration - 1) {
            m_selectedLabel->setText(MLT.producer()->frames_to_time(MLT.producer()->get_playtime()));
        } else if (!MLT.producer() || MLT.producer()->get_in() == 0) {
            m_selectedLabel->setText("--:--:--:--");
        }
    }
}

void Player::onInChanged(int in)
{
    if (in != m_previousIn)
        emit inChanged(in);
    m_previousIn = in;
    updateSelection();
}

void Player::onOutChanged(int out)
{
    if (out != m_previousOut)
        emit outChanged(out);
    m_previousOut = out;
    updateSelection();
}

void Player::on_actionSkipNext_triggered()
{
    emit showStatusMessage(actionSkipNext->toolTip());
    if (m_scrubber->markers().size() > 0) {
        foreach (int x, m_scrubber->markers()) {
            if (x > m_position) {
                emit seeked(x);
                return;
            }
        }
        emit seeked(m_duration - 1);
    }
    else {
        emit nextSought(m_position);
        emit nextSought();
    }
}

void Player::on_actionSkipPrevious_triggered()
{
    emit showStatusMessage(actionSkipPrevious->toolTip());
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
    }
    else {
        emit previousSought(m_position);
        emit previousSought();
    }
}

void Player::rewind()
{
    if (m_isSeekable)
        emit rewound();
}

void Player::fastForward()
{
    if (m_isSeekable)
        emit fastForwarded();
    else
        play();
}

void Player::showPaused()
{
    actionPlay->setIcon(m_playIcon);
    actionPlay->setText(tr("Play"));
    actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::showPlaying()
{
    actionPlay->setIcon(m_pauseIcon);
    actionPlay->setText(tr("Pause"));
    actionPlay->setToolTip(tr("Pause playback (K)"));
}

void Player::switchToTab(TabIndex index)
{
    m_tabs->setCurrentIndex(index);
}

void Player::enableTab(TabIndex index, bool enabled)
{
    m_tabs->setTabEnabled(index, enabled);
}

void Player::onTabBarClicked(int index)
{
    switch (index) {
    case SourceTabIndex:
        if (MLT.savedProducer() && MLT.savedProducer()->is_valid() && MLT.producer()
            && MLT.producer()->get_producer() != MLT.savedProducer()->get_producer()) {
            m_pauseAfterOpen = true;
            MAIN.open(new Mlt::Producer(MLT.savedProducer()));
        }
        break;
    case ProgramTabIndex:
        if (MAIN.multitrack()) {
            if (!MLT.isMultitrack() && MAIN.multitrack())
                MAIN.seekTimeline(MAIN.multitrack()->position());
        } else {
            if (!MLT.isPlaylist() && MAIN.playlist())
                MAIN.seekPlaylist(MAIN.playlist()->position());
        }
        break;
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

void Player::moveVideoToScreen(int screen)
{
    if (screen == m_monitorScreen) return;
    if (screen == -2) {
        // -2 = embedded
        if (!m_videoScrollWidget->isFullScreen()) return;
        m_videoScrollWidget->showNormal();
        m_videoLayout->insertWidget(0, m_videoScrollWidget, 10);
    } else if (QApplication::desktop()->screenCount() > 1) {
        // -1 = find first screen the app is not using
        for (int i = 0; screen == -1 && i < QApplication::desktop()->screenCount(); i++) {
            if (i != QApplication::desktop()->screenNumber(this))
                screen = i;
        }
        m_videoScrollWidget->setParent(QApplication::desktop()->screen(screen));
        m_videoScrollWidget->move(QApplication::desktop()->screenGeometry(screen).bottomLeft());
        m_videoScrollWidget->showFullScreen();
    }
    m_monitorScreen = screen;
}

void Player::setPauseAfterOpen(bool pause)
{
    m_pauseAfterOpen = pause;
}

Player::TabIndex Player::tabIndex() const
{
    return (TabIndex)m_tabs->currentIndex();
}

//----------------------------------------------------------------------------
// IEC standard dB scaling -- as borrowed from meterbridge (c) Steve Harris

static inline float IEC_dB ( float fScale )
{
	float dB = 0.0f;

	if (fScale < 0.025f)	    // IEC_Scale(-60.0f)
		dB = (fScale / 0.0025f) - 70.0f;
	else if (fScale < 0.075f)	// IEC_Scale(-50.0f)
		dB = (fScale - 0.025f) / 0.005f - 60.0f;
	else if (fScale < 0.15f)	// IEC_Scale(-40.0f)
		dB = (fScale - 0.075f) / 0.0075f - 50.0f;
	else if (fScale < 0.3f)		// IEC_Scale(-30.0f)
		dB = (fScale - 0.15f) / 0.015f - 40.0f;
	else if (fScale < 0.5f)		// IEC_Scale(-20.0f)
		dB = (fScale - 0.3f) / 0.02f - 30.0f;
	else /* if (fScale < 1.0f)	// IED_Scale(0.0f)) */
		dB = (fScale - 0.5f) / 0.025f - 20.0f;

	return (dB > -0.001f && dB < 0.001f ? 0.0f : dB);
}

void Player::onVolumeChanged(int volume)
{
    const double gain = setVolume(volume);
    emit showStatusMessage(QString("%L1 dB").arg(IEC_dB(gain)));
    Settings.setPlayerVolume(volume);
    Settings.setPlayerMuted(false);
    m_muteButton->setChecked(false);
}

void Player::onCaptureStateChanged(bool active)
{
    actionPlay->setDisabled(active);
}

void Player::resetProfile()
{
    MLT.setProfile(Settings.playerProfile());
    emit profileChanged();
}

void Player::on_actionVolume_triggered()
{
    int x = (m_volumePopup->width() - m_volumeWidget->width()) / 2;
    x = mapToParent(m_volumeWidget->geometry().bottomLeft()).x() - x;
    int y = 8 - m_volumePopup->height();
    m_volumePopup->move(mapToGlobal(m_scrubber->geometry().bottomLeft()) + QPoint(x, y));
    m_volumePopup->show();
    m_volumeWidget->hide();
    m_volumeWidget->show();
}

void Player::onMuteButtonToggled(bool checked)
{
    if (checked) {
        m_savedVolume = MLT.volume();
        MLT.setVolume(0);
    } else {
        MLT.setVolume(m_savedVolume);
    }
    Settings.setPlayerMuted(checked);
    m_volumePopup->hide();
}

void Player::setZoom(float factor, const QIcon& icon)
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

void Player::zoomFit()
{
    setZoom(0.0f, m_zoomFitAction->icon());
}

void Player::zoomOriginal()
{
    setZoom(1.0f, m_zoomOriginalAction->icon());
}

void Player::zoomOut()
{
    setZoom(0.5f, m_zoomOutAction->icon());
}

void Player::zoomIn()
{
    setZoom(2.0f, m_zoomInAction->icon());
}

void Player::toggleZoom(bool checked)
{
    if (!checked || m_zoomToggleFactor == 0.0f)
        zoomFit();
    else if (m_zoomToggleFactor == 1.0f)
        zoomOriginal();
    else if (m_zoomToggleFactor == 0.5f)
        zoomOut();
    else if (m_zoomToggleFactor == 2.0f)
        zoomIn();
}
