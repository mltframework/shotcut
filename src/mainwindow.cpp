/*
 * Copyright (c) 2011 Meltytech, LLC
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scrubbar.h"
#include "openotherdialog.h"
#include "player.h"

#include "widgets/alsawidget.h"
#include "widgets/colorbarswidget.h"
#include "widgets/colorproducerwidget.h"
#include "widgets/decklinkproducerwidget.h"
#include "widgets/isingwidget.h"
#include "widgets/jackproducerwidget.h"
#include "widgets/lissajouswidget.h"
#include "widgets/networkproducerwidget.h"
#include "widgets/noisewidget.h"
#include "widgets/plasmawidget.h"
#include "widgets/pulseaudiowidget.h"
#include "widgets/video4linuxwidget.h"
#include "widgets/x11grabwidget.h"
#include "widgets/avformatproducerwidget.h"
#include "widgets/imageproducerwidget.h"
#include "docks/recentdock.h"
#include "docks/encodedock.h"
#include "docks/jobsdock.h"
#include "jobqueue.h"
#include "docks/playlistdock.h"
#include "glwidget.h"
#include "sdlwidget.h"
#include "mvcp/meltedserverdock.h"
#include "mvcp/meltedplaylistdock.h"
#include "mvcp/meltedunitsmodel.h"
#include "mvcp/meltedplaylistmodel.h"
#include "docks/filtersdock.h"
#include "dialogs/customprofiledialog.h"

#include <QtWidgets>
#include <QDebug>
#include <QThreadPool>

static const int STATUS_TIMEOUT_MS = 3000;

MainWindow::MainWindow()
    : QMainWindow(0)
    , ui(new Ui::MainWindow)
    , m_isKKeyPressed(false)
    , m_isPlaylistLoaded(false)
{
    QThreadPool::globalInstance()->setMaxThreadCount(1);

    // Create the UI.
    ui->setupUi(this);
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif
#ifdef Q_OS_MAC
    // Qt 5 on OS X supports the standard Full Screen window widget.
    ui->mainToolBar->removeAction(ui->actionFullscreen);
    // OS X has a standard Full Screen shortcut we should use.
    ui->actionEnter_Full_Screen->setShortcut(QKeySequence((Qt::CTRL + Qt::META + Qt::Key_F)));
#endif
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setDockNestingEnabled(true);

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));
    connect(ui->actionFullscreen, SIGNAL(triggered()), this, SLOT(on_actionEnter_Full_Screen_triggered()));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Setup the undo stack.
    m_undoStack = new QUndoStack(this);
    QAction *undoAction = m_undoStack->createUndoAction(this);
    QAction *redoAction = m_undoStack->createRedoAction(this);
    undoAction->setIcon(QIcon::fromTheme("edit-undo"));
    redoAction->setIcon(QIcon::fromTheme("edit-redo"));
    undoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Z", 0));
    redoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+Z", 0));
    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);
    ui->actionUndo->setIcon(undoAction->icon());
    ui->actionRedo->setIcon(redoAction->icon());
    ui->actionUndo->setToolTip(undoAction->toolTip());
    ui->actionRedo->setToolTip(redoAction->toolTip());
    connect(m_undoStack, SIGNAL(canUndoChanged(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect(m_undoStack, SIGNAL(canRedoChanged(bool)), ui->actionRedo, SLOT(setEnabled(bool)));

    // Add the player widget.
    m_player = new Player;
    ui->stackedWidget->addWidget(m_player);
    connect(this, SIGNAL(producerOpened()), m_player, SLOT(onProducerOpened()));
    connect(m_player, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_player, SIGNAL(inChanged(int)), this, SLOT(onCutModified()));
    connect(m_player, SIGNAL(outChanged(int)), this, SLOT(onCutModified()));
    connect(MLT.videoWidget(), SIGNAL(started()), SLOT(processMultipleFiles()));
    connect(MLT.videoWidget(), SIGNAL(paused()), m_player, SLOT(showPaused()));

    setupSettingsMenu();
    readPlayerSettings();
    configureVideoWidget();

    // Add the docks.
    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->hide();
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setWindowIcon(ui->actionProperties->icon());
    m_propertiesDock->toggleViewAction()->setIcon(ui->actionProperties->icon());
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    ui->menuView->addAction(m_propertiesDock->toggleViewAction());
    connect(m_propertiesDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPropertiesDockTriggered(bool)));
    connect(ui->actionProperties, SIGNAL(triggered()), this, SLOT(onPropertiesDockTriggered()));

    m_recentDock = new RecentDock(this);
    m_recentDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_recentDock);
    ui->menuView->addAction(m_recentDock->toggleViewAction());
    connect(m_recentDock, SIGNAL(itemActivated(QString)), this, SLOT(open(QString)));
    connect(m_recentDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onRecentDockTriggered(bool)));
    connect(ui->actionRecent, SIGNAL(triggered()), this, SLOT(onRecentDockTriggered()));
    connect(this, SIGNAL(openFailed(QString)), m_recentDock, SLOT(remove(QString)));

    m_playlistDock = new PlaylistDock(this);
    m_playlistDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_playlistDock);
    ui->menuView->addAction(m_playlistDock->toggleViewAction());
    connect(m_playlistDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPlaylistDockTriggered(bool)));
    connect(ui->actionPlaylist, SIGNAL(triggered()), this, SLOT(onPlaylistDockTriggered()));
    connect(m_playlistDock, SIGNAL(clipOpened(void*,int,int)), this, SLOT(openCut(void*, int, int)));
    connect(m_playlistDock, SIGNAL(itemActivated(int)), this, SLOT(seekPlaylist(int)));
    connect(m_playlistDock, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_playlistDock->model(), SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(m_playlistDock->model(), SIGNAL(closed()), this, SLOT(onPlaylistClosed()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(updateMarkers()));
    if (!m_settings.value("player/gpu").toBool())
        connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(updateThumbnails()));

    m_filtersDock = new FiltersDock(this);
    m_filtersDock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_filtersDock);
    ui->menuView->addAction(m_filtersDock->toggleViewAction());
    connect(m_filtersDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onFiltersDockTriggered(bool)));
    connect(ui->actionFilters, SIGNAL(triggered()), this, SLOT(onFiltersDockTriggered()));
    connect(this, SIGNAL(producerOpened()), m_filtersDock, SLOT(onProducerOpened()));
    connect(m_filtersDock->model(), SIGNAL(changed()), this, SLOT(onCutModified()));

    m_historyDock = new QDockWidget(tr("History"), this);
    m_historyDock->hide();
    m_historyDock->setObjectName("historyDock");
    m_historyDock->setWindowIcon(ui->actionHistory->icon());
    m_historyDock->toggleViewAction()->setIcon(ui->actionHistory->icon());
    addDockWidget(Qt::LeftDockWidgetArea, m_historyDock);
    ui->menuView->addAction(m_historyDock->toggleViewAction());
    connect(m_historyDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onHistoryDockTriggered(bool)));
    connect(ui->actionHistory, SIGNAL(triggered()), this, SLOT(onHistoryDockTriggered()));
    QUndoView* undoView = new QUndoView(m_undoStack, m_historyDock);
    undoView->setObjectName("historyView");
    undoView->setAlternatingRowColors(true);
    undoView->setSpacing(2);
    m_historyDock->setWidget(undoView);
    ui->actionUndo->setDisabled(true);
    ui->actionRedo->setDisabled(true);

    tabifyDockWidget(m_propertiesDock, m_recentDock);
    tabifyDockWidget(m_recentDock, m_playlistDock);
    tabifyDockWidget(m_playlistDock, m_historyDock);
    m_recentDock->raise();

    m_encodeDock = new EncodeDock(this);
    m_encodeDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_encodeDock);
    ui->menuView->addAction(m_encodeDock->toggleViewAction());
    connect(this, SIGNAL(producerOpened()), m_encodeDock, SLOT(onProducerOpened()));
    connect(ui->actionEncode, SIGNAL(triggered()), this, SLOT(onEncodeTriggered()));
    connect(m_encodeDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onEncodeTriggered(bool)));
    connect(m_encodeDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onEncodeVisibilityChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_player, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_propertiesDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_recentDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_filtersDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpen, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpenOther, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionExit, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), this, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_historyDock, SLOT(setDisabled(bool)));
    connect(m_player, SIGNAL(profileChanged()), m_encodeDock, SLOT(onProfileChanged()));
    connect(this, SIGNAL(profileChanged()), m_encodeDock, SLOT(onProfileChanged()));
    m_encodeDock->onProfileChanged();

    m_jobsDock = new JobsDock(this);
    m_jobsDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_jobsDock);
    tabifyDockWidget(m_encodeDock, m_jobsDock);
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(show()));
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(raise()));
    connect(m_jobsDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onJobsVisibilityChanged(bool)));

    m_meltedServerDock = new MeltedServerDock(this);
    m_meltedServerDock->hide();
    addDockWidget(Qt::TopDockWidgetArea, m_meltedServerDock);
    m_meltedServerDock->toggleViewAction()->setIcon(m_meltedServerDock->windowIcon());
    ui->menuView->addAction(m_meltedServerDock->toggleViewAction());

    m_meltedPlaylistDock = new MeltedPlaylistDock(this);
    m_meltedPlaylistDock->hide();
    addDockWidget(Qt::TopDockWidgetArea, m_meltedPlaylistDock);
    splitDockWidget(m_meltedServerDock, m_meltedPlaylistDock, Qt::Horizontal);
    m_meltedPlaylistDock->toggleViewAction()->setIcon(m_meltedPlaylistDock->windowIcon());
    ui->menuView->addAction(m_meltedPlaylistDock->toggleViewAction());
    connect(m_meltedServerDock, SIGNAL(connected(QString, quint16)), m_meltedPlaylistDock, SLOT(onConnected(QString,quint16)));
    connect(m_meltedServerDock, SIGNAL(disconnected()), m_meltedPlaylistDock, SLOT(onDisconnected()));
    connect(m_meltedServerDock, SIGNAL(unitActivated(quint8)), m_meltedPlaylistDock, SLOT(onUnitChanged(quint8)));
    connect(m_meltedServerDock, SIGNAL(unitActivated(quint8)), this, SLOT(onMeltedUnitActivated()));
    connect(m_meltedPlaylistDock, SIGNAL(appendRequested()), m_meltedServerDock, SLOT(onAppendRequested()));
    connect(m_meltedServerDock, SIGNAL(append(QString,int,int)), m_meltedPlaylistDock, SLOT(onAppend(QString,int,int)));
    connect(m_meltedPlaylistDock, SIGNAL(insertRequested(int)), m_meltedServerDock, SLOT(onInsertRequested(int)));
    connect(m_meltedServerDock, SIGNAL(insert(QString,int,int,int)), m_meltedPlaylistDock, SLOT(onInsert(QString,int,int,int)));
    connect(m_meltedServerDock, SIGNAL(unitOpened(quint8)), this, SLOT(onMeltedUnitOpened()));
    connect(m_meltedServerDock, SIGNAL(unitOpened(quint8)), m_player, SLOT(onMeltedUnitOpened()));
    connect(m_meltedServerDock->actionFastForward(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(fastForward()));
    connect(m_meltedServerDock->actionPause(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(pause()));
    connect(m_meltedServerDock->actionPlay(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(play()));
    connect(m_meltedServerDock->actionRewind(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(rewind()));
    connect(m_meltedServerDock->actionStop(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(stop()));
    connect(m_meltedServerDock, SIGNAL(openLocal(QString)), SLOT(open(QString)));

    MeltedUnitsModel* unitsModel = (MeltedUnitsModel*) m_meltedServerDock->unitsModel();
    MeltedPlaylistModel* playlistModel = (MeltedPlaylistModel*) m_meltedPlaylistDock->model();
    connect(m_meltedServerDock, SIGNAL(connected(QString,quint16)), unitsModel, SLOT(onConnected(QString,quint16)));
    connect(unitsModel, SIGNAL(clipIndexChanged(quint8, int)), playlistModel, SLOT(onClipIndexChanged(quint8, int)));
    connect(unitsModel, SIGNAL(generationChanged(quint8)), playlistModel, SLOT(onGenerationChanged(quint8)));

    // connect video widget signals
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
    connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
    connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
    connect(videoWidget, SIGNAL(gpuNotSupported()), this, SLOT(onGpuNotSupported()));
#else
    if (m_settings.value("player/opengl", true).toBool()) {
        Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
        connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
        connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
        connect(videoWidget, SIGNAL(gpuNotSupported()), this, SLOT(onGpuNotSupported()));
    }
    else {
        Mlt::SDLWidget* videoWidget = (Mlt::SDLWidget*) &(MLT);
        connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
        connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
        onGpuNotSupported();
    }
#endif

    readWindowSettings();

    setFocus();
    setCurrentFile("");
}

MainWindow& MainWindow::singleton()
{
    static MainWindow* instance = new MainWindow;
    return *instance;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupSettingsMenu()
{
    QActionGroup* deinterlaceGroup = new QActionGroup(this);
    deinterlaceGroup->addAction(ui->actionOneField);
    deinterlaceGroup->addAction(ui->actionLinearBlend);
    deinterlaceGroup->addAction(ui->actionYadifTemporal);
    deinterlaceGroup->addAction(ui->actionYadifSpatial);
    QActionGroup* interpolationGroup = new QActionGroup(this);
    interpolationGroup->addAction(ui->actionNearest);
    interpolationGroup->addAction(ui->actionBilinear);
    interpolationGroup->addAction(ui->actionBicubic);
    interpolationGroup->addAction(ui->actionHyper);
    m_profileGroup = new QActionGroup(this);
    m_profileGroup->addAction(ui->actionProfileAutomatic);
    ui->actionProfileAutomatic->setData(QString());
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 720p 50 fps", "atsc_720p_50"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 720p 59.94 fps", "atsc_720p_5994"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 720p 60 fps", "atsc_720p_60"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080i 25 fps", "atsc_1080i_50"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080i 29.97 fps", "atsc_1080i_5994"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 23.98 fps", "atsc_1080p_2398"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 24 fps", "atsc_1080p_24"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 25 fps", "atsc_1080p_25"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 29.97 fps", "atsc_1080p_2997"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 30 fps", "atsc_1080p_30"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "SD NTSC", "dv_ntsc"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "SD PAL", "dv_pal"));
    QMenu* menu = ui->menuProfile->addMenu(tr("Non-Broadcast"));
    menu->addAction(addProfile(m_profileGroup, "HD 720p 23.98 fps", "atsc_720p_2398"));
    menu->addAction(addProfile(m_profileGroup, "HD 720p 24 fps", "atsc_720p_24"));
    menu->addAction(addProfile(m_profileGroup, "HD 720p 25 fps", "atsc_720p_25"));
    menu->addAction(addProfile(m_profileGroup, "HD 720p 29.97 fps", "atsc_720p_2997"));
    menu->addAction(addProfile(m_profileGroup, "HD 720p 30 fps", "atsc_720p_30"));
    menu->addAction(addProfile(m_profileGroup, "HD 1080i 60 fps", "atsc_1080i_60"));
    menu->addAction(addProfile(m_profileGroup, "HDV 1080i 25 fps", "hdv_1080_50i"));
    menu->addAction(addProfile(m_profileGroup, "HDV 1080i 29.97 fps", "hdv_1080_60i"));
    menu->addAction(addProfile(m_profileGroup, "HDV 1080p 25 fps", "hdv_1080_25p"));
    menu->addAction(addProfile(m_profileGroup, "HDV 1080p 29.97 fps", "hdv_1080_30p"));
    menu->addAction(addProfile(m_profileGroup, tr("DVD Widescreen NTSC"), "dv_ntsc_wide"));
    menu->addAction(addProfile(m_profileGroup, tr("DVD Widescreen PAL"), "dv_pal_wide"));
    menu->addAction(addProfile(m_profileGroup, "640x480 4:3 NTSC", "square_ntsc"));
    menu->addAction(addProfile(m_profileGroup, "768x576 4:3 PAL", "square_pal"));
    menu->addAction(addProfile(m_profileGroup, "854x480 16:9 NTSC", "square_ntsc_wide"));
    menu->addAction(addProfile(m_profileGroup, "1024x576 16:9 PAL", "square_pal_wide"));
    m_customProfileMenu = ui->menuProfile->addMenu(tr("Custom"));
    m_customProfileMenu->addAction(ui->actionAddCustomProfile);
    // Load custom profiles
    QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    if (dir.cd("profiles")) {
        QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        if (profiles.length() > 0)
            m_customProfileMenu->addSeparator();
        foreach (QString name, profiles)
            m_customProfileMenu->addAction(addProfile(m_profileGroup, name, dir.filePath(name)));
    }

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    delete ui->actionOpenGL;
    ui->actionOpenGL = 0;
#else
    if (!m_settings.value("player/opengl", true).toBool()) {
        ui->actionGPU->setChecked(false);
        ui->actionGPU->setEnabled(false);
        m_settings.setValue("player/gpu", false);
    }
#endif
#ifdef Q_OS_WIN
    // GL shared context on separate thread is not working on Windows in Qt 5.1.1.
    ui->menuSettings->removeAction(ui->actionGPU);
    m_settings.setValue("player/gpu", false);
#endif

    // Add the SDI and HDMI devices to the Settings menu.
    m_externalGroup = new QActionGroup(this);
    ui->actionExternalNone->setData(QString());
    m_externalGroup->addAction(ui->actionExternalNone);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    Mlt::Consumer linsys(MLT.profile(), "sdi");
    if (linsys.is_valid()) {
        QAction* action = new QAction("DVEO VidPort", this);
        action->setCheckable(true);
        action->setData(QString("sdi"));
        m_externalGroup->addAction(action);
    }
#endif
    Mlt::Profile profile;
    Mlt::Consumer decklink(profile, "decklink:");
    if (decklink.is_valid()) {
        decklink.set("list_devices", 1);
        int n = decklink.get_int("devices");
        for (int i = 0; i < n; ++i) {
            QString device(decklink.get(QString("device.%1").arg(i).toLatin1().constData()));
            if (!device.isEmpty()) {
                QAction* action = new QAction(device, this);
                action->setCheckable(true);
                action->setData(QString("decklink:%1").arg(i));
                m_externalGroup->addAction(action);
            }
        }
    }
    if (m_externalGroup->actions().count() > 1)
        ui->menuExternal->addActions(m_externalGroup->actions());
    else {
        delete ui->menuExternal;
        ui->menuExternal = 0;
    }
    connect(m_externalGroup, SIGNAL(triggered(QAction*)), this, SLOT(onExternalTriggered(QAction*)));
    connect(m_profileGroup, SIGNAL(triggered(QAction*)), this, SLOT(onProfileTriggered(QAction*)));

    // Setup the language menu actions
    m_languagesGroup = new QActionGroup(this);
    QAction* a = new QAction(QLocale::languageToString(QLocale::Chinese), m_languagesGroup);
    a->setCheckable(true);
    a->setData("zh");
    a = new QAction(QLocale::languageToString(QLocale::Czech), m_languagesGroup);
    a->setCheckable(true);
    a->setData("cs");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::English), m_languagesGroup);
    a->setCheckable(true);
    a->setData("en");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::French), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fr");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::German), m_languagesGroup);
    a->setCheckable(true);
    a->setData("de");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Portuguese), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Spanish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("es");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    const QString locale = m_settings.value("language", QLocale::system().name()).toString();
    foreach (QAction* action, m_languagesGroup->actions())
        if (locale.startsWith(action->data().toString()))
            action->setChecked(true);
    connect(m_languagesGroup, SIGNAL(triggered(QAction*)), this, SLOT(onLanguageTriggered(QAction*)));

    // Setup the themes actions
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->addAction(ui->actionSystemTheme);
    themeGroup->addAction(ui->actionFusionDark);
    themeGroup->addAction(ui->actionFusionLight);
    QString theme = m_settings.value("theme", "dark").toString();
    if (theme == "dark")
        ui->actionFusionDark->setChecked(true);
    else if (theme == "light")
        ui->actionFusionLight->setChecked(true);
    else
        ui->actionSystemTheme->setChecked(true);
}

QAction* MainWindow::addProfile(QActionGroup* actionGroup, const QString& desc, const QString& name)
{
    QAction* action = new QAction(desc, this);
    action->setCheckable(true);
    action->setData(name);
    actionGroup->addAction(action);
    return action;
}

void MainWindow::open(Mlt::Producer* producer)
{
    if (!producer->is_valid())
        ui->statusBar->showMessage(tr("Failed to open "), STATUS_TIMEOUT_MS);
    else if (producer->get_int("error"))
        ui->statusBar->showMessage(tr("Failed to open ") + producer->get("resource"), STATUS_TIMEOUT_MS);
    // no else here because open() will delete the producer if open fails
    if (!MLT.open(producer))
        emit producerOpened();
    m_player->setFocus();
}

void MainWindow::open(const QString& url, const Mlt::Properties* properties)
{
    if (url.endsWith(".mlt") || url.endsWith(".xml")) {
        // only check for a modified project when loading a project, not a simple producer
        if (!continueModified())
            return;
        // close existing project
        if (m_playlistDock->model()->playlist())
            m_playlistDock->model()->close();
        // let the new project change the profile
        MLT.profile().set_explicit(false);
    }
    else if (!m_playlistDock->model()->playlist()) {
        if (!continueModified())
            return;
        setCurrentFile("");
        setWindowModified(false);
    }
    if (!MLT.open(url.toUtf8().constData())) {
        Mlt::Properties* props = const_cast<Mlt::Properties*>(properties);
        if (props && props->is_valid())
            mlt_properties_inherit(MLT.producer()->get_properties(), props->get_properties());
        open(MLT.producer());
        m_recentDock->add(url.toUtf8().constData());
    }
    else {
        ui->statusBar->showMessage(tr("Failed to open ") + url, STATUS_TIMEOUT_MS);
        emit openFailed(url);
    }
}

void MainWindow::openVideo()
{
    QString settingKey("openPath");
    QString directory(m_settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open File"), directory);

    if (filenames.length() > 0) {
        m_settings.setValue(settingKey, QFileInfo(filenames.first()).path());
        activateWindow();
        if (filenames.length() > 1)
            m_multipleFiles = filenames;
        open(filenames.first());
    }
    else {
        // If file invalid, then on some platforms the dialog messes up SDL.
        MLT.onWindowResize();
        activateWindow();
    }
}

void MainWindow::openCut(void* producer, int in, int out)
{
    double speed = MLT.producer()? MLT.producer()->get_speed(): 0;
    open((Mlt::Producer*) producer);
    m_player->setIn(in);
    m_player->setOut(out);
    MLT.seek(in);
    if (speed == 0)
        m_player->pause();
    else
        m_player->play(speed);
}

void MainWindow::showStatusMessage(QString message)
{
    ui->statusBar->showMessage(message, STATUS_TIMEOUT_MS);
}

void MainWindow::seekPlaylist(int start)
{
    if (!m_playlistDock->model()->playlist()) return;
    double speed = MLT.producer()? MLT.producer()->get_speed(): 0;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if ((void*) MLT.producer()->get_producer() != (void*) m_playlistDock->model()->playlist()->get_playlist())
        MLT.open(new Mlt::Producer(*(m_playlistDock->model()->playlist())));
    m_player->setIn(-1);
    m_player->setOut(-1);
    // since we do not emit producerOpened, these components need updating
    on_actionJack_triggered(ui->actionJack->isChecked());
    m_player->onProducerOpened();
    m_encodeDock->onProducerOpened();
    m_filtersDock->onProducerOpened();
    updateMarkers();
    if (speed == 0)
        m_player->pause();
    else
        m_player->play(speed);
    MLT.seek(start);
    m_player->setFocus();
}

void MainWindow::readPlayerSettings()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    ui->actionOpenGL->setChecked(m_settings.value("player/opengl", true).toBool());
#endif
    ui->actionRealtime->setChecked(m_settings.value("player/realtime", true).toBool());
    ui->actionProgressive->setChecked(m_settings.value("player/progressive", true).toBool());
    ui->actionJack->setChecked(m_settings.value("player/jack", false).toBool());
    ui->actionGPU->setChecked(m_settings.value("player/gpu", false).toBool());
    MLT.videoWidget()->setProperty("gpu", ui->actionGPU->isChecked());
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
    foreach (QAction* a, m_externalGroup->actions()) {
        if (a->data() == external) {
            a->setChecked(true);
            break;
        }
    }

    QVariant profile = m_settings.value("player/profile", "");
    // Automatic not permitted for SDI/HDMI
    if (!external.toString().isEmpty() && profile.toString().isEmpty())
        profile = QVariant("atsc_720p_50");
    foreach (QAction* a, m_profileGroup->actions()) {
        // Automatic not permitted for SDI/HDMI
        if (a->data().toString().isEmpty() && !external.toString().isEmpty())
            a->setDisabled(true);
        if (a->data() == profile) {
            a->setChecked(true);
            break;
        }
    }
}

void MainWindow::readWindowSettings()
{
    restoreGeometry(m_settings.value("geometry").toByteArray());
    restoreState(m_settings.value("windowState").toByteArray());
    m_jobsVisible = m_jobsDock->isVisible();
}

void MainWindow::writeSettings()
{
    if (isFullScreen())
        showNormal();
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("windowState", saveState());
}

void MainWindow::configureVideoWidget()
{
    MLT.videoWidget()->setProperty("mlt_service",
        ui->menuExternal? m_externalGroup->checkedAction()->data() : QString());
    MLT.setProfile(m_profileGroup->checkedAction()->data().toString());
    MLT.videoWidget()->setProperty("realtime", ui->actionRealtime->isChecked());
    if (!ui->menuExternal || m_externalGroup->checkedAction()->data().toString().isEmpty())
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
    MLT.videoWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void MainWindow::setCurrentFile(const QString &filename)
{
    QString shownName = "Untitled";
    m_currentFile = filename;
    if (!m_currentFile.isEmpty())
        shownName = QFileInfo(m_currentFile).fileName();
    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(qApp->applicationName()));
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version %1</h1>"
                "<p><a href=\"http://www.shotcut.org/\">Shotcut</a> is a free, open source, cross platform video editor.</p>"
                "<small><p>Copyright &copy; 2011-2013 <a href=\"http://www.meltytech.com/\">Meltytech</a>, LLC</p>"
                "<p>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program proudly uses the following projects:<ul>"
                "<li><a href=\"http://www.qt-project.org/\">Qt</a> application and UI framework</li>"
                "<li><a href=\"http://www.mltframework.org/\">MLT</a> multimedia authoring framework</li>"
                "<li><a href=\"http://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries</li>"
                "<li><a href=\"http://www.videolan.org/developers/x264.html\">x264</a> H.264 encoder</li>"
                "<li><a href=\"http://www.webmproject.org/\">WebM</a> VP8 encoder</li>"
                "<li><a href=\"http://lame.sourceforge.net/\">LAME</a> MP3 encoder</li>"
                "<li><a href=\"http://www.dyne.org/software/frei0r/\">Frei0r</a> video plugins</li>"
                "<li><a href=\"http://www.ladspa.org/\">LADSPA</a> audio plugins</li>"
                "<li><a href=\"http://www.defaulticon.com/\">DefaultIcon</a> icon collection by <a href=\"http://www.interactivemania.com/\">interactivemania</a></li>"
                "<li><a href=\"http://www.oxygen-icons.org/\">Oxygen</a> icon collection</li>"
                "</ul></p>"
                "<p>The source code used to build this program can be downloaded from "
                "<a href=\"http://www.shotcut.org/\">shotcut.org</a>.</p>"
                "This program is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</small>"
                ).arg(qApp->applicationVersion()));
}


void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Home:
        m_player->seek(0);
        break;
    case Qt::Key_End:
        m_player->seek(MLT.producer()->get_length() - 1);
        break;
    case Qt::Key_Left:
        m_player->seek(m_player->position() - 1);
        break;
    case Qt::Key_Right:
        m_player->seek(m_player->position() + 1);
        break;
    case Qt::Key_PageUp:
        m_player->seek(m_player->position() - qRound(MLT.profile().fps()));
        break;
    case Qt::Key_PageDown:
        m_player->seek(m_player->position() + qRound(MLT.profile().fps()));
        break;
    case Qt::Key_J:
        if (m_isKKeyPressed)
            m_player->seek(m_player->position() - 1);
        else
            m_player->rewind();
        break;
    case Qt::Key_K:
        m_player->pause();
        m_isKKeyPressed = true;
        break;
    case Qt::Key_L:
        if (m_isKKeyPressed)
            m_player->seek(m_player->position() + 1);
        else
            m_player->fastForward();
        break;
    case Qt::Key_I:
        if (MLT.isSeekable() && !MLT.isPlaylist())
            m_player->setIn(m_player->position());
        break;
    case Qt::Key_O:
        if (MLT.isSeekable() && !MLT.isPlaylist())
            m_player->setOut(m_player->position());
        break;
    case Qt::Key_V: // Avid Splice In
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->on_actionInsertCut_triggered();
        break;
    case Qt::Key_B: // Avid Overwrite
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->on_actionUpdate_triggered();
        break;
    case Qt::Key_Escape: // Avid Toggle Active Monitor
        if (MLT.isPlaylist())
            m_playlistDock->on_actionOpen_triggered();
        else if (m_playlistDock->position() >= 0) {
            m_playlistDock->show();
            seekPlaylist(m_playlistDock->position());
        }
        break;
    case Qt::Key_Up:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            if (event->modifiers() == Qt::ControlModifier)
                m_playlistDock->moveClipUp();
            m_playlistDock->decrementIndex();
        }
        break;
    case Qt::Key_Down:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            if (event->modifiers() == Qt::ControlModifier)
                m_playlistDock->moveClipDown();
            m_playlistDock->incrementIndex();
        }
        break;
    case Qt::Key_1:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(0);
        }
        break;
    case Qt::Key_2:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(1);
        }
        break;
    case Qt::Key_3:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(2);
        }
        break;
    case Qt::Key_4:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(3);
        }
        break;
    case Qt::Key_5:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(4);
        }
        break;
    case Qt::Key_6:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(5);
        }
        break;
    case Qt::Key_7:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(6);
        }
        break;
    case Qt::Key_8:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(7);
        }
        break;
    case Qt::Key_9:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(8);
        }
        break;
    case Qt::Key_0:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(9);
        }
        break;
    case Qt::Key_X: // Avid Extract
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->on_removeButton_clicked();
        break;
    case Qt::Key_Enter: // Seek to current playlist item
    case Qt::Key_Return:
        if (m_playlistDock->position() >= 0)
            seekPlaylist(m_playlistDock->position());
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_K)
        m_isKKeyPressed = false;
    else
        QMainWindow::keyPressEvent(event);
}

// Drag-n-drop events

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QMap<int,  QVariant> roleDataMap;
        while (!stream.atEnd()) {
            int row, col;
            stream >> row >> col >> roleDataMap;
        }
        if (roleDataMap.contains(Qt::ToolTipRole)) {
            // DisplayRole is just basename, ToolTipRole contains full path
            open(roleDataMap[Qt::ToolTipRole].toString());
            event->acceptProposedAction();
        }
    }
    else if (mimeData->hasUrls()) {
        if (mimeData->urls().length() > 1) {
            foreach (QUrl url, mimeData->urls())
                m_multipleFiles.append(url.path());
        }
        QString path = mimeData->urls().first().url();
        if (mimeData->urls().first().scheme() == "file")
            path = mimeData->urls().first().url(QUrl::RemoveScheme);
        if (path.length() > 2 && path.startsWith("///"))
#ifdef Q_OS_WIN
            path.remove(0, 3);
#else
            path.remove(0, 2);
#endif
        open(path);
        event->acceptProposedAction();
    }
    else if (mimeData->hasFormat("application/mlt+xml")) {
        m_playlistDock->on_actionOpen_triggered();
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (continueModified()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::on_actionOpenOther_triggered()
{
    // these static are used to open dialog with previous configuration
    OpenOtherDialog dialog(this);

    if (MLT.producer())
        dialog.load(MLT.producer());
    if (dialog.exec() == QDialog::Accepted)
        open(dialog.producer(MLT.profile()));
}

void MainWindow::onProducerOpened()
{
    QString service(MLT.producer()->get("mlt_service"));
    QString resource(MLT.resource());
    QWidget* w = 0;

    m_meltedServerDock->disconnect(SIGNAL(positionUpdated(int,double,int,int,int,bool)));
    m_player->connectTransport(MLT.transportControl());
    delete m_propertiesDock->widget();

    // Remove the help page.
    if (ui->stackedWidget->count() > 1)
        delete ui->stackedWidget->widget(0);

    if (resource.startsWith("video4linux2:"))
        w = new Video4LinuxWidget(this);
    else if (resource.startsWith("pulse:"))
        w = new PulseAudioWidget(this);
    else if (resource.startsWith("jack:"))
        w = new JackProducerWidget(this);
    else if (resource.startsWith("alsa:"))
        w = new AlsaWidget(this);
    else if (resource.startsWith("x11grab:"))
        w = new X11grabWidget(this);
    else if (service == "avformat") {
        AvformatProducerWidget* avw = new AvformatProducerWidget(this);
        w = avw;
        connect(avw, SIGNAL(producerReopened()), m_player, SLOT(onProducerOpened()));
    }
    else if (service == "pixbuf" || service == "qimage") {
        ImageProducerWidget* avw = new ImageProducerWidget(this);
        w = avw;
        connect(avw, SIGNAL(producerReopened()), m_player, SLOT(onProducerOpened()));
    }
    else if (service == "decklink" || resource.contains("decklink"))
        w = new DecklinkProducerWidget(this);
    else if (service == "color")
        w = new ColorProducerWidget(this);
    else if (service == "noise")
        w = new NoiseWidget(this);
    else if (service == "frei0r.ising0r")
        w = new IsingWidget(this);
    else if (service == "frei0r.lissajous0r")
        w = new LissajousWidget(this);
    else if (service == "frei0r.plasma")
        w = new PlasmaWidget(this);
    else if (service == "frei0r.test_pat_B")
        w = new ColorBarsWidget(this);
    else if (MLT.isPlaylist()) {
        m_playlistDock->model()->load();
        if (m_playlistDock->model()->playlist()) {
            m_isPlaylistLoaded = true;
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_playlistDock->setVisible(true);
            m_playlistDock->raise();
        }
    }
    if (QString(MLT.producer()->get("xml")) == "was here")
        setCurrentFile(MLT.URL());
    if (w) {
        dynamic_cast<AbstractProducerWidget*>(w)->setProducer(MLT.producer());
        if (-1 != w->metaObject()->indexOfSignal("producerChanged()"))
            connect(w, SIGNAL(producerChanged()), this, SLOT(onProducerChanged()));
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setWidget(w);
        m_propertiesDock->setWidget(scroll);
    }
    onProducerChanged();
    on_actionJack_triggered(ui->actionJack->isChecked());
}

void MainWindow::onProducerChanged()
{
    MLT.refreshConsumer();
}

bool MainWindow::on_actionSave_triggered()
{
    if (m_currentFile.isEmpty()) {
        return on_actionSave_As_triggered();
    } else {
        saveXML(m_currentFile);
        setCurrentFile(m_currentFile);
        setWindowModified(false);
        showStatusMessage(tr("Saved %1").arg(m_currentFile));
        m_undoStack->setClean();
        return true;
    }
}

bool MainWindow::on_actionSave_As_triggered()
{
    if (!MLT.producer())
        return true;
    QString settingKey("openPath");
    QString directory(m_settings.value(settingKey,
        QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString());
    QString filename = QFileDialog::getSaveFileName(this, tr("Save XML"), directory, tr("MLT XML (*.mlt)"));
    if (!filename.isEmpty()) {
        saveXML(filename);
        setCurrentFile(filename);
        setWindowModified(false);
        showStatusMessage(tr("Saved %1").arg(m_currentFile));
        m_undoStack->setClean();
        m_recentDock->add(filename);
    }
    return filename.isEmpty();
}

bool MainWindow::continueModified()
{
    if (isWindowModified()) {
        QMessageBox dialog(QMessageBox::Warning,
                                     qApp->applicationName(),
                                     tr("The project has been modified.\n"
                                        "Do you want to save your changes?"),
                                     QMessageBox::No |
                                     QMessageBox::Cancel |
                                     QMessageBox::Yes,
                                     this);
        dialog.setWindowModality(Qt::WindowModal);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::Cancel);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            return on_actionSave_triggered();
        } else if (r == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

QUndoStack* MainWindow::undoStack() const
{
    return m_undoStack;
}

void MainWindow::onEncodeTriggered(bool checked)
{
    m_encodeDock->setVisible(checked);
    if (checked) {
        m_jobsDock->setVisible(m_jobsVisible);
        m_encodeDock->raise();
    } else {
        bool saveVisibility = m_jobsDock->isVisible();
        m_jobsDock->setVisible(false);
        m_jobsVisible = saveVisibility;
    }
}

void MainWindow::onCaptureStateChanged(bool started)
{
    if (started && MLT.resource().startsWith("x11grab:")
                && !MLT.producer()->get_int("shotcut_bgcapture"))
        showMinimized();
}

void MainWindow::onEncodeVisibilityChanged(bool checked)
{
    if (m_encodeDock->isHidden())
        m_jobsDock->hide();
}

void MainWindow::onJobsVisibilityChanged(bool checked)
{
    m_jobsVisible = checked;
    if (checked)
        m_jobsDock->raise();
}

void MainWindow::onRecentDockTriggered(bool checked)
{
    if (checked) {
        m_recentDock->show();
        m_recentDock->raise();
    }
}

void MainWindow::onPropertiesDockTriggered(bool checked)
{
    if (checked) {
        m_propertiesDock->show();
        m_propertiesDock->raise();
    }
}

void MainWindow::onPlaylistDockTriggered(bool checked)
{
    if (checked) {
        m_playlistDock->show();
        m_playlistDock->raise();
    }
}

void MainWindow::onHistoryDockTriggered(bool checked)
{
    if (checked) {
        m_historyDock->show();
        m_historyDock->raise();
    }
}

void MainWindow::onFiltersDockTriggered(bool checked)
{
    if (checked) {
        m_filtersDock->show();
        m_filtersDock->raise();
    }
}

void MainWindow::onPlaylistCreated()
{
    setCurrentFile("");
}

void MainWindow::onPlaylistCleared()
{
    open(new Mlt::Producer(MLT.profile(), "color:"));
    m_player->pause();
    m_player->seek(0);
    setWindowModified(true);
}

void MainWindow::onPlaylistClosed()
{
    m_player->resetProfile();
    onPlaylistCleared();
    setCurrentFile("");
    setWindowModified(false);
    m_undoStack->clear();
}

void MainWindow::onPlaylistModified()
{
    setWindowModified(true);
    if ((void*) MLT.producer()->get_producer() == (void*) m_playlistDock->model()->playlist()->get_playlist())
        m_player->onProducerModified();
}

void MainWindow::onCutModified()
{
    if (!m_playlistDock->model()->playlist())
        setWindowModified(true);
}

void MainWindow::updateMarkers()
{
    if (m_playlistDock->model()->playlist()) {
        QList<int> markers;
        int n = m_playlistDock->model()->playlist()->count();
        for (int i = 0; i < n; i++)
            markers.append(m_playlistDock->model()->playlist()->clip_start(i));
        m_player->setMarkers(markers);
    }
}

void MainWindow::updateThumbnails()
{
    if (m_settings.value("playlist/thumbnails").toString() != "hidden")
        m_playlistDock->model()->refreshThumbnails();
}

void MainWindow::on_actionUndo_triggered()
{
    m_undoStack->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    m_undoStack->redo();
}

void MainWindow::on_actionFAQ_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.shotcut.org/bin/view/Shotcut/FrequentlyAskedQuestions"));
}

void MainWindow::on_actionForum_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.shotcut.org/bin/view/Shotcut/DiscussionForum"));
}

void MainWindow::saveXML(const QString &filename)
{
    if (m_playlistDock->model()->playlist()) {
        int in = MLT.producer()->get_in();
        int out = MLT.producer()->get_out();
        MLT.producer()->set_in_and_out(0, MLT.producer()->get_length() - 1);
        MLT.saveXML(filename, m_playlistDock->model()->playlist());
        MLT.producer()->set_in_and_out(in, out);
    } else {
        MLT.saveXML(filename);
    }
}

void MainWindow::changeTheme(const QString &theme)
{
    if (theme == "dark") {
        QApplication::setStyle("Fusion");
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(50,50,50));
        palette.setColor(QPalette::WindowText, QColor(220,220,220));
        palette.setColor(QPalette::Base, QColor(35,35,35));
        palette.setColor(QPalette::AlternateBase, QColor(31,31,31));
        palette.setColor(QPalette::Highlight, QColor(23,92,118));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::ToolTipBase, palette.color(QPalette::Highlight));
        palette.setColor(QPalette::ToolTipText, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Text, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Button, palette.color(QPalette::Window));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Link, palette.color(QPalette::Highlight).lighter());
        palette.setColor(QPalette::LinkVisited, palette.color(QPalette::Highlight));
        QApplication::setPalette(palette);
        QIcon::setThemeName("dark");
    } else if (theme == "light") {
        QStyle* style = QStyleFactory::create("Fusion");
        QApplication::setStyle(style);
        QApplication::setPalette(style->standardPalette());
        QIcon::setThemeName("light");
    } else {
        QApplication::setStyle(qApp->property("system-style").toString());
        QIcon::setThemeName("oxygen");
    }
}

void MainWindow::onMeltedUnitOpened()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "color:");
    MLT.open(producer);
    MLT.play(0);
    // Remove the help page.
    if (ui->stackedWidget->count() > 1)
        delete ui->stackedWidget->widget(0);
    delete m_propertiesDock->widget();
    m_player->connectTransport(m_meltedPlaylistDock->transportControl());
    connect(m_meltedServerDock, SIGNAL(positionUpdated(int,double,int,int,int,bool)),
            m_player, SLOT(onShowFrame(int,double,int,int,int,bool)));
    onProducerChanged();
}

void MainWindow::onMeltedUnitActivated()
{
    m_meltedPlaylistDock->setVisible(true);
    m_meltedPlaylistDock->raise();
}

void MainWindow::on_actionEnter_Full_Screen_triggered()
{
    if (isFullScreen()) {
        showNormal();
        ui->actionEnter_Full_Screen->setText(tr("Enter Full Screen"));
    } else {
        showFullScreen();
        ui->actionEnter_Full_Screen->setText(tr("Exit Full Screen"));
    }
}

void MainWindow::onGpuNotSupported()
{
    m_settings.setValue("player/gpu", false);
    ui->actionGPU->setChecked(false);
    ui->actionGPU->setDisabled(true);
    showStatusMessage(tr("GPU Processing is not supported"));
}

void MainWindow::on_actionOpenGL_triggered(bool checked)
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

void MainWindow::on_actionRealtime_triggered(bool checked)
{
    MLT.videoWidget()->setProperty("realtime", checked);
    if (MLT.consumer()) {
        MLT.consumer()->set("real_time", checked? 1 : -1);
        MLT.restart();
    }
    m_settings.setValue("player/realtime", checked);
}

void MainWindow::on_actionProgressive_triggered(bool checked)
{
    MLT.videoWidget()->setProperty("progressive", checked);
    if (MLT.consumer() && !MLT.profile().progressive()) {
        MLT.consumer()->set("progressive", checked);
        MLT.restart();
    }
    m_settings.setValue("player/progressive", checked);
}

void MainWindow::changeDeinterlacer(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("deinterlace_method", method);
        if (MLT.consumer()) {
            MLT.consumer()->set("deinterlace_method", method);
            MLT.restart();
        }
    }
    m_settings.setValue("player/deinterlacer", method);
}

void MainWindow::on_actionOneField_triggered(bool checked)
{
    changeDeinterlacer(checked, "onefield");
}

void MainWindow::on_actionLinearBlend_triggered(bool checked)
{
    changeDeinterlacer(checked, "linearblend");
}

void MainWindow::on_actionYadifTemporal_triggered(bool checked)
{
    changeDeinterlacer(checked, "yadif-nospatial");
}

void MainWindow::on_actionYadifSpatial_triggered(bool checked)
{
    changeDeinterlacer(checked, "yadif");
}

void MainWindow::changeInterpolation(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("rescale", method);
        if (MLT.consumer()) {
            MLT.consumer()->set("rescale", method);
            MLT.restart();
        }
    }
    m_settings.setValue("player/interpolation", method);
}

class AppendTask : public QRunnable
{
public:
    AppendTask(PlaylistModel* model, QString filename)
        : QRunnable()
        , model(model)
        , filename(filename)
    {
    }
    void run()
    {
        Mlt::Producer p(MLT.profile(), filename.toUtf8().constData());
        if (p.is_valid()) {
            QString service(p.get("mlt_service"));
            if (service == "pixbuf" || service == "qimage") {
                p.set("ttl", 1);
                p.set("length", qRound(MLT.profile().fps() * 4.0));
                p.set("out", p.get_length() - 1);
            }
            MAIN.undoStack()->push(new Playlist::AppendCommand(*model, MLT.saveXML("string", &p)));
        }
    }
private:
    PlaylistModel* model;
    QString filename;
};

void MainWindow::processMultipleFiles()
{
    if (m_multipleFiles.length() > 0) {
        PlaylistModel* model = m_playlistDock->model();
        m_playlistDock->show();
        m_playlistDock->raise();
        foreach (QString filename, m_multipleFiles) {
            QThreadPool::globalInstance()->start(new AppendTask(model, filename));
            m_recentDock->add(filename.toUtf8().constData());
        }
        m_multipleFiles.clear();
    }
    if (m_isPlaylistLoaded && m_settings.value("player/gpu").toBool()) {
        updateThumbnails();
        m_isPlaylistLoaded = false;
    }
}

void MainWindow::onLanguageTriggered(QAction* action)
{
    m_settings.setValue("language", action->data());
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to switch to the new language.\n"
                          "Do you want to exit now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(Qt::WindowModal);
    if (dialog.exec() == QMessageBox::Yes)
        QApplication::closeAllWindows();
}

void MainWindow::on_actionNearest_triggered(bool checked)
{
    changeInterpolation(checked, "nearest");
}

void MainWindow::on_actionBilinear_triggered(bool checked)
{
    changeInterpolation(checked, "bilinear");
}

void MainWindow::on_actionBicubic_triggered(bool checked)
{
    changeInterpolation(checked, "bicubic");
}

void MainWindow::on_actionHyper_triggered(bool checked)
{
    changeInterpolation(checked, "hyper");
}

void MainWindow::on_actionJack_triggered(bool checked)
{
    m_settings.setValue("player/jack", checked);
    if (!MLT.enableJack(checked)) {
        ui->actionJack->setChecked(false);
        m_settings.setValue("player/jack", false);
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to connect to JACK.\nPlease verify that JACK is installed and running."));
    }
}

void MainWindow::on_actionGPU_triggered(bool checked)
{
    m_settings.setValue("player/gpu", checked);
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to switch using GPU processing.\n"
                          "Do you want to exit now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(Qt::WindowModal);
    if (dialog.exec() == QMessageBox::Yes)
        QApplication::closeAllWindows();
}

void MainWindow::onExternalTriggered(QAction *action)
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
        emit profileChanged();
        foreach (QAction* a, m_profileGroup->actions()) {
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
    m_profileGroup->actions().at(0)->setEnabled(!isExternal);

    // Disable progressive option when SDI/HDMI
    ui->actionProgressive->setEnabled(!isExternal);
    bool isProgressive = isExternal
            ? MLT.profile().progressive()
            : ui->actionProgressive->isChecked();
    MLT.videoWidget()->setProperty("progressive", isProgressive);
    if (MLT.consumer()) {
        MLT.consumer()->set("progressive", isProgressive);
        MLT.restart();
    }
}

void MainWindow::onProfileTriggered(QAction *action)
{
    m_settings.setValue("player/profile", action->data());
    MLT.setProfile(action->data().toString());
    emit profileChanged();
}

void MainWindow::on_actionAddCustomProfile_triggered()
{
    CustomProfileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    if (dialog.exec() == QDialog::Accepted) {
        QDir dir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
        if (dir.cd("profiles")) {
            QString name = dialog.profileName();
            QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
            if (profiles.length() == 1)
                m_customProfileMenu->addSeparator();
            QAction* action = addProfile(m_profileGroup, name, dir.filePath(name));
            action->setChecked(true);
            m_customProfileMenu->addAction(action);
        }
    }
}

void MainWindow::on_actionSystemTheme_triggered()
{
    changeTheme("system");
    QApplication::setPalette(QApplication::style()->standardPalette());
    m_settings.setValue("theme", "system");
}

void MainWindow::on_actionFusionDark_triggered()
{
    changeTheme("dark");
    m_settings.setValue("theme", "dark");
}

void MainWindow::on_actionFusionLight_triggered()
{
    changeTheme("light");
    m_settings.setValue("theme", "light");
}
