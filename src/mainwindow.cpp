/*
 * Copyright (c) 2011-2018 Meltytech, LLC
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
#include "widgets/countproducerwidget.h"
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
#include "widgets/avfoundationproducerwidget.h"
#include "dialogs/textviewerdialog.h"
#include "widgets/gdigrabwidget.h"
#include "models/audiolevelstask.h"
#include "widgets/trackpropertieswidget.h"
#include "widgets/timelinepropertieswidget.h"
#include "dialogs/unlinkedfilesdialog.h"
#include "docks/keyframesdock.h"
#include "util.h"
#include "models/keyframesmodel.h"
#include "dialogs/listselectiondialog.h"
#include "widgets/textproducerwidget.h"

#include <QtWidgets>
#include <Logger.h>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <QMutexLocker>
#include <QQuickItem>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJSEngine>
#include <QDirIterator>

static bool eventDebugCallback(void **data)
{
    QEvent *event = reinterpret_cast<QEvent*>(data[1]);
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QObject *receiver = reinterpret_cast<QObject*>(data[0]);
        LOG_DEBUG() << event << "->" << receiver;
    }
    else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        QObject *receiver = reinterpret_cast<QObject*>(data[0]);
        LOG_DEBUG() << event << "->" << receiver;
    }
    return false;
}

static const int AUTOSAVE_TIMEOUT_MS = 30000;

MainWindow::MainWindow()
    : QMainWindow(0)
    , ui(new Ui::MainWindow)
    , m_isKKeyPressed(false)
    , m_keyerGroup(0)
    , m_keyerMenu(0)
    , m_isPlaylistLoaded(false)
    , m_exitCode(EXIT_SUCCESS)
    , m_navigationPosition(0)
    , m_upgradeUrl("https://www.shotcut.org/download/")
    , m_keyframesDock(0)
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
    QLibrary libSDL("libSDL2-2.0.so.0");
    if (!libSDL.load()) {
        QMessageBox::critical(this, qApp->applicationName(),
            tr("Error: This program requires the SDL 2 library.\n\nPlease install it using your package manager. It may be named libsdl2-2.0-0, SDL2, or similar."));
        ::exit(EXIT_FAILURE);
    } else {
        libSDL.unload();
    }
#endif

    if (!qgetenv("OBSERVE_FOCUS").isEmpty()) {
        connect(qApp, &QApplication::focusChanged,
                this, &MainWindow::onFocusChanged);
        connect(qApp, &QGuiApplication::focusObjectChanged,
                this, &MainWindow::onFocusObjectChanged);
        connect(qApp, &QGuiApplication::focusWindowChanged,
                this, &MainWindow::onFocusWindowChanged);
    }

    if (!qgetenv("EVENT_DEBUG").isEmpty())
        QInternal::registerCallback(QInternal::EventNotifyCallback, eventDebugCallback);

    LOG_DEBUG() << "begin";
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
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (Settings.theme() == "light" || Settings.theme() == "dark" )
#endif
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#ifdef Q_OS_MAC
    // Qt 5 on OS X supports the standard Full Screen window widget.
    ui->mainToolBar->removeAction(ui->actionFullscreen);
    // OS X has a standard Full Screen shortcut we should use.
    ui->actionEnter_Full_Screen->setShortcut(QKeySequence((Qt::CTRL + Qt::META + Qt::Key_F)));
#endif
#ifdef Q_OS_WIN
    // Fullscreen on Windows is not allowing popups and other app windows to appear.
    delete ui->actionFullscreen;
    ui->actionFullscreen = 0;
    delete ui->actionEnter_Full_Screen;
    ui->actionEnter_Full_Screen = 0;
#endif
    setDockNestingEnabled(true);
    ui->statusBar->hide();

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));
    if (ui->actionFullscreen)
        connect(ui->actionFullscreen, SIGNAL(triggered()), this, SLOT(on_actionEnter_Full_Screen_triggered()));
    connect(ui->mainToolBar, SIGNAL(visibilityChanged(bool)), SLOT(onToolbarVisibilityChanged(bool)));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Setup the undo stack.
    m_undoStack = new QUndoStack(this);
    QAction *undoAction = m_undoStack->createUndoAction(this);
    QAction *redoAction = m_undoStack->createRedoAction(this);
    undoAction->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/oxygen/32x32/actions/edit-undo.png")));
    redoAction->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/oxygen/32x32/actions/edit-redo.png")));
    undoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Z", 0));
#ifdef Q_OS_WIN
    redoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Y", 0));
#else
    redoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+Z", 0));
#endif
    ui->menuEdit->insertAction(ui->actionCut, undoAction);
    ui->menuEdit->insertAction(ui->actionCut, redoAction);
    ui->menuEdit->insertSeparator(ui->actionCut);
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
    connect(m_player, SIGNAL(tabIndexChanged(int)), SLOT(onPlayerTabIndexChanged(int)));
    connect(MLT.videoWidget(), SIGNAL(started()), SLOT(processMultipleFiles()));
    connect(MLT.videoWidget(), SIGNAL(paused()), m_player, SLOT(showPaused()));
    connect(MLT.videoWidget(), SIGNAL(playing()), m_player, SLOT(showPlaying()));

    setupSettingsMenu();
    setupOpenOtherMenu();
    readPlayerSettings();
    configureVideoWidget();

#ifndef SHOTCUT_NOUPGRADE
    if (Settings.noUpgrade() || qApp->property("noupgrade").toBool())
#endif
        delete ui->actionUpgrade;

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
    connect(m_playlistDock, SIGNAL(clipOpened(Mlt::Producer*)), this, SLOT(openCut(Mlt::Producer*)));
    connect(m_playlistDock, SIGNAL(itemActivated(int)), this, SLOT(seekPlaylist(int)));
    connect(m_playlistDock, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_playlistDock->model(), SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(updateAutoSave()));
    connect(m_playlistDock->model(), SIGNAL(closed()), this, SLOT(onPlaylistClosed()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(updateAutoSave()));
    connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(this, SIGNAL(producerOpened()), m_playlistDock, SLOT(onProducerOpened()));
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
    connect(m_timelineDock, SIGNAL(clipClicked()), SLOT(onTimelineClipSelected()));
    connect(m_timelineDock, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_timelineDock->model(), SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_timelineDock->model(), SIGNAL(created()), SLOT(onMultitrackCreated()));
    connect(m_timelineDock->model(), SIGNAL(closed()), SLOT(onMultitrackClosed()));
    connect(m_timelineDock->model(), SIGNAL(modified()), SLOT(onMultitrackModified()));
    connect(m_timelineDock->model(), SIGNAL(modified()), SLOT(updateAutoSave()));
    connect(m_timelineDock->model(), SIGNAL(durationChanged()), SLOT(onMultitrackDurationChanged()));
    connect(m_timelineDock, SIGNAL(clipOpened(Mlt::Producer*)), SLOT(openCut(Mlt::Producer*)));
    connect(m_timelineDock->model(), SIGNAL(seeked(int)), SLOT(seekTimeline(int)));
    connect(m_timelineDock->model(), SIGNAL(scaleFactorChanged()), m_player, SLOT(pause()));
    connect(m_timelineDock, SIGNAL(selected(Mlt::Producer*)), SLOT(loadProducerWidget(Mlt::Producer*)));
    connect(m_timelineDock, SIGNAL(selectionChanged()), SLOT(onTimelineSelectionChanged()));
    connect(m_timelineDock, SIGNAL(clipCopied()), SLOT(onClipCopied()));
    connect(m_timelineDock, SIGNAL(filteredClicked()), SLOT(onFiltersDockTriggered()));
    connect(m_playlistDock, SIGNAL(addAllTimeline(Mlt::Playlist*)), SLOT(onTimelineDockTriggered()));
    connect(m_playlistDock, SIGNAL(addAllTimeline(Mlt::Playlist*)), SLOT(onAddAllToTimeline(Mlt::Playlist*)));
    connect(m_player, SIGNAL(previousSought()), m_timelineDock, SLOT(seekPreviousEdit()));
    connect(m_player, SIGNAL(nextSought()), m_timelineDock, SLOT(seekNextEdit()));

    m_filterController = new FilterController(this);
    m_filtersDock = new FiltersDock(m_filterController->metadataModel(), m_filterController->attachedModel(), this);
    m_filtersDock->setMinimumSize(400, 300);
    m_filtersDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_filtersDock);
    ui->menuView->addAction(m_filtersDock->toggleViewAction());
    connect(m_filtersDock, SIGNAL(currentFilterRequested(int)), m_filterController, SLOT(setCurrentFilter(int)), Qt::QueuedConnection);
    connect(m_filtersDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onFiltersDockTriggered(bool)));
    connect(ui->actionFilters, SIGNAL(triggered()), this, SLOT(onFiltersDockTriggered()));
    connect(m_filterController, SIGNAL(currentFilterChanged(QmlFilter*, QmlMetadata*, int)), m_filtersDock, SLOT(setCurrentFilter(QmlFilter*, QmlMetadata*, int)));
    connect(this, SIGNAL(producerOpened()), m_filterController, SLOT(setProducer()));
    connect(m_filterController->attachedModel(), SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filtersDock, SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filterController, SIGNAL(filterChanged(Mlt::Filter*)),
            m_timelineDock->model(), SLOT(onFilterChanged(Mlt::Filter*)));
    connect(m_filterController->attachedModel(), SIGNAL(addedOrRemoved(Mlt::Producer*)),
            m_timelineDock->model(), SLOT(filterAddedOrRemoved(Mlt::Producer*)));
    connect(&QmlApplication::singleton(), SIGNAL(filtersPasted(Mlt::Producer*)),
            m_timelineDock->model(), SLOT(filterAddedOrRemoved(Mlt::Producer*)));
    connect(m_filterController, SIGNAL(statusChanged(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_timelineDock, SIGNAL(fadeInChanged(int)), m_filterController, SLOT(onFadeInChanged()));
    connect(m_timelineDock, SIGNAL(fadeOutChanged(int)), m_filterController, SLOT(onFadeOutChanged()));
    connect(m_timelineDock, SIGNAL(selected(Mlt::Producer*)), m_filterController, SLOT(setProducer(Mlt::Producer*)));
    connect(m_player, SIGNAL(seeked(int)), m_filtersDock, SLOT(onSeeked(int)), Qt::QueuedConnection);
    connect(m_filtersDock, SIGNAL(seeked(int)), SLOT(seekKeyframes(int)));
    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), m_filtersDock, SLOT(onShowFrame(const SharedFrame&)));
    connect(m_player, SIGNAL(inChanged(int)), m_filtersDock, SIGNAL(producerInChanged(int)));
    connect(m_player, SIGNAL(outChanged(int)), m_filtersDock, SIGNAL(producerOutChanged(int)));
    connect(m_player, SIGNAL(inChanged(int)), m_filterController, SLOT(onFilterInChanged(int)));
    connect(m_player, SIGNAL(outChanged(int)), m_filterController, SLOT(onFilterOutChanged(int)));
    connect(m_timelineDock->model(), SIGNAL(filterInChanged(int, Mlt::Filter*)), m_filterController, SLOT(onFilterInChanged(int, Mlt::Filter*)));
    connect(m_timelineDock->model(), SIGNAL(filterOutChanged(int, Mlt::Filter*)), m_filterController, SLOT(onFilterOutChanged(int, Mlt::Filter*)));

    m_keyframesDock = new KeyframesDock(m_filtersDock->qmlProducer(), this);
    m_keyframesDock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, m_keyframesDock);
    ui->menuView->addAction(m_keyframesDock->toggleViewAction());
    connect(m_keyframesDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onKeyframesDockTriggered(bool)));
    connect(ui->actionKeyframes, SIGNAL(triggered()), this, SLOT(onKeyframesDockTriggered()));
    connect(m_filterController, SIGNAL(currentFilterChanged(QmlFilter*, QmlMetadata*, int)), m_keyframesDock, SLOT(setCurrentFilter(QmlFilter*, QmlMetadata*)));
    connect(m_keyframesDock, SIGNAL(visibilityChanged(bool)), m_filtersDock->qmlProducer(), SLOT(remakeAudioLevels(bool)));

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
    connect(ui->actionExportVideo, SIGNAL(triggered()), this, SLOT(onEncodeTriggered()));
    connect(m_encodeDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onEncodeTriggered(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_player, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_propertiesDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_recentDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_filtersDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_keyframesDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpen, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpenOther, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionExit, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), this, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_historyDock, SLOT(setDisabled(bool)));
    connect(this, SIGNAL(profileChanged()), m_encodeDock, SLOT(onProfileChanged()));
    connect(this, SIGNAL(profileChanged()), SLOT(onProfileChanged()));
    connect(this, SIGNAL(audioChannelsChanged()), m_encodeDock, SLOT(onAudioChannelsChanged()));
    connect(m_playlistDock->model(), SIGNAL(modified()), m_encodeDock, SLOT(onProducerOpened()));
    connect(m_timelineDock, SIGNAL(clipCopied()), m_encodeDock, SLOT(onProducerOpened()));
    m_encodeDock->onProfileChanged();

    m_jobsDock = new JobsDock(this);
    m_jobsDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_jobsDock);
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(onJobAdded()));
    connect(m_jobsDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onJobsDockTriggered(bool)));

    tabifyDockWidget(m_propertiesDock, m_playlistDock);
    tabifyDockWidget(m_playlistDock, m_filtersDock);
    tabifyDockWidget(m_filtersDock, m_encodeDock);
    QDockWidget* audioWaveformDock = findChild<QDockWidget*>("AudioWaveformDock");
    splitDockWidget(m_recentDock, audioWaveformDock, Qt::Vertical);
    splitDockWidget(audioMeterDock, m_recentDock, Qt::Horizontal);
    tabifyDockWidget(m_recentDock, m_historyDock);
    tabifyDockWidget(m_historyDock, m_jobsDock);
    tabifyDockWidget(m_keyframesDock, m_timelineDock);
    m_recentDock->raise();

    // Configure the View menu.
    ui->menuView->addSeparator();
    ui->menuView->addAction(ui->actionApplicationLog);

    // connect video widget signals
    Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
    connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
    connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
    connect(videoWidget, SIGNAL(gpuNotSupported()), this, SLOT(onGpuNotSupported()));
    connect(videoWidget, SIGNAL(frameDisplayed(const SharedFrame&)), m_scopeController, SIGNAL(newFrame(const SharedFrame&)));
    connect(m_filterController, SIGNAL(currentFilterChanged(QmlFilter*, QmlMetadata*, int)), videoWidget, SLOT(setCurrentFilter(QmlFilter*, QmlMetadata*)));

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

    connect(&m_network, SIGNAL(finished(QNetworkReply*)), SLOT(onUpgradeCheckFinished(QNetworkReply*)));

    QThreadPool::globalInstance()->setMaxThreadCount(qMin(4, QThreadPool::globalInstance()->maxThreadCount()));

    LOG_DEBUG() << "end";
}

void MainWindow::onFocusWindowChanged(QWindow *) const
{
    LOG_DEBUG() << "Focuswindow changed";
    LOG_DEBUG() << "Current focusWidget:" << QApplication::focusWidget();
    LOG_DEBUG() << "Current focusObject:" << QApplication::focusObject();
    LOG_DEBUG() << "Current focusWindow:" << QApplication::focusWindow();
}

void MainWindow::onFocusObjectChanged(QObject *) const
{
    LOG_DEBUG() << "Focusobject changed";
    LOG_DEBUG() << "Current focusWidget:" << QApplication::focusWidget();
    LOG_DEBUG() << "Current focusObject:" << QApplication::focusObject();
    LOG_DEBUG() << "Current focusWindow:" << QApplication::focusWindow();
}

void MainWindow::onTimelineClipSelected()
{
    // Synchronize navigation position with timeline selection.
    TimelineDock * t = m_timelineDock;

    if (t->selection().isEmpty())
        return;

    m_navigationPosition = t->centerOfClip(t->currentTrack(), t->selection().first());

    // Switch to Project player.
    if (m_player->tabIndex() != Player::ProjectTabIndex) {
        t->saveAndClearSelection();
        m_player->onTabBarClicked(Player::ProjectTabIndex);
    }
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
    delete ui;
    Mlt::Controller::destroy();
}

void MainWindow::setupSettingsMenu()
{
    LOG_DEBUG() << "begin";
    QActionGroup* group = new QActionGroup(this);
    group->addAction(ui->actionChannels1);
    group->addAction(ui->actionChannels2);
    group->addAction(ui->actionChannels6);
    group = new QActionGroup(this);
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
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 59.94 fps", "atsc_1080p_5994"));
    ui->menuProfile->addAction(addProfile(m_profileGroup, "HD 1080p 60 fps", "atsc_1080p_60"));
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
    QDir dir(Settings.appDataLocation());
    if (dir.cd("profiles")) {
        QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        if (profiles.length() > 0) {
            m_customProfileMenu->addAction(ui->actionProfileRemove);
            m_customProfileMenu->addSeparator();
        }
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
    a = new QAction(QLocale::languageToString(QLocale::Chinese).append(" (China)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("zh_CN");
    a = new QAction(QLocale::languageToString(QLocale::Chinese).append(" (Taiwan)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("zh_TW");
    a = new QAction(QLocale::languageToString(QLocale::Czech), m_languagesGroup);
    a->setCheckable(true);
    a->setData("cs");
    a = new QAction(QLocale::languageToString(QLocale::Danish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("da");
    a = new QAction(QLocale::languageToString(QLocale::Dutch), m_languagesGroup);
    a->setCheckable(true);
    a->setData("nl");
    a = new QAction(QLocale::languageToString(QLocale::English), m_languagesGroup);
    a->setCheckable(true);
    a->setData("en");
    a = new QAction(QLocale::languageToString(QLocale::Estonian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("et");
    a = new QAction(QLocale::languageToString(QLocale::Finnish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fi");
    a = new QAction(QLocale::languageToString(QLocale::French), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fr");
    a = new QAction(QLocale::languageToString(QLocale::Gaelic), m_languagesGroup);
    a->setCheckable(true);
    a->setData("gd");
    a = new QAction(QLocale::languageToString(QLocale::German), m_languagesGroup);
    a->setCheckable(true);
    a->setData("de");
    a = new QAction(QLocale::languageToString(QLocale::Greek), m_languagesGroup);
    a->setCheckable(true);
    a->setData("el");
    a = new QAction(QLocale::languageToString(QLocale::Hungarian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("hu");
    a = new QAction(QLocale::languageToString(QLocale::Italian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("it");
    a = new QAction(QLocale::languageToString(QLocale::Japanese), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ja");
    a = new QAction(QLocale::languageToString(QLocale::Nepali), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ne");
    a = new QAction(QLocale::languageToString(QLocale::NorwegianBokmal), m_languagesGroup);
    a->setCheckable(true);
    a->setData("nb");
    a = new QAction(QLocale::languageToString(QLocale::Occitan), m_languagesGroup);
    a->setCheckable(true);
    a->setData("oc");
    a = new QAction(QLocale::languageToString(QLocale::Polish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pl");
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Brazil)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_BR");
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Portugal)"), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_PT");
    a = new QAction(QLocale::languageToString(QLocale::Russian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ru");
    a = new QAction(QLocale::languageToString(QLocale::Slovak), m_languagesGroup);
    a->setCheckable(true);
    a->setData("sk");
    a = new QAction(QLocale::languageToString(QLocale::Slovenian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("sl");
    a = new QAction(QLocale::languageToString(QLocale::Spanish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("es");
    a = new QAction(QLocale::languageToString(QLocale::Turkish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("tr");
    a = new QAction(QLocale::languageToString(QLocale::Ukrainian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("uk");
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

#ifdef Q_OS_WIN
    // On Windows, if there is no JACK or it is not running
    // then Shotcut crashes inside MLT's call to jack_client_open().
    // Therefore, the JACK option for Shotcut is banned on Windows.
    delete ui->actionJack;
    ui->actionJack = 0;

    // Setup the display method actions.
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
            ui->actionDrawingDirectX->setChecked(true);
            break;
        case Qt::AA_UseSoftwareOpenGL:
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

    // Add custom layouts to View > Layout submenu.
    m_layoutGroup = new QActionGroup(this);
    connect(m_layoutGroup, SIGNAL(triggered(QAction*)), SLOT(onLayoutTriggered(QAction*)));
    if (Settings.layouts().size() > 0) {
        ui->menuLayout->addAction(ui->actionLayoutRemove);
        ui->menuLayout->addSeparator();
    }
    foreach (QString name, Settings.layouts())
        ui->menuLayout->addAction(addLayout(m_layoutGroup, name));
    LOG_DEBUG() << "end";
}

void MainWindow::setupOpenOtherMenu()
{
    // Open Other toolbar menu button
    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QMenu* otherMenu = new QMenu(this);
    ui->actionOpenOther2->setMenu(otherMenu);

    // populate the generators
    if (mltProducers->get_data("color")) {
        otherMenu->addAction(tr("Color"), this, SLOT(onOpenOtherColor()));
        if (!Settings.playerGPU() && mltProducers->get_data("qtext") && mltFilters->get_data("dynamictext"))
            otherMenu->addAction(tr("Text"), this, SLOT(onOpenOtherText()));
    }
    if (mltProducers->get_data("noise"))
        otherMenu->addAction(tr("Noise"), this, SLOT(onOpenOtherNoise()));
    if (mltProducers->get_data("frei0r.ising0r"))
        otherMenu->addAction(tr("Ising"), this, SLOT(onOpenOtherIsing()));
    if (mltProducers->get_data("frei0r.lissajous0r"))
        otherMenu->addAction(tr("Lissajous"), this, SLOT(onOpenOtherLissajous()));
    if (mltProducers->get_data("frei0r.plasma"))
        otherMenu->addAction(tr("Plasma"), this, SLOT(onOpenOtherPlasma()));
    if (mltProducers->get_data("frei0r.test_pat_B"))
        otherMenu->addAction(tr("Color Bars"), this, SLOT(onOpenOtherColorBars()));
    if (mltProducers->get_data("tone"))
        otherMenu->addAction(tr("Audio Tone"), this, SLOT(onOpenOtherTone()));
    if (mltProducers->get_data("count"))
        otherMenu->addAction(tr("Count"), this, SLOT(onOpenOtherCount()));

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    otherMenu->addAction(tr("Video4Linux"), this, SLOT(onOpenOtherV4L2()));
    otherMenu->addAction(tr("PulseAudio"), this, SLOT(onOpenOtherPulse()));
    otherMenu->addAction(tr("JACK Audio"), this, SLOT(onOpenOtherJack()));
    otherMenu->addAction(tr("ALSA Audio"), this, SLOT(onOpenOtherAlsa()));
#elif defined(Q_OS_WIN) || defined(Q_OS_MAC)
    otherMenu->addAction(tr("Audio/Video Device"), this, SLOT(onOpenOtherDevice()));
#endif
    if (mltProducers->get_data("decklink"))
        otherMenu->addAction(tr("SDI/HDMI"), this, SLOT(onOpenOtherDecklink()));
}

QAction* MainWindow::addProfile(QActionGroup* actionGroup, const QString& desc, const QString& name)
{
    QAction* action = new QAction(desc, this);
    action->setCheckable(true);
    action->setData(name);
    actionGroup->addAction(action);
    return action;
}

QAction*MainWindow::addLayout(QActionGroup* actionGroup, const QString& name)
{
    QAction* action = new QAction(name, this);
    actionGroup->addAction(action);
    return action;
}

void MainWindow::open(Mlt::Producer* producer)
{
    if (!producer->is_valid())
        showStatusMessage(tr("Failed to open "));
    else if (producer->get_int("error"))
        showStatusMessage(tr("Failed to open ") + producer->get("resource"));

    bool ok = false;
    int screen = Settings.playerExternal().toInt(&ok);
    if (ok && screen != QApplication::desktop()->screenNumber(this))
        m_player->moveVideoToScreen(screen);

    // no else here because open() will delete the producer if open fails
    if (!MLT.setProducer(producer)) {
        emit producerOpened();
        if (!MLT.profile().is_explicit() || MLT.isMultitrack() || MLT.isPlaylist())
            emit profileChanged();
    }
    m_player->setFocus();
    m_playlistDock->setUpdateButtonEnabled(false);

    // Needed on Windows. Upon first file open, window is deactivated, perhaps OpenGL-related.
    activateWindow();
}

bool MainWindow::isCompatibleWithGpuMode(MltXmlChecker& checker)
{
    if (checker.needsGPU() && !Settings.playerGPU()) {
        LOG_INFO() << "file uses GPU but GPU not enabled";
        QMessageBox dialog(QMessageBox::Question,
           qApp->applicationName(),
           tr("The file you opened uses GPU effects, but GPU effects are not enabled.\n"
              "Do you want to enable GPU effects and restart?"),
           QMessageBox::No |
           QMessageBox::Yes,
           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            ui->actionGPU->setChecked(true);
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        return false;
    }
    else if (checker.needsCPU() && Settings.playerGPU()) {
        LOG_INFO() << "file uses GPU incompatible filters but GPU is enabled";
        QMessageBox dialog(QMessageBox::Question,
           qApp->applicationName(),
           tr("The file you opened uses CPU effects that are incompatible with GPU effects, but GPU effects are enabled.\n"
              "Do you want to disable GPU effects and restart?"),
           QMessageBox::No |
           QMessageBox::Yes,
           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            ui->actionGPU->setChecked(false);
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        return false;
    }
    return true;
}

bool MainWindow::saveRepairedXmlFile(MltXmlChecker& checker, QString& fileName)
{
    QFileInfo fi(fileName);
    QFile repaired(QString("%1/%2 - %3.%4").arg(fi.path())
        .arg(fi.completeBaseName()).arg(tr("Repaired")).arg(fi.suffix()));
    repaired.open(QIODevice::WriteOnly);
    LOG_INFO() << "repaired MLT XML file name" << repaired.fileName();
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
    LOG_WARNING() << "repairing failed";
    return false;
}

bool MainWindow::isXmlRepaired(MltXmlChecker& checker, QString& fileName)
{
    bool result = true;
    if (checker.isCorrected()) {
        LOG_WARNING() << fileName;
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
        if (r == QMessageBox::Yes)
            result = saveRepairedXmlFile(checker, fileName);
    }
    else if (checker.unlinkedFilesModel().rowCount() > 0) {
        UnlinkedFilesDialog dialog(this);
        dialog.setModel(checker.unlinkedFilesModel());
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QDialog::Accepted) {
            if (checker.check(fileName) && checker.isCorrected())
                result = saveRepairedXmlFile(checker, fileName);
        } else {
            result = false;
        }
    }
    return result;
}

bool MainWindow::checkAutoSave(QString &url)
{
    QMutexLocker locker(&m_autosaveMutex);

    // check whether autosave files exist:
    QSharedPointer<AutoSaveFile> stale(AutoSaveFile::getFile(url));
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
                LOG_WARNING() << "failed to recover autosave file" << url;
            } else {
                m_autosaveFile = stale;
                url = stale->fileName();
                return true;
            }
        }
    }

    // create new autosave object
    m_autosaveFile.reset(new AutoSaveFile(url));

    return false;
}

void MainWindow::stepLeftBySeconds(int sec)
{
    m_player->seek(m_player->position() + sec * qRound(MLT.profile().fps()));
}

void MainWindow::doAutosave()
{
    QMutexLocker locker(&m_autosaveMutex);
    if (m_autosaveFile) {
        if (m_autosaveFile->isOpen() || m_autosaveFile->open(QIODevice::ReadWrite)) {
            saveXML(m_autosaveFile->fileName(), false /* without relative paths */);
        } else {
            LOG_ERROR() << "failed to open autosave file for writing" << m_autosaveFile->fileName();
        }
    }
}

void MainWindow::setFullScreen(bool isFullScreen)
{
    if (isFullScreen) {
#ifndef Q_OS_WIN
        showFullScreen();
        ui->actionEnter_Full_Screen->setVisible(false);
        ui->actionFullscreen->setVisible(false);
#endif
    }
}

QString MainWindow::untitledFileName() const
{
    QDir dir = Settings.appDataLocation();
    if (!dir.exists()) dir.mkpath(dir.path());
    return dir.filePath("__untitled__.mlt");
}

QString MainWindow::getFileHash(const QString& path) const
{
    // This routine is intentionally copied from Kdenlive.
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData;
         // 1 MB = 1 second per 450 files (or faster)
         // 10 MB = 9 seconds per 450 files (or faster)
        if (file.size() > 1000000*2) {
            fileData = file.read(1000000);
            if (file.seek(file.size() - 1000000))
                fileData.append(file.readAll());
        } else {
            fileData = file.readAll();
        }
        file.close();
        return QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
    }
    return QString();
}

QString MainWindow::getHash(Mlt::Properties& properties) const
{
    QString hash = properties.get(kShotcutHashProperty);
    if (hash.isEmpty()) {
        QString service = properties.get("mlt_service");
        QString resource = QString::fromUtf8(properties.get("resource"));

        if (service == "timewarp")
            resource = QString::fromUtf8(properties.get("warp_resource"));
        else if (service == "vidstab")
            resource = QString::fromUtf8(properties.get("filename"));
        QString hash = getFileHash(resource);
        if (!hash.isEmpty())
            properties.set(kShotcutHashProperty, hash.toLatin1().constData());
    }
    return hash;
}

void MainWindow::setProfile(const QString &profile_name)
{
    LOG_DEBUG() << profile_name;
    MLT.setProfile(profile_name);
    emit profileChanged();
}

bool MainWindow::isSourceClipMyProject(QString resource)
{
    if (m_player->tabIndex() == Player::ProjectTabIndex && MLT.savedProducer() && MLT.savedProducer()->is_valid())
        resource = QString::fromUtf8(MLT.savedProducer()->get("resource"));
    if (QDir::toNativeSeparators(resource) == QDir::toNativeSeparators(MAIN.fileName())) {
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("You cannot add a project to itself!"),
                           QMessageBox::Ok,
                           this);
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Ok);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.exec();
        return true;
    }
    return false;
}

bool MainWindow::keyframesDockIsVisible() const
{
    return m_keyframesDock && m_keyframesDock->isVisible();
}

void MainWindow::setAudioChannels(int channels)
{
    LOG_DEBUG() << channels;
    MLT.videoWidget()->setProperty("audio_channels", channels);
    MLT.setAudioChannels(channels);
    if (channels == 1)
        ui->actionChannels1->setChecked(true);
    else if (channels == 2)
        ui->actionChannels2->setChecked(true);
    else if (channels == 6)
        ui->actionChannels6->setChecked(true);
    emit audioChannelsChanged();
}

static void autosaveTask(MainWindow* p)
{
    LOG_DEBUG_TIME();
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
    LOG_DEBUG() << url;
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
        if (!isXmlRepaired(checker, url))
            return;
        modified = checkAutoSave(url);
        // let the new project change the profile
        if (modified || QFile::exists(url)) {
            MLT.profile().set_explicit(false);
            setWindowModified(modified);
        }
    }
    if (!playlist() && !multitrack()) {
        if (!modified && !continueModified())
            return;
        setCurrentFile("");
        setWindowModified(modified);
        MLT.resetURL();
        // Return to automatic video mode if selected.
        if (m_profileGroup->checkedAction() && m_profileGroup->checkedAction()->data().toString().isEmpty())
            MLT.profile().set_explicit(false);
    }
    if (!MLT.open(url)) {
        Mlt::Properties* props = const_cast<Mlt::Properties*>(properties);
        if (props && props->is_valid())
            mlt_properties_inherit(MLT.producer()->get_properties(), props->get_properties());
        m_player->setPauseAfterOpen(!MLT.isClip());

        if (MLT.producer() && MLT.producer()->is_valid())
            setAudioChannels(MLT.audioChannels());

        open(MLT.producer());
        if (url.startsWith(AutoSaveFile::path())) {
            if (m_autosaveFile && m_autosaveFile->managedFileName() != untitledFileName()) {
                m_recentDock->add(m_autosaveFile->managedFileName());
                LOG_INFO() << m_autosaveFile->managedFileName();
            }
        } else {
            m_recentDock->add(url);
            LOG_INFO() << url;
        }
    }
    else if (url != untitledFileName()) {
        showStatusMessage(tr("Failed to open ") + url);
        emit openFailed(url);
    }
}

void MainWindow::openMultiple(const QStringList& paths)
{
    if (paths.size() > 1) {
        QList<QUrl> urls;
        foreach (const QString& s, paths)
            urls << s;
        openMultiple(urls);
    } else if (!paths.isEmpty()) {
        open(paths.first());
    }
}

void MainWindow::openMultiple(const QList<QUrl>& urls)
{
    if (urls.size() > 1) {
        m_multipleFiles = Util::sortedFileList(Util::expandDirectories(urls));
        open(m_multipleFiles.first());
    } else {
        QUrl url = urls.first();
        open(Util::removeFileScheme(url));
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

void MainWindow::openCut(Mlt::Producer* producer)
{
    m_player->setPauseAfterOpen(true);
    open(producer);
    MLT.seek(producer->get_in());
}

void MainWindow::hideProducer()
{
    // This is a hack to release references to the old producer, but it
    // probably leaves a reference to the new color producer somewhere not
    // yet identified (root cause).
    openCut(new Mlt::Producer(MLT.profile(), "color:_hide"));
    QCoreApplication::processEvents();
    openCut(new Mlt::Producer(MLT.profile(), "color:_hide"));
    QCoreApplication::processEvents();

    QScrollArea* scrollArea = (QScrollArea*) m_propertiesDock->widget();
    delete scrollArea->widget();
    scrollArea->setWidget(0);
    m_player->reset();

    QCoreApplication::processEvents();
}

void MainWindow::closeProducer()
{
    hideProducer();
    MLT.stop();
    MLT.close();
    MLT.setSavedProducer(0);
}

void MainWindow::showStatusMessage(QAction* action, int timeoutSeconds)
{
    // This object takes ownership of the passed action.
    // This version does not currently log its message.
    m_statusBarAction.reset(action);
    action->setParent(0);
    m_player->setStatusLabel(action->text(), timeoutSeconds, action);
}

void MainWindow::showStatusMessage(const QString& message, int timeoutSeconds)
{
    LOG_INFO() << message;
    m_player->setStatusLabel(message, timeoutSeconds, 0 /* QAction */);
    m_statusBarAction.reset();
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
    on_actionJack_triggered(ui->actionJack && ui->actionJack->isChecked());
    m_player->onProducerOpened(false);
    m_encodeDock->onProducerOpened();
    m_filterController->setProducer();
    updateMarkers();
    MLT.seek(start);
    m_player->setFocus();
    m_player->switchToTab(Player::ProjectTabIndex);
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
        on_actionJack_triggered(ui->actionJack && ui->actionJack->isChecked());
        m_player->onProducerOpened(false);
        m_encodeDock->onProducerOpened();
        m_filterController->setProducer();
        updateMarkers();
        m_player->setFocus();
        m_player->switchToTab(Player::ProjectTabIndex);
        m_timelineDock->emitSelectedFromSelection();
    }
    m_player->seek(position);
}

void MainWindow::seekKeyframes(int position)
{
    m_player->seek(position);
}

void MainWindow::readPlayerSettings()
{
    LOG_DEBUG() << "begin";
    ui->actionRealtime->setChecked(Settings.playerRealtime());
    ui->actionProgressive->setChecked(Settings.playerProgressive());
    ui->actionScrubAudio->setChecked(Settings.playerScrubAudio());
    if (ui->actionJack)
        ui->actionJack->setChecked(Settings.playerJACK());
    if (ui->actionGPU) {
        ui->actionGPU->setChecked(Settings.playerGPU());
        MLT.videoWidget()->setProperty("gpu", ui->actionGPU->isChecked());
    }

    setAudioChannels(Settings.playerAudioChannels());

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

    LOG_DEBUG() << "end";
}

void MainWindow::readWindowSettings()
{
    LOG_DEBUG() << "begin";
    Settings.setWindowGeometryDefault(saveGeometry());
    Settings.setWindowStateDefault(saveState());
    Settings.sync();
    if (!Settings.windowGeometry().isEmpty()) {
        restoreGeometry(Settings.windowGeometry());
        restoreState(Settings.windowState());
    } else {
        on_actionLayoutTimeline_triggered();
    }
    LOG_DEBUG() << "end";
}

void MainWindow::writeSettings()
{
#ifndef Q_OS_MAC
    if (isFullScreen())
        showNormal();
#endif
    Settings.setPlayerGPU(ui->actionGPU->isChecked());
    Settings.setWindowGeometry(saveGeometry());
    Settings.setWindowState(saveState());
    Settings.sync();
}

void MainWindow::configureVideoWidget()
{
    LOG_DEBUG() << "begin";
    if (m_profileGroup->checkedAction())
        setProfile(m_profileGroup->checkedAction()->data().toString());
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
    if (ui->actionChannels1->isChecked())
        setAudioChannels(1);
    else if (ui->actionChannels2->isChecked())
        setAudioChannels(2);
    else
        setAudioChannels(6);
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
    LOG_DEBUG() << "end";
}

void MainWindow::setCurrentFile(const QString &filename)
{
    QString shownName = tr("Untitled");
    if (filename == untitledFileName())
        m_currentFile.clear();
    else
        m_currentFile = filename;
    if (!m_currentFile.isEmpty())
        shownName = QFileInfo(m_currentFile).fileName();
#ifdef Q_OS_MAC
    setWindowTitle(QString("%1 - %2").arg(shownName).arg(qApp->applicationName()));
#else
    setWindowTitle(QString("%1[*] - %2").arg(shownName).arg(qApp->applicationName()));
#endif
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version %1</h1>"
                "<p><a href=\"https://www.shotcut.org/\">Shotcut</a> is a free, open source, cross platform video editor.</p>"
                "<small><p>Copyright &copy; 2011-2018 <a href=\"https://www.meltytech.com/\">Meltytech</a>, LLC</p>"
                "<p>Licensed under the <a href=\"https://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program proudly uses the following projects:<ul>"
                "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
                "<li><a href=\"https://www.mltframework.org/\">MLT</a> multimedia authoring framework</li>"
                "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries</li>"
                "<li><a href=\"https://www.videolan.org/developers/x264.html\">x264</a> H.264 encoder</li>"
                "<li><a href=\"https://www.webmproject.org/\">WebM</a> VP8 and VP9 encoders</li>"
                "<li><a href=\"http://lame.sourceforge.net/\">LAME</a> MP3 encoder</li>"
                "<li><a href=\"https://www.dyne.org/software/frei0r/\">Frei0r</a> video plugins</li>"
                "<li><a href=\"https://www.ladspa.org/\">LADSPA</a> audio plugins</li>"
                "<li><a href=\"http://www.defaulticon.com/\">DefaultIcon</a> icon collection by <a href=\"http://www.interactivemania.com/\">interactivemania</a></li>"
                "<li><a href=\"http://www.oxygen-icons.org/\">Oxygen</a> icon collection</li>"
                "</ul></p>"
                "<p>The source code used to build this program can be downloaded from "
                "<a href=\"https://www.shotcut.org/\">shotcut.org</a>.</p>"
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
    case Qt::Key_A:
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
    case Qt::Key_C:
        if (event->modifiers() == Qt::ShiftModifier && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->show();
            m_playlistDock->raise();
            if (m_playlistDock->position() >= 0)
                m_playlistDock->on_actionOpen_triggered();
        } else {
            m_timelineDock->show();
            m_timelineDock->raise();
            if (!m_timelineDock->selection().isEmpty())
                m_timelineDock->copyClip(m_timelineDock->currentTrack(), m_timelineDock->selection().first());
        }
        break;
    case Qt::Key_D:
        if (event->modifiers() & Qt::ControlModifier)
            m_timelineDock->setSelection();
        else
            handled = false;
        break;
    case Qt::Key_H:
#ifdef Q_OS_MAC
        // OS X uses Cmd+H to hide an app.
        if (event->modifiers() & Qt::MetaModifier && isMultitrackValid())
#else
        if (event->modifiers() & Qt::ControlModifier && isMultitrackValid())
#endif
            m_timelineDock->toggleTrackHidden(m_timelineDock->currentTrack());
        break;
    case Qt::Key_J:
        if (m_isKKeyPressed)
            m_player->seek(m_player->position() - 1);
        else
            m_player->rewind(false);
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
        if (event->modifiers() & Qt::MetaModifier && isMultitrackValid())
#else
        if (event->modifiers() & Qt::ControlModifier && isMultitrackValid())
#endif
            m_timelineDock->setTrackLock(m_timelineDock->currentTrack(), !m_timelineDock->isTrackLocked(m_timelineDock->currentTrack()));
        else if (m_isKKeyPressed)
            m_player->seek(m_player->position() + 1);
        else
            m_player->fastForward(false);
        break;
    case Qt::Key_M:
#ifdef Q_OS_MAC
        // OS X uses Cmd+M to minimize an app.
        if (event->modifiers() & Qt::MetaModifier && isMultitrackValid())
#else
        if (event->modifiers() & Qt::ControlModifier && isMultitrackValid())
#endif
            m_timelineDock->toggleTrackMute(m_timelineDock->currentTrack());
        break;
    case Qt::Key_I:
        if (event->modifiers() == Qt::ControlModifier) {
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->addVideoTrack();
        } else {
            setInToCurrent(event->modifiers() & Qt::ShiftModifier);
        }
        break;
    case Qt::Key_O:
        setOutToCurrent(event->modifiers() & Qt::ShiftModifier);
        break;
    case Qt::Key_P:
        if (event->modifiers() == Qt::ControlModifier) {
            Settings.setTimelineSnap(!Settings.timelineSnap());
        }
        break;
    case Qt::Key_R:
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->modifiers() & Qt::AltModifier) {
                Settings.setTimelineRippleAllTracks(!Settings.timelineRippleAllTracks());
            } else if (event->modifiers() & Qt::ShiftModifier) {
                Settings.setTimelineRippleAllTracks(!Settings.timelineRipple());
                Settings.setTimelineRipple(!Settings.timelineRipple());
            } else {
                Settings.setTimelineRipple(!Settings.timelineRipple());
            }
        }
        break;
    case Qt::Key_S:
        if (isMultitrackValid())
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
    case Qt::Key_B:
        if (event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::AltModifier) {
            // Toggle track blending.
            int trackIndex = m_timelineDock->currentTrack();
            bool isBottomVideo = m_timelineDock->model()->data(m_timelineDock->model()->index(trackIndex), MultitrackModel::IsBottomVideoRole).toBool();
            if (!isBottomVideo) {
                bool isComposite = m_timelineDock->model()->data(m_timelineDock->model()->index(trackIndex), MultitrackModel::IsCompositeRole).toBool();
                m_timelineDock->setTrackComposite(trackIndex, !isComposite);
            }
        } else if (event->modifiers() == Qt::ShiftModifier) {
            if (m_playlistDock->model()->rowCount() > 0) {
                // Update playlist item.
                m_playlistDock->show();
                m_playlistDock->raise();
                m_playlistDock->on_actionUpdate_triggered();
            }
        } else {
            // Overwrite on timeline.
            m_timelineDock->show();
            m_timelineDock->raise();
            m_timelineDock->overwrite(-1);
        }
        break;
    case Qt::Key_Escape: // Avid Toggle Active Monitor
        if (MLT.isPlaylist()) {
            if (isMultitrackValid())
                m_player->onTabBarClicked(Player::ProjectTabIndex);
            else if (MLT.savedProducer())
                m_player->onTabBarClicked(Player::SourceTabIndex);
            else
                m_playlistDock->on_actionOpen_triggered();
        } else if (MLT.isMultitrack()) {
            if (MLT.savedProducer())
                m_player->onTabBarClicked(Player::SourceTabIndex);
            // TODO else open clip under playhead of current track if available
        } else {
            if (isMultitrackValid() || (playlist() && playlist()->count() > 0))
                m_player->onTabBarClicked(Player::ProjectTabIndex);
        }
        break;
    case Qt::Key_Up:
        if (m_playlistDock->isVisible() && event->modifiers() & Qt::AltModifier && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->raise();
            m_playlistDock->decrementIndex();
            m_playlistDock->on_actionOpen_triggered();
        } else if (isMultitrackValid()) {
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

        } else if (m_playlistDock->isVisible() && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->raise();
            if (event->modifiers() & Qt::ControlModifier)
                m_playlistDock->moveClipUp();
            m_playlistDock->decrementIndex();
        }
        break;
    case Qt::Key_Down:
        if (m_playlistDock->isVisible() && event->modifiers() & Qt::AltModifier && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->raise();
            m_playlistDock->incrementIndex();
            m_playlistDock->on_actionOpen_triggered();
        } else if (isMultitrackValid()) {
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

        } else if (m_playlistDock->isVisible() && m_playlistDock->model()->rowCount() > 0) {
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
        if (m_playlistDock->isVisible() && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(event->key() - Qt::Key_1);
        }
        break;
    case Qt::Key_0:
        if (m_timelineDock->isVisible()) {
            if (event->modifiers() & Qt::ControlModifier)
                m_timelineDock->model()->setTrackHeight(50);
            else
                m_timelineDock->resetZoom();
        } else if (m_playlistDock->isVisible() && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->raise();
            m_playlistDock->setIndex(9);
        }
        break;
    case Qt::Key_X: // Avid Extract
        if (event->modifiers() == Qt::ShiftModifier && m_playlistDock->model()->rowCount() > 0) {
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
        if (isMultitrackValid()) {
            m_timelineDock->show();
            m_timelineDock->raise();
            if (event->modifiers() == Qt::ShiftModifier)
                m_timelineDock->removeSelection();
            else
                m_timelineDock->liftSelection();
        } else if (m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_removeButton_clicked();
        }
        break;
    case Qt::Key_Z: // Avid Lift
        if (event->modifiers() == Qt::ShiftModifier && m_playlistDock->model()->rowCount() > 0) {
            m_playlistDock->show();
            m_playlistDock->raise();
            m_playlistDock->on_removeButton_clicked();
        } else if (isMultitrackValid() && event->modifiers() == Qt::NoModifier) {
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
        if (m_playlistDock->position() >= 0) {
            if (event->modifiers() == Qt::ShiftModifier)
                seekPlaylist(m_playlistDock->position());
            else
                m_playlistDock->on_actionOpen_triggered();
        }
        break;
    case Qt::Key_F5:
        m_timelineDock->model()->reload();
        m_keyframesDock->model().reload();
        break;
    case Qt::Key_F1:
        LOG_DEBUG() << "Current focusWidget:" << QApplication::focusWidget();
        LOG_DEBUG() << "Current focusObject:" << QApplication::focusObject();
        LOG_DEBUG() << "Current focusWindow:" << QApplication::focusWindow();
        break;
    default:
        handled = false;
        break;
    }

    if (handled)
        event->setAccepted(handled);
    else
        QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_K) {
        m_isKKeyPressed = false;
        event->setAccepted(true);
    } else {
        QMainWindow::keyReleaseEvent(event);
    }
}

void MainWindow::hideSetDataDirectory()
{
    delete ui->actionAppDataSet;
}

// Drag-n-drop events

bool MainWindow::eventFilter(QObject* target, QEvent* event)
{
    if (event->type() == QEvent::DragEnter && target == MLT.videoWidget()) {
        dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return true;
    } else if (event->type() == QEvent::Drop && target == MLT.videoWidget()) {
        dropEvent(static_cast<QDropEvent*>(event));
        return true;
    } else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QQuickWidget * focusedQuickWidget = qobject_cast<QQuickWidget*>(qApp->focusWidget());
        if (focusedQuickWidget && focusedQuickWidget->quickWindow()->activeFocusItem()) {
            event->accept();
            focusedQuickWidget->quickWindow()->sendEvent(focusedQuickWidget->quickWindow()->activeFocusItem(), event);
            QWidget * w = focusedQuickWidget->parentWidget();
            if (!event->isAccepted())
                qApp->sendEvent(w, event);
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Simulate the player firing a dragStarted even to make the playlist close
    // its help text view. This lets one drop a clip directly into the playlist
    // from a fresh start.
    Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &Mlt::Controller::singleton();
    emit videoWidget->dragStarted();

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
        openMultiple(mimeData->urls());
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
            LOG_DEBUG() << "begin";
            writeSettings();
            MLT.stop();
            QThreadPool::globalInstance()->clear();
            AudioLevelsTask::closeAll();
            event->accept();
            emit aboutToShutDown();
            QApplication::exit(m_exitCode);
            LOG_DEBUG() << "end";
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

    windowHandle()->installEventFilter(this);

#ifndef SHOTCUT_NOUPGRADE
    if (!Settings.noUpgrade() && !qApp->property("noupgrade").toBool())
        QTimer::singleShot(0, this, SLOT(showUpgradePrompt()));
#endif
}

void MainWindow::on_actionOpenOther_triggered()
{
    // these static are used to open dialog with previous configuration
    OpenOtherDialog dialog(this);

    if (MLT.producer())
        dialog.load(MLT.producer());
    if (dialog.exec() == QDialog::Accepted) {
        closeProducer();
        open(dialog.newProducer(MLT.profile()));
    }
}

void MainWindow::onProducerOpened()
{
    QWidget* w = loadProducerWidget(MLT.producer());
    if (w && !MLT.producer()->get(kMultitrackItemProperty)) {
        if (-1 != w->metaObject()->indexOfSignal("producerReopened()"))
            connect(w, SIGNAL(producerReopened()), m_player, SLOT(onProducerOpened()));
    }
    else if (MLT.isPlaylist()) {
        m_playlistDock->model()->load();
        if (playlist()) {
            m_isPlaylistLoaded = true;
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_playlistDock->setVisible(true);
            m_playlistDock->raise();
            m_player->enableTab(Player::ProjectTabIndex);
            m_player->switchToTab(Player::ProjectTabIndex);
        }
    }
    else if (MLT.isMultitrack()) {
        m_timelineDock->model()->load();
        if (isMultitrackValid()) {
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_timelineDock->setVisible(true);
            m_timelineDock->raise();
            m_player->enableTab(Player::ProjectTabIndex);
            m_player->switchToTab(Player::ProjectTabIndex);
            m_timelineDock->selectMultitrack();
        }
    }
    if (MLT.isClip()) {
        m_player->enableTab(Player::SourceTabIndex);
        m_player->switchToTab(Player::SourceTabIndex);
        getHash(*MLT.producer());
        ui->actionPaste->setEnabled(true);
    }
    if (m_autosaveFile)
        setCurrentFile(m_autosaveFile->managedFileName());
    else if (!MLT.URL().isEmpty())
        setCurrentFile(MLT.URL());
    on_actionJack_triggered(ui->actionJack && ui->actionJack->isChecked());
}

void MainWindow::onProducerChanged()
{
    MLT.refreshConsumer();
    if (playlist() && MLT.producer() && MLT.producer()->is_valid()
        && MLT.producer()->get_int(kPlaylistIndexProperty))
        m_playlistDock->setUpdateButtonEnabled(true);
}

bool MainWindow::on_actionSave_triggered()
{
    if (m_currentFile.isEmpty()) {
        return on_actionSave_As_triggered();
    } else {
        if (Util::warnIfNotWritable(m_currentFile, this, tr("Save XML")))
            return false;
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
    QString path = Settings.savePath();
    if (m_currentFile.isEmpty())
        path.append("/.mlt");
    else
        path = m_currentFile;
    QString caption = tr("Save XML");
    QString filename = QFileDialog::getSaveFileName(this, caption, path, tr("MLT XML (*.mlt)"));
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        Settings.setSavePath(fi.path());
        if (fi.suffix() != "mlt")
            filename += ".mlt";

        if (Util::warnIfNotWritable(filename, this, caption))
            return false;

        saveXML(filename);
        if (m_autosaveFile)
            m_autosaveFile->changeManagedFile(filename);
        else
            m_autosaveFile.reset(new AutoSaveFile(filename));
        setCurrentFile(filename);
        setWindowModified(false);
        if (MLT.producer())
            showStatusMessage(tr("Saved %1").arg(m_currentFile));
        m_undoStack->setClean();
        m_recentDock->add(filename);
    }
    return !filename.isEmpty();
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
        if (r == QMessageBox::Yes || r == QMessageBox::No) {
            QMutexLocker locker(&m_autosaveMutex);
            m_autosaveFile.reset();
            if (r == QMessageBox::Yes)
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
    if (m_encodeDock->isExportInProgress()) {
        QMessageBox dialog(QMessageBox::Warning,
                                     qApp->applicationName(),
                                     tr("An export is in progress.\n"
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
    if (started && (MLT.resource().startsWith("x11grab:") ||
                    MLT.resource().startsWith("gdigrab:") ||
                    MLT.resource().startsWith("avfoundation"))
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

void MainWindow::onKeyframesDockTriggered(bool checked)
{
    if (checked) {
        m_keyframesDock->show();
        m_keyframesDock->raise();
    }
}

void MainWindow::onPlaylistCreated()
{
    if (!playlist() || playlist()->count() == 0) return;
    m_player->enableTab(Player::ProjectTabIndex, true);
}

void MainWindow::onPlaylistLoaded()
{
    updateMarkers();
    m_player->enableTab(Player::ProjectTabIndex, true);
}

void MainWindow::onPlaylistCleared()
{
    m_player->onTabBarClicked(Player::SourceTabIndex);
    setWindowModified(true);
}

void MainWindow::onPlaylistClosed()
{
    closeProducer();
    setProfile(Settings.playerProfile());
    setAudioChannels(Settings.playerAudioChannels());
    setCurrentFile("");
    setWindowModified(false);
    m_undoStack->clear();
    MLT.resetURL();
    m_autosaveFile.reset(new AutoSaveFile(untitledFileName()));
    if (!isMultitrackValid())
        m_player->enableTab(Player::ProjectTabIndex, false);
}

void MainWindow::onPlaylistModified()
{
    setWindowModified(true);
    if (MLT.producer() && playlist() && (void*) MLT.producer()->get_producer() == (void*) playlist()->get_playlist())
        m_player->onDurationChanged();
    updateMarkers();
    m_player->enableTab(Player::ProjectTabIndex, true);
}

void MainWindow::onMultitrackCreated()
{
    m_player->enableTab(Player::ProjectTabIndex, true);
}

void MainWindow::onMultitrackClosed()
{
    setAudioChannels(Settings.playerAudioChannels());
    closeProducer();
    setProfile(Settings.playerProfile());
    setCurrentFile("");
    setWindowModified(false);
    m_undoStack->clear();
    MLT.resetURL();
    m_autosaveFile.reset(new AutoSaveFile(untitledFileName()));
    if (!playlist() || playlist()->count() == 0)
        m_player->enableTab(Player::ProjectTabIndex, false);
}

void MainWindow::onMultitrackModified()
{
    setWindowModified(true);

    // Reflect this playlist info onto the producer for keyframes dock.
    if (!m_timelineDock->selection().isEmpty()) {
        int clipIndex = m_timelineDock->selection().first();
        QScopedPointer<Mlt::ClipInfo> info(m_timelineDock->getClipInfo(m_timelineDock->currentTrack(), clipIndex));
        if (info && info->producer && info->producer->is_valid()) {
            info->producer->set(kPlaylistStartProperty, info->start);
            if (info->frame_in != info->producer->get_int(kFilterInProperty)) {
                info->producer->set(kFilterInProperty, info->frame_in);
                int delta = info->frame_in - info->producer->get_int(kFilterInProperty);
                emit m_filtersDock->producerInChanged(delta);
            }
            if (info->frame_out != info->producer->get_int(kFilterOutProperty)) {
                info->producer->set(kFilterOutProperty, info->frame_out);
                int delta = info->frame_out - info->producer->get_int(kFilterOutProperty);
                emit m_filtersDock->producerOutChanged(delta);
            }
        }
    }
}

void MainWindow::onMultitrackDurationChanged()
{
    if (MLT.producer() && (void*) MLT.producer()->get_producer() == (void*) multitrack()->get_producer())
        m_player->onDurationChanged();
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

void MainWindow::onProducerModified()
{
    setWindowModified(true);
    updateAutoSave();
    if (playlist())
        m_playlistDock->setUpdateButtonEnabled(true);
}

void MainWindow::onFilterModelChanged()
{
    MLT.refreshConsumer();
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
    QDesktopServices::openUrl(QUrl("https://www.shotcut.org/FAQ/"));
}

void MainWindow::on_actionForum_triggered()
{
    QDesktopServices::openUrl(QUrl("https://forum.shotcut.org/"));
}

void MainWindow::saveXML(const QString &filename, bool withRelativePaths)
{
    if (m_timelineDock->model()->rowCount() > 0) {
        MLT.saveXML(filename, multitrack(), withRelativePaths);
    } else if (m_playlistDock->model()->rowCount() > 0) {
        int in = MLT.producer()->get_in();
        int out = MLT.producer()->get_out();
        MLT.producer()->set_in_and_out(0, MLT.producer()->get_length() - 1);
        MLT.saveXML(filename, playlist(), withRelativePaths);
        MLT.producer()->set_in_and_out(in, out);
    } else if (MLT.producer()) {
        MLT.saveXML(filename, (MLT.isMultitrack() || MLT.isPlaylist())? MLT.savedProducer() : 0, withRelativePaths);
    }
}

void MainWindow::changeTheme(const QString &theme)
{
    LOG_DEBUG() << "begin";
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
        palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
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
    LOG_DEBUG() << "end";
}

Mlt::Playlist* MainWindow::playlist() const
{
    return m_playlistDock->model()->playlist();
}

Mlt::Producer *MainWindow::multitrack() const
{
    return m_timelineDock->model()->tractor();
}

bool MainWindow::isMultitrackValid() const
{
    return m_timelineDock->model()->tractor()
       && !m_timelineDock->model()->trackList().empty();
}

QWidget *MainWindow::loadProducerWidget(Mlt::Producer* producer)
{
    QWidget* w = 0;
    QScrollArea* scrollArea = (QScrollArea*) m_propertiesDock->widget();

    if (!producer || !producer->is_valid()) {
        if (scrollArea->widget())
            scrollArea->widget()->deleteLater();
        return  w;
    } else {
        scrollArea->show();
    }

    QString service(producer->get("mlt_service"));
    QString resource = QString::fromUtf8(producer->get("resource"));
    QString shotcutProducer(producer->get(kShotcutProducerProperty));

    if (resource.startsWith("video4linux2:") || QString::fromUtf8(producer->get("resource1")).startsWith("video4linux2:"))
        w = new Video4LinuxWidget(this);
    else if (resource.startsWith("pulse:"))
        w = new PulseAudioWidget(this);
    else if (resource.startsWith("jack:"))
        w = new JackProducerWidget(this);
    else if (resource.startsWith("alsa:"))
        w = new AlsaWidget(this);
    else if (resource.startsWith("dshow:") || QString::fromUtf8(producer->get("resource1")).startsWith("dshow:"))
        w = new DirectShowVideoWidget(this);
    else if (resource.startsWith("avfoundation:"))
        w = new AvfoundationProducerWidget(this);
    else if (resource.startsWith("x11grab:"))
        w = new X11grabWidget(this);
    else if (resource.startsWith("gdigrab:"))
        w = new GDIgrabWidget(this);
    else if (service.startsWith("avformat") || shotcutProducer == "avformat")
        w = new AvformatProducerWidget(this);
    else if (MLT.isImageProducer(producer)) {
        ImageProducerWidget* ipw = new ImageProducerWidget(this);
        connect(m_player, SIGNAL(outChanged(int)), ipw, SLOT(updateDuration()));
        connect(m_timelineDock, SIGNAL(imageDurationChanged()), ipw, SLOT(updateDuration()));
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
    else if (service == "count")
        w = new CountProducerWidget(this);
    else if (producer->parent().get(kShotcutTransitionProperty)) {
        w = new LumaMixTransition(producer->parent(), this);
        scrollArea->setWidget(w);
        return w;
    } else if (playlist_type == producer->type()) {
        int trackIndex = m_timelineDock->currentTrack();
        bool isBottomVideo = m_timelineDock->model()->data(m_timelineDock->model()->index(trackIndex), MultitrackModel::IsBottomVideoRole).toBool();
        if (!isBottomVideo) {
            w = new TrackPropertiesWidget(*producer, this);
            scrollArea->setWidget(w);
            return w;
        }
    } else if (tractor_type == producer->type()) {
        w = new TimelinePropertiesWidget(*producer, this);
        scrollArea->setWidget(w);
        return w;
    }
    if (w) {
        dynamic_cast<AbstractProducerWidget*>(w)->setProducer(producer);
        if (-1 != w->metaObject()->indexOfSignal("producerChanged(Mlt::Producer*)")) {
            connect(w, SIGNAL(producerChanged(Mlt::Producer*)), SLOT(onProducerChanged()));
            connect(w, SIGNAL(producerChanged(Mlt::Producer*)), m_filterController, SLOT(setProducer(Mlt::Producer*)));
            if (producer->get(kMultitrackItemProperty))
                connect(w, SIGNAL(producerChanged(Mlt::Producer*)), m_timelineDock, SLOT(onProducerChanged(Mlt::Producer*)));
        }
        if (-1 != w->metaObject()->indexOfSignal("modified()")) {
            connect(w, SIGNAL(modified()), SLOT(onProducerModified()));
        }
        scrollArea->setWidget(w);
        onProducerChanged();
    } else if (scrollArea->widget()) {
        scrollArea->widget()->deleteLater();
    }
    return w;
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
    LOG_WARNING() << "";
    QMessageBox::critical(this, qApp->applicationName(),
        tr("GPU effects are not supported"));
}

void MainWindow::editHTML(const QString &fileName)
{
    bool isNew = !m_htmlEditor;
    if (isNew) {
        m_htmlEditor.reset(new HtmlEditor);
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

void MainWindow::setInToCurrent(bool ripple)
{
    if (m_player->tabIndex() == Player::ProjectTabIndex && isMultitrackValid()) {
        m_timelineDock->trimClipAtPlayhead(TimelineDock::TrimInPoint, ripple);
    } else if (MLT.isSeekable() && MLT.isClip()) {
        m_player->setIn(m_player->position());
        int delta = m_player->position() - MLT.producer()->get_in();
        emit m_player->inChanged(delta);
    }
}

void MainWindow::setOutToCurrent(bool ripple)
{
    if (m_player->tabIndex() == Player::ProjectTabIndex && isMultitrackValid()) {
        m_timelineDock->trimClipAtPlayhead(TimelineDock::TrimOutPoint, ripple);
    } else if (MLT.isSeekable() && MLT.isClip()) {
        m_player->setOut(m_player->position());
        int delta = m_player->position() - MLT.producer()->get_out();
        emit m_player->outChanged(delta);
    }
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

void MainWindow::showUpgradePrompt()
{
    QAction* action = new QAction(tr("Click here to check for a new version of Shotcut."), 0);
    connect(action, SIGNAL(triggered(bool)), SLOT(on_actionUpgrade_triggered()));
    showStatusMessage(action, 15 /* seconds */);
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

void MainWindow::changeAudioChannels(bool checked, int channels)
{
    if( checked ) {
        Settings.setPlayerAudioChannels(channels);
        setAudioChannels(Settings.playerAudioChannels());
    }
}

void MainWindow::on_actionChannels1_triggered(bool checked)
{
    changeAudioChannels(checked, 1);
}

void MainWindow::on_actionChannels2_triggered(bool checked)
{
    changeAudioChannels(checked, 2);
}

void MainWindow::on_actionChannels6_triggered(bool checked)
{
    changeAudioChannels(checked, 6);
}

void MainWindow::changeDeinterlacer(bool checked, const char* method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("deinterlace_method", method);
        if (MLT.consumer()) {
            MLT.consumer()->set("deinterlace_method", method);
            MLT.refreshConsumer();
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
            MLT.refreshConsumer();
        }
    }
    Settings.setPlayerInterpolation(method);
}

void AppendTask::run()
{
    foreach (QString filename, filenames) {
        LOG_DEBUG() << filename;
        Mlt::Producer p(MLT.profile(), filename.toUtf8().constData());
        if (p.is_valid()) {
            // Convert avformat to avformat-novalidate so that XML loads faster.
            if (!qstrcmp(p.get("mlt_service"), "avformat")) {
                p.set("mlt_service", "avformat-novalidate");
                p.set("mute_on_pause", 0);
            }
            if (QDir::toNativeSeparators(filename) == QDir::toNativeSeparators(MAIN.fileName())) {
                MAIN.showStatusMessage(QObject::tr("You cannot add a project to itself!"));
                continue;
            }
            MLT.setImageDurationFromDefault(&p);
            MAIN.getHash(p);
            emit appendToPlaylist(MLT.XML(&p));
        }
    }
    emit done();
}

void MainWindow::processMultipleFiles()
{
    if (m_multipleFiles.length() > 1) {
        m_playlistDock->show();
        m_playlistDock->raise();
        AppendTask* task = new AppendTask(m_multipleFiles);
        connect(task, SIGNAL(appendToPlaylist(QString)), SLOT(onAppendToPlaylist(QString)));
        connect(task, SIGNAL(done()), SLOT(onAppendTaskDone()));
        QThreadPool::globalInstance()->start(task, 9);
        foreach (QString filename, m_multipleFiles)
            m_recentDock->add(filename.toUtf8().constData());
    }
    m_multipleFiles.clear();
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
        if (ui->actionJack)
            ui->actionJack->setChecked(false);
        Settings.setPlayerJACK(false);
        QMessageBox::warning(this, qApp->applicationName(),
            tr("Failed to connect to JACK.\nPlease verify that JACK is installed and running."));
    }
}

void MainWindow::on_actionGPU_triggered(bool checked)
{
    if (checked) {
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("GPU effects are experimental and may cause instability on some systems. "
                              "Some CPU effects are incompatible with GPU effects and will be disabled. "
                              "A project created with GPU effects can not be converted to a CPU only project later."
                              "\n\n"
                              "Do you want to enable GPU effects and restart Shotcut?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Yes) {
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        else {
            ui->actionGPU->setChecked(false);
        }
    }
    else
    {
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("Shotcut must restart to disable GPU effects."
                              "\n\n"
                              "Disable GPU effects and restart?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Yes) {
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        else {
            ui->actionGPU->setChecked(true);
        }
    }
}

void MainWindow::onExternalTriggered(QAction *action)
{
    LOG_DEBUG() << action->data().toString();
    bool isExternal = !action->data().toString().isEmpty();
    Settings.setPlayerExternal(action->data().toString());

    bool ok = false;
    int screen = action->data().toInt(&ok);
    if (ok || action->data().toString().isEmpty()) {
        m_player->moveVideoToScreen(ok? screen : -2);
        isExternal = false;
        MLT.videoWidget()->setProperty("mlt_service", QVariant());
    } else {
        m_player->moveVideoToScreen(-2);
        MLT.videoWidget()->setProperty("mlt_service", action->data());
    }

    QString profile = Settings.playerProfile();
    // Automatic not permitted for SDI/HDMI
    if (isExternal && profile.isEmpty()) {
        profile = "atsc_720p_50";
        Settings.setPlayerProfile(profile);
        setProfile(profile);
        MLT.restart();
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
    LOG_DEBUG() << action->data().toString();
    MLT.videoWidget()->setProperty("keyer", action->data());
    MLT.consumerChanged();
    Settings.setPlayerKeyerMode(action->data().toInt());
}

void MainWindow::onProfileTriggered(QAction *action)
{
    Settings.setPlayerProfile(action->data().toString());
    if (MLT.producer() && MLT.producer()->is_valid()) {
        // Save the XML to get correct in/out points before profile is changed.
        QString xml = MLT.XML();
        setProfile(action->data().toString());
        MLT.restart(xml);
    } else {
        setProfile(action->data().toString());
    }
}

void MainWindow::onProfileChanged()
{
    if (multitrack() && MLT.isMultitrack() &&
       (m_timelineDock->selection().isEmpty() || m_timelineDock->currentTrack() == -1)) {
        emit m_timelineDock->selected(multitrack());
    }
}

void MainWindow::on_actionAddCustomProfile_triggered()
{
    QString xml;
    if (MLT.producer() && MLT.producer()->is_valid()) {
        // Save the XML to get correct in/out points before profile is changed.
        xml = MLT.XML();
    }
    CustomProfileDialog dialog(this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.profileName();
        if (!name.isEmpty()) {
            // Add new profile to the menu.
            QDir dir(Settings.appDataLocation());
            if (dir.cd("profiles")) {
                QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
                if (profiles.length() == 1) {
                    m_customProfileMenu->addAction(ui->actionProfileRemove);
                    m_customProfileMenu->addSeparator();
                }
                QAction* action = addProfile(m_profileGroup, name, dir.filePath(name));
                action->setChecked(true);
                m_customProfileMenu->addAction(action);
                Settings.setPlayerProfile(dir.filePath(name));
                Settings.sync();
            }
        } else if (m_profileGroup->checkedAction()) {
            m_profileGroup->checkedAction()->setChecked(false);
        }
        // Use the new profile.
        emit profileChanged();
        if (!xml.isEmpty())
            MLT.restart(xml);
    }
}

void MainWindow::on_actionSystemTheme_triggered()
{
    changeTheme("system");
    QApplication::setPalette(QApplication::style()->standardPalette());
    Settings.setTheme("system");
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#endif
}

void MainWindow::on_actionFusionDark_triggered()
{
    changeTheme("dark");
    Settings.setTheme("dark");
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::on_actionFusionLight_triggered()
{
    changeTheme("light");
    Settings.setTheme("light");
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::on_actionTutorials_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.shotcut.org/tutorials/"));
}

void MainWindow::on_actionRestoreLayout_triggered()
{
    restoreGeometry(Settings.windowGeometryDefault());
    restoreState(Settings.windowStateDefault());
    on_actionLayoutTimeline_triggered();
    ui->actionShowTitleBars->setChecked(true);
    on_actionShowTitleBars_triggered(true);
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
    showStatusMessage("Checking for upgrade...");
    m_network.get(QNetworkRequest(QUrl("http://check.shotcut.org/version.json")));
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
            LOG_INFO() << url;
        }
        else {
            showStatusMessage(tr("Failed to open ") + url);
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

void MainWindow::onFocusChanged(QWidget *, QWidget * ) const
{
    LOG_DEBUG() << "Focuswidget changed";
    LOG_DEBUG() << "Current focusWidget:" << QApplication::focusWidget();
    LOG_DEBUG() << "Current focusObject:" << QApplication::focusObject();
    LOG_DEBUG() << "Current focusWindow:" << QApplication::focusWindow();
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

void MainWindow::on_actionApplicationLog_triggered()
{
    TextViewerDialog dialog(this);
    QDir dir = Settings.appDataLocation();
    QFile logFile(dir.filePath("shotcut-log.txt"));
    logFile.open(QIODevice::ReadOnly | QIODevice::Text);
    dialog.setText(logFile.readAll());
    logFile.close();
    dialog.setWindowTitle(tr("Application Log"));
    dialog.exec();
}

void MainWindow::on_actionClose_triggered()
{
    if (MAIN.continueModified()) {
        LOG_DEBUG() << "";
        if (multitrack())
            m_timelineDock->model()->close();
        if (playlist())
            m_playlistDock->model()->close();
        else
            onMultitrackClosed();
    }
}

void MainWindow::onPlayerTabIndexChanged(int index)
{
    if (Player::SourceTabIndex == index)
        m_timelineDock->saveAndClearSelection();
    else
        m_timelineDock->restoreSelection();
}

void MainWindow::onUpgradeCheckFinished(QNetworkReply* reply)
{
    if (!reply->error()) {
        QByteArray response = reply->readAll();
        LOG_DEBUG() << "response: " << response;
        QJsonDocument json = QJsonDocument::fromJson(response);
        if (!json.isNull() && json.object().value("version_string").type() == QJsonValue::String) {
            QString version = json.object().value("version_string").toString();
            if (version != qApp->applicationVersion()) {
                QAction* action = new QAction(tr("Shotcut version %1 is available! Click here to get it.").arg(version), 0);
                connect(action, SIGNAL(triggered(bool)), SLOT(onUpgradeTriggered()));
                if (!json.object().value("url").isUndefined())
                    m_upgradeUrl = json.object().value("url").toString();
                showStatusMessage(action, 10 /* seconds */);
            } else {
                showStatusMessage(tr("You are running the latest version of Shotcut."));
            }
            reply->deleteLater();
            return;
        } else {
            LOG_WARNING() << "failed to parse version.json";
        }
    } else {
        LOG_WARNING() << reply->errorString();
    }
    QAction* action = new QAction(tr("Failed to read version.json when checking. Click here to go to the Web site."), 0);
    connect(action, SIGNAL(triggered(bool)), SLOT(onUpgradeTriggered()));
    showStatusMessage(action);
    reply->deleteLater();
}

void MainWindow::onUpgradeTriggered()
{
    QDesktopServices::openUrl(QUrl(m_upgradeUrl));
}

void MainWindow::onTimelineSelectionChanged()
{
    bool enable = (m_timelineDock->selection().size() > 0);
    ui->actionCut->setEnabled(enable);
    ui->actionCopy->setEnabled(enable);
}

void MainWindow::on_actionCut_triggered()
{
    m_timelineDock->show();
    m_timelineDock->raise();
    m_timelineDock->removeSelection(true);
}

void MainWindow::on_actionCopy_triggered()
{
    m_timelineDock->show();
    m_timelineDock->raise();
    if (!m_timelineDock->selection().isEmpty())
        m_timelineDock->copyClip(m_timelineDock->currentTrack(), m_timelineDock->selection().first());
}

void MainWindow::on_actionPaste_triggered()
{
    m_timelineDock->show();
    m_timelineDock->raise();
    m_timelineDock->insert(-1);
}

void MainWindow::onClipCopied()
{
    m_player->enableTab(Player::SourceTabIndex);
}

void MainWindow::on_actionExportEDL_triggered()
{
    // Dialog to get export file name.
    QString path = Settings.savePath();
    path.append("/.edl");
    QString caption = tr("Export EDL");
    QString saveFileName = QFileDialog::getSaveFileName(this, caption, path, tr("EDL (*.edl)"));
    if (!saveFileName.isEmpty()) {
        QFileInfo fi(saveFileName);
        if (fi.suffix() != "edl")
            saveFileName += ".edl";

        if (Util::warnIfNotWritable(saveFileName, this, caption))
            return;

        // Locate the JavaScript file in the filesystem.
        QDir qmlDir = QmlUtilities::qmlDir();
        qmlDir.cd("export-edl");
        QString jsFileName = qmlDir.absoluteFilePath("export-edl.js");
        QFile scriptFile(jsFileName);
        if (scriptFile.open(QIODevice::ReadOnly)) {
            // Read JavaScript into a string.
            QTextStream stream(&scriptFile);
            stream.setCodec("UTF-8");
            stream.setAutoDetectUnicode(true);
            QString contents = stream.readAll();
            scriptFile.close();

            // Evaluate JavaScript.
            QJSEngine jsEngine;
            QJSValue result = jsEngine.evaluate(contents, jsFileName);
            if (!result.isError()) {
                // Call the JavaScript main function.
                QJSValue options = jsEngine.newObject();
                options.setProperty("useBaseNameForReelName", true);
                options.setProperty("useBaseNameForClipComment", true);
                options.setProperty("channelsAV", "AA/V");
                QJSValueList args;
                args << MLT.XML(0, true, true) << options;
                result = result.call(args);
                if (!result.isError()) {
                    // Save the result with the export file name.
                    QFile f(saveFileName);
                    f.open(QIODevice::WriteOnly | QIODevice::Text);
                    f.write(result.toString().toLatin1());
                    f.close();
                }
            }
            if (result.isError()) {
                LOG_ERROR() << "Uncaught exception at line"
                            << result.property("lineNumber").toInt()
                            << ":" << result.toString();
                showStatusMessage(tr("A JavaScript error occurred during export."));
            }
        } else {
            showStatusMessage(tr("Failed to open export-edl.js"));
        }
    }
}

void MainWindow::on_actionExportFrame_triggered()
{
    if (Settings.playerGPU()) {
        Mlt::GLWidget* glw = qobject_cast<Mlt::GLWidget*>(MLT.videoWidget());
        connect(glw, SIGNAL(imageReady()), SLOT(onGLWidgetImageReady()));
        glw->requestImage();
        MLT.refreshConsumer();
    } else {
        onGLWidgetImageReady();
    }
}

void MainWindow::onGLWidgetImageReady()
{
    Mlt::GLWidget* glw = qobject_cast<Mlt::GLWidget*>(MLT.videoWidget());
    QImage image = glw->image();
    if (Settings.playerGPU())
        disconnect(glw, SIGNAL(imageReady()), this, 0);
    if (!image.isNull()) {
        QString path = Settings.savePath();
        path.append("/.png");
        QString caption = tr("Export Frame");
        QString saveFileName = QFileDialog::getSaveFileName(this, caption, path);
        if (!saveFileName.isEmpty()) {
            QFileInfo fi(saveFileName);
            if (fi.suffix().isEmpty())
                saveFileName += ".png";
            if (Util::warnIfNotWritable(saveFileName, this, caption))
                return;
            image.save(saveFileName);
            Settings.setSavePath(fi.path());
            m_recentDock->add(saveFileName);
        }
    } else {
        showStatusMessage(tr("Unable to export frame."));
    }
}

void MainWindow::on_actionAppDataSet_triggered()
{
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart Shotcut to change the data directory.\n"
                          "Do you want to continue?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() != QMessageBox::Yes) return;

    QString dirName = QFileDialog::getExistingDirectory(this, tr("Data Directory"), Settings.appDataLocation());
    if (!dirName.isEmpty()) {
        // Move the data files.
        QDirIterator it(Settings.appDataLocation());
        while (it.hasNext()) {
            if (!it.filePath().isEmpty() && it.fileName() != "." && it.fileName() != "..") {
                if (!QFile::exists(dirName + "/" + it.fileName())) {
                    if (it.fileInfo().isDir()) {
                        if (!QFile::rename(it.filePath(), dirName + "/" + it.fileName()))
                            LOG_WARNING() << "Failed to move" << it.filePath() << "to" << dirName + "/" + it.fileName();
                    } else {
                        if (!QFile::copy(it.filePath(), dirName + "/" + it.fileName()))
                            LOG_WARNING() << "Failed to copy" << it.filePath() << "to" << dirName + "/" + it.fileName();
                    }
                }
            }
            it.next();
        }
        writeSettings();
        Settings.setAppDataLocally(dirName);

        m_exitCode = EXIT_RESTART;
        QApplication::closeAllWindows();
    }
}

void MainWindow::on_actionAppDataShow_triggered()
{
    Util::showInFolder(Settings.appDataLocation());
}

void MainWindow::on_actionNew_triggered()
{
    on_actionClose_triggered();
}

void MainWindow::on_actionKeyboardShortcuts_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.shotcut.org/howtos/keyboard-shortcuts/"));
}

void MainWindow::on_actionLayoutPlayer_triggered()
{
    restoreState(Settings.windowStateDefault());
}

void MainWindow::on_actionLayoutPlaylist_triggered()
{
    restoreState(Settings.windowStateDefault());
    m_recentDock->show();
    m_recentDock->raise();
    m_playlistDock->show();
    m_playlistDock->raise();
}

void MainWindow::on_actionLayoutTimeline_triggered()
{
    restoreState(Settings.windowStateDefault());
    QDockWidget* audioMeterDock = findChild<QDockWidget*>("AudioPeakMeterDock");
    if (audioMeterDock) {
        audioMeterDock->show();
        audioMeterDock->raise();
    }
    m_recentDock->show();
    m_recentDock->raise();
    m_filtersDock->show();
    m_filtersDock->raise();
    m_timelineDock->show();
    m_timelineDock->raise();
}

void MainWindow::on_actionLayoutClip_triggered()
{
    restoreState(Settings.windowStateDefault());
    m_recentDock->show();
    m_recentDock->raise();
    m_filtersDock->show();
    m_filtersDock->raise();
}

void MainWindow::on_actionLayoutAdd_triggered()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Custom Layout"),
                                         tr("Name"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !name.isEmpty()) {
        if (Settings.setLayout(name, saveGeometry(), saveState())) {
            Settings.sync();
            if (Settings.layouts().size() == 1) {
                ui->menuLayout->addAction(ui->actionLayoutRemove);
                ui->menuLayout->addSeparator();
            }
            ui->menuLayout->addAction(addLayout(m_layoutGroup, name));
        }
    }
}

void MainWindow::onLayoutTriggered(QAction* action)
{
    restoreGeometry(Settings.layoutGeometry(action->text()));
    restoreState(Settings.layoutState(action->text()));
}

void MainWindow::on_actionProfileRemove_triggered()
{
    QDir dir(Settings.appDataLocation());
    if (dir.cd("profiles")) {
        // Setup the dialog.
        QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        ListSelectionDialog dialog(profiles, this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setWindowTitle(tr("Remove Video Mode"));

        // Show the dialog.
        if (dialog.exec() == QDialog::Accepted) {
            foreach(const QString& profile, dialog.selection()) {
                // Remove the file.
                dir.remove(profile);
                // Locate the menu item.
                foreach (QAction* a, m_customProfileMenu->actions()) {
                    if (a->text() == profile) {
                        // Remove the menu item.
                        delete a;
                        break;
                    }
                }
            }
            // If no more custom video modes.
            if (m_customProfileMenu->actions().size() == 3) {
                // Remove the Remove action and separator.
                m_customProfileMenu->removeAction(ui->actionProfileRemove);
                foreach (QAction* a, m_customProfileMenu->actions()) {
                    if (a->isSeparator()) {
                        delete a;
                        break;
                    }
                }
            }
        }
    }
}

void MainWindow::on_actionLayoutRemove_triggered()
{
    // Setup the dialog.
    ListSelectionDialog dialog(Settings.layouts(), this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setWindowTitle(tr("Remove Layout"));

    // Show the dialog.
    if (dialog.exec() == QDialog::Accepted) {
        foreach(const QString& layout, dialog.selection()) {
            // Update the configuration.
            if (Settings.removeLayout(layout))
                Settings.sync();
            // Locate the menu item.
            foreach (QAction* action, ui->menuLayout->actions()) {
                if (action->text() == layout) {
                    // Remove the menu item.
                    delete action;
                    break;
                }
            }
        }
        // If no more custom layouts.
        if (Settings.layouts().size() == 0) {
            // Remove the Remove action and separator.
            ui->menuLayout->removeAction(ui->actionLayoutRemove);
            bool isSecondSeparator = false;
            foreach (QAction* action, ui->menuLayout->actions()) {
                if (action->isSeparator()) {
                    if (isSecondSeparator) {
                        delete action;
                        break;
                    } else {
                        isSecondSeparator = true;
                    }
                }
            }
        }
    }
}

void MainWindow::onAppendToPlaylist(const QString& xml)
{
    MAIN.undoStack()->push(new Playlist::AppendCommand(*m_playlistDock->model(), xml, false));
}

void MainWindow::onAppendTaskDone()
{
    qApp->processEvents();
    emit m_playlistDock->model()->modified();
}

void MainWindow::on_actionOpenOther2_triggered()
{
    ui->actionOpenOther2->menu()->popup(mapToGlobal(ui->mainToolBar->geometry().bottomLeft()) + QPoint(64, 0));
}

void MainWindow::onOpenOtherTriggered(QWidget* widget)
{
    QDialog dialog(this);
    QVBoxLayout vlayout(&dialog);
    vlayout.addWidget(widget);
    QDialogButtonBox buttonBox(&dialog);
    buttonBox.setOrientation(Qt::Horizontal);
    buttonBox.setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    vlayout.addWidget(&buttonBox);
    connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    QString name = widget->objectName();
    if (name == "NoiseWidget" || dialog.exec() == QDialog::Accepted) {
        open(dynamic_cast<AbstractProducerWidget*>(widget)->newProducer(MLT.profile()));
        if (name == "TextProducerWidget") {
            m_filtersDock->show();
            m_filtersDock->raise();
        } else {
            m_propertiesDock->show();
            m_propertiesDock->raise();
        }
    }
    delete widget;
}

void MainWindow::onOpenOtherColor()
{
    onOpenOtherTriggered(new ColorProducerWidget(this));
}

void MainWindow::onOpenOtherText()
{
    onOpenOtherTriggered(new TextProducerWidget(this));
}

void MainWindow::onOpenOtherNoise()
{
    onOpenOtherTriggered(new NoiseWidget(this));
}

void MainWindow::onOpenOtherIsing()
{
    onOpenOtherTriggered(new IsingWidget(this));
}

void MainWindow::onOpenOtherLissajous()
{
    onOpenOtherTriggered(new LissajousWidget(this));
}

void MainWindow::onOpenOtherPlasma()
{
    onOpenOtherTriggered(new PlasmaWidget(this));
}

void MainWindow::onOpenOtherColorBars()
{
    onOpenOtherTriggered(new ColorBarsWidget(this));
}

void MainWindow::onOpenOtherTone()
{
    onOpenOtherTriggered(new ToneProducerWidget(this));
}

void MainWindow::onOpenOtherCount()
{
    onOpenOtherTriggered(new CountProducerWidget(this));
}

void MainWindow::onOpenOtherV4L2()
{
    onOpenOtherTriggered(new Video4LinuxWidget(this));
}

void MainWindow::onOpenOtherPulse()
{
    onOpenOtherTriggered(new PulseAudioWidget(this));
}

void MainWindow::onOpenOtherJack()
{
    onOpenOtherTriggered(new JackProducerWidget(this));
}

void MainWindow::onOpenOtherAlsa()
{
    onOpenOtherTriggered(new AlsaWidget(this));
}

void MainWindow::onOpenOtherDevice()
{
#if defined(Q_OS_MAC)
    onOpenOtherTriggered(new AvfoundationProducerWidget(this));
#elif defined(Q_OS_WIN)
    onOpenOtherTriggered(new DirectShowVideoWidget(this));
#endif
}

void MainWindow::onOpenOtherDecklink()
{
    onOpenOtherTriggered(new DecklinkProducerWidget(this));
}
