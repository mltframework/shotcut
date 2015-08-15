/*
 * Copyright (c) 2011-2015 Meltytech, LLC
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
#include "widgets/directshowvideowidget.h"
#include "widgets/isingwidget.h"
#include "widgets/jackproducerwidget.h"
#include "widgets/toneproducerwidget.h"
#include "widgets/lissajouswidget.h"
#include "widgets/networkproducerwidget.h"
#include "widgets/noisewidget.h"
#include "widgets/plasmawidget.h"
#include "widgets/pulseaudiowidget.h"
#include "widgets/video4linuxwidget.h"
#include "widgets/x11grabwidget.h"
#include "widgets/avformatproducerwidget.h"
#include "widgets/imageproducerwidget.h"
#include "widgets/webvfxproducer.h"
#include "docks/recentdock.h"
#include "docks/encodedock.h"
#include "docks/jobsdock.h"
#include "jobqueue.h"
#include "docks/playlistdock.h"
#include "glwidget.h"
#include "mvcp/meltedserverdock.h"
#include "mvcp/meltedplaylistdock.h"
#include "mvcp/meltedunitsmodel.h"
#include "mvcp/meltedplaylistmodel.h"
#include "controllers/filtercontroller.h"
#include "controllers/scopecontroller.h"
#include "docks/filtersdock.h"
#include "dialogs/customprofiledialog.h"
#include "htmleditor/htmleditor.h"
#include "settings.h"
#include "leapnetworklistener.h"
#include "database.h"
#include "widgets/gltestwidget.h"
#include "docks/timelinedock.h"
#include "widgets/lumamixtransition.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlapplication.h"
#include "autosavefile.h"
#include "commands/playlistcommands.h"
#include "shotcut_mlt_properties.h"

#include <QtWidgets>
#include <QDebug>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <QMutexLocker>

static const int STATUS_TIMEOUT_MS = 5000;
static const int AUTOSAVE_TIMEOUT_MS = 10000;

MainWindow::MainWindow()
    : QMainWindow(0)
    , ui(new Ui::MainWindow)
    , m_isKKeyPressed(false)
    , m_keyerGroup(0)
    , m_keyerMenu(0)
    , m_isPlaylistLoaded(false)
    , m_htmlEditor(0)
    , m_autosaveFile(0)
    , m_exitCode(EXIT_SUCCESS)
    , m_navigationPosition(0)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QLibrary libJack("libjack.so.0");
    if (!libJack.load()) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Error: This program requires the JACK 1 library.\n\nPlease install it using your package manager. It may be named libjack0, jack-audio-connection-kit, jack, or similar."));
        ::exit(EXIT_FAILURE);
    } else {
        libJack.unload();
    }
    QLibrary libSDL("libSDL-1.2.so.0");
    if (!libSDL.load()) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Error: This program requires the SDL 1.2 library.\n\nPlease install it using your package manager. It may be named libsdl1.2debian, SDL, or similar."));
        ::exit(EXIT_FAILURE);
    } else {
        libSDL.unload();
    }
#endif

    if (!qgetenv("OBSERVE_FOCUS").isEmpty())
        connect(qApp, &QApplication::focusChanged,
                this, &MainWindow::onFocusChanged);

    qDebug() << "begin";
#ifndef Q_OS_WIN
    new GLTestWidget(this);
#endif
    Database::singleton(this);
    m_autosaveTimer.setSingleShot(true);
    m_autosaveTimer.setInterval(AUTOSAVE_TIMEOUT_MS);
    connect(&m_autosaveTimer, SIGNAL(timeout()), this, SLOT(onAutosaveTimeout()));

    // Initialize all QML types
    QmlUtilities::registerCommonTypes();

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
    setDockNestingEnabled(true);

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));
    connect(ui->actionFullscreen, SIGNAL(triggered()), this, SLOT(on_actionEnter_Full_Screen_triggered()));
    connect(ui->mainToolBar, SIGNAL(visibilityChanged(bool)), SLOT(onToolbarVisibilityChanged(bool)));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Setup the undo stack.
    m_undoStack = new QUndoStack(this);
    QAction *undoAction = m_undoStack->createUndoAction(this);
    QAction *redoAction = m_undoStack->createRedoAction(this);
    undoAction->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/oxygen/16x16/actions/edit-undo.png")));
    redoAction->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/oxygen/32x32/actions/edit-redo.png")));
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
    MLT.videoWidget()->installEventFilter(this);
    ui->centralWidget->layout()->addWidget(m_player);
    connect(this, SIGNAL(producerOpened()), m_player, SLOT(onProducerOpened()));
    connect(m_player, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_player, SIGNAL(inChanged(int)), this, SLOT(onCutModified()));
    connect(m_player, SIGNAL(outChanged(int)), this, SLOT(onCutModified()));
    connect(MLT.videoWidget(), SIGNAL(started()), SLOT(processMultipleFiles()));
    connect(MLT.videoWidget(), SIGNAL(paused()), m_player, SLOT(showPaused()));
    connect(MLT.videoWidget(), SIGNAL(playing()), m_player, SLOT(showPlaying()));

    setupSettingsMenu();
    readPlayerSettings();
    configureVideoWidget();

    // Add the docks.
    m_scopeController = new ScopeController(this, ui->menuView);
    QDockWidget* audioMeterDock = findChild<QDockWidget*>("AudioPeakMeterDock");
    if (audioMeterDock) {
        connect(ui->actionAudioMeter, SIGNAL(triggered()), audioMeterDock->toggleViewAction(), SLOT(trigger()));
    }

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->hide();
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setWindowIcon(ui->actionProperties->icon());
    m_propertiesDock->toggleViewAction()->setIcon(ui->actionProperties->icon());
    m_propertiesDock->setMinimumWidth(300);
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    m_propertiesDock->setWidget(scroll);
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    ui->menuView->addAction(m_propertiesDock->toggleViewAction());
    connect(m_propertiesDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPropertiesDockTriggered(bool)));
    connect(ui->actionProperties, SIGNAL(triggered()), this, SLOT(onPropertiesDockTriggered()));

    m_recentDock = new RecentDock(this);
    m_recentDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_recentDock);
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
    connect(m_playlistDock, SIGNAL(clipOpened(void*)), this, SLOT(openCut(void*)));
    connect(m_playlistDock, SIGNAL(itemActivated(int)), this, SLOT(seekPlaylist(int)));
    connect(m_playlistDock, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_playlistDock->model(), SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(updateAutoSave()));
    connect(m_playlistDock->model(), SIGNAL(closed()), this, SLOT(onPlaylistClosed()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(updateAutoSave()));
    connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    if (!Settings.playerGPU())
        connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(updateThumbnails()));

    m_timelineDock = new TimelineDock(this);
    m_timelineDock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);
    ui->menuView->addAction(m_timelineDock->toggleViewAction());
    connect(m_timelineDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onTimelineDockTriggered(bool)));
    connect(ui->actionTimeline, SIGNAL(triggered()), SLOT(onTimelineDockTriggered()));
    connect(m_player, SIGNAL(seeked(int)), m_timelineDock, SLOT(onSeeked(int)));
    connect(m_timelineDock, SIGNAL(seeked(int)), SLOT(seekTimeline(int)));
    connect(m_timelineDock, SIGNAL(clipClicked()), SLOT(moveNavigationPositionToCurrentSelection()));
    connect(m_timelineDock->model(), SIGNAL(created()), SLOT(onMultitrackCreated()));
    connect(m_timelineDock->model(), SIGNAL(closed()), SLOT(onMultitrackClosed()));
    connect(m_timelineDock->model(), SIGNAL(modified()), SLOT(onMultitrackModified()));
    connect(m_timelineDock->model(), SIGNAL(modified()), SLOT(updateAutoSave()));
    connect(m_timelineDock, SIGNAL(clipOpened(void*)), SLOT(openCut(void*)));
    connect(m_timelineDock->model(), SIGNAL(seeked(int)), SLOT(seekTimeline(int)));
    connect(m_playlistDock, SIGNAL(addAllTimeline(Mlt::Playlist*)), SLOT(onTimelineDockTriggered()));
    connect(m_playlistDock, SIGNAL(addAllTimeline(Mlt::Playlist*)), SLOT(onAddAllToTimeline(Mlt::Playlist*)));
    connect(m_player, SIGNAL(previousSought()), m_timelineDock, SLOT(seekPreviousEdit()));
    connect(m_player, SIGNAL(nextSought()), m_timelineDock, SLOT(seekNextEdit()));

    m_filterController = new FilterController(this);
    m_filtersDock = new FiltersDock(m_filterController->metadataModel(), m_filterController->attachedModel(), this);
    m_filtersDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_filtersDock);
    ui->menuView->addAction(m_filtersDock->toggleViewAction());
    connect(m_filtersDock, SIGNAL(currentFilterRequested(int)), m_filterController, SLOT(setCurrentFilter(int)), Qt::QueuedConnection);
    connect(m_filtersDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onFiltersDockTriggered(bool)));
    connect(ui->actionFilters, SIGNAL(triggered()), this, SLOT(onFiltersDockTriggered()));
    connect(m_filterController, SIGNAL(currentFilterChanged(QmlFilter*, QmlMetadata*, int)), m_filtersDock, SLOT(setCurrentFilter(QmlFilter*, QmlMetadata*, int)), Qt::QueuedConnection);
    connect(m_filterController, SIGNAL(currentFilterAboutToChange()), m_filtersDock, SLOT(clearCurrentFilter()));
    connect(this, SIGNAL(producerOpened()), m_filterController, SLOT(setProducer()));
    connect(m_filterController->attachedModel(), SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filtersDock, SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filterController, SIGNAL(statusChanged(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_timelineDock, SIGNAL(fadeInChanged(int)), m_filtersDock, SLOT(setFadeInDuration(int)));
    connect(m_timelineDock, SIGNAL(fadeOutChanged(int)), m_filtersDock, SLOT(setFadeOutDuration(int)));
    connect(m_timelineDock, SIGNAL(trackSelected(Mlt::Producer*)), m_filterController, SLOT(setProducer(Mlt::Producer*)));
    connect(m_timelineDock, SIGNAL(clipSelected(Mlt::Producer*)), m_filterController, SLOT(setProducer(Mlt::Producer*)));

    m_historyDock = new QDockWidget(tr("History"), this);
    m_historyDock->hide();
    m_historyDock->setObjectName("historyDock");
    m_historyDock->setWindowIcon(ui->actionHistory->icon());
    m_historyDock->toggleViewAction()->setIcon(ui->actionHistory->icon());
    m_historyDock->setMinimumWidth(150);
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);
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

    m_encodeDock = new EncodeDock(this);
    m_encodeDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_encodeDock);
    ui->menuView->addAction(m_encodeDock->toggleViewAction());
    connect(this, SIGNAL(producerOpened()), m_encodeDock, SLOT(onProducerOpened()));
    connect(ui->actionEncode, SIGNAL(triggered()), this, SLOT(onEncodeTriggered()));
    connect(m_encodeDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onEncodeTriggered(bool)));
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
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(show()));
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(raise()));
    connect(m_jobsDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onJobsDockTriggered(bool)));

    tabifyDockWidget(m_propertiesDock, m_playlistDock);
    tabifyDockWidget(m_playlistDock, m_filtersDock);
    tabifyDockWidget(m_filtersDock, m_encodeDock);
    QDockWidget* audioWaveformDock = findChild<QDockWidget*>("AudioWaveformDock");
    splitDockWidget(m_recentDock, audioWaveformDock, Qt::Vertical);
    splitDockWidget(audioMeterDock, m_recentDock, Qt::Horizontal);
    tabifyDockWidget(m_recentDock, m_historyDock);
    tabifyDockWidget(m_historyDock, m_jobsDock);
    m_recentDock->raise();

    m_meltedServerDock = new MeltedServerDock(this);
    m_meltedServerDock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_meltedServerDock);
    m_meltedServerDock->toggleViewAction()->setIcon(m_meltedServerDock->windowIcon());
    ui->menuView->addAction(m_meltedServerDock->toggleViewAction());

    m_meltedPlaylistDock = new MeltedPlaylistDock(this);
    m_meltedPlaylistDock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_meltedPlaylistDock);
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
    Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
    connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
    connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
    connect(videoWidget, SIGNAL(gpuNotSupported()), this, SLOT(onGpuNotSupported()));
    connect(videoWidget, SIGNAL(frameDisplayed(const SharedFrame&)), m_scopeController, SLOT(onFrameDisplayed(const SharedFrame&)));
    connect(m_filterController, SIGNAL(currentFilterChanged(QmlFilter*, QmlMetadata*, int)), videoWidget, SLOT(setCurrentFilter(QmlFilter*, QmlMetadata*)), Qt::QueuedConnection);
    connect(m_filterController, SIGNAL(currentFilterAboutToChange()), videoWidget, SLOT(setBlankScene()));

    readWindowSettings();
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    setDockNestingEnabled(true);

    setFocus();
    setCurrentFile("");

    LeapNetworkListener* leap = new LeapNetworkListener(this);
    connect(leap, SIGNAL(shuttle(float)), SLOT(onShuttle(float)));
    connect(leap, SIGNAL(jogRightFrame()), SLOT(stepRightOneFrame()));
    connect(leap, SIGNAL(jogRightSecond()), SLOT(stepRightOneSecond()));
    connect(leap, SIGNAL(jogLeftFrame()), SLOT(stepLeftOneFrame()));
    connect(leap, SIGNAL(jogLeftSecond()), SLOT(stepLeftOneSecond()));
    qDebug() << "end";
}

void MainWindow::moveNavigationPositionToCurrentSelection()
{
    TimelineDock * t = m_timelineDock;

    if (t->selection().isEmpty())
        return;

    m_navigationPosition = t->centerOfClip(t->currentTrack(), t->selection().first());
}

void MainWindow::onAddAllToTimeline(Mlt::Playlist* playlist)
{
    // We stop the player because of a bug on Windows that results in some
    // strange memory leak when using Add All To Timeline, more noticeable
    // with (high res?) still image files.
    if (MLT.isSeekable())
        m_player->pause();
    else
        m_player->stop();
    m_timelineDock->appendFromPlaylist(playlist);
}

MainWindow& MainWindow::singleton()
{
    static MainWindow* instance = new MainWindow;
    return *instance;
}

MainWindow::~MainWindow()
{
    m_autosaveMutex.lock();
    delete m_autosaveFile;
    m_autosaveFile = 0;
    m_autosaveMutex.unlock();

    delete m_htmlEditor;
    delete ui;
    Mlt::Controller::destroy();
}

void MainWindow::setupSettingsMenu()
{
    qDebug() << "begin";
    QActionGroup* group = new QActionGroup(this);
    group->addAction(ui->actionOneField);
    group->addAction(ui->actionLinearBlend);
    group->addAction(ui->actionYadifTemporal);
    group->addAction(ui->actionYadifSpatial);
    group = new QActionGroup(this);
    group->addAction(ui->actionNearest);
    group->addAction(ui->actionBilinear);
    group->addAction(ui->actionBicubic);
    group->addAction(ui->actionHyper);
    if (Settings.playerGPU()) {
        group = new QActionGroup(this);
        group->addAction(ui->actionGammaRec709);
        group->addAction(ui->actionGammaSRGB);
    } else {
        delete ui->menuGamma;
    }
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
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 23.98 fps", "uhd_2160p_2398"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 24 fps", "uhd_2160p_24"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 25 fps", "uhd_2160p_25"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 29.97 fps", "uhd_2160p_2997"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 30 fps", "uhd_2160p_30"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 50 fps", "uhd_2160p_50"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 59.94 fps", "uhd_2160p_5994"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "UHD 2160p 60 fps", "uhd_2160p_60"));
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

    // Add the SDI and HDMI devices to the Settings menu.
    m_externalGroup = new QActionGroup(this);
    ui->actionExternalNone->setData(QString());
    m_externalGroup->addAction(ui->actionExternalNone);

    int n = QApplication::desktop()->screenCount();
    for (int i = 0; n > 1 && i < n; i++) {
        QAction* action = new QAction(tr("Screen %1").arg(i), this);
        action->setCheckable(true);
        action->setData(i);
        m_externalGroup->addAction(action);
    }

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

                if (!m_keyerGroup) {
                    m_keyerGroup = new QActionGroup(this);
                    action = new QAction(tr("Off"), m_keyerGroup);
                    action->setData(QVariant(0));
                    action->setCheckable(true);
                    action = new QAction(tr("Internal"), m_keyerGroup);
                    action->setData(QVariant(1));
                    action->setCheckable(true);
                    action = new QAction(tr("External"), m_keyerGroup);
                    action->setData(QVariant(2));
                    action->setCheckable(true);
                }
            }
        }
    }
    if (m_externalGroup->actions().count() > 1)
        ui->menuExternal->addActions(m_externalGroup->actions());
    else {
        delete ui->menuExternal;
        ui->menuExternal = 0;
    }
    if (m_keyerGroup) {
        m_keyerMenu = ui->menuExternal->addMenu(tr("DeckLink Keyer"));
        m_keyerMenu->addActions(m_keyerGroup->actions());
        m_keyerMenu->setDisabled(true);
        connect(m_keyerGroup, SIGNAL(triggered(QAction*)), this, SLOT(onKeyerTriggered(QAction*)));
    }
    connect(m_externalGroup, SIGNAL(triggered(QAction*)), this, SLOT(onExternalTriggered(QAction*)));
    connect(m_profileGroup, SIGNAL(triggered(QAction*)), this, SLOT(onProfileTriggered(QAction*)));

    // Setup the language menu actions
    m_languagesGroup = new QActionGroup(this);
    QAction* a = new QAction(QLocale::languageToString(QLocale::Catalan), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ca");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Chinese), m_languagesGroup);
    a->setCheckable(true);
    a->setData("zh");
    a = new QAction(QLocale::languageToString(QLocale::Czech), m_languagesGroup);
    a->setCheckable(true);
    a->setData("cs");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Danish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("da");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Dutch), m_languagesGroup);
    a->setCheckable(true);
    a->setData("nl");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::English), m_languagesGroup);
    a->setCheckable(true);
    a->setData("en");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Greek), m_languagesGroup);
    a->setCheckable(true);
    a->setData("el");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::French), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fr");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::German), m_languagesGroup);
    a->setCheckable(true);
    a->setData("de");
    a = new QAction(QLocale::languageToString(QLocale::Italian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("it");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Polish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pl");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Brazil)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_BR");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Portugal)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_PT");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Spanish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("es");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    a = new QAction(QLocale::languageToString(QLocale::Russian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ru");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    const QString locale = Settings.language();
    foreach (QAction* action, m_languagesGroup->actions()) {
        if (action->data().toString().startsWith(locale)) {
            action->setChecked(true);
            break;
        }
    }
    connect(m_languagesGroup, SIGNAL(triggered(QAction*)), this, SLOT(onLanguageTriggered(QAction*)));

    // Setup the themes actions
    group = new QActionGroup(this);
    group->addAction(ui->actionSystemTheme);
    group->addAction(ui->actionFusionDark);
    group->addAction(ui->actionFusionLight);
    if (Settings.theme() == "dark")
        ui->actionFusionDark->setChecked(true);
    else if (Settings.theme() == "light")
        ui->actionFusionLight->setChecked(true);
    else
        ui->actionSystemTheme->setChecked(true);

    // Setup the display method actions.
#ifdef Q_OS_WIN
    if (!Settings.playerGPU()) {
        group = new QActionGroup(this);
        ui->actionDrawingAutomatic->setData(0);
        group->addAction(ui->actionDrawingAutomatic);
        ui->actionDrawingDirectX->setData(Qt::AA_UseOpenGLES);
        group->addAction(ui->actionDrawingDirectX);
        ui->actionDrawingOpenGL->setData(Qt::AA_UseDesktopOpenGL);
        group->addAction(ui->actionDrawingOpenGL);
        // Software rendering is not currently working.
        delete ui->actionDrawingSoftware;
        // ui->actionDrawingSoftware->setData(Qt::AA_UseSoftwareOpenGL);
        // group->addAction(ui->actionDrawingSoftware);
        connect(group, SIGNAL(triggered(QAction*)), this, SLOT(onDrawingMethodTriggered(QAction*)));
        switch (Settings.drawMethod()) {
        case Qt::AA_UseDesktopOpenGL:
            ui->actionDrawingOpenGL->setChecked(true);
            break;
        case Qt::AA_UseOpenGLES:
            delete ui->actionGPU;
            ui->actionGPU = 0;
            ui->actionDrawingDirectX->setChecked(true);
            break;
        case Qt::AA_UseSoftwareOpenGL:
            delete ui->actionGPU;
            ui->actionGPU = 0;
            ui->actionDrawingSoftware->setChecked(true);
            break;
        default:
            ui->actionDrawingAutomatic->setChecked(true);
            break;
        }
    } else {
        // GPU mode only works with OpenGL.
        delete ui->menuDrawingMethod;
        ui->menuDrawingMethod = 0;
    }
#else
    delete ui->menuDrawingMethod;
    ui->menuDrawingMethod = 0;
#endif
    qDebug() << "end";
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

    bool ok = false;
    int screen = Settings.playerExternal().toInt(&ok);
    if (ok && screen != QApplication::desktop()->screenNumber(this))
        m_player->moveVideoToScreen(screen);

    // no else here because open() will delete the producer if open fails
    if (!MLT.setProducer(producer))
        emit producerOpened();
    m_player->setFocus();
    m_playlistDock->setUpdateButtonEnabled(false);

    // Needed on Windows. Upon first file open, window is deactivated, perhaps OpenGL-related.
    activateWindow();
}

bool MainWindow::isCompatibleWithGpuMode(MltXmlChecker& checker)
{
    if (checker.needsGPU() && !Settings.playerGPU()) {
        QMessageBox dialog(QMessageBox::Question,
           qApp->applicationName(),
           tr("The file you opened uses GPU effects, but GPU processing is not enabled.\n"
              "Do you want to enable GPU processing and restart?"),
           QMessageBox::No |
           QMessageBox::Yes,
           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            Settings.setPlayerGPU(true);
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        return false;
    }
    return true;
}

bool MainWindow::isXmlRepaired(MltXmlChecker& checker, QString& fileName)
{
    if (checker.isCorrected()) {
        QMessageBox dialog(QMessageBox::Question,
           qApp->applicationName(),
           tr("Shotcut noticed some problems in your project.\n"
              "Do you want Shotcut to try to repair it?\n\n"
              "If you choose Yes, Shotcut will create a copy of your project\n"
              "with \"- Repaired\" in the file name and open it."),
           QMessageBox::No |
           QMessageBox::Yes,
           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            QFileInfo fi(fileName);
            QFile repaired(QString("%1/%2 - %3.%4").arg(fi.path())
                .arg(fi.completeBaseName()).arg(tr("Repaired")).arg(fi.suffix()));
            repaired.open(QIODevice::WriteOnly);
            qDebug() << "repaired MLT XML file name" << repaired.fileName();
            QFile temp(checker.tempFileName());
            if (temp.exists() && repaired.exists()) {
                temp.open(QIODevice::ReadOnly);
                QByteArray xml = temp.readAll();
                temp.close();

                qint64 n = repaired.write(xml);
                while (n > 0 && n < xml.size()) {
                    qint64 x = repaired.write(xml.right(xml.size() - n));
                    if (x > 0)
                        n += x;
                    else
                        n = x;
                }
                repaired.close();
                if (n == xml.size()) {
                    fileName = repaired.fileName();
                    return true;
                }
            }
            QMessageBox::warning(this, qApp->applicationName(), tr("Repairing the project failed."));
        }
    }
    return false;
}

bool MainWindow::checkAutoSave(QString &url)
{
    QMutexLocker locker(&m_autosaveMutex);

    // check whether autosave files exist:
    AutoSaveFile* stale = AutoSaveFile::getFile(url);
    if (stale) {
        QMessageBox dialog(QMessageBox::Question, qApp->applicationName(),
           tr("Auto-saved files exist. Do you want to recover them now?"),
           QMessageBox::No | QMessageBox::Yes, this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            if (!stale->open(QIODevice::ReadWrite)) {
                qWarning() << "failed to recover autosave file" << url;
                delete stale;
            } else {
                delete m_autosaveFile;
                m_autosaveFile = stale;
                url = stale->fileName();
                return true;
            }
        } else {
            // remove the stale file
            delete stale;
        }
    }

    // create new autosave object
    delete m_autosaveFile;
    m_autosaveFile = new AutoSaveFile(url);

    return false;
}

void MainWindow::stepLeftBySeconds(int sec)
{
    m_player->seek(m_player->position() + sec * qRound(MLT.profile().fps()));
}

void MainWindow::doAutosave()
{
    m_autosaveMutex.lock();
    if (m_autosaveFile) {
        if (m_autosaveFile->isOpen() || m_autosaveFile->open(QIODevice::ReadWrite)) {
            saveXML(m_autosaveFile->fileName());
        } else {
            qWarning() << "failed to open autosave file for writing" << m_autosaveFile->fileName();
        }
    }
    m_autosaveMutex.unlock();
}

void MainWindow::setFullScreen(bool isFullScreen)
{
    if (isFullScreen) {
        showFullScreen();
        ui->actionEnter_Full_Screen->setVisible(false);
        ui->actionFullscreen->setVisible(false);
    }
}

QString MainWindow::removeFileScheme(QUrl &url)
{
    QString path = url.url();
    if (url.scheme() == "file")
        path = url.url(QUrl::RemoveScheme);
    if (path.length() > 2 && path.startsWith("///"))
#ifdef Q_OS_WIN
        path.remove(0, 3);
#else
        path.remove(0, 2);
#endif
    return path;
}

static void autosaveTask(MainWindow* p)
{
    qDebug() << "running";
    p->doAutosave();
}

void MainWindow::onAutosaveTimeout()
{
    if (isWindowModified())
        QtConcurrent::run(autosaveTask, this);
}

void MainWindow::updateAutoSave()
{
    if (!m_autosaveTimer.isActive())
        m_autosaveTimer.start();
}

void MainWindow::open(QString url, const Mlt::Properties* properties)
{
    bool modified = false;
    MltXmlChecker checker;
    if (checker.check(url)) {
        if (!isCompatibleWithGpuMode(checker))
            return;
    }
    if (url.endsWith(".mlt") || url.endsWith(".xml")) {
        // only check for a modified project when loading a project, not a simple producer
        if (!continueModified())
            return;
        // close existing project
        if (playlist())
            m_playlistDock->model()->close();
        if (multitrack())
            m_timelineDock->model()->close();
        // let the new project change the profile
        MLT.profile().set_explicit(false);
        if (!isXmlRepaired(checker, url))
            modified = checkAutoSave(url);
        setWindowModified(modified);
    }
    if (!playlist() && !multitrack()) {
        if (!modified && !continueModified())
            return;
        setCurrentFile("");
        setWindowModified(modified);
        MLT.resetURL();
        // Return to automatic video mode if selected.
        if (m_profileGroup->checkedAction()->data().toString().isEmpty())
            MLT.profile().set_explicit(false);
    }
    if (!MLT.open(url)) {
        Mlt::Properties* props = const_cast<Mlt::Properties*>(properties);
        if (props && props->is_valid())
            mlt_properties_inherit(MLT.producer()->get_properties(), props->get_properties());
        m_player->setPauseAfterOpen(!MLT.isClip());
        open(MLT.producer());
        m_recentDock->add(m_autosaveFile? m_autosaveFile->managedFileName() : url);
    }
    else {
        ui->statusBar->showMessage(tr("Failed to open ") + url, STATUS_TIMEOUT_MS);
        emit openFailed(url);
    }
}

void MainWindow::openVideo()
{
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open File"), path);

    if (filenames.length() > 0) {
        Settings.setOpenPath(QFileInfo(filenames.first()).path());
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

void MainWindow::openCut(void* producer)
{
    m_player->setPauseAfterOpen(true);
    Mlt::Producer* p = (Mlt::Producer*) producer;
    open(p);
    MLT.seek(p->get_in());
}

void MainWindow::showStatusMessage(QString message)
{
    ui->statusBar->showMessage(message, STATUS_TIMEOUT_MS);
}

void MainWindow::seekPlaylist(int start)
{
    if (!playlist()) return;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if (!MLT.producer() || (void*) MLT.producer()->get_producer() != (void*) playlist()->get_playlist())
        MLT.setProducer(new Mlt::Producer(*playlist()));
    m_player->setIn(-1);
    m_player->setOut(-1);
    // since we do not emit producerOpened, these components need updating
    on_actionJack_triggered(ui->actionJack->isChecked());
    m_player->onProducerOpened(false);
    m_encodeDock->onProducerOpened();
    m_filterController->setProducer();
    updateMarkers();
    MLT.seek(start);
    m_player->setFocus();
    m_player->switchToTab(Player::ProgramTabIndex);
}

void MainWindow::seekTimeline(int position)
{
    if (!multitrack()) return;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if (MLT.producer() && (void*) MLT.producer()->get_producer() != (void*) multitrack()->get_producer()) {
        MLT.setProducer(new Mlt::Producer(*multitrack()));
        m_player->setIn(-1);
        m_player->setOut(-1);
        // since we do not emit producerOpened, these components need updating
        on_actionJack_triggered(ui->actionJack->isChecked());
        m_player->onProducerOpened(false);
        m_encodeDock->onProducerOpened();
        m_filterController->setProducer();
        updateMarkers();
        m_player->setFocus();
        m_player->switchToTab(Player::ProgramTabIndex);
    }
    m_player->seek(position);
}

void MainWindow::readPlayerSettings()
{
    qDebug() << "begin";
    ui->actionRealtime->setChecked(Settings.playerRealtime());
    ui->actionProgressive->setChecked(Settings.playerProgressive());
    ui->actionScrubAudio->setChecked(Settings.playerScrubAudio());
    ui->actionJack->setChecked(Settings.playerJACK());
    if (ui->actionGPU) {
        ui->actionGPU->setChecked(Settings.playerGPU());
        MLT.videoWidget()->setProperty("gpu", ui->actionGPU->isChecked());
    }
    QString deinterlacer = Settings.playerDeinterlacer();
    QString interpolation = Settings.playerInterpolation();

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

    QString external = Settings.playerExternal();
    bool ok = false;
    external.toInt(&ok);
    foreach (QAction* a, m_externalGroup->actions()) {
        if (a->data() == external) {
            a->setChecked(true);
            if (a->data().toString().startsWith("decklink") && m_keyerMenu)
                m_keyerMenu->setEnabled(true);
            break;
        }
    }

    if (m_keyerGroup) {
        int keyer = Settings.playerKeyerMode();
        foreach (QAction* a, m_keyerGroup->actions()) {
            if (a->data() == keyer) {
                a->setChecked(true);
                break;
            }
        }
    }

    QString profile = Settings.playerProfile();
    // Automatic not permitted for SDI/HDMI
    if (!external.isEmpty() && !ok && profile.isEmpty())
        profile = "atsc_720p_50";
    foreach (QAction* a, m_profileGroup->actions()) {
        // Automatic not permitted for SDI/HDMI
        if (a->data().toString().isEmpty() && !external.isEmpty() && !ok)
            a->setDisabled(true);
        if (a->data().toString() == profile) {
            a->setChecked(true);
            break;
        }
    }

    QString gamma = Settings.playerGamma();
    if (gamma == "bt709")
        ui->actionGammaRec709->setChecked(true);
    else
        ui->actionGammaSRGB->setChecked(true);

    qDebug() << "end";
}

void MainWindow::readWindowSettings()
{
    qDebug() << "begin";
    Settings.setWindowGeometryDefault(saveGeometry());
    Settings.setWindowStateDefault(saveState());
    Settings.sync();
    restoreGeometry(Settings.windowGeometry());
    restoreState(Settings.windowState());
    qDebug() << "end";
}

void MainWindow::writeSettings()
{
#ifndef Q_OS_MAC
    if (isFullScreen())
        showNormal();
#endif
    Settings.setWindowGeometry(saveGeometry());
    Settings.setWindowState(saveState());
    Settings.sync();
}

void MainWindow::configureVideoWidget()
{
    qDebug() << "begin";
    MLT.setProfile(m_profileGroup->checkedAction()->data().toString());
    MLT.videoWidget()->setProperty("realtime", ui->actionRealtime->isChecked());
    bool ok = false;
    m_externalGroup->checkedAction()->data().toInt(&ok);
    if (!ui->menuExternal || m_externalGroup->checkedAction()->data().toString().isEmpty() || ok) {
        MLT.videoWidget()->setProperty("progressive", ui->actionProgressive->isChecked());
    } else {
        MLT.videoWidget()->setProperty("mlt_service", m_externalGroup->checkedAction()->data());
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
    if (m_keyerGroup)
        MLT.videoWidget()->setProperty("keyer", m_keyerGroup->checkedAction()->data());
    qDebug() << "end";
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
                "<small><p>Copyright &copy; 2011-2015 <a href=\"http://www.meltytech.com/\">Meltytech</a>, LLC</p>"
                "<p>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program proudly uses the following projects:<ul>"
                "<li><a href=\"http://www.qt-project.org/\">Qt</a> application and UI framework</li>"
                "<li><a href=\"http://www.mltframework.org/\">MLT</a> multimedia authoring framework</li>"
                "<li><a href=\"http://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries</li>"
                "<li><a href=\"http://www.videolan.org/developers/x264.html\">x264</a> H.264 encoder</li>"
                "<li><a href=\"http://www.webmproject.org/\">WebM</a> VP8 and VP9 encoders</li>"
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
    bool handled = true;


    switch (event->key()) {
    case Qt::Key_Home:
        m_player->seek(0);
        break;
    case Qt::Key_End:
        if (MLT.producer())
            m_player->seek(MLT.producer()->get_length() - 1);
        break;
    case Qt::Key_Left:
        if ((event->modifiers() & Qt::ControlModifier) && m_timelineDock->isVisible()) {
            if (m_timelineDock->selection().isEmpty()) {
                m_timelineDock->selectClipUnderPlayhead();
            } else if (m_timelineDock->selection().size() == 1) {
                int newIndex = m_timelineDock->selection().first() - 1;
                if (newIndex < 0)
                    break;
                m_timelineDock->setSelection(QList<int>() << newIndex);
                m_navigationPosition = m_timelineDock->centerOfClip(m_timelineDock->currentTrack(), newIndex);
            }
        } else {
            stepLeftOneFrame();
        }
        break;
    case Qt::Key_Right:
        if ((event->modifiers() & Qt::ControlModifier) && m_timelineDock->isVisible()) {
            if (m_timelineDock->selection().isEmpty()) {
                m_timelineDock->selectClipUnderPlayhead();
            } else if (m_timelineDock->selection().size() == 1) {
                int newIndex = m_timelineDock->selection().first() + 1;
                if (newIndex >= m_timelineDock->clipCount(-1))
                    break;
                m_timelineDock->setSelection(QList<int>() << newIndex);
                m_navigationPosition = m_timelineDock->centerOfClip(m_timelineDock->currentTrack(), newIndex);
            }
        } else {
            stepRightOneFrame();
        }
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        {
            int directionMultiplier = event->key() == Qt::Key_PageUp ? -1 : 1;
            int seconds = 1;
            if (event->modifiers() & Qt::ControlModifier)
                seconds *= 5;
            if (event->modifiers() & Qt::ShiftModifier)
                seconds *= 2;
            stepLeftBySeconds(seconds * directionMultiplier);
        }
        break;
    case Qt::Key_Space:
#ifdef Q_OS_MAC
        // Spotlight defaults to Cmd+Space, so also accept Ctrl+Space.
        if ((event->modifiers() == Qt::MetaModifier || (event->modifiers() & Qt::ControlModifier)) && m_timelineDock->isVisible())
#else
        if (event->modifiers() == Qt::ControlModifier && m_timelineDock->isVisible())
#endif
            m_timelineDock->selectClipUnderPlayhead();
        else
            handled = false;
        break;
    case Qt::Key_C:
        if (event->modifiers() == Qt::ShiftModifier) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_actionAppendCut_triggered();
        } else {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->append(-1);
        }
        break;
    case Qt::Key_D:
        if (event->modifiers() & Qt::ControlModifier)
            m_timelineDock->setSelection(QList<int>());
        else
            handled = false;
        break;
    case Qt::Key_H:
#ifdef Q_OS_MAC
        // OS X uses Cmd+H to hide an app.
        if (event->modifiers() & Qt::MetaModifier && multitrack())
#else
        if (event->modifiers() & Qt::ControlModifier && multitrack())
#endif
            m_timelineDock->toggleTrackHidden(m_timelineDock->currentTrack());
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
#ifdef Q_OS_MAC
        // OS X uses Cmd+H to hide an app and Cmd+M to minimize. Therefore, we force
        // it to be the apple keyboard control key aka meta. Therefore, to be
        // consistent with all track header toggles, we make the lock toggle also use
        // meta.
        if (event->modifiers() & Qt::MetaModifier && multitrack())
#else
        if (event->modifiers() & Qt::ControlModifier && multitrack())
#endif
            m_timelineDock->setTrackLock(m_timelineDock->currentTrack(), !m_timelineDock->isTrackLocked(m_timelineDock->currentTrack()));
        else if (m_isKKeyPressed)
            m_player->seek(m_player->position() + 1);
        else
            m_player->fastForward();
        break;
    case Qt::Key_M:
#ifdef Q_OS_MAC
        // OS X uses Cmd+M to minimize an app.
        if (event->modifiers() & Qt::MetaModifier && multitrack())
#else
        if (event->modifiers() & Qt::ControlModifier && multitrack())
#endif
            m_timelineDock->toggleTrackMute(m_timelineDock->currentTrack());
        break;
    case Qt::Key_I:
        setInToCurrent();
        break;
    case Qt::Key_O:
        setOutToCurrent();
        break;
    case Qt::Key_S:
        if (multitrack())
            m_timelineDock->splitClip();
        break;
    case Qt::Key_U:
        if (event->modifiers() == Qt::ControlModifier) {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->addAudioTrack();
        }
        break;
    case Qt::Key_V: // Avid Splice In
        if (event->modifiers() == Qt::ShiftModifier) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_actionInsertCut_triggered();
        } else {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->insert(-1);
        }
        break;
    case Qt::Key_B: // Avid Overwrite
        if (event->modifiers() == Qt::ShiftModifier) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_actionUpdate_triggered();
        } else {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->overwrite(-1);
        }
        break;
    case Qt::Key_Escape: // Avid Toggle Active Monitor
        if (MLT.isPlaylist()) {
            if (multitrack())
                m_player->onTabBarClicked(Player::ProgramTabIndex);
            else if (MLT.savedProducer())
                m_player->onTabBarClicked(Player::SourceTabIndex);
            else
                m_playlistDock->on_actionOpen_triggered();
        } else if (MLT.isMultitrack()) {
            if (MLT.savedProducer())
                m_player->onTabBarClicked(Player::SourceTabIndex);
            // TODO else open clip under playhead of current track if available
        } else {
            if (multitrack() || (playlist() && playlist()->count() > 0))
                m_player->onTabBarClicked(Player::ProgramTabIndex);
        }
        break;
    case Qt::Key_Up:
        if (m_playlistDock->isVisible() && event->modifiers() & Qt::AltModifier) {
            m_playlistDock->raise();
            m_playlistDock->decrementIndex();
            m_playlistDock->on_actionOpen_triggered();
        } else if (multitrack()) {
            int newClipIndex = -1;
            if ((event->modifiers() & Qt::ControlModifier) &&
                    !m_timelineDock->selection().isEmpty() &&
                    m_timelineDock->currentTrack() > 0) {

                newClipIndex = m_timelineDock->clipIndexAtPosition(m_timelineDock->currentTrack() - 1, m_navigationPosition);
            }

            m_timelineDock->selectTrack(-1);

            if (newClipIndex >= 0) {
                newClipIndex = qMin(newClipIndex, m_timelineDock->clipCount(m_timelineDock->currentTrack()) - 1);
                m_timelineDock->setSelection(QList<int>() << newClipIndex);
            }

        } else if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            if (event->modifiers() & Qt::ControlModifier)
                m_playlistDock->moveClipUp();
            m_playlistDock->decrementIndex();
        }
        break;
    case Qt::Key_Down:
        if (m_playlistDock->isVisible() && event->modifiers() & Qt::AltModifier) {
            m_playlistDock->raise();
            m_playlistDock->incrementIndex();
            m_playlistDock->on_actionOpen_triggered();
        } else if (multitrack()) {
            int newClipIndex = -1;
            if ((event->modifiers() & Qt::ControlModifier) &&
                    !m_timelineDock->selection().isEmpty() &&
                    m_timelineDock->currentTrack() < m_timelineDock->model()->trackList().count() - 1) {

                newClipIndex = m_timelineDock->clipIndexAtPosition(m_timelineDock->currentTrack() + 1, m_navigationPosition);
            }

            m_timelineDock->selectTrack(1);

            if (newClipIndex >= 0) {
                newClipIndex = qMin(newClipIndex, m_timelineDock->clipCount(m_timelineDock->currentTrack()) - 1);
                m_timelineDock->setSelection(QList<int>() << newClipIndex);
            }

        } else if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            if (event->modifiers() & Qt::ControlModifier)
                m_playlistDock->moveClipDown();
            m_playlistDock->incrementIndex();
        }
        break;
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(event->key() - Qt::Key_1);
        }
        break;
    case Qt::Key_0:
        if (m_playlistDock->isVisible()) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(9);
        }
        else if (m_timelineDock->isVisible()) {
            m_timelineDock->resetZoom();
        }
        break;
    case Qt::Key_W:
        if (event->modifiers() & Qt::ControlModifier) {
            if (multitrack())
                m_timelineDock->close();
            else if (playlist())
                m_playlistDock->on_actionClose_triggered();
            else
                onMultitrackClosed();
        }
        break;
    case Qt::Key_X: // Avid Extract
        if (event->modifiers() == Qt::ShiftModifier) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_removeButton_clicked();
        } else {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->removeSelection();
        }
        break;
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        if (multitrack()) {
            m_timelineDock->show();
            m_timelineDock->raise();
            if (event->modifiers() == Qt::ShiftModifier)
                m_timelineDock->removeSelection();
            else
                m_timelineDock->liftSelection();
        } else {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_removeButton_clicked();
        }
        break;
    case Qt::Key_Y:
        if (event->modifiers() == Qt::ControlModifier) {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->addVideoTrack();
        }
        break;
    case Qt::Key_Z: // Avid Lift
        if (event->modifiers() == Qt::ShiftModifier) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_removeButton_clicked();
        } else if (multitrack() && event->modifiers() == Qt::NoModifier) {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->liftSelection();
        }
        break;
    case Qt::Key_Minus:
        if (m_timelineDock->isVisible()) {
            if (event->modifiers() & Qt::ControlModifier)
                m_timelineDock->makeTracksShorter();
            else
                m_timelineDock->zoomOut();
        }
        break;
    case Qt::Key_Equal:
    case Qt::Key_Plus:
        if (m_timelineDock->isVisible()) {
            if (event->modifiers() & Qt::ControlModifier)
                m_timelineDock->makeTracksTaller();
            else
                m_timelineDock->zoomIn();
        }
        break;
    case Qt::Key_Enter: // Seek to current playlist item
    case Qt::Key_Return:
        if (!m_timelineDock->selection().isEmpty()) {
            m_timelineDock->openClip(m_timelineDock->currentTrack(), m_timelineDock->selection().first());
        }
        else if (m_playlistDock->position() >= 0) {
            if (event->modifiers() == Qt::ShiftModifier)
                seekPlaylist(m_playlistDock->position());
            else
                m_playlistDock->on_actionOpen_triggered();
        }
        break;
    case Qt::Key_F5:
        m_timelineDock->model()->reload();
        break;
    case Qt::Key_F1:
        qDebug() << "Current focusWidget:" << QApplication::focusWidget();
        break;
    default:
        break;
    }

    if (!handled)
        QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_K)
        m_isKKeyPressed = false;
    else
        QMainWindow::keyReleaseEvent(event);
}

// Drag-n-drop events

bool MainWindow::eventFilter(QObject* target, QEvent* event)
{
    if (event->type() == QEvent::DragEnter) {
        dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return true;
    } else if (event->type() == QEvent::Drop) {
        dropEvent(static_cast<QDropEvent*>(event));
        return true;
    }
    return QMainWindow::eventFilter(target, event);
}

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
            foreach (QUrl url, mimeData->urls()) {
                QString path = removeFileScheme(url);
                m_multipleFiles.append(path);
            }
        }
        QString path = removeFileScheme(mimeData->urls().first());
        open(path);
        event->acceptProposedAction();
    }
    else if (mimeData->hasFormat(Mlt::XmlMimeType )) {
        m_playlistDock->on_actionOpen_triggered();
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (continueJobsRunning() && continueModified()) {
        if (!m_htmlEditor || m_htmlEditor->close()) {
            writeSettings();
            event->accept();
            QApplication::exit(m_exitCode);
            return;
        }
    }
    event->ignore();
}

void MainWindow::showEvent(QShowEvent* event)
{
    // This is needed to prevent a crash on windows on startup when timeline
    // is visible and dock title bars are hidden.
    Q_UNUSED(event)
    ui->actionShowTitleBars->setChecked(Settings.showTitleBars());
    on_actionShowTitleBars_triggered(Settings.showTitleBars());
    ui->actionShowToolbar->setChecked(Settings.showToolBar());
    on_actionShowToolbar_triggered(Settings.showToolBar());
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
    m_meltedServerDock->disconnect(SIGNAL(positionUpdated(int,double,int,int,int,bool)));

    QWidget* w = loadProducerWidget(MLT.producer());
    if (w) {
        if (-1 != w->metaObject()->indexOfSignal("producerReopened()"))
            connect(w, SIGNAL(producerChanged()), m_player, SLOT(onProducerOpened()));
    }
    else if (MLT.isPlaylist()) {
        m_playlistDock->model()->load();
        if (playlist()) {
            m_isPlaylistLoaded = true;
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_playlistDock->setVisible(true);
            m_playlistDock->raise();
            m_player->enableTab(Player::ProgramTabIndex);
            m_player->switchToTab(Player::ProgramTabIndex);
        }
    }
    else if (MLT.isMultitrack()) {
        m_timelineDock->model()->load();
        if (multitrack()) {
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_timelineDock->setVisible(true);
            m_timelineDock->raise();
            m_player->enableTab(Player::ProgramTabIndex);
            m_player->switchToTab(Player::ProgramTabIndex);
        }
    }
    if (MLT.isClip())
        m_player->switchToTab(Player::SourceTabIndex);
    if (m_autosaveFile)
        setCurrentFile(m_autosaveFile->managedFileName());
    else if (!MLT.URL().isEmpty())
        setCurrentFile(MLT.URL());
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
    QString path = Settings.savePath();
    path.append("/.mlt");
    QString filename = QFileDialog::getSaveFileName(this, tr("Save XML"), path, tr("MLT XML (*.mlt)"));
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        Settings.setSavePath(fi.path());
        if (fi.suffix() != "mlt")
            filename += ".mlt";
        saveXML(filename);
        if (m_autosaveFile)
            m_autosaveFile->changeManagedFile(filename);
        else
            m_autosaveFile = new AutoSaveFile(filename);
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
        dialog.setWindowModality(QmlApplication::dialogModality());
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

bool MainWindow::continueJobsRunning()
{
    if (JOBS.hasIncomplete()) {
        QMessageBox dialog(QMessageBox::Warning,
                                     qApp->applicationName(),
                                     tr("There are incomplete jobs.\n"
                                        "Do you want to still want to exit?"),
                                     QMessageBox::No |
                                     QMessageBox::Yes,
                                     this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        return (dialog.exec() == QMessageBox::Yes);
    }
    return true;
}

QUndoStack* MainWindow::undoStack() const
{
    return m_undoStack;
}

void MainWindow::onEncodeTriggered(bool checked)
{
    if (checked) {
        m_encodeDock->show();
        m_encodeDock->raise();
    }
}

void MainWindow::onCaptureStateChanged(bool started)
{
    if (started && MLT.resource().startsWith("x11grab:")
                && !MLT.producer()->get_int(kBackgroundCaptureProperty))
        showMinimized();
}

void MainWindow::onJobsDockTriggered(bool checked)
{
    if (checked) {
        m_jobsDock->show();
        m_jobsDock->raise();
    }
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

void MainWindow::onTimelineDockTriggered(bool checked)
{
    if (checked) {
        m_timelineDock->show();
        m_timelineDock->raise();
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
    if (!playlist() || playlist()->count() == 0) return;
    m_player->enableTab(Player::ProgramTabIndex, true);
}

void MainWindow::onPlaylistLoaded()
{
    updateMarkers();
    m_player->enableTab(Player::ProgramTabIndex, true);
}

void MainWindow::onPlaylistCleared()
{
    m_player->setPauseAfterOpen(true);
    open(new Mlt::Producer(MLT.profile(), "color:"));
    m_player->seek(0);
    setWindowModified(true);
}

void MainWindow::onPlaylistClosed()
{
    m_player->resetProfile();
    setCurrentFile("");
    setWindowModified(false);
    m_undoStack->clear();
    MLT.resetURL();
    if (!multitrack())
        m_player->enableTab(Player::ProgramTabIndex, false);
}

void MainWindow::onPlaylistModified()
{
    setWindowModified(true);
    if ((void*) MLT.producer()->get_producer() == (void*) playlist()->get_playlist())
        m_player->onProducerModified();
    updateMarkers();
    m_player->enableTab(Player::ProgramTabIndex, true);
}

void MainWindow::onMultitrackCreated()
{
    m_player->enableTab(Player::ProgramTabIndex, true);
}

void MainWindow::onMultitrackClosed()
{
    m_player->resetProfile();
    onPlaylistCleared();
    setCurrentFile("");
    setWindowModified(false);
    m_undoStack->clear();
    MLT.resetURL();
    if (!playlist() || playlist()->count() == 0)
        m_player->enableTab(Player::ProgramTabIndex, false);
}

void MainWindow::onMultitrackModified()
{
    setWindowModified(true);
    if (MLT.producer() && (void*) MLT.producer()->get_producer() == (void*) multitrack()->get_producer())
        m_player->onProducerModified();
    updateMarkers();
}

void MainWindow::onCutModified()
{
    if (!playlist() && !multitrack()) {
        setWindowModified(true);
        updateAutoSave();
    }
    if (playlist())
        m_playlistDock->setUpdateButtonEnabled(true);
}

void MainWindow::onFilterModelChanged()
{
    setWindowModified(true);
    updateAutoSave();
    if (playlist())
        m_playlistDock->setUpdateButtonEnabled(true);
}

void MainWindow::updateMarkers()
{
    if (playlist() && MLT.isPlaylist()) {
        QList<int> markers;
        int n = playlist()->count();
        for (int i = 0; i < n; i++)
            markers.append(playlist()->clip_start(i));
        m_player->setMarkers(markers);
    }
}

void MainWindow::updateThumbnails()
{
    if (Settings.playlistThumbnails() != "hidden")
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
    if (multitrack()) {
        MLT.saveXML(filename, multitrack());
    } else if (playlist()) {
        int in = MLT.producer()->get_in();
        int out = MLT.producer()->get_out();
        MLT.producer()->set_in_and_out(0, MLT.producer()->get_length() - 1);
        MLT.saveXML(filename, playlist());
        MLT.producer()->set_in_and_out(in, out);
    } else {
        MLT.saveXML(filename);
    }
}

void MainWindow::changeTheme(const QString &theme)
{
    qDebug() << "begin";
    if (theme == "dark") {
        QApplication::setStyle("Fusion");
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(50,50,50));
        palette.setColor(QPalette::WindowText, QColor(220,220,220));
        palette.setColor(QPalette::Base, QColor(30,30,30));
        palette.setColor(QPalette::AlternateBase, QColor(40,40,40));
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
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        QIcon::setThemeName("");
#else
        QIcon::setThemeName("oxygen");
#endif
    }
    emit QmlApplication::singleton().paletteChanged();
    qDebug() << "end";
}

Mlt::Playlist* MainWindow::playlist() const
{
    return m_playlistDock->model()->playlist();
}

Mlt::Producer *MainWindow::multitrack() const
{
    return m_timelineDock->model()->tractor();
}

QWidget *MainWindow::loadProducerWidget(Mlt::Producer* producer)
{
    QWidget* w = 0;

    if (!producer || !producer->is_valid())
        return  w;

    QString service(producer->get("mlt_service"));
    QString resource = QString::fromUtf8(producer->get("resource"));
    QScrollArea* scrollArea = (QScrollArea*) m_propertiesDock->widget();

    delete scrollArea->widget();
    if (resource.startsWith("video4linux2:"))
        w = new Video4LinuxWidget(this);
    else if (resource.startsWith("pulse:"))
        w = new PulseAudioWidget(this);
    else if (resource.startsWith("jack:"))
        w = new JackProducerWidget(this);
    else if (resource.startsWith("alsa:"))
        w = new AlsaWidget(this);
    else if (resource.startsWith("dshow:"))
        w = new DirectShowVideoWidget(this);
    else if (resource.startsWith("x11grab:"))
        w = new X11grabWidget(this);
    else if (service.startsWith("avformat"))
        w = new AvformatProducerWidget(this);
    else if (MLT.isImageProducer(producer)) {
        ImageProducerWidget* ipw = new ImageProducerWidget(this);
        connect(m_player, SIGNAL(outChanged(int)), ipw, SLOT(setOutPoint(int)));
        w = ipw;
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
    else if (service == "webvfx")
        w = new WebvfxProducer(this);
    else if (service == "tone")
        w = new ToneProducerWidget(this);
    else if (producer->parent().get(kShotcutTransitionProperty)) {
        w = new LumaMixTransition(producer->parent(), this);
        scrollArea->setWidget(w);
        return w;
    }
    if (w) {
        dynamic_cast<AbstractProducerWidget*>(w)->setProducer(producer);
        if (-1 != w->metaObject()->indexOfSignal("producerChanged()"))
            connect(w, SIGNAL(producerChanged()), SLOT(onProducerChanged()));
        scrollArea->setWidget(w);
        onProducerChanged();
    }
    return w;
}

void MainWindow::onMeltedUnitOpened()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "color:");
    MLT.setProducer(producer);
    MLT.play(0);
    QScrollArea* scrollArea = (QScrollArea*) m_propertiesDock->widget();
    delete scrollArea->widget();
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
    Settings.setPlayerGPU(false);
    if (ui->actionGPU) {
        ui->actionGPU->setChecked(false);
        ui->actionGPU->setDisabled(true);
    }
    QMessageBox::critical(this, qApp->applicationName(),
        tr("GPU Processing is not supported"));
}

void MainWindow::editHTML(const QString &fileName)
{
    bool isNew = !m_htmlEditor;
    if (!m_htmlEditor) {
        m_htmlEditor = new HtmlEditor;
        m_htmlEditor->setWindowIcon(windowIcon());
    }
    m_htmlEditor->load(fileName);
    m_htmlEditor->show();
    m_htmlEditor->raise();
    if (Settings.playerZoom() >= 1.0f) {
        m_htmlEditor->changeZoom(100 * m_player->videoSize().width() / MLT.profile().width());
        m_htmlEditor->resizeWebView(m_player->videoSize().width(), m_player->videoSize().height());
    } else {
        m_htmlEditor->changeZoom(100 * MLT.displayWidth() / MLT.profile().width());
        m_htmlEditor->resizeWebView(MLT.displayWidth(), MLT.displayHeight());
    }
    if (isNew) {
        // Center the new window over the main window.
        QPoint point = pos();
        QPoint halfSize(width(), height());
        halfSize /= 2;
        point += halfSize;
        halfSize = QPoint(m_htmlEditor->width(), m_htmlEditor->height());
        halfSize /= 2;
        point -= halfSize;
        m_htmlEditor->move(point);
    }
}

void MainWindow::stepLeftOneFrame()
{
    m_player->seek(m_player->position() - 1);
}

void MainWindow::stepRightOneFrame()
{
    m_player->seek(m_player->position() + 1);
}

void MainWindow::stepLeftOneSecond()
{
    stepLeftBySeconds(-1);
}

void MainWindow::stepRightOneSecond()
{
    stepLeftBySeconds(1);
}

void MainWindow::setInToCurrent()
{
    if (MLT.isSeekable() && MLT.isClip())
        m_player->setIn(m_player->position());
}

void MainWindow::setOutToCurrent()
{
    if (MLT.isSeekable() && MLT.isClip())
        m_player->setOut(m_player->position());
}

void MainWindow::onShuttle(float x)
{
    if (x == 0) {
        m_player->pause();
    } else if (x > 0) {
        m_player->play(10.0 * x);
    } else {
        m_player->play(20.0 * x);
    }
}

void MainWindow::on_actionRealtime_triggered(bool checked)
{
    Settings.setPlayerRealtime(checked);
    if (Settings.playerGPU())
        MLT.pause();
    if (MLT.consumer()) {
        MLT.restart();
    }

}

void MainWindow::on_actionProgressive_triggered(bool checked)
{
    MLT.videoWidget()->setProperty("progressive", checked);
    if (Settings.playerGPU())
        MLT.pause();
    if (MLT.consumer()) {
        MLT.profile().set_progressive(checked);
        MLT.restart();
    }
    Settings.setPlayerProgressive(checked);
}

void MainWindow::changeDeinterlacer(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("deinterlace_method", method);
        if (MLT.consumer()) {
            MLT.consumer()->set("deinterlace_method", method);
        }
    }
    Settings.setPlayerDeinterlacer(method);
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
        }
    }
    Settings.setPlayerInterpolation(method);
}

class AppendTask : public QRunnable
{
public:
    AppendTask(PlaylistModel* model, const QStringList& filenames)
        : QRunnable()
        , model(model)
        , filenames(filenames)
    {
    }
    void run()
    {
        foreach (QString filename, filenames) {
            Mlt::Producer p(MLT.profile(), filename.toUtf8().constData());
            if (p.is_valid()) {
                // Convert avformat to avformat-novalidate so that XML loads faster.
                if (!qstrcmp(p.get("mlt_service"), "avformat")) {
                    p.set("mlt_service", "avformat-novalidate");
                    p.set("mute_on_pause", 0);
                }
                MLT.setImageDurationFromDefault(&p);
                MAIN.undoStack()->push(new Playlist::AppendCommand(*model, MLT.XML(&p)));
            }
        }
    }
private:
    PlaylistModel* model;
    const QStringList& filenames;
};

void MainWindow::processMultipleFiles()
{
    if (m_multipleFiles.length() > 0) {
        PlaylistModel* model = m_playlistDock->model();
        m_playlistDock->show();
        m_playlistDock->raise();
        QThreadPool::globalInstance()->start(new AppendTask(model, m_multipleFiles));
        foreach (QString filename, m_multipleFiles)
            m_recentDock->add(filename.toUtf8().constData());
        m_multipleFiles.clear();
    }
    if (m_isPlaylistLoaded && Settings.playerGPU()) {
        updateThumbnails();
        m_isPlaylistLoaded = false;
    }
}

void MainWindow::onLanguageTriggered(QAction* action)
{
    Settings.setLanguage(action->data().toString());
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to switch to the new language.\n"
                          "Do you want to restart now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        m_exitCode = EXIT_RESTART;
        QApplication::closeAllWindows();
    }
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
    Settings.setPlayerJACK(checked);
    if (!MLT.enableJack(checked)) {
        ui->actionJack->setChecked(false);
        Settings.setPlayerJACK(false);
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to connect to JACK.\nPlease verify that JACK is installed and running."));
    }
}

void MainWindow::on_actionGPU_triggered(bool checked)
{
    Settings.setPlayerGPU(checked);
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to switch using GPU processing.\n"
                          "Do you want to restart now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        m_exitCode = EXIT_RESTART;
        QApplication::closeAllWindows();
    }
}

void MainWindow::onExternalTriggered(QAction *action)
{
    bool isExternal = !action->data().toString().isEmpty();
    Settings.setPlayerExternal(action->data().toString());

    bool ok = false;
    int screen = action->data().toInt(&ok);
    if (ok || action->data().toString().isEmpty()) {
        m_player->moveVideoToScreen(ok? screen : -2);
        isExternal = false;
        MLT.videoWidget()->setProperty("mlt_service", QVariant());
    } else {
        MLT.videoWidget()->setProperty("mlt_service", action->data());
    }

    QString profile = Settings.playerProfile();
    // Automatic not permitted for SDI/HDMI
    if (isExternal && profile.isEmpty()) {
        profile = "atsc_720p_50";
        Settings.setPlayerProfile(profile);
        MLT.setProfile(profile);
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
    if (m_keyerMenu)
        m_keyerMenu->setEnabled(action->data().toString().startsWith("decklink"));
}

void MainWindow::onKeyerTriggered(QAction *action)
{
    MLT.videoWidget()->setProperty("keyer", action->data());
    MLT.consumerChanged();
    Settings.setPlayerKeyerMode(action->data().toInt());
}

void MainWindow::onProfileTriggered(QAction *action)
{
    Settings.setPlayerProfile(action->data().toString());
    MLT.setProfile(action->data().toString());
    emit profileChanged();
}

void MainWindow::on_actionAddCustomProfile_triggered()
{
    CustomProfileDialog dialog(this);
    dialog.setWindowModality(QmlApplication::dialogModality());
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
    Settings.setTheme("system");
}

void MainWindow::on_actionFusionDark_triggered()
{
    changeTheme("dark");
    Settings.setTheme("dark");
}

void MainWindow::on_actionFusionLight_triggered()
{
    changeTheme("light");
    Settings.setTheme("light");
}

void MainWindow::on_actionTutorials_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.shotcut.org/bin/view/Shotcut/Tutorials"));
}

void MainWindow::on_actionRestoreLayout_triggered()
{
    restoreGeometry(Settings.windowGeometryDefault());
    restoreState(Settings.windowStateDefault());
}

void MainWindow::on_actionShowTitleBars_triggered(bool checked)
{
    QList <QDockWidget *> docks = findChildren<QDockWidget *>();
    for (int i = 0; i < docks.count(); i++) {
        QDockWidget* dock = docks.at(i);
        if (checked) {
            dock->setTitleBarWidget(0);
        } else {
            if (!dock->isFloating()) {
                dock->setTitleBarWidget(new QWidget);
            }
        }
    }
    Settings.setShowTitleBars(checked);
}

void MainWindow::on_actionShowToolbar_triggered(bool checked)
{
    ui->mainToolBar->setVisible(checked);
}

void MainWindow::onToolbarVisibilityChanged(bool visible)
{
    ui->actionShowToolbar->setChecked(visible);
    Settings.setShowToolBar(visible);
}

void MainWindow::on_menuExternal_aboutToShow()
{
    foreach (QAction* action, m_externalGroup->actions()) {
        bool ok = false;
        int i = action->data().toInt(&ok);
        if (ok) {
            if (i == QApplication::desktop()->screenNumber(this)) {
                if (action->isChecked()) {
                    m_externalGroup->actions().first()->setChecked(true);
                    Settings.setPlayerExternal(QString());
                }
                action->setDisabled(true);
            }  else {
                action->setEnabled(true);
            }
        }
    }
}

void MainWindow::on_actionUpgrade_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.shotcut.org/bin/view/Shotcut/Download"));
}

void MainWindow::on_actionOpenXML_triggered()
{
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open File"), path,
        tr("MLT XML (*.mlt);;All Files (*)"));
    if (filenames.length() > 0) {
        QString url = filenames.first();
        MltXmlChecker checker;
        if (checker.check(url)) {
            if (!isCompatibleWithGpuMode(checker))
                return;
            isXmlRepaired(checker, url);
        }
        Settings.setOpenPath(QFileInfo(url).path());
        activateWindow();
        if (filenames.length() > 1)
            m_multipleFiles = filenames;
        if (!MLT.openXML(url)) {
            open(MLT.producer());
            m_recentDock->add(url);
        }
        else {
            ui->statusBar->showMessage(tr("Failed to open ") + url, STATUS_TIMEOUT_MS);
            emit openFailed(url);
        }
    }
}

void MainWindow::on_actionGammaSRGB_triggered(bool checked)
{
    Q_UNUSED(checked)
    Settings.setPlayerGamma("iec61966_2_1");
    MLT.restart();
    MLT.refreshConsumer();
}

void MainWindow::on_actionGammaRec709_triggered(bool checked)
{
    Q_UNUSED(checked)
    Settings.setPlayerGamma("bt709");
    MLT.restart();
    MLT.refreshConsumer();
}

void MainWindow::onFocusChanged(QWidget *, QWidget * now) const
{
    qDebug() << "Focuswidget changed to" << now;
}

void MainWindow::on_actionScrubAudio_triggered(bool checked)
{
    Settings.setPlayerScrubAudio(checked);
}

#ifdef Q_OS_WIN
void MainWindow::onDrawingMethodTriggered(QAction *action)
{
    Settings.setDrawMethod(action->data().toInt());
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to change the display method.\n"
                          "Do you want to restart now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        m_exitCode = EXIT_RESTART;
        QApplication::closeAllWindows();
    }
}
#endif
