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
#include <QtGui>

#define VOLUME_KNEE (88)
#define SEEK_INACTIVE (-1)

QT_BEGIN_NAMESPACE

namespace Ui {
class Player
{
public:
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

    void addProfile(QWidget* widget, const QString& desc, const QString& name)
    {
        QAction* action = new QAction(desc, widget);
        action->setCheckable(true);
        action->setData(name);
        profileGroup->addAction(action);
    }

    void setupActions(QWidget* widget)
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
        actionProgressive = new QAction(widget);
        actionProgressive->setObjectName(QString::fromUtf8("actionProgressive"));
        actionProgressive->setCheckable(true);
        actionOneField = new QAction(widget);
        actionOneField->setObjectName(QString::fromUtf8("actionOneField"));
        actionOneField->setCheckable(true);
        actionLinearBlend = new QAction(widget);
        actionLinearBlend->setObjectName(QString::fromUtf8("actionLinearBlend"));
        actionLinearBlend->setCheckable(true);
        actionYadifTemporal = new QAction(widget);
        actionYadifTemporal->setObjectName(QString::fromUtf8("actionYadifTemporal"));
        actionYadifTemporal->setCheckable(true);
        actionYadifSpatial = new QAction(widget);
        actionYadifSpatial->setObjectName(QString::fromUtf8("actionYadifSpatial"));
        actionYadifSpatial->setCheckable(true);
        actionNearest = new QAction(widget);
        actionNearest->setObjectName(QString::fromUtf8("actionNearest"));
        actionNearest->setCheckable(true);
        actionBilinear = new QAction(widget);
        actionBilinear->setObjectName(QString::fromUtf8("actionBilinear"));
        actionBilinear->setCheckable(true);
        actionBicubic = new QAction(widget);
        actionBicubic->setObjectName(QString::fromUtf8("actionBicubic"));
        actionBicubic->setCheckable(true);
        actionHyper = new QAction(widget);
        actionHyper->setObjectName(QString::fromUtf8("actionHyper"));
        actionHyper->setCheckable(true);
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
        actionRealtime = new QAction(widget);
        actionRealtime->setObjectName(QString::fromUtf8("actionRealtime"));
        actionRealtime->setCheckable(true);
#ifdef Q_WS_X11
        actionOpenGL = new QAction(widget);
        actionOpenGL->setObjectName(QString::fromUtf8("actionOpenGL"));
        actionOpenGL->setCheckable(true);
#endif
        actionJack = new QAction(widget);
        actionJack->setObjectName(QString::fromUtf8("actionJack"));
        actionJack->setCheckable(true);

        // Make a list of profiles
        profileGroup = new QActionGroup(widget);
        QAction* action = new QAction(widget);
        const char* name = widget->objectName().toUtf8().constData();
        action->setText(QApplication::translate(name, "Automatic", 0, QApplication::UnicodeUTF8));
        action->setCheckable(true);
        action->setData(QString());
        profileGroup->addAction(action);
        addProfile(widget, "HD 720p 50 fps", "atsc_720p_50");
        addProfile(widget, "HD 720p 59.94 fps", "atsc_720p_5994");
        addProfile(widget, "HD 720p 60 fps", "atsc_720p_60");
        addProfile(widget, "HD 1080i 25 fps", "atsc_1080i_50");
        addProfile(widget, "HD 1080i 29.97 fps", "atsc_1080i_5994");
        addProfile(widget, "HD 1080p 23.98 fps", "atsc_1080p_2398");
        addProfile(widget, "HD 1080p 24 fps", "atsc_1080p_24");
        addProfile(widget, "HD 1080p 25 fps", "atsc_1080p_25");
        addProfile(widget, "HD 1080p 29.97 fps", "atsc_1080p_2997");
        addProfile(widget, "HD 1080p 30 fps", "atsc_1080p_30");
        addProfile(widget, "SD NTSC", "dv_ntsc");
        addProfile(widget, "SD PAL", "dv_pal");

        // Make a list of the SDI and HDMI devices
        externalGroup = new QActionGroup(widget);
        action = new QAction(widget);
        action->setText(QApplication::translate(name, "None", 0, QApplication::UnicodeUTF8));
        action->setCheckable(true);
        action->setData(QString());
        externalGroup->addAction(action);
#ifdef Q_WS_X11
        Mlt::Consumer linsys(MLT.profile(), "sdi");
        if (linsys.is_valid()) {
            action = new QAction("DVEO VidPort", widget);
            action->setCheckable(true);
            action->setData(QString("sdi"));
            externalGroup->addAction(action);
        }
#endif
        Mlt::Profile profile;
        Mlt::Consumer decklink(profile, "decklink:");
        if (decklink.is_valid()) {
            decklink.set("list_devices", 1);
            int n = decklink.get_int("devices");
            for (int i = 0; i < n; ++i) {
                QString device(decklink.get(QString("device.%1").arg(i).toAscii().constData()));
                if (!device.isEmpty()) {
                    action = new QAction(device, widget);
                    action->setCheckable(true);
                    action->setData(QString("decklink:%1").arg(i));
                    externalGroup->addAction(action);
                }
            }
        }

        retranslateUi(widget);
        QMetaObject::connectSlotsByName(widget);
    }

    void retranslateUi(QWidget* widget)
    {
        const char* name = widget->objectName().toUtf8().constData();
        actionPlay->setText(QApplication::translate(name, "Play", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionPlay->setToolTip(QApplication::translate(name, "Start playback (L)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionPlay->setShortcut(QApplication::translate(name, "Space", 0, QApplication::UnicodeUTF8));
        actionPause->setText(QApplication::translate(name, "Pause", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionPause->setToolTip(QApplication::translate(name, "Pause playback (K)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionPause->setShortcut(QApplication::translate(name, "Backspace", 0, QApplication::UnicodeUTF8));
        actionSkipNext->setText(QApplication::translate(name, "Skip Next", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionSkipNext->setToolTip(QApplication::translate(name, "Skip to the next point (Alt+Right)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionSkipNext->setShortcut(QApplication::translate(name, "Alt+Right", 0, QApplication::UnicodeUTF8));
        actionSkipPrevious->setText(QApplication::translate(name, "Skip Previous", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionSkipPrevious->setToolTip(QApplication::translate(name, "Skip to the previous point (Alt+Left)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionSkipPrevious->setShortcut(QApplication::translate(name, "Alt+Left", 0, QApplication::UnicodeUTF8));
        actionProgressive->setText(QApplication::translate(name, "Progressive", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionProgressive->setToolTip(QApplication::translate(name, "Force the video preview to deinterlace if needed", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionOneField->setText(QApplication::translate(name, "One Field (fast)", 0, QApplication::UnicodeUTF8));
        actionLinearBlend->setText(QApplication::translate(name, "Linear Blend (fast)", 0, QApplication::UnicodeUTF8));
        actionYadifTemporal->setText(QApplication::translate(name, "YADIF - temporal only (good)", 0, QApplication::UnicodeUTF8));
        actionYadifSpatial->setText(QApplication::translate(name, "YADIF - temporal + spatial (best)", 0, QApplication::UnicodeUTF8));
        actionNearest->setText(QApplication::translate(name, "Nearest Neighbor (fast)", 0, QApplication::UnicodeUTF8));
        actionBilinear->setText(QApplication::translate(name, "Bilinear (good)", 0, QApplication::UnicodeUTF8));
        actionBicubic->setText(QApplication::translate(name, "Bicubic (better)", 0, QApplication::UnicodeUTF8));
        actionHyper->setText(QApplication::translate(name, "Hyper/Lanczoz (best)", 0, QApplication::UnicodeUTF8));
        actionRewind->setText(QApplication::translate(name, "Rewind", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionRewind->setToolTip(QApplication::translate(name, "Play quickly backwards (J)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionFastForward->setText(QApplication::translate(name, "Fast Forward", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionFastForward->setToolTip(QApplication::translate(name, "Play quickly forwards (L)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionRealtime->setText(QApplication::translate(name, "Realtime (frame dropping)", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionRealtime->setToolTip(QApplication::translate(name, "Allow the player to drop video frames to try to play in realtime", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifdef Q_WS_X11
        actionOpenGL->setText(QApplication::translate(name, "Use OpenGL", 0, QApplication::UnicodeUTF8));
#endif
        actionJack->setText(QApplication::translate(name, "Use JACK Audio", 0, QApplication::UnicodeUTF8));
    }
};
} // namespace Ui

QT_END_NAMESPACE

Player::Player(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Player)
    , m_position(0)
    , m_seekPosition(SEEK_INACTIVE)
{
    setObjectName("Player");
    Mlt::Controller::singleton(this);

    ui->setupActions(this);
    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    ui->actionPlay->setIcon(QIcon::fromTheme("media-playback-start", ui->actionPlay->icon()));
    ui->actionPause->setIcon(QIcon::fromTheme("media-playback-pause", ui->actionPause->icon()));
    ui->actionSkipNext->setIcon(QIcon::fromTheme("media-skip-forward", ui->actionSkipNext->icon()));
    ui->actionSkipPrevious->setIcon(QIcon::fromTheme("media-skip-backward", ui->actionSkipPrevious->icon()));
    ui->actionRewind->setIcon(QIcon::fromTheme("media-seek-backward", ui->actionRewind->icon()));
    ui->actionFastForward->setIcon(QIcon::fromTheme("media-seek-forward", ui->actionFastForward->icon()));
    ui->actionPlay->setEnabled(false);
    ui->actionSkipPrevious->setEnabled(false);
    ui->actionSkipNext->setEnabled(false);
    ui->actionRewind->setEnabled(false);
    ui->actionFastForward->setEnabled(false);
    m_playIcon = ui->actionPlay->icon();
    m_pauseIcon = ui->actionPause->icon();

    readSettings();

    // Create a layout.
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setObjectName("playerLayout");
    vlayout->setMargin(0);
    vlayout->setSpacing(4);

    // Create MLT video widget.
    MLT.videoWidget()->setProperty("mlt_service", ui->externalGroup->checkedAction()->data());
    MLT.setProfile(ui->profileGroup->checkedAction()->data().toString());
    MLT.videoWidget()->setProperty("realtime", ui->actionRealtime->isChecked());
    if (ui->externalGroup->checkedAction()->data().toString().isEmpty())
        MLT.videoWidget()->setProperty("progressive", ui->actionProgressive->isChecked());
    else {
        MLT.videoWidget()->setProperty("progressive", MLT.profile().progressive());
        ui->actionProgressive->setEnabled(false);
    }
    if (ui->actionOneField->isChecked())
        MLT.videoWidget()->setProperty("deinterlace_method", "onefield");
    else if (ui->actionLinearBlend->isChecked())
        MLT.videoWidget()->setProperty("deinterlace_method", "linearblend");
    else if (ui->actionYadifTemporal->isChecked())
        MLT.videoWidget()->setProperty("deinterlace_method", "yadif-nospatial");
    else
        MLT.videoWidget()->setProperty("deinterlace_method", "yadif");
    if (ui->actionNearest->isChecked())
        MLT.videoWidget()->setProperty("rescale", "nearest");
    else if (ui->actionBilinear->isChecked())
        MLT.videoWidget()->setProperty("rescale", "bilinear");
    else if (ui->actionBicubic->isChecked())
        MLT.videoWidget()->setProperty("rescale", "bicubic");
    else
        MLT.videoWidget()->setProperty("rescale", "hyper");
    MLT.videoWidget()->setContentsMargins(0, 0, 0, 0);
    MLT.videoWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
    MLT.videoWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

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
    m_volumeSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
    connect(this, SIGNAL(audioSamplesSignal(const QVector<int16_t>&, const int&, const int&, const int&)),
                         m_audioSignal, SLOT(slotReceiveAudio(const QVector<int16_t>&, const int&, const int&, const int&)));

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
#ifdef Q_WS_MAC
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
    toolbar->addAction(ui->actionSkipPrevious);
    toolbar->addAction(ui->actionRewind);
    toolbar->addAction(ui->actionPlay);
    toolbar->addAction(ui->actionFastForward);
    toolbar->addAction(ui->actionSkipNext);
    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    toolbar->addWidget(m_inPointLabel);
    toolbar->addWidget(m_selectedLabel);
    vlayout->addWidget(toolbar);

    connect(ui->externalGroup, SIGNAL(triggered(QAction*)), this, SLOT(onExternalTriggered(QAction*)));
    connect(ui->profileGroup, SIGNAL(triggered(QAction*)), this, SLOT(onProfileTriggered(QAction*)));
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onShowFrame(Mlt::QFrame)));
    connect(MLT.videoWidget(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onVideoWidgetContextMenu(QPoint)));
    connect(ui->actionPlay, SIGNAL(triggered()), this, SLOT(togglePlayPaused()));
    connect(ui->actionPause, SIGNAL(triggered()), this, SLOT(pause()));
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(seek(int)));
    connect(m_scrubber, SIGNAL(inChanged(int)), this, SLOT(onInChanged(int)));
    connect(m_scrubber, SIGNAL(outChanged(int)), this, SLOT(onOutChanged(int)));
    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    connect(m_positionSpinner, SIGNAL(editingFinished()), this, SLOT(setFocus()));
    connect(this, SIGNAL(seeked()), this, SLOT(pause()));
    connect(this, SIGNAL(endOfStream()), this, SLOT(pause()));
    setFocusPolicy(Qt::StrongFocus);
}

void Player::readSettings()
{
#ifdef Q_WS_X11
    ui->actionOpenGL->setChecked(m_settings.value("player/opengl", true).toBool());
#endif
    ui->actionRealtime->setChecked(m_settings.value("player/realtime", true).toBool());
    ui->actionProgressive->setChecked(m_settings.value("player/progressive", true).toBool());
    ui->actionJack->setChecked(m_settings.value("player/jack", false).toBool());
    QString deinterlacer = m_settings.value("player/deinterlacer", "onefield").toString();
    QString interpolation = m_settings.value("player/interpolation", "nearest").toString();

    if (deinterlacer == "onefield")
        ui->actionOneField->setChecked(true);
    else if (deinterlacer == "linearblend")
        ui->actionLinearBlend->setChecked(true);
    else if (deinterlacer == "yadif-nospatial")
        ui->actionYadifTemporal->setChecked(true);
    else
        ui->actionYadifSpatial->setChecked(true);

    if (interpolation == "nearest")
        ui->actionNearest->setChecked(true);
    else if (interpolation == "bilinear")
        ui->actionBilinear->setChecked(true);
    else if (interpolation == "bicubic")
        ui->actionBicubic->setChecked(true);
    else
        ui->actionHyper->setChecked(true);

    QVariant external = m_settings.value("player/external", "");
    foreach (QAction* a, ui->externalGroup->actions()) {
        if (a->data() == external) {
            a->setChecked(true);
            break;
        }
    }

    QVariant profile = m_settings.value("player/profile", "");
    // Automatic not permitted for SDI/HDMI
    if (!external.toString().isEmpty() && profile.toString().isEmpty())
        profile = QVariant("atsc_720p_50");
    foreach (QAction* a, ui->profileGroup->actions()) {
        // Automatic not permitted for SDI/HDMI
        if (a->data().toString().isEmpty() && !external.toString().isEmpty())
            a->setDisabled(true);
        if (a->data() == profile) {
            a->setChecked(true);
            break;
        }
    }
}

void Player::setIn(unsigned pos)
{
    m_scrubber->setInPoint(pos);
}

void Player::setOut(unsigned pos)
{
    m_scrubber->setOutPoint(pos);
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
    MLT.play(speed);
    // TODO: use stop icon for live sources
    if (MLT.isSeekable()) {
        ui->actionPlay->setIcon(m_pauseIcon);
        ui->actionPlay->setText(tr("Pause"));
        ui->actionPlay->setToolTip(tr("Pause playback (K)"));
    }
    else {
        ui->actionPlay->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/icons/icons/media-playback-stop.png")));
        ui->actionPlay->setText(tr("Stop"));
        ui->actionPlay->setToolTip(tr("Stop playback (K)"));
    }
}

void Player::pause()
{
    MLT.pause();
    ui->actionPlay->setIcon(m_playIcon);
    ui->actionPlay->setText(tr("Play"));
    ui->actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::stop()
{
    MLT.stop();
    ui->actionPlay->setIcon(m_playIcon);
    ui->actionPlay->setText(tr("Play"));
    ui->actionPlay->setToolTip(tr("Start playback (L)"));
}

void Player::togglePlayPaused()
{
    if (ui->actionPlay->icon().cacheKey() == m_playIcon.cacheKey())
        play();
    else if (MLT.producer() && (
             MLT.isSeekable() ||
                 // generators can pause and show property changes
                 QString(MLT.producer()->get("mlt_service")) == "color" ||
                 QString(MLT.producer()->get("mlt_service")).startsWith("frei0r.")))
        pause();
    else
        stop();
}

void Player::seek(int position)
{
    if (MLT.isSeekable()) {
        emit seeked();
        if (position >= 0) {
            if (m_seekPosition == SEEK_INACTIVE)
                MLT.seek(qMin(position, MLT.producer()->get_length() - 1));
            m_seekPosition = qMin(position, MLT.producer()->get_length() - 1);
        }
    }
}

void Player::onProducerOpened()
{
    int len = MLT.producer()->get_length();
    bool seekable = MLT.isSeekable();

    MLT.producer()->set("ignore_points", 1);
    m_scrubber->setFramerate(MLT.profile().fps());
    m_scrubber->setScale(len);
    m_scrubber->setMarkers(QList<int>());
    m_inPointLabel->setText("--:--:--:-- / ");
    m_selectedLabel->setText("--:--:--:--");
    if (seekable) {
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
        m_scrubber->setMarkers(QList<int>());
    }
    m_positionSpinner->setEnabled(seekable);
    on_actionJack_triggered(ui->actionJack->isChecked());
    onVolumeChanged(m_volumeSlider->value());
    onMuteButtonToggled(m_settings.value("player/muted", false).toBool());

    ui->actionPlay->setEnabled(true);
    ui->actionSkipPrevious->setEnabled(seekable);
    ui->actionSkipNext->setEnabled(seekable);
    ui->actionRewind->setEnabled(seekable);
    ui->actionFastForward->setEnabled(seekable);

    play();
}

void Player::onShowFrame(Mlt::QFrame frame)
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        int position = frame.frame()->get_position();
        if (position < MLT.producer()->get_length()) {
            m_position = position;
            m_positionSpinner->blockSignals(true);
            m_positionSpinner->setValue(position);
            m_positionSpinner->blockSignals(false);
            m_scrubber->onSeek(position);
        }
        if (position >= MLT.producer()->get_length() - 1)
            emit endOfStream();
        showAudio(frame.frame());
        if (m_seekPosition != SEEK_INACTIVE)
            MLT.seek(m_seekPosition);
        m_seekPosition = SEEK_INACTIVE;
    }
}

void Player::updateSelection()
{
    if (MLT.producer() && MLT.producer()->get_in() > 0) {
        m_inPointLabel->setText(QString(MLT.producer()->get_time("in")).append(" / "));
        MLT.producer()->set("_shotcut_selected", MLT.producer()->get_playtime());
        m_selectedLabel->setText(MLT.producer()->get_time("_shotcut_selected"));
    } else {
        m_inPointLabel->setText("--:--:--:-- / ");
        if (MLT.producer() && MLT.resource() != "<playlist>" &&
                MLT.producer()->get_out() < MLT.producer()->get_length() - 1) {
            MLT.producer()->set("_shotcut_selected", MLT.producer()->get_playtime());
            m_selectedLabel->setText(MLT.producer()->get_time("_shotcut_selected"));
        } else if (!MLT.producer() || MLT.producer()->get_in() == 0) {
            m_selectedLabel->setText("--:--:--:--");
        }
    }
}

void Player::onInChanged(int in)
{
    if (MLT.producer())
        MLT.producer()->set("in", in);
    if (in != m_previousIn)
        emit inChanged(in);
    m_previousIn = in;
    updateSelection();
}

void Player::onOutChanged(int out)
{
    if (MLT.producer())
        MLT.producer()->set("out", out);
    if (out != m_previousOut)
        emit outChanged(out);
    m_previousOut = out;
    updateSelection();
}

void Player::on_actionSkipNext_triggered()
{
    int pos = position();
    if (m_previousIn == -1 && m_previousOut == -1) {
        foreach (int x, m_scrubber->markers()) {
            if (x > pos) {
                MLT.seek(x);
                emit showStatusMessage(ui->actionSkipNext->toolTip());
                return;
            }
        }
        MLT.seek(MLT.producer()->get_length() - 1);
    }
    else if (pos < MLT.producer()->get_in())
        MLT.seek(MLT.producer()->get_in());
    else if (pos >= MLT.producer()->get_out())
        MLT.seek(MLT.producer()->get_length() - 1);
    else
        MLT.seek(MLT.producer()->get_out());
    emit showStatusMessage(ui->actionSkipNext->toolTip());
}

void Player::on_actionSkipPrevious_triggered()
{
    int pos = position();
    if (m_previousIn == -1 && m_previousOut == -1) {
        QList<int> markers = m_scrubber->markers();
        int n = markers.count();
        while (n--) {
            if (markers[n] < pos) {
                MLT.seek(markers[n]);
                emit showStatusMessage(ui->actionSkipPrevious->toolTip());
                return;
            }
        }
        MLT.seek(0);
    }
    else if (pos > MLT.producer()->get_out())
        MLT.seek(MLT.producer()->get_out());
    else if (pos <= MLT.producer()->get_in())
        MLT.seek(0);
    else
        MLT.seek(MLT.producer()->get_in());
    emit showStatusMessage(ui->actionSkipPrevious->toolTip());
}

void Player::rewind()
{
    if (MLT.isSeekable()) {
        if (MLT.producer()->get_speed() >= 0)
            play(-1.0);
        else
            MLT.producer()->set_speed(MLT.producer()->get_speed() * 2);
    }
}

void Player::fastForward()
{
    if (MLT.isSeekable()) {
        if (MLT.producer()->get_speed() <= 0)
            play();
        else
            MLT.producer()->set_speed(MLT.producer()->get_speed() * 2);
    } else {
        play();
    }
}

void Player::onVideoWidgetContextMenu(const QPoint& pos)
{
    QMenu menu(this);
#ifdef Q_WS_X11
    // Only offer the option to disable OpenGL if SDL is available.
    Mlt::Consumer* c = new Mlt::Consumer(MLT.profile(), "sdl");
    if (c->is_valid())
        menu.addAction(ui->actionOpenGL);
    delete c;
#endif
    menu.addAction(ui->actionRealtime);
    menu.addAction(ui->actionProgressive);

    QMenu* sub = menu.addMenu(tr("Deinterlacer"));
    QActionGroup deinterlacerGroup(sub);
    deinterlacerGroup.addAction(ui->actionOneField);
    deinterlacerGroup.addAction(ui->actionLinearBlend);
    deinterlacerGroup.addAction(ui->actionYadifTemporal);
    deinterlacerGroup.addAction(ui->actionYadifSpatial);
    sub->addActions(deinterlacerGroup.actions());

    sub = menu.addMenu(tr("Interpolation"));
    QActionGroup scalerGroup(sub);
    scalerGroup.addAction(ui->actionNearest);
    scalerGroup.addAction(ui->actionBilinear);
    scalerGroup.addAction(ui->actionBicubic);
    scalerGroup.addAction(ui->actionHyper);
    sub->addActions(scalerGroup.actions());

    if (ui->externalGroup->actions().count() > 1) {
        sub = menu.addMenu(tr("Preview on SDI/HDMI"));
        sub->addActions(ui->externalGroup->actions());
    };

    sub = menu.addMenu(tr("Video Mode"));
    sub->addActions(ui->profileGroup->actions());

    menu.addAction(ui->actionJack);

    menu.exec(this->mapToGlobal(pos));
}

#ifdef Q_WS_X11
void Player::on_actionOpenGL_triggered(bool checked)
{
    m_settings.setValue("player/opengl", checked);
    int r = QMessageBox::information(this, qApp->applicationName(),
                                 tr("You must restart Shotcut to switch using OpenGL.\n"
                                    "Do you want to exit now?"),
                                 QMessageBox::Yes | QMessageBox::Default,
                                 QMessageBox::No | QMessageBox::Escape);
    if (r == QMessageBox::Yes)
        QApplication::closeAllWindows();
}
#endif

void Player::on_actionRealtime_triggered(bool checked)
{
    MLT.videoWidget()->setProperty("realtime", checked);
    if (MLT.consumer()) {
        MLT.consumer()->stop();
        MLT.consumer()->set("real_time", checked? 1 : -1);
        MLT.consumer()->start();
    }
    m_settings.setValue("player/realtime", checked);
}

void Player::on_actionProgressive_triggered(bool checked)
{
    MLT.videoWidget()->setProperty("progressive", checked);
    if (MLT.consumer() && !MLT.profile().progressive()) {
        MLT.consumer()->stop();
        MLT.consumer()->set("progressive", checked);
        MLT.consumer()->start();
    }
    m_settings.setValue("player/progressive", checked);
}

void Player::changeDeinterlacer(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("deinterlace_method", method);
        if (MLT.consumer()) {
            MLT.consumer()->stop();
            MLT.consumer()->set("deinterlace_method", method);
            MLT.consumer()->start();
        }
    }
    m_settings.setValue("player/deinterlacer", method);
}

void Player::on_actionOneField_triggered(bool checked)
{
    changeDeinterlacer(checked, "onefield");
}

void Player::on_actionLinearBlend_triggered(bool checked)
{
    changeDeinterlacer(checked, "linearblend");
}

void Player::on_actionYadifTemporal_triggered(bool checked)
{
    changeDeinterlacer(checked, "yadif-nospatial");
}

void Player::on_actionYadifSpatial_triggered(bool checked)
{
    changeDeinterlacer(checked, "yadif");
}

void Player::changeInterpolation(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("rescale", method);
        if (MLT.consumer()) {
            MLT.consumer()->stop();
            MLT.consumer()->set("rescale", method);
            MLT.consumer()->start();
        }
    }
    m_settings.setValue("player/interpolation", method);
}

void Player::on_actionNearest_triggered(bool checked)
{
    changeInterpolation(checked, "nearest");
}

void Player::on_actionBilinear_triggered(bool checked)
{
    changeInterpolation(checked, "bilinear");
}

void Player::on_actionBicubic_triggered(bool checked)
{
    changeInterpolation(checked, "bicubic");
}

void Player::on_actionHyper_triggered(bool checked)
{
    changeInterpolation(checked, "hyper");
}

void Player::on_actionJack_triggered(bool checked)
{
    m_settings.setValue("player/jack", checked);
    if (!MLT.enableJack(checked)) {
        ui->actionJack->setChecked(false);
        m_settings.setValue("player/jack", false);
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to connect to JACK.\nPlease verify that JACK is installed and running."));
    }
}

void Player::onExternalTriggered(QAction *action)
{
    bool isExternal = !action->data().toString().isEmpty();
    m_settings.setValue("player/external", action->data());
    MLT.videoWidget()->setProperty("mlt_service", action->data());

    QVariant profile = m_settings.value("player/profile", "");
    // Automatic not permitted for SDI/HDMI
    if (isExternal && profile.toString().isEmpty()) {
        profile = QVariant("atsc_720p_50");
        m_settings.setValue("player/profile", profile);
        MLT.setProfile(profile.toString());
        foreach (QAction* a, ui->profileGroup->actions()) {
            if (a->data() == profile) {
                a->setChecked(true);
                break;
            }
        }
    }
    else {
        MLT.consumerChanged();
    }
    // Automatic not permitted for SDI/HDMI
    ui->profileGroup->actions().at(0)->setEnabled(!isExternal);

    // Disable progressive option when SDI/HDMI
    ui->actionProgressive->setEnabled(!isExternal);
    bool isProgressive = isExternal
            ? MLT.profile().progressive()
            : ui->actionProgressive->isChecked();
    MLT.videoWidget()->setProperty("progressive", isProgressive);
    if (MLT.consumer()) {
        MLT.consumer()->stop();
        MLT.consumer()->set("progressive", isProgressive);
        MLT.consumer()->start();
    }
}

void Player::onProfileTriggered(QAction *action)
{
    m_settings.setValue("player/profile", action->data());
    MLT.setProfile(action->data().toString());
}

void Player::showAudio(Mlt::Frame* frame)
{
    if (frame->get_int("test_audio"))
        return;
    mlt_audio_format format = mlt_audio_s16;
    int frequency = 0;
    int channels = 0;
    int samples = 0;
    int16_t* data = (int16_t*) frame->get_audio(format, frequency, channels, samples);

    if (samples && data) {
        QVector<int16_t> pcm(samples * channels);
        memcpy(pcm.data(), data, samples * channels * sizeof(int16_t));
        emit audioSamplesSignal(pcm, frequency, channels, samples);
    }
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
    ui->actionPlay->setDisabled(active);
    if (active)
        MLT.videoWidget()->disconnect(SIGNAL(customContextMenuRequested(QPoint)));
    else
        connect(MLT.videoWidget(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onVideoWidgetContextMenu(QPoint)));
}

void Player::resetProfile()
{
    MLT.setProfile(m_settings.value("player/profile").toString());
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
