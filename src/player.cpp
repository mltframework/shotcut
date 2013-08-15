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

#include "player.h"
#include "scrubbar.h"
#include "mainwindow.h"
#include "widgets/timespinbox.h"
#include "widgets/audiosignal.h"
#include <QtWidgets>

#define VOLUME_KNEE (88)
#define SEEK_INACTIVE (-1)

Player::Player(QWidget *parent)
    : QWidget(parent)
    , m_position(0)
    , m_seekPosition(SEEK_INACTIVE)
    , m_isMeltedPlaying(-1)
{
    setObjectName("Player");
    Mlt::Controller::singleton(this);
    setupActions(this);
    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    actionPlay->setIcon(QIcon::fromTheme("media-playback-start", actionPlay->icon()));
    actionPause->setIcon(QIcon::fromTheme("media-playback-pause", actionPause->icon()));
    actionSkipNext->setIcon(QIcon::fromTheme("media-skip-forward", actionSkipNext->icon()));
    actionSkipPrevious->setIcon(QIcon::fromTheme("media-skip-backward", actionSkipPrevious->icon()));
    actionRewind->setIcon(QIcon::fromTheme("media-seek-backward", actionRewind->icon()));
    actionFastForward->setIcon(QIcon::fromTheme("media-seek-forward", actionFastForward->icon()));
    actionPlay->setEnabled(false);
    actionSkipPrevious->setEnabled(false);
    actionSkipNext->setEnabled(false);
    actionRewind->setEnabled(false);
    actionFastForward->setEnabled(false);
    m_playIcon = actionPlay->icon();
    m_pauseIcon = actionPause->icon();

    // Create a layout.
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setObjectName("playerLayout");
    vlayout->setMargin(0);
    vlayout->setSpacing(4);

    // Add the volume and signal level meter
    QWidget* tmp = new QWidget(this);
    vlayout->addWidget(tmp, 10);
    vlayout->addStretch();
    QHBoxLayout *hlayout = new QHBoxLayout(tmp);
    hlayout->setSpacing(4);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->addWidget(MLT.videoWidget(), 10);
    QVBoxLayout *volumeLayoutV = new QVBoxLayout;
    volumeLayoutV->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    QHBoxLayout *volumeLayoutH = new QHBoxLayout;
    volumeLayoutH->setSpacing(0);
    volumeLayoutH->setContentsMargins(0, 0, 0, 0);
    m_volumeSlider = new QSlider(Qt::Vertical);
    m_audioSignal = new AudioSignal(this);
    m_volumeSlider->setMinimumHeight(m_audioSignal->minimumHeight());
    m_volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    volumeLayoutH->addWidget(m_volumeSlider);
    volumeLayoutH->addWidget(m_audioSignal);
    volumeLayoutV->addLayout(volumeLayoutH);
    hlayout->addLayout(volumeLayoutV);
    m_volumeSlider->setRange(0, 99);
    m_volumeSlider->setValue(m_settings.value("player/volume", VOLUME_KNEE).toInt());
    onVolumeChanged(m_volumeSlider->value());
    m_savedVolume = MLT.volume();
    m_volumeSlider->setToolTip(tr("Adjust the audio volume"));
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)));
    connect(this, SIGNAL(audioLevels(QVector<double>)), m_audioSignal, SLOT(slotAudioLevels(QVector<double>)));

    QPushButton* volumeButton = new QPushButton(this);
    volumeButton->setObjectName(QString::fromUtf8("volumeButton"));
    volumeButton->setToolTip(tr("Show or hide the volume control"));
    volumeButton->setIcon(QIcon::fromTheme("player-volume", QIcon(":/icons/icons/player-volume.png")));
    volumeButton->setCheckable(true);
    volumeButton->setChecked(m_settings.value("player/volume-visible", false).toBool());
    onVolumeButtonToggled(volumeButton->isChecked());
    volumeLayoutV->addWidget(volumeButton);
    connect(volumeButton, SIGNAL(toggled(bool)), this, SLOT(onVolumeButtonToggled(bool)));

    QPushButton* muteButton = new QPushButton(this);
    muteButton->setObjectName(QString::fromUtf8("muteButton"));
    muteButton->setText(tr("Mute"));
    muteButton->setToolTip(tr("Silence the audio"));
    muteButton->setCheckable(true);
    muteButton->setChecked(m_settings.value("player/muted", false).toBool());
    volumeLayoutV->addWidget(muteButton);
    connect(muteButton, SIGNAL(toggled(bool)), this, SLOT(onMuteButtonToggled(bool)));

    // Add the scrub bar.
    m_scrubber = new ScrubBar(this);
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
//    m_durationLabel->setContentsMargins(0, 5, 0, 0);
    m_durationLabel->setFixedWidth(m_positionSpinner->width());
    m_inPointLabel = new QLabel(this);
    m_inPointLabel->setText("--:--:--:--");
    m_inPointLabel->setToolTip(tr("In Point"));
    m_inPointLabel->setAlignment(Qt::AlignRight);
#ifdef Q_OS_MAC
    m_inPointLabel->setContentsMargins(0, 5, 0, 0);
#else
    m_inPointLabel->setContentsMargins(0, 4, 0, 0);
#endif
    m_inPointLabel->setFixedWidth(m_inPointLabel->width());
    m_selectedLabel = new QLabel(this);
    m_selectedLabel->setText("--:--:--:--");
    m_selectedLabel->setToolTip(tr("Selected Duration"));
//    m_selectedLabel->setContentsMargins(0, 5, 0, 0);
    m_selectedLabel->setFixedWidth(m_selectedLabel->width());
    toolbar->addWidget(m_positionSpinner);
    toolbar->addWidget(m_durationLabel);
    toolbar->addWidget(spacer);
    toolbar->addAction(actionSkipPrevious);
    toolbar->addAction(actionRewind);
    toolbar->addAction(actionPlay);
    toolbar->addAction(actionFastForward);
    toolbar->addAction(actionSkipNext);
    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    toolbar->addWidget(m_inPointLabel);
    toolbar->addWidget(m_selectedLabel);
    vlayout->addWidget(toolbar);

    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onShowFrame(Mlt::QFrame)));
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
    setFocusPolicy(Qt::StrongFocus);
}

void Player::connectTransport(const TransportControllable* receiver)
{
    disconnect(SIGNAL(played(double)));
    disconnect(SIGNAL(paused()));
    disconnect(SIGNAL(stopped()));
    disconnect(SIGNAL(seeked(int)));
    disconnect(SIGNAL(rewound()));
    disconnect(SIGNAL(fastForwarded()));
    disconnect(SIGNAL(previousSought(int)));
    disconnect(SIGNAL(nextSought(int)));
    disconnect(SIGNAL(inChanged(int)));
    disconnect(SIGNAL(outChanged(int)));
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
    QIcon icon2;
    icon2.addFile(QString::fromUtf8(":/icons/icons/media-playback-start.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionPlay->setIcon(icon2);
    actionPause = new QAction(widget);
    actionPause->setObjectName(QString::fromUtf8("actionPause"));
    QIcon icon3;
    icon3.addFile(QString::fromUtf8(":/icons/icons/media-playback-pause.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionPause->setIcon(icon3);
    actionSkipNext = new QAction(widget);
    actionSkipNext->setObjectName(QString::fromUtf8("actionSkipNext"));
    QIcon icon4;
    icon4.addFile(QString::fromUtf8(":/icons/icons/media-skip-forward.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionSkipNext->setIcon(icon4);
    actionSkipPrevious = new QAction(widget);
    actionSkipPrevious->setObjectName(QString::fromUtf8("actionSkipPrevious"));
    QIcon icon5;
    icon5.addFile(QString::fromUtf8(":/icons/icons/media-skip-backward.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionSkipPrevious->setIcon(icon5);
    actionRewind = new QAction(widget);
    actionRewind->setObjectName(QString::fromUtf8("actionRewind"));
    QIcon icon6;
    icon6.addFile(QString::fromUtf8(":/icons/icons/media-seek-backward.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionRewind->setIcon(icon6);
    actionFastForward = new QAction(widget);
    actionFastForward->setObjectName(QString::fromUtf8("actionFastForward"));
    QIcon icon7;
    icon7.addFile(QString::fromUtf8(":/icons/icons/media-seek-forward.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionFastForward->setIcon(icon7);
    retranslateUi(widget);
    QMetaObject::connectSlotsByName(widget);
}

void Player::retranslateUi(QWidget* widget)
{
    actionPlay->setText(tr("Play"));
#ifndef QT_NO_TOOLTIP
    actionPlay->setToolTip(tr("Start playback (L)"));
#endif // QT_NO_TOOLTIP
    actionPlay->setShortcut(tr("Space"));
    actionPause->setText(tr("Pause"));
#ifndef QT_NO_TOOLTIP
    actionPause->setToolTip(tr("Pause playback (K)"));
#endif // QT_NO_TOOLTIP
    actionPause->setShortcut(tr("Backspace"));
    actionSkipNext->setText(tr("Skip Next"));
#ifndef QT_NO_TOOLTIP
    actionSkipNext->setToolTip(tr("Skip to the next point (Alt+Right)"));
#endif // QT_NO_TOOLTIP
    actionSkipNext->setShortcut(tr("Alt+Right"));
    actionSkipPrevious->setText(tr("Skip Previous"));
#ifndef QT_NO_TOOLTIP
    actionSkipPrevious->setToolTip(tr("Skip to the previous point (Alt+Left)"));
#endif // QT_NO_TOOLTIP
    actionSkipPrevious->setShortcut(tr("Alt+Left"));
    actionRewind->setText(tr("Rewind"));
#ifndef QT_NO_TOOLTIP
    actionRewind->setToolTip(tr("Play quickly backwards (J)"));
#endif // QT_NO_TOOLTIP
    actionFastForward->setText(tr("Fast Forward"));
#ifndef QT_NO_TOOLTIP
    actionFastForward->setToolTip(tr("Play quickly forwards (L)"));
#endif // QT_NO_TOOLTIP
}

void Player::setIn(int pos)
{
    m_scrubber->setInPoint(pos);
    if (pos >= 0 && pos >= m_previousOut)
        setOut(m_duration - 1);
}

void Player::setOut(int pos)
{
    m_scrubber->setOutPoint(pos);
    if (pos >= 0 && pos <= m_previousIn)
        setIn(0);
}

void Player::setMarkers(const QList<int> &markers)
{
    m_scrubber->setMarkers(markers);
}

void Player::resizeEvent(QResizeEvent*)
{
    MLT.onWindowResize();
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
        actionPlay->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/icons/icons/media-playback-stop.png")));
        actionPlay->setText(tr("Stop"));
        actionPlay->setToolTip(tr("Stop playback (K)"));
    }
}

void Player::pause()
{
    emit paused();
    actionPlay->setIcon(m_playIcon);
    actionPlay->setText(tr("Play"));
    actionPlay->setToolTip(tr("Start playback (L)"));
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
        emit seeked();
        if (position >= 0) {
            if (m_seekPosition == SEEK_INACTIVE)
                emit seeked(qMin(position, m_duration - 1));
            m_seekPosition = qMin(position, m_duration - 1);
        }
    }
    // Seek implies pause.
    actionPlay->setIcon(m_playIcon);
    actionPlay->setText(tr("Play"));
    actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::onProducerOpened()
{
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
        m_previousIn = MLT.producer()->get_in();
        m_scrubber->setEnabled(true);
        m_scrubber->setInPoint(m_previousIn);
        m_previousOut = MLT.producer()->get_out();
        m_scrubber->setOutPoint(m_previousOut);
    }
    else {
        m_durationLabel->setText(tr("Live").prepend(" / "));
        m_scrubber->setDisabled(true);
        // cause scrubber redraw
        m_scrubber->setScale(m_duration);
    }
    m_positionSpinner->setEnabled(m_isSeekable);
    onMuteButtonToggled(m_settings.value("player/muted", false).toBool());
    onVolumeChanged(m_volumeSlider->value());

    actionPlay->setEnabled(true);
    actionSkipPrevious->setEnabled(m_isSeekable);
    actionSkipNext->setEnabled(m_isSeekable);
    actionRewind->setEnabled(m_isSeekable);
    actionFastForward->setEnabled(m_isSeekable);

    if (!MLT.profile().is_explicit())
        emit profileChanged();
    play();
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
    onVolumeChanged(m_volumeSlider->value());
    onMuteButtonToggled(m_settings.value("player/muted", false).toBool());
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
    if (m_seekPosition != SEEK_INACTIVE)
        emit seeked(m_seekPosition);
    m_seekPosition = SEEK_INACTIVE;
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

void Player::onShowFrame(Mlt::QFrame frame)
{
    int position = frame.frame()->get_position();
    if (position < m_duration) {
        m_position = position;
        m_positionSpinner->blockSignals(true);
        m_positionSpinner->setValue(position);
        m_positionSpinner->blockSignals(false);
        m_scrubber->onSeek(position);
    }
    if (position >= m_duration)
        emit endOfStream();
    if (m_seekPosition != SEEK_INACTIVE)
        emit seeked(m_seekPosition);
    m_seekPosition = SEEK_INACTIVE;
    showAudio(frame.frame());
}

void Player::updateSelection()
{
    if (MLT.producer() && MLT.producer()->get_in() > 0) {
        m_inPointLabel->setText(QString(MLT.producer()->get_time("in")).append(" / "));
        MLT.producer()->set("_shotcut_selected", MLT.producer()->get_playtime());
        m_selectedLabel->setText(MLT.producer()->get_time("_shotcut_selected"));
    } else {
        m_inPointLabel->setText("--:--:--:-- / ");
        if (MLT.producer() && !MLT.isPlaylist() &&
                MLT.producer()->get_out() < m_duration - 1) {
            MLT.producer()->set("_shotcut_selected", MLT.producer()->get_playtime());
            m_selectedLabel->setText(MLT.producer()->get_time("_shotcut_selected"));
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

void Player::showAudio(Mlt::Frame* frame)
{
    if (frame->get_int("test_audio"))
        return;
    QVector<double> channels;
    int n = frame->get_int("audio_channels");
    while (n--) {
        QString s = QString("meta.media.audio_level.%1").arg(n);
        channels << frame->get_double(s.toLatin1().constData());
    }
    emit audioLevels(channels);
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
    const double gain = double(volume) / VOLUME_KNEE;
    MLT.setVolume(gain);
    emit showStatusMessage(QString("%1 dB").arg(IEC_dB(gain)));
    m_settings.setValue("player/volume", volume);
}

void Player::onCaptureStateChanged(bool active)
{
    actionPlay->setDisabled(active);
}

void Player::resetProfile()
{
    MLT.setProfile(m_settings.value("player/profile").toString());
    emit profileChanged();
}

void Player::onVolumeButtonToggled(bool checked)
{
    m_audioSignal->setVisible(checked);
    m_volumeSlider->setVisible(checked);
    m_settings.setValue("player/volume-visible", checked);
}

void Player::onMuteButtonToggled(bool checked)
{
    if (checked) {
        m_savedVolume = MLT.volume();
        MLT.setVolume(0);
    } else {
        MLT.setVolume(m_savedVolume);
    }
    m_settings.setValue("player/muted", checked);
}
