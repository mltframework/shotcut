/*
 * Copyright (c) 2011-2025 Meltytech, LLC
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

#include "Logger.h"
#include "actions.h"
#include "autosavefile.h"
#include "commands/playlistcommands.h"
#include "controllers/filtercontroller.h"
#include "controllers/scopecontroller.h"
#include "database.h"
#include "defaultlayouts.h"
#include "dialogs/actionsdialog.h"
#include "dialogs/customprofiledialog.h"
#include "dialogs/listselectiondialog.h"
#include "dialogs/longuitask.h"
#include "dialogs/resourcedialog.h"
#include "dialogs/saveimagedialog.h"
#include "dialogs/systemsyncdialog.h"
#include "dialogs/textviewerdialog.h"
#include "dialogs/unlinkedfilesdialog.h"
#include "docks/encodedock.h"
#include "docks/filesdock.h"
#include "docks/filtersdock.h"
#include "docks/findanalysisfilterparser.h"
#include "docks/jobsdock.h"
#include "docks/keyframesdock.h"
#include "docks/markersdock.h"
#include "docks/notesdock.h"
#include "docks/playlistdock.h"
#include "docks/recentdock.h"
#include "docks/subtitlesdock.h"
#include "docks/timelinedock.h"
#include "jobqueue.h"
#include "jobs/screencapturejob.h"
#include "models/audiolevelstask.h"
#include "models/keyframesmodel.h"
#include "models/motiontrackermodel.h"
#include "openotherdialog.h"
#include "player.h"
#include "proxymanager.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlprofile.h"
#include "qmltypes/qmlutilities.h"
#include "screencapture/screencapture.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "videowidget.h"
#include "widgets/alsawidget.h"
#include "widgets/avformatproducerwidget.h"
#include "widgets/avfoundationproducerwidget.h"
#include "widgets/blipproducerwidget.h"
#include "widgets/colorbarswidget.h"
#include "widgets/colorproducerwidget.h"
#include "widgets/countproducerwidget.h"
#include "widgets/decklinkproducerwidget.h"
#include "widgets/directshowvideowidget.h"
#include "widgets/glaxnimateproducerwidget.h"
#include "widgets/htmlgeneratorwidget.h"
#include "widgets/imageproducerwidget.h"
#include "widgets/isingwidget.h"
#include "widgets/lissajouswidget.h"
#include "widgets/lumamixtransition.h"
#include "widgets/mltclipproducerwidget.h"
#include "widgets/newprojectfolder.h"
#include "widgets/noisewidget.h"
#include "widgets/plasmawidget.h"
#include "widgets/pulseaudiowidget.h"
#include "widgets/textproducerwidget.h"
#include "widgets/timelinepropertieswidget.h"
#include "widgets/toneproducerwidget.h"
#include "widgets/trackpropertieswidget.h"
#include "widgets/video4linuxwidget.h"
#if defined(Q_OS_WIN) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include "windowstools.h"
#endif

#include <QApplication>
#include <QClipboard>
#include <QDirIterator>
#include <QFileDialog>
#include <QImageReader>
#include <QJSEngine>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QThreadPool>
#include <QTimer>
#include <QVersionNumber>
#include <QtConcurrent/QtConcurrentRun>
#include <QtNetwork>
#include <QtWidgets>

#include <algorithm>

#define SHOTCUT_THEME

static bool eventDebugCallback(void **data)
{
    QEvent *event = reinterpret_cast<QEvent *>(data[1]);
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QObject *receiver = reinterpret_cast<QObject *>(data[0]);
        LOG_DEBUG() << event << "->" << receiver;
    } else if (event->type() == QEvent::MouseButtonPress
               || event->type() == QEvent::MouseButtonRelease) {
        QObject *receiver = reinterpret_cast<QObject *>(data[0]);
        LOG_DEBUG() << event << "->" << receiver;
    }
    return false;
}

static const int AUTOSAVE_TIMEOUT_MS = 60000;
static const char *kReservedLayoutPrefix = "__%1";
static const char *kLayoutSwitcherName("layoutSwitcherGrid");
static QRegularExpression kBackupFileRegex("^(.+) "
                                           "([0-9]{4})-(1[0-2]|0[1-9])-(3[01]|0[1-9]|[12][0-9])T(2["
                                           "0-3]|[01][0-9])-([0-5][0-9])-([0-5][0-9]).mlt$");

MainWindow::MainWindow()
    : QMainWindow(0)
    , ui(new Ui::MainWindow)
    , m_isKKeyPressed(false)
    , m_keyerGroup(0)
    , m_previewScaleGroup(0)
    , m_keyerMenu(0)
    , m_multipleFilesLoading(false)
    , m_isPlaylistLoaded(false)
    , m_exitCode(EXIT_SUCCESS)
    , m_upgradeUrl("https://www.shotcut.org/download/")
    , m_keyframesDock(0)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QLibrary libSDL("libSDL2-2.0.so.0");
    if (!libSDL.load()) {
        QMessageBox::critical(
            this,
            qApp->applicationName(),
            tr("Error: This program requires the SDL 2 library.\n\nPlease install it using your "
               "package manager. It may be named libsdl2-2.0-0, SDL2, or similar."));
        ::exit(EXIT_FAILURE);
    } else {
        libSDL.unload();
    }
#endif

    connectFocusSignals();

    registerDebugCallback();

    LOG_DEBUG() << "begin";
    LOG_INFO() << "device pixel ratio =" << devicePixelRatioF();
    connect(&m_autosaveTimer, SIGNAL(timeout()), this, SLOT(onAutosaveTimeout()));
    m_autosaveTimer.start(AUTOSAVE_TIMEOUT_MS);

    // Initialize all QML types
    QmlUtilities::registerCommonTypes();

    // Create the UI.
    ui->setupUi(this);
    setDockNestingEnabled(true);
    const auto highlight = palette().highlight().color();
    setStyleSheet(QString("QMainWindow::separator {"
                          "  width: 10px;"
                          "}"
                          "QMainWindow::separator:hover {"
                          "  background-color: rgba(%1, %2, %3, 51);"
                          "}")
                      .arg(highlight.red())
                      .arg(highlight.green())
                      .arg(highlight.blue()));

    ui->statusBar->hide();

    connectUISignals();

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    setupAndConnectUndoStack();

    // Add the player widget.
    setupAndConnectPlayerWidget();

    setupSettingsMenu();
    setupOpenOtherMenu();
    readPlayerSettings();
    configureVideoWidget();

    centerLayoutInRemainingToolbarSpace();

#ifndef SHOTCUT_NOUPGRADE
    if (Settings.noUpgrade() || qApp->property("noupgrade").toBool())
#endif
        delete ui->actionUpgrade;

    setupAndConnectDocks();
    setupMenuFile();
    setupMenuView();
    connectVideoWidgetSignals();
    readWindowSettings();
    setupActions();
    setupLayoutSwitcher();

    setFocus();
    setCurrentFile("");

    connect(&m_network,
            SIGNAL(finished(QNetworkReply *)),
            SLOT(onUpgradeCheckFinished(QNetworkReply *)));
    resetSourceUpdated();
    m_clipboardUpdatedAt.setSecsSinceEpoch(0);
    onClipboardChanged();
    connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(onClipboardChanged()));

    QThreadPool::globalInstance()->setMaxThreadCount(
        qMin(4, QThreadPool::globalInstance()->maxThreadCount()));
    QThreadPool::globalInstance()->setThreadPriority(QThread::LowPriority);
    QImageReader::setAllocationLimit(1024);

    ProxyManager::removePending();

    LOG_DEBUG() << "end";
}

void MainWindow::connectFocusSignals()
{
    if (!qgetenv("OBSERVE_FOCUS").isEmpty()) {
        connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);
        connect(qApp, &QGuiApplication::focusObjectChanged, this, &MainWindow::onFocusObjectChanged);
        connect(qApp, &QGuiApplication::focusWindowChanged, this, &MainWindow::onFocusWindowChanged);
    }
}

void MainWindow::registerDebugCallback()
{
    if (!qgetenv("EVENT_DEBUG").isEmpty())
        QInternal::registerCallback(QInternal::EventNotifyCallback, eventDebugCallback);
}

void MainWindow::connectUISignals()
{
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(this, &MainWindow::producerOpened, this, &MainWindow::onProducerOpened);
    connect(ui->mainToolBar,
            SIGNAL(visibilityChanged(bool)),
            SLOT(onToolbarVisibilityChanged(bool)));
    ui->actionSave->setEnabled(false);
    connect(this, &MainWindow::audioChannelsChanged, this, &MainWindow::updateWindowTitle);
    connect(this, &MainWindow::producerOpened, this, &MainWindow::updateWindowTitle);
    connect(this, &MainWindow::profileChanged, this, &MainWindow::updateWindowTitle);
}

void MainWindow::setupAndConnectUndoStack()
{
    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(Settings.undoLimit());
    QAction *undoAction = m_undoStack->createUndoAction(this);
    QAction *redoAction = m_undoStack->createRedoAction(this);
    undoAction->setIcon(
        QIcon::fromTheme("edit-undo", QIcon(":/icons/oxygen/32x32/actions/edit-undo.png")));
    redoAction->setIcon(
        QIcon::fromTheme("edit-redo", QIcon(":/icons/oxygen/32x32/actions/edit-redo.png")));
    undoAction->setShortcut(QString::fromLatin1("Ctrl+Z"));
#ifdef Q_OS_WIN
    redoAction->setShortcut(QString::fromLatin1("Ctrl+Y"));
#else
    redoAction->setShortcut(QString::fromLatin1("Ctrl+Shift+Z"));
#endif
    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);
    ui->menuEdit->addSeparator();
    ui->actionUndo->setIcon(undoAction->icon());
    ui->actionRedo->setIcon(redoAction->icon());
    ui->actionUndo->setToolTip(undoAction->toolTip());
    ui->actionRedo->setToolTip(redoAction->toolTip());
    connect(m_undoStack, SIGNAL(canUndoChanged(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect(m_undoStack, SIGNAL(canRedoChanged(bool)), ui->actionRedo, SLOT(setEnabled(bool)));
}

void MainWindow::setupAndConnectPlayerWidget()
{
    m_player = new Player;
    MLT.videoWidget()->installEventFilter(this);
    ui->centralWidget->layout()->addWidget(m_player);
    connect(this, &MainWindow::producerOpened, m_player, &Player::onProducerOpened);
    connect(m_player, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_player, SIGNAL(inChanged(int)), this, SLOT(onCutModified()));
    connect(m_player, SIGNAL(outChanged(int)), this, SLOT(onCutModified()));
    connect(m_player, SIGNAL(tabIndexChanged(int)), SLOT(onPlayerTabIndexChanged(int)));
    connect(MLT.videoWidget(), SIGNAL(started()), SLOT(processMultipleFiles()));
    connect(MLT.videoWidget(), SIGNAL(started()), SLOT(processSingleFile()));
    connect(MLT.videoWidget(), SIGNAL(paused()), m_player, SLOT(showPaused()));
    connect(MLT.videoWidget(), SIGNAL(playing()), m_player, SLOT(showPlaying()));
    connect(MLT.videoWidget(), SIGNAL(toggleZoom(bool)), m_player, SLOT(toggleZoom(bool)));
    ui->menuPlayer->addAction(Actions["playerPlayPauseAction"]);
    ui->menuPlayer->addAction(Actions["playerLoopAction"]);
    QMenu *loopRangeMenu = new QMenu(tr("Set Loop Range"), this);
    loopRangeMenu->addAction(Actions["playerLoopRangeAllAction"]);
    loopRangeMenu->addAction(Actions["playerLoopRangeMarkerAction"]);
    loopRangeMenu->addAction(Actions["playerLoopRangeSelectionAction"]);
    loopRangeMenu->addAction(Actions["playerLoopRangeAroundAction"]);
    ui->menuPlayer->addMenu(loopRangeMenu);
    ui->menuPlayer->addAction(Actions["playerFastForwardAction"]);
    ui->menuPlayer->addAction(Actions["playerRewindAction"]);
    ui->menuPlayer->addAction(Actions["playerSkipNextAction"]);
    ui->menuPlayer->addAction(Actions["playerSkipPreviousAction"]);
    ui->menuPlayer->addAction(Actions["playerSeekStartAction"]);
    ui->menuPlayer->addAction(Actions["playerSeekEndAction"]);
    ui->menuPlayer->addAction(Actions["playerNextFrameAction"]);
    ui->menuPlayer->addAction(Actions["playerPreviousFrameAction"]);
    ui->menuPlayer->addAction(Actions["playerForwardOneSecondAction"]);
    ui->menuPlayer->addAction(Actions["playerBackwardOneSecondAction"]);
    ui->menuPlayer->addAction(Actions["playerForwardTwoSecondsAction"]);
    ui->menuPlayer->addAction(Actions["playerBackwardTwoAction"]);
    ui->menuPlayer->addAction(Actions["playerForwardFiveSecondsAction"]);
    ui->menuPlayer->addAction(Actions["playerBackwardFiveSecondsAction"]);
    ui->menuPlayer->addAction(Actions["playerForwardTenSecondsAction"]);
    ui->menuPlayer->addAction(Actions["playerBackwardTenSecondsAction"]);
    ui->menuPlayer->addAction(Actions["playerForwardJumpAction"]);
    ui->menuPlayer->addAction(Actions["playerBackwardJumpAction"]);
    ui->menuPlayer->addAction(Actions["playerSetJumpAction"]);
    ui->menuPlayer->addAction(Actions["playerSetInAction"]);
    ui->menuPlayer->addAction(Actions["playerSetOutAction"]);
    ui->menuPlayer->addAction(Actions["playerSetPositionAction"]);
    ui->menuPlayer->addAction(Actions["playerToggleVui"]);
    ui->menuPlayer->addAction(Actions["playerSwitchSourceProgramAction"]);
}

void MainWindow::setupLayoutSwitcher()
{
    auto group = new QActionGroup(this);
    group->addAction(ui->actionLayoutLogging);
    group->addAction(ui->actionLayoutEditing);
    group->addAction(ui->actionLayoutEffects);
    group->addAction(ui->actionLayoutAudio);
    group->addAction(ui->actionLayoutColor);
    group->addAction(ui->actionLayoutPlayer);
    switch (Settings.layoutMode()) {
    case LayoutMode::Custom:
        break;
    case LayoutMode::Logging:
        ui->actionLayoutLogging->setChecked(true);
        break;
    case LayoutMode::Editing:
        ui->actionLayoutEditing->setChecked(true);
        break;
    case LayoutMode::Effects:
        ui->actionLayoutEffects->setChecked(true);
        break;
    case LayoutMode::Color:
        ui->actionLayoutColor->setChecked(true);
        filterController()->metadataModel()->setFilter(MetadataModel::VideoFilter);
        filterController()->metadataModel()->setSearch("#color");
        break;
    case LayoutMode::Audio:
        ui->actionLayoutAudio->setChecked(true);
        filterController()->metadataModel()->setFilter(MetadataModel::AudioFilter);
        break;
    case LayoutMode::PlayerOnly:
        ui->actionLayoutPlayer->setChecked(true);
        break;
    default:
        ui->actionLayoutEditing->setChecked(true);
        break;
    }
}

void MainWindow::centerLayoutInRemainingToolbarSpace()
{
    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->insertWidget(ui->dummyAction, spacer);
    spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->addWidget(spacer);
    updateLayoutSwitcher();
}
void MainWindow::setupAndConnectDocks()
{
    m_scopeController = new ScopeController(this, ui->menuView);
    QDockWidget *audioMeterDock = findChild<QDockWidget *>("AudioPeakMeterDock");
    if (audioMeterDock) {
        audioMeterDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
        connect(ui->actionAudioMeter,
                SIGNAL(triggered()),
                audioMeterDock->toggleViewAction(),
                SLOT(trigger()));
    }

    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->hide();
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setWindowIcon(ui->actionProperties->icon());
    m_propertiesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    m_propertiesDock->toggleViewAction()->setIcon(ui->actionProperties->icon());
    m_propertiesDock->setMinimumWidth(300);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    m_propertiesDock->setWidget(scroll);
    ui->menuView->addAction(m_propertiesDock->toggleViewAction());
    connect(m_propertiesDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onPropertiesDockTriggered(bool)));
    connect(ui->actionProperties, SIGNAL(triggered()), this, SLOT(onPropertiesDockTriggered()));

    m_recentDock = new RecentDock(this);
    m_recentDock->hide();
    m_recentDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    ui->menuView->addAction(m_recentDock->toggleViewAction());
    connect(m_recentDock, SIGNAL(itemActivated(QString)), this, SLOT(open(QString)));
    connect(m_recentDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onRecentDockTriggered(bool)));
    connect(ui->actionRecent, SIGNAL(triggered()), this, SLOT(onRecentDockTriggered()));
    connect(this, SIGNAL(openFailed(QString)), m_recentDock, SLOT(remove(QString)));
    connect(m_recentDock,
            &RecentDock::deleted,
            m_player->projectWidget(),
            &NewProjectFolder::updateRecentProjects);

    m_playlistDock = new PlaylistDock(this);
    m_playlistDock->hide();
    m_playlistDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    ui->menuView->addAction(m_playlistDock->toggleViewAction());
    connect(m_playlistDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onPlaylistDockTriggered(bool)));
    connect(ui->actionPlaylist, SIGNAL(triggered()), this, SLOT(onPlaylistDockTriggered()));
    connect(m_playlistDock,
            SIGNAL(clipOpened(Mlt::Producer *, bool)),
            this,
            SLOT(openCut(Mlt::Producer *, bool)));
    connect(m_playlistDock, SIGNAL(itemActivated(int)), this, SLOT(seekPlaylist(int)));
    connect(m_playlistDock,
            SIGNAL(showStatusMessage(QString)),
            this,
            SLOT(showStatusMessage(QString)));
    connect(m_playlistDock->model(), SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(m_playlistDock->model(), SIGNAL(closed()), this, SLOT(onPlaylistClosed()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(this, SIGNAL(producerOpened()), m_playlistDock, SLOT(onProducerOpened()));
    if (!Settings.playerGPU())
        connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(updateThumbnails()));
    connect(m_player, &Player::inChanged, m_playlistDock, &PlaylistDock::onInChanged);
    connect(m_player, &Player::outChanged, m_playlistDock, &PlaylistDock::onOutChanged);
    connect(m_playlistDock->model(),
            &PlaylistModel::inChanged,
            this,
            &MainWindow::onPlaylistInChanged);
    connect(m_playlistDock->model(),
            &PlaylistModel::outChanged,
            this,
            &MainWindow::onPlaylistOutChanged);
    QMenu *viewModeMenu = ui->menuPlaylist->addMenu(tr("View Mode"));
    viewModeMenu->addAction(Actions["playlistViewDetailsAction"]);
    viewModeMenu->addAction(Actions["playlistViewTilesAction"]);
    viewModeMenu->addAction(Actions["playlistViewIconsAction"]);
    QMenu *subMenu = ui->menuPlaylist->addMenu(tr("Thumbnails"));
    subMenu->addAction(Actions["playlistThumbnailsHiddenAction"]);
    subMenu->addAction(Actions["playlistThumbnailsLeftAndRightAction"]);
    subMenu->addAction(Actions["playlistThumbnailsTopAndBottomAction"]);
    subMenu->addAction(Actions["playlistThumbnailsInOnlySmallAction"]);
    subMenu->addAction(Actions["playlistThumbnailsInOnlyLargeAction"]);
    ui->menuPlaylist->addAction(Actions["playlistPlayAfterOpenAction"]);

    m_filesDock = new FilesDock(this);
    m_filesDock->hide();
    m_filesDock->toggleViewAction()->setShortcuts(
        {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_4), QKeySequence(Qt::Key_F10)});
    ui->menuView->addAction(m_filesDock->toggleViewAction());
    // connect(m_filesDock, SIGNAL(itemActivated(QString)), this, SLOT(open(QString)));
    connect(m_filesDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onFilesDockTriggered(bool)));
    connect(ui->actionFiles, SIGNAL(triggered()), this, SLOT(onFilesDockTriggered()));

    m_timelineDock = new TimelineDock(this);
    m_timelineDock->hide();
    m_timelineDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));
    ui->menuView->addAction(m_timelineDock->toggleViewAction());
    connect(m_timelineDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onTimelineDockTriggered(bool)));
    connect(ui->actionTimeline, SIGNAL(triggered()), SLOT(onTimelineDockTriggered()));
    connect(m_player, SIGNAL(seeked(int)), m_timelineDock, SLOT(onSeeked(int)));
    connect(m_timelineDock, SIGNAL(seeked(int)), SLOT(seekTimeline(int)));
    connect(m_timelineDock, SIGNAL(clipClicked()), SLOT(onTimelineClipSelected()));
    connect(m_timelineDock,
            SIGNAL(showStatusMessage(QString)),
            this,
            SLOT(showStatusMessage(QString)));
    connect(m_timelineDock->model(),
            SIGNAL(showStatusMessage(QString)),
            this,
            SLOT(showStatusMessage(QString)));
    connect(m_timelineDock->model(), SIGNAL(created()), SLOT(onMultitrackCreated()));
    connect(m_timelineDock->model(), SIGNAL(closed()), SLOT(onMultitrackClosed()));
    connect(m_timelineDock->model(), SIGNAL(modified()), SLOT(onMultitrackModified()));
    connect(m_timelineDock->model(),
            &QAbstractItemModel::rowsInserted,
            m_playlistDock,
            &PlaylistDock::refreshTimelineSmartBins);
    connect(m_timelineDock->model(),
            &QAbstractItemModel::rowsRemoved,
            m_playlistDock,
            &PlaylistDock::refreshTimelineSmartBins);
    connect(m_timelineDock->model(), SIGNAL(durationChanged()), SLOT(onMultitrackDurationChanged()));
    connect(m_timelineDock, SIGNAL(clipOpened(Mlt::Producer *)), SLOT(openCut(Mlt::Producer *)));
    connect(m_timelineDock->model(), &MultitrackModel::seeked, this, &MainWindow::seekTimeline);
    connect(m_timelineDock->markersModel(), SIGNAL(modified()), SLOT(onMultitrackModified()));
    connect(m_timelineDock,
            SIGNAL(selected(Mlt::Producer *)),
            SLOT(loadProducerWidget(Mlt::Producer *)));
    connect(m_timelineDock, SIGNAL(clipCopied()), SLOT(onClipCopied()));
    connect(m_timelineDock, SIGNAL(filteredClicked()), SLOT(onFiltersDockTriggered()));
    connect(m_playlistDock,
            SIGNAL(addAllTimeline(Mlt::Playlist *)),
            SLOT(onTimelineDockTriggered()));
    connect(m_playlistDock,
            SIGNAL(addAllTimeline(Mlt::Playlist *, bool, bool)),
            SLOT(onAddAllToTimeline(Mlt::Playlist *, bool, bool)));
    connect(m_player, SIGNAL(previousSought()), m_timelineDock, SLOT(seekPreviousEdit()));
    connect(m_player, SIGNAL(nextSought()), m_timelineDock, SLOT(seekNextEdit()));
    connect(m_player, SIGNAL(loopChanged(int, int)), m_timelineDock, SLOT(onLoopChanged(int, int)));
    connect(m_timelineDock,
            SIGNAL(isRecordingChanged(bool)),
            m_player,
            SLOT(onMuteButtonToggled(bool)));
    connect(m_player, SIGNAL(trimIn()), m_timelineDock, SLOT(trimClipIn()));
    connect(m_player, SIGNAL(trimOut()), m_timelineDock, SLOT(trimClipOut()));
    ui->menuEdit->addAction(Actions["timelineCutAction"]);
    ui->menuEdit->addAction(Actions["timelineCopyAction"]);
    ui->menuEdit->addAction(Actions["timelinePasteAction"]);
    ui->menuTimeline->addAction(Actions["timelineSnapAction"]);
    ui->menuTimeline->addAction(Actions["timelineSnapAction"]);
    ui->menuTimeline->addAction(Actions["timelineScrubDragAction"]);
    ui->menuTimeline->addAction(Actions["timelineRippleAction"]);
    ui->menuTimeline->addAction(Actions["timelineRippleAllTracksAction"]);
    ui->menuTimeline->addAction(Actions["timelineRippleMarkersAction"]);
    ui->menuTimeline->addAction(Actions["timelineToggleRippleAndAllTracksAction"]);
    ui->menuTimeline->addAction(Actions["timelineToggleRippleAllTracksAndMarkersAction"]);
    ui->menuTimeline->addSeparator();
    ui->menuTimeline->addAction(Actions["timelineAdjustGainAction"]);
    ui->menuTimeline->addAction(Actions["timelineAutoAddTracksAction"]);
    ui->menuTimeline->addAction(Actions["timelineRectangleSelectAction"]);
    ui->menuTimeline->addAction(Actions["timelineShowWaveformsAction"]);
    ui->menuTimeline->addAction(Actions["timelineShowThumbnailsAction"]);
    auto submenu = ui->menuTimeline->addMenu(tr("Scrolling"));
    auto *group = new QActionGroup(this);
    submenu->addAction(Actions["timelineScrollingCenterPlayhead"]);
    group->addAction(Actions["timelineScrollingCenterPlayhead"]);
    submenu->addAction(Actions["timelineScrollingNo"]);
    group->addAction(Actions["timelineScrollingNo"]);
    submenu->addAction(Actions["timelineScrollingPage"]);
    group->addAction(Actions["timelineScrollingPage"]);
    submenu->addAction(Actions["timelineScrollingSmooth"]);
    group->addAction(Actions["timelineScrollingSmooth"]);
    submenu->addAction(Actions["timelineScrollZoomAction"]);

    m_filterController = new FilterController(this);
    m_filtersDock = new FiltersDock(m_filterController->metadataModel(),
                                    m_filterController->attachedModel(),
                                    m_filterController->motionTrackerModel(),
                                    m_timelineDock->subtitlesModel(),
                                    this);
    m_filtersDock->setMinimumSize(400, 300);
    m_filtersDock->hide();
    m_filtersDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_6));
    ui->menuView->addAction(m_filtersDock->toggleViewAction());
    connect(m_filtersDock,
            SIGNAL(currentFilterRequested(int)),
            m_filterController,
            SLOT(setCurrentFilter(int)),
            Qt::QueuedConnection);
    connect(m_filtersDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onFiltersDockTriggered(bool)));
    connect(ui->actionFilters, SIGNAL(triggered()), this, SLOT(onFiltersDockTriggered()));
    connect(m_filterController,
            SIGNAL(currentFilterChanged(QmlFilter *, QmlMetadata *, int)),
            m_filtersDock,
            SLOT(setCurrentFilter(QmlFilter *, QmlMetadata *, int)));
    connect(m_filterController, &FilterController::undoOrRedo, m_filtersDock, &FiltersDock::load);
    connect(this, SIGNAL(producerOpened()), m_filterController, SLOT(setProducer()));
    connect(m_filterController->attachedModel(), SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filtersDock, SIGNAL(changed()), SLOT(onFilterModelChanged()));
    connect(m_filterController,
            SIGNAL(filterChanged(Mlt::Service *)),
            m_timelineDock->model(),
            SLOT(onFilterChanged(Mlt::Service *)));
    connect(m_filterController->attachedModel(),
            SIGNAL(changed()),
            m_timelineDock,
            SLOT(onFilterModelChanged()));
    connect(m_filtersDock, SIGNAL(changed()), m_timelineDock, SLOT(onFilterModelChanged()));
    connect(m_filterController->attachedModel(),
            SIGNAL(addedOrRemoved(Mlt::Producer *)),
            m_timelineDock->model(),
            SLOT(filterAddedOrRemoved(Mlt::Producer *)));
    connect(&QmlApplication::singleton(),
            SIGNAL(filtersPasted(Mlt::Producer *)),
            m_timelineDock->model(),
            SLOT(filterAddedOrRemoved(Mlt::Producer *)));
    connect(&QmlApplication::singleton(),
            &QmlApplication::filtersPasted,
            this,
            &MainWindow::onProducerModified);
    connect(m_filterController,
            SIGNAL(statusChanged(QString)),
            this,
            SLOT(showStatusMessage(QString)));
    connect(m_timelineDock, SIGNAL(gainChanged(double)), m_filterController, SLOT(onGainChanged()));
    connect(m_timelineDock, SIGNAL(fadeInChanged(int)), m_filterController, SLOT(onFadeInChanged()));
    connect(m_timelineDock,
            SIGNAL(fadeOutChanged(int)),
            m_filterController,
            SLOT(onFadeOutChanged()));
    connect(m_timelineDock,
            SIGNAL(selected(Mlt::Producer *)),
            m_filterController,
            SLOT(setProducer(Mlt::Producer *)));
    connect(m_player, SIGNAL(seeked(int)), m_filtersDock, SLOT(onSeeked(int)), Qt::QueuedConnection);
    connect(m_filtersDock, SIGNAL(seeked(int)), SLOT(seekKeyframes(int)));
    connect(MLT.videoWidget(),
            SIGNAL(frameDisplayed(const SharedFrame &)),
            m_filtersDock,
            SLOT(onShowFrame(const SharedFrame &)));
    connect(m_player, SIGNAL(inChanged(int)), m_filtersDock, SIGNAL(producerInChanged(int)));
    connect(this,
            SIGNAL(serviceInChanged(int, Mlt::Service *)),
            m_filtersDock,
            SLOT(onServiceInChanged(int, Mlt::Service *)));
    connect(m_player, SIGNAL(outChanged(int)), m_filtersDock, SIGNAL(producerOutChanged(int)));
    connect(m_player, SIGNAL(inChanged(int)), m_filterController, SLOT(onServiceInChanged(int)));
    connect(m_player, SIGNAL(outChanged(int)), m_filterController, SLOT(onServiceOutChanged(int)));
    connect(this,
            SIGNAL(serviceInChanged(int, Mlt::Service *)),
            m_filterController,
            SLOT(onServiceInChanged(int, Mlt::Service *)));
    connect(this,
            SIGNAL(serviceOutChanged(int, Mlt::Service *)),
            m_filterController,
            SLOT(onServiceOutChanged(int, Mlt::Service *)));
    connect(m_playlistDock->model(),
            &PlaylistModel::removing,
            m_filterController->motionTrackerModel(),
            &MotionTrackerModel::removeFromService);
    connect(m_timelineDock->model(),
            &MultitrackModel::removing,
            m_filterController->motionTrackerModel(),
            &MotionTrackerModel::removeFromService);
    connect(this, SIGNAL(audioChannelsChanged()), m_filterController, SLOT(setProducer()));
    connect(this, SIGNAL(processingModeChanged()), m_filterController, SLOT(setProducer()));
    connect(m_timelineDock,
            &TimelineDock::trimStarted,
            m_filterController,
            &FilterController::pauseUndoTracking);
    connect(m_timelineDock,
            &TimelineDock::trimEnded,
            m_filterController,
            &FilterController::resumeUndoTracking);

    m_markersDock = new MarkersDock(this);
    m_markersDock->hide();
    m_markersDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_6));
    m_markersDock->setModel(m_timelineDock->markersModel());
    ui->menuView->addAction(m_markersDock->toggleViewAction());
    connect(m_markersDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onMarkersDockTriggered(bool)));
    connect(ui->actionMarkers, SIGNAL(triggered()), this, SLOT(onMarkersDockTriggered()));
    connect(m_markersDock, SIGNAL(seekRequested(int)), SLOT(seekTimeline(int)));
    connect(m_markersDock, SIGNAL(addRequested()), m_timelineDock, SLOT(createMarker()));
    connect(m_markersDock,
            SIGNAL(addAroundSelectionRequested()),
            m_timelineDock,
            SLOT(createOrEditSelectionMarker()));
    connect(m_timelineDock,
            SIGNAL(markerSeeked(int)),
            m_markersDock,
            SLOT(onMarkerSelectionRequest(int)));

    m_keyframesDock = new KeyframesDock(m_filtersDock->qmlProducer(), this);
    m_keyframesDock->hide();
    m_keyframesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_7));
    ui->menuView->addAction(m_keyframesDock->toggleViewAction());
    connect(m_keyframesDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onKeyframesDockTriggered(bool)));
    connect(ui->actionKeyframes, SIGNAL(triggered()), this, SLOT(onKeyframesDockTriggered()));
    connect(m_filterController,
            SIGNAL(currentFilterChanged(QmlFilter *, QmlMetadata *, int)),
            m_keyframesDock,
            SLOT(setCurrentFilter(QmlFilter *, QmlMetadata *)));
    connect(m_keyframesDock,
            SIGNAL(visibilityChanged(bool)),
            m_filtersDock->qmlProducer(),
            SLOT(remakeAudioLevels(bool)));
    connect(m_filterController,
            &FilterController::undoOrRedo,
            m_keyframesDock,
            &KeyframesDock::reload);

    m_historyDock = new QDockWidget(tr("History"), this);
    m_historyDock->hide();
    m_historyDock->setObjectName("historyDock");
    m_historyDock->setWindowIcon(ui->actionHistory->icon());
    m_historyDock->toggleViewAction()->setIcon(ui->actionHistory->icon());
    m_historyDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_8));
    m_historyDock->setMinimumWidth(150);
    ui->menuView->addAction(m_historyDock->toggleViewAction());
    connect(m_historyDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onHistoryDockTriggered(bool)));
    connect(ui->actionHistory, SIGNAL(triggered()), this, SLOT(onHistoryDockTriggered()));
    QUndoView *undoView = new QUndoView(m_undoStack, m_historyDock);
    undoView->setObjectName("historyView");
    undoView->setAlternatingRowColors(true);
    undoView->setSpacing(2);
    m_historyDock->setWidget(undoView);
    ui->actionUndo->setDisabled(true);
    ui->actionRedo->setDisabled(true);

    m_encodeDock = new EncodeDock(this);
    m_encodeDock->hide();
    m_encodeDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_9));
    ui->menuView->addAction(m_encodeDock->toggleViewAction());
    connect(this, SIGNAL(producerOpened()), m_encodeDock, SLOT(onProducerOpened()));
    connect(ui->actionEncode, SIGNAL(triggered()), this, SLOT(onEncodeTriggered()));
    connect(ui->actionExportVideo,
            SIGNAL(triggered()),
            m_encodeDock,
            SLOT(on_encodeButton_clicked()));
    connect(m_encodeDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onEncodeTriggered(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            m_player,
            SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            m_propertiesDock,
            SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_recentDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_filtersDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            m_keyframesDock,
            SLOT(setDisabled(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            ui->actionOpen,
            SLOT(setDisabled(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            ui->actionOpenOther,
            SLOT(setDisabled(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            ui->actionExit,
            SLOT(setDisabled(bool)));
    connect(m_encodeDock,
            SIGNAL(captureStateChanged(bool)),
            this,
            SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_historyDock, SLOT(setDisabled(bool)));
    connect(this, SIGNAL(profileChanged()), m_encodeDock, SLOT(onProfileChanged()));
    connect(this, SIGNAL(profileChanged()), SLOT(onProfileChanged()));
    connect(this, SIGNAL(profileChanged()), &QmlProfile::singleton(), SIGNAL(profileChanged()));
    connect(this, SIGNAL(audioChannelsChanged()), m_encodeDock, SLOT(onAudioChannelsChanged()));
    connect(m_playlistDock->model(), SIGNAL(modified()), m_encodeDock, SLOT(onProducerOpened()));
    connect(m_timelineDock, SIGNAL(clipCopied()), m_encodeDock, SLOT(onProducerOpened()));
    connect(m_timelineDock, SIGNAL(markerRangesChanged()), m_encodeDock, SLOT(onProducerOpened()));
    connect(m_timelineDock->model(),
            &MultitrackModel::filteredChanged,
            m_encodeDock,
            &EncodeDock::onProfileChanged);
    connect(m_filterController, &FilterController::filterChanged, this, [&](Mlt::Service *filter) {
        if (filter && filter->is_valid()
            && !::qstrcmp("reframe", filter->get(kShotcutFilterProperty))) {
            m_encodeDock->onReframeChanged();
        }
    });
    connect(m_filterController->attachedModel(),
            &AttachedFiltersModel::addedOrRemoved,
            this,
            [&](Mlt::Producer *producer) { m_encodeDock->onReframeChanged(); });

    m_jobsDock = new JobsDock(this);
    m_jobsDock->hide();
    m_jobsDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(onJobAdded()));
    connect(m_jobsDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onJobsDockTriggered(bool)));
    connect(ui->actionJobs, SIGNAL(triggered()), this, SLOT(onJobsDockTriggered()));

    m_notesDock = new NotesDock(this);
    m_notesDock->hide();
    m_notesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_3));
    ui->menuView->insertAction(m_playlistDock->toggleViewAction(), m_notesDock->toggleViewAction());
    connect(m_notesDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onNotesDockTriggered(bool)));
    connect(ui->actionNotes, SIGNAL(triggered()), this, SLOT(onNotesDockTriggered()));
    connect(m_notesDock, SIGNAL(modified()), this, SLOT(onNoteModified()));

    m_subtitlesDock = new SubtitlesDock(this);
    m_subtitlesDock->hide();
    m_subtitlesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_9));
    m_subtitlesDock->setModel(m_timelineDock->subtitlesModel(),
                              m_timelineDock->subtitlesSelectionModel());
    ui->menuView->addAction(m_subtitlesDock->toggleViewAction());
    connect(m_subtitlesDock->toggleViewAction(),
            SIGNAL(triggered(bool)),
            this,
            SLOT(onSubtitlesDockTriggered(bool)));
    connect(ui->actionSubtitles, SIGNAL(triggered()), this, SLOT(onSubtitlesDockTriggered()));
    connect(m_subtitlesDock, SIGNAL(seekRequested(int)), SLOT(seekTimeline(int)));
    connect(m_timelineDock,
            SIGNAL(positionChanged(int)),
            m_subtitlesDock,
            SLOT(onPositionChanged(int)));
    connect(m_subtitlesDock,
            SIGNAL(addAllTimeline(Mlt::Playlist *, bool, bool)),
            SLOT(onTimelineDockTriggered()));
    connect(m_subtitlesDock,
            SIGNAL(addAllTimeline(Mlt::Playlist *, bool, bool)),
            SLOT(onAddAllToTimeline(Mlt::Playlist *, bool, bool)));
    connect(m_subtitlesDock,
            &SubtitlesDock::createOrEditFilterOnOutput,
            this,
            &MainWindow::onCreateOrEditFilterOnOutput);
    connect(m_encodeDock,
            &EncodeDock::createOrEditFilterOnOutput,
            this,
            &MainWindow::onCreateOrEditFilterOnOutput);
    connect(m_timelineDock->subtitlesModel(), SIGNAL(modified()), this, SLOT(onSubtitleModified()));

    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    addDockWidget(Qt::RightDockWidgetArea, m_recentDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_playlistDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_filtersDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_keyframesDock);
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_encodeDock);
    addDockWidget(Qt::RightDockWidgetArea, m_jobsDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_notesDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_subtitlesDock);
    addDockWidget(Qt::RightDockWidgetArea, m_filesDock);
    splitDockWidget(m_timelineDock, m_markersDock, Qt::Horizontal);
    tabifyDockWidget(m_propertiesDock, m_playlistDock);
    tabifyDockWidget(m_playlistDock, m_filtersDock);
    tabifyDockWidget(m_filtersDock, m_encodeDock);
    tabifyDockWidget(m_encodeDock, m_notesDock);
    tabifyDockWidget(m_notesDock, m_subtitlesDock);
    splitDockWidget(m_recentDock, findChild<QDockWidget *>("AudioWaveformDock"), Qt::Vertical);
    splitDockWidget(audioMeterDock, m_recentDock, Qt::Horizontal);
    tabifyDockWidget(m_recentDock, m_filesDock);
    tabifyDockWidget(m_filesDock, m_historyDock);
    tabifyDockWidget(m_historyDock, m_jobsDock);
    tabifyDockWidget(m_keyframesDock, m_timelineDock);
    m_recentDock->raise();
    resetDockCorners();
}

void MainWindow::setupMenuFile()
{
#ifdef Q_OS_MAC
    static auto sep = "        ";
#else
    static auto sep = "\t";
#endif
    connect(ui->menuOtherVersions, &QMenu::aboutToShow, this, [&] {
        ui->menuOtherVersions->clear();
        QDir dir(QFileInfo(m_currentFile).absolutePath());
        auto name = Util::baseName(m_currentFile, true);
        auto match = kBackupFileRegex.match(name);
        QStringList filters;
        if (match.hasMatch())
            filters << match.captured(1) + "*.mlt";
        else
            filters << QFileInfo(m_currentFile).baseName().split(" - ").first() + "*.mlt";
        for (auto &fileInfo : dir.entryInfoList(filters, QDir::Files, QDir::Time)) {
            auto filename = fileInfo.fileName();
            if (filename != name) {
                auto text = filename;
                if (!kBackupFileRegex.match(filename).hasMatch())
                    text += QString::fromLatin1("%1(%2)").arg(sep,
                                                              fileInfo.lastModified().toString(
                                                                  Qt::ISODate));
                ui->menuOtherVersions->addAction(text)->setData(dir.filePath(filename));
            }
        }
        QCoreApplication::processEvents();
    });
    connect(ui->menuOtherVersions, &QMenu::triggered, this, [&](QAction *action) {
        open(action->data().toString());
    });
}

void MainWindow::setupMenuView()
{
    ui->menuView->addSeparator();
    ui->menuView->addAction(ui->actionResources);
    ui->menuView->addAction(ui->actionApplicationLog);
}

void MainWindow::connectVideoWidgetSignals()
{
    auto videoWidget = static_cast<Mlt::VideoWidget *>(&MLT);
    connect(videoWidget,
            &Mlt::VideoWidget::dragStarted,
            m_playlistDock,
            &PlaylistDock::onPlayerDragStarted);
    connect(videoWidget, &Mlt::VideoWidget::seekTo, m_player, &Player::seek);
    connect(videoWidget, &Mlt::VideoWidget::gpuNotSupported, this, &MainWindow::onGpuNotSupported);
    connect(videoWidget->quickWindow(),
            &QQuickWindow::sceneGraphInitialized,
            videoWidget,
            &Mlt::VideoWidget::initialize,
            Qt::DirectConnection);
    connect(videoWidget->quickWindow(),
            &QQuickWindow::beforeRendering,
            videoWidget,
            &Mlt::VideoWidget::beforeRendering,
            Qt::DirectConnection);
    connect(videoWidget->quickWindow(),
            &QQuickWindow::beforeRenderPassRecording,
            videoWidget,
            &Mlt::VideoWidget::renderVideo,
            Qt::DirectConnection);
    connect(videoWidget->quickWindow(),
            &QQuickWindow::sceneGraphInitialized,
            this,
            &MainWindow::onSceneGraphInitialized,
            Qt::QueuedConnection);
    connect(videoWidget->quickWindow(),
            &QQuickWindow::sceneGraphInitialized,
            m_timelineDock,
            &TimelineDock::initLoad);
    connect(videoWidget,
            &Mlt::VideoWidget::frameDisplayed,
            m_scopeController,
            &ScopeController::newFrame);
    connect(m_filterController,
            &FilterController::currentFilterChanged,
            videoWidget,
            &Mlt::VideoWidget::setCurrentFilter);
    connect(m_player, &Player::toggleVuiRequested, videoWidget, &Mlt::VideoWidget::toggleVuiDisplay);
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
    // Switch to Project player.
    if (m_player->tabIndex() != Player::ProjectTabIndex) {
        m_timelineDock->saveAndClearSelection();
        m_player->onTabBarClicked(Player::ProjectTabIndex);
    }
}

void MainWindow::onAddAllToTimeline(Mlt::Playlist *playlist, bool skipProxy, bool emptyTrack)
{
    // We stop the player because of a bug on Windows that results in some
    // strange memory leak when using Add All To Timeline, more noticeable
    // with (high res?) still image files.
    if (MLT.isSeekable())
        m_player->pause();
    else
        m_player->stop();
    m_timelineDock->appendFromPlaylist(playlist, skipProxy, emptyTrack);
}

MainWindow &MainWindow::singleton()
{
    static MainWindow *instance = new MainWindow;
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

    Mlt::Filter filter(MLT.profile(), "color_transform");
    if (!filter.is_valid()) {
        ui->actionLinear8bitCpu->setVisible(false);
#if LIBMLT_VERSION_INT < ((7 << 16) + (34 << 8))
        ui->actionNative10bitCpu->setVisible(false);
#endif
        ui->actionLinear10bitCpu->setVisible(false);
    }
    QActionGroup *group = new QActionGroup(this);
    ui->actionNative8bitCpu->setData(ShotcutSettings::Native8Cpu);
    if (ui->actionLinear8bitCpu->isVisible())
        ui->actionLinear8bitCpu->setData(ShotcutSettings::Linear8Cpu);
    if (ui->actionNative10bitCpu->isVisible())
        ui->actionNative10bitCpu->setData(ShotcutSettings::Native10Cpu);
    if (ui->actionLinear10bitCpu->isVisible())
        ui->actionLinear10bitCpu->setData(ShotcutSettings::Linear10Cpu);
    ui->actionNative10bitGpuCpu->setData(ShotcutSettings::Linear10GpuCpu);
    group->addAction(ui->actionNative8bitCpu);
    group->addAction(ui->actionLinear8bitCpu);
    group->addAction(ui->actionNative10bitCpu);
    group->addAction(ui->actionLinear10bitCpu);
    group->addAction(ui->actionNative10bitGpuCpu);
    for (auto a : group->actions()) {
        ShotcutSettings::ProcessingMode mode = (ShotcutSettings::ProcessingMode) a->data().toInt();
        if (Settings.processingMode() == mode) {
            a->setChecked(true);
            setProcessingMode(mode);
            break;
        }
    }
    connect(group, &QActionGroup::triggered, this, [&](QAction *action) {
        ShotcutSettings::ProcessingMode oldMode = Settings.processingMode();
        ShotcutSettings::ProcessingMode newMode
            = (ShotcutSettings::ProcessingMode) action->data().toInt();
        if (oldMode == newMode)
            return;
        LOG_INFO() << "Processing Mode" << oldMode << "->" << newMode;
        if (newMode == ShotcutSettings::Linear10GpuCpu) {
            QMessageBox
                dialog(QMessageBox::Warning,
                       qApp->applicationName(),
                       tr("GPU processing is experimental and does not work on all computers. "
                          "Plan to do some testing after turning this on.\n"
                          "At this time, a project created with GPU processing cannot be "
                          "converted to a CPU-only project later.\n"
                          "Do you want to enable GPU processing and restart Shotcut?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
            dialog.setDefaultButton(QMessageBox::Yes);
            dialog.setEscapeButton(QMessageBox::No);
            dialog.setWindowModality(QmlApplication::dialogModality());
            dialog.adjustSize();
            if (dialog.exec() == QMessageBox::Yes) {
                Settings.setProcessingMode(newMode);
                m_exitCode = EXIT_RESTART;
                QApplication::closeAllWindows();
            }
        } else if (oldMode == ShotcutSettings::Linear10GpuCpu) {
            QMessageBox dialog(QMessageBox::Information,
                               qApp->applicationName(),
                               tr("Shotcut must restart to disable GPU processing.\n"
                                  "Disable GPU processing and restart?"),
                               QMessageBox::No | QMessageBox::Yes,
                               this);
            dialog.setDefaultButton(QMessageBox::Yes);
            dialog.setEscapeButton(QMessageBox::No);
            dialog.setWindowModality(QmlApplication::dialogModality());
            dialog.adjustSize();
            if (dialog.exec() == QMessageBox::Yes) {
                Settings.setProcessingMode(newMode);
                m_exitCode = EXIT_RESTART;
                QApplication::closeAllWindows();
            }
        } else {
            setProcessingMode((ShotcutSettings::ProcessingMode) action->data().toInt());
        }
    });

    group = new QActionGroup(this);
    group->addAction(ui->actionChannels1);
    group->addAction(ui->actionChannels2);
    group->addAction(ui->actionChannels4);
    group->addAction(ui->actionChannels6);
    group = new QActionGroup(this);
    group->addAction(ui->actionOneField);
    group->addAction(ui->actionLinearBlend);
    group->addAction(ui->actionYadifTemporal);
    group->addAction(ui->actionYadifSpatial);
    group->addAction(ui->actionBwdif);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    auto submenu = new QMenu(tr("Audio API"));
    ui->menuPlayerSettings->insertMenu(ui->actionSync, submenu);
    group = new QActionGroup(this);
#if defined(Q_OS_WIN)
    group->addAction(submenu->addAction("DirectSound"))->setData("directsound");
    group->addAction(submenu->addAction("WASAPI"))->setData("wasapi");
    group->addAction(submenu->addAction("Waveform Audio"))->setData("winmm");
#else
    group->addAction(submenu->addAction("ALSA"))->setData("alsa");
    group->addAction(submenu->addAction("ESound"))->setData("esd");
    group->addAction(submenu->addAction("OSS"))->setData("dsp");
    group->addAction(submenu->addAction("PipeWire"))->setData("pipewire");
    group->addAction(submenu->addAction("PulseAudio"))->setData("pulseaudio");
#endif
    for (auto a : group->actions()) {
        a->setCheckable(true);
        const auto data = a->data().toString();
        if (data == Settings.playerAudioDriver())
            a->setChecked(true);
        if ((data == "directsound" && Settings.playerAudioChannels() > 2)
            || (data == "winmm" && Settings.playerAudioChannels() <= 2) || (data == "pulseaudio")) {
            a->setText(a->text() + QString::fromLatin1(" (%1)").arg(tr("default")));
        }
    }
    connect(group, &QActionGroup::triggered, this, [&](QAction *action) {
        Settings.setPlayerAudioDriver(action->data().toString());
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("You must restart Shotcut to change the audio API.\n"
                              "Do you want to restart now?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Yes) {
            ::qunsetenv("SDL_AUDIODRIVER");
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
    });
#endif

    group = new QActionGroup(this);
    ui->actionBackupManually->setData(0);
    group->addAction(ui->actionBackupManually);
    ui->actionBackupHourly->setData(60);
    group->addAction(ui->actionBackupHourly);
    ui->actionBackupDaily->setData(24 * 60);
    group->addAction(ui->actionBackupDaily);
    ui->actionBackupWeekly->setData(7 * 24 * 60);
    group->addAction(ui->actionBackupWeekly);
    for (auto a : group->actions()) {
        if (Settings.backupPeriod() == a->data().toInt())
            a->setChecked(true);
    }
    connect(group, &QActionGroup::triggered, this, [&](QAction *action) {
        Settings.setBackupPeriod(action->data().toInt());
    });

    m_previewScaleGroup = new QActionGroup(this);
    m_previewScaleGroup->addAction(ui->actionPreviewNone);
    m_previewScaleGroup->addAction(ui->actionPreview360);
    m_previewScaleGroup->addAction(ui->actionPreview540);
    m_previewScaleGroup->addAction(ui->actionPreview720);
    m_previewScaleGroup->addAction(ui->actionPreview1080);
    ui->menuPreviewScaling->addAction(ui->actionPreview1080);

    group = new QActionGroup(this);
    group->addAction(ui->actionTimeFrames);
    ui->actionTimeFrames->setData(mlt_time_frames);
    group->addAction(ui->actionTimeClock);
    ui->actionTimeClock->setData(mlt_time_clock);
    group->addAction(ui->actionTimeDF);
    ui->actionTimeDF->setData(mlt_time_smpte_df);
    group->addAction(ui->actionTimeNDF);
    ui->actionTimeNDF->setData(mlt_time_smpte_ndf);
    switch (Settings.timeFormat()) {
    case mlt_time_frames:
        ui->actionTimeFrames->setChecked(true);
        break;
    case mlt_time_clock:
        ui->actionTimeClock->setChecked(true);
        break;
    case mlt_time_smpte_df:
        ui->actionTimeDF->setChecked(true);
        break;
    default:
        ui->actionTimeNDF->setChecked(true);
        break;
    }
    connect(group, &QActionGroup::triggered, this, [&](QAction *action) {
        Settings.setTimeFormat(action->data().toInt());
    });

    group = new QActionGroup(this);
    group->addAction(ui->actionNearest);
    group->addAction(ui->actionBilinear);
    group->addAction(ui->actionBicubic);
    group->addAction(ui->actionHyper);
    m_profileGroup = new QActionGroup(this);
    m_profileGroup->addAction(ui->actionProfileAutomatic);
    ui->actionProfileAutomatic->setData(QString());
    buildVideoModeMenu(ui->menuProfile,
                       m_customProfileMenu,
                       m_profileGroup,
                       ui->actionAddCustomProfile,
                       ui->actionProfileRemove);

    // Add the SDI and HDMI devices to the Settings menu.
    m_externalGroup = new QActionGroup(this);
    ui->actionExternalNone->setData(QString());
    m_externalGroup->addAction(ui->actionExternalNone);

#ifdef USE_SCREENS_FOR_EXTERNAL_MONITORING
    QList<QScreen *> screens = QGuiApplication::screens();
    int n = screens.size();
    for (int i = 0; n > 1 && i < n; i++) {
        QAction *action
            = new QAction(tr("Screen %1 (%2 x %3 @ %4 Hz)")
                              .arg(i)
                              .arg(screens[i]->size().width() * screens[i]->devicePixelRatio())
                              .arg(screens[i]->size().height() * screens[i]->devicePixelRatio())
                              .arg(screens[i]->refreshRate()),
                          this);
        action->setCheckable(true);
        action->setData(i);
        m_externalGroup->addAction(action);
    }
#endif

    Mlt::Profile profile;
    Mlt::Consumer decklink(profile, "decklink:");
    if (decklink.is_valid()) {
        decklink.set("list_devices", 1);
        int n = decklink.get_int("devices");
        for (int i = 0; i < n; ++i) {
            QString device(decklink.get(QStringLiteral("device.%1").arg(i).toLatin1().constData()));
            if (!device.isEmpty()) {
                QAction *action = new QAction(device, this);
                action->setCheckable(true);
                action->setData(QStringLiteral("decklink:%1").arg(i));
                m_externalGroup->addAction(action);

                if (!m_decklinkGammaGroup) {
                    m_decklinkGammaGroup = new QActionGroup(this);
                    action = new QAction(tr("SDR"), m_decklinkGammaGroup);
                    action->setData(QVariant(0));
                    action->setCheckable(true);
                    action = new QAction(tr("HLG HDR"), m_decklinkGammaGroup);
                    action->setData(QVariant(1));
                    action->setCheckable(true);
                }
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
    if (m_decklinkGammaGroup) {
        m_decklinkGammaMenu = ui->menuExternal->addMenu(tr("DeckLink Gamma"));
        m_decklinkGammaMenu->addActions(m_decklinkGammaGroup->actions());
        m_decklinkGammaMenu->setDisabled(true);
        connect(m_decklinkGammaGroup,
                &QActionGroup::triggered,
                this,
                &MainWindow::onDecklinkGammaTriggered);
    }
    if (m_keyerGroup) {
        m_keyerMenu = ui->menuExternal->addMenu(tr("DeckLink Keyer"));
        m_keyerMenu->addActions(m_keyerGroup->actions());
        m_keyerMenu->setDisabled(true);
        connect(m_keyerGroup, &QActionGroup::triggered, this, &MainWindow::onKeyerTriggered);
    }
    connect(m_externalGroup,
            SIGNAL(triggered(QAction *)),
            this,
            SLOT(onExternalTriggered(QAction *)));
    connect(m_profileGroup, SIGNAL(triggered(QAction *)), this, SLOT(onProfileTriggered(QAction *)));

    // Setup the language menu actions
    m_languagesGroup = new QActionGroup(this);
    QAction *a;
    a = new QAction(QLocale::languageToString(QLocale::Arabic), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ar");
    a = new QAction(QLocale::languageToString(QLocale::Basque), m_languagesGroup);
    a->setCheckable(true);
    a->setData("eu");
    a = new QAction(QLocale::languageToString(QLocale::Catalan), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ca");
    a = new QAction(QLocale::languageToString(QLocale::Chinese).append(" (China)"),
                    m_languagesGroup);
    a->setCheckable(true);
    a->setData("zh_CN");
    a = new QAction(QLocale::languageToString(QLocale::Chinese).append(" (Taiwan)"),
                    m_languagesGroup);
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
    a = new QAction(QLocale::languageToString(QLocale::English).append(" (Great Britain)"),
                    m_languagesGroup);
    a->setCheckable(true);
    a->setData("en_GB");
    a = new QAction(QLocale::languageToString(QLocale::English).append(" (United States)"),
                    m_languagesGroup);
    a->setCheckable(true);
    a->setData("en_US");
    a = new QAction(QLocale::languageToString(QLocale::Estonian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("et");
    a = new QAction(QLocale::languageToString(QLocale::Finnish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fi");
    a = new QAction(QLocale::languageToString(QLocale::French), m_languagesGroup);
    a->setCheckable(true);
    a->setData("fr");
    a = new QAction("French (Canada)", m_languagesGroup);
    a->setCheckable(true);
    a->setData("fr_CA");
    a = new QAction(QLocale::languageToString(QLocale::Gaelic), m_languagesGroup);
    a->setCheckable(true);
    a->setData("gd");
    a = new QAction(QLocale::languageToString(QLocale::Galician), m_languagesGroup);
    a->setCheckable(true);
    a->setData("gl");
    a = new QAction(QLocale::languageToString(QLocale::German), m_languagesGroup);
    a->setCheckable(true);
    a->setData("de");
    a = new QAction(QLocale::languageToString(QLocale::Greek), m_languagesGroup);
    a->setCheckable(true);
    a->setData("el");
    a = new QAction(QLocale::languageToString(QLocale::Hebrew), m_languagesGroup);
    a->setCheckable(true);
    a->setData("he_IL");
    a = new QAction(QLocale::languageToString(QLocale::Hungarian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("hu");
    a = new QAction(QLocale::languageToString(QLocale::Irish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ga");
    a = new QAction(QLocale::languageToString(QLocale::Italian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("it");
    a = new QAction(QLocale::languageToString(QLocale::Japanese), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ja");
    a = new QAction(QLocale::languageToString(QLocale::Korean), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ko");
    a = new QAction(QLocale::languageToString(QLocale::Lithuanian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("lt");
    a = new QAction(QLocale::languageToString(QLocale::Nepali), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ne");
    a = new QAction(QLocale::languageToString(QLocale::NorwegianBokmal), m_languagesGroup);
    a->setCheckable(true);
    a->setData("nb");
    a = new QAction(QLocale::languageToString(QLocale::NorwegianNynorsk), m_languagesGroup);
    a->setCheckable(true);
    a->setData("nn");
    a = new QAction(QLocale::languageToString(QLocale::Occitan), m_languagesGroup);
    a->setCheckable(true);
    a->setData("oc");
    a = new QAction(QLocale::languageToString(QLocale::Polish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("pl");
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Brazil)"),
                    m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_BR");
    a = new QAction(QLocale::languageToString(QLocale::Portuguese).append(" (Portugal)"),
                    m_languagesGroup);
    a->setCheckable(true);
    a->setData("pt_PT");
    a = new QAction(QLocale::languageToString(QLocale::Romanian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("ro");
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
    a = new QAction(QLocale::languageToString(QLocale::Swedish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("sv");
    a = new QAction(QLocale::languageToString(QLocale::Thai), m_languagesGroup);
    a->setCheckable(true);
    a->setData("th");
    a = new QAction(QLocale::languageToString(QLocale::Turkish), m_languagesGroup);
    a->setCheckable(true);
    a->setData("tr");
    a = new QAction(QLocale::languageToString(QLocale::Ukrainian), m_languagesGroup);
    a->setCheckable(true);
    a->setData("uk");
    ui->menuLanguage->addActions(m_languagesGroup->actions());
    const QString locale = Settings.language();
    foreach (QAction *action, m_languagesGroup->actions()) {
        if (action->data().toString().startsWith(locale)) {
            action->setChecked(true);
            break;
        }
    }
    connect(m_languagesGroup,
            SIGNAL(triggered(QAction *)),
            this,
            SLOT(onLanguageTriggered(QAction *)));

    // Setup the themes actions
#if defined(SHOTCUT_THEME)
    group = new QActionGroup(this);
    group->addAction(ui->actionSystemTheme);
    group->addAction(ui->actionSystemFusion);
    group->addAction(ui->actionFusionDark);
    group->addAction(ui->actionFusionLight);
    if (Settings.theme() == "dark")
        ui->actionFusionDark->setChecked(true);
    else if (Settings.theme() == "light")
        ui->actionFusionLight->setChecked(true);
    else if (Settings.theme() == "system-fusion")
        ui->actionSystemFusion->setChecked(true);
    else
        ui->actionSystemTheme->setChecked(true);
#else
    delete ui->menuTheme;
#endif

#if defined(Q_OS_WIN)
    // On Windows, if there is no JACK or it is not running
    // then Shotcut crashes inside MLT's call to jack_client_open().
    // Therefore, the JACK option for Shotcut is banned on Windows.
    delete ui->actionJack;
    ui->actionJack = nullptr;
#else
    std::unique_ptr<Mlt::Properties> filters(MLT.repository()->filters());
    if (filters && !filters->get_data("jack")) {
        delete ui->actionJack;
        ui->actionJack = nullptr;
    }
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Setup the display method actions.
    if (!Settings.playerGPU()) {
        group = new QActionGroup(this);
        delete ui->actionDrawingAutomatic;
        delete ui->actionDrawingDirectX;
        ui->actionDrawingOpenGL->setData(Qt::AA_UseDesktopOpenGL);
        group->addAction(ui->actionDrawingOpenGL);
        ui->actionDrawingSoftware->setData(Qt::AA_UseSoftwareOpenGL);
        group->addAction(ui->actionDrawingSoftware);
        connect(group,
                SIGNAL(triggered(QAction *)),
                this,
                SLOT(onDrawingMethodTriggered(QAction *)));
        switch (Settings.drawMethod()) {
        case Qt::AA_UseDesktopOpenGL:
            ui->actionDrawingOpenGL->setChecked(true);
            break;
        case Qt::AA_UseSoftwareOpenGL:
            ui->actionDrawingSoftware->setChecked(true);
            break;
        default:
            ui->actionDrawingOpenGL->setChecked(true);
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

    // Setup the job priority actions
    group = new QActionGroup(this);
    group->addAction(ui->actionJobPriorityLow);
    group->addAction(ui->actionJobPriorityNormal);
    if (Settings.jobPriority() == QThread::LowPriority)
        ui->actionJobPriorityLow->setChecked(true);
    else
        ui->actionJobPriorityNormal->setChecked(true);

    // Add custom layouts to View > Layout submenu.
    m_layoutGroup = new QActionGroup(this);
    connect(m_layoutGroup, SIGNAL(triggered(QAction *)), SLOT(onLayoutTriggered(QAction *)));
    if (Settings.layouts().size() > 0) {
        ui->menuLayout->addAction(ui->actionLayoutRemove);
        ui->menuLayout->addSeparator();
    }
    foreach (QString name, Settings.layouts())
        ui->menuLayout->addAction(addLayout(m_layoutGroup, name));

    if (qApp->property("clearRecent").toBool()) {
        ui->actionClearRecentOnExit->setVisible(false);
        Settings.setRecent(QStringList());
        Settings.setClearRecent(true);
    } else {
        ui->actionClearRecentOnExit->setChecked(Settings.clearRecent());
    }

    // Initialize the proxy submenu
    ui->actionUseProxy->setChecked(Settings.proxyEnabled());
    ui->actionProxyUseProjectFolder->setChecked(Settings.proxyUseProjectFolder());
    ui->actionProxyUseHardware->setChecked(Settings.proxyUseHardware());

    LOG_DEBUG() << "end";
}

void MainWindow::setupOpenOtherMenu()
{
    // Open Other toolbar menu button
    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QMenu *otherMenu = new QMenu(this);
    ui->actionOpenOther2->setMenu(otherMenu);
    ui->menuNew->addSeparator();

    // populate the generators
    if (mltProducers->get_data("color")) {
        ui->menuNew->addAction(tr("Color"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("color");
        otherMenu->addAction(ui->menuNew->actions().constLast());
        if (mltProducers->get_data("qtext") && mltFilters->get_data("dynamictext")) {
            ui->menuNew->addAction(tr("Text"), this, SLOT(onOpenOtherTriggered()))
                ->setObjectName("text");
            otherMenu->addAction(ui->menuNew->actions().constLast());
        }
    }
    if (mltProducers->get_data("glaxnimate")) {
        ui->menuNew->addAction(tr("Drawing/Animation"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("glaxnimate");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
#ifdef EXTERNAL_LAUNCHERS
    ui->menuNew->addAction(tr("Image/Video from HTML"), this, SLOT(onHtmlGeneratorTriggered()))
        ->setObjectName("html");
#endif
    otherMenu->addAction(ui->menuNew->actions().constLast());
    if (mltProducers->get_data("noise")) {
        ui->menuNew->addAction(tr("Noise"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("noise");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
    if (mltProducers->get_data("frei0r.test_pat_B")) {
        ui->menuNew->addAction(tr("Color Bars"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("test_pat_B");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
    if (mltProducers->get_data("tone")) {
        ui->menuNew->addAction(tr("Audio Tone"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("tone");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
    if (mltProducers->get_data("count")) {
        ui->menuNew->addAction(tr("Count"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("count");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
    if (mltProducers->get_data("blipflash")) {
        ui->menuNew->addAction(tr("Blip Flash"), this, SLOT(onOpenOtherTriggered()))
            ->setObjectName("blipflash");
        otherMenu->addAction(ui->menuNew->actions().constLast());
    }
#if defined(EXTERNAL_LAUNCHERS)
    ui->actionScreenSnapshot->setVisible(true);
    ui->menuNew->addAction(tr("Screen Snapshot"), this, SLOT(on_actionScreenSnapshot_triggered()))
        ->setObjectName("screenSnapshot");
    otherMenu->addAction(ui->menuNew->actions().constLast());
    ui->actionScreenRecording->setVisible(true);
    ui->menuNew->addAction(tr("Screen Recording"), this, SLOT(on_actionScreenRecording_triggered()))
        ->setObjectName("screenRecording");
    otherMenu->addAction(ui->menuNew->actions().constLast());
#endif
}

QAction *MainWindow::addProfile(QActionGroup *actionGroup, const QString &desc, const QString &name)
{
    QAction *action = new QAction(desc, this);
    action->setCheckable(true);
    action->setData(name);
    actionGroup->addAction(action);
    return action;
}

QAction *MainWindow::addLayout(QActionGroup *actionGroup, const QString &name)
{
    QAction *action = new QAction(name, this);
    actionGroup->addAction(action);
    return action;
}

void MainWindow::open(Mlt::Producer *producer, bool play)
{
    if (!producer->is_valid())
        showStatusMessage(tr("Failed to open "));
    else if (producer->get_int("error"))
        showStatusMessage(tr("Failed to open ") + producer->get("resource"));

    //    bool ok = false;
    //    int screen = Settings.playerExternal().toInt(&ok);
    //    if (ok && screen < QGuiApplication::screens().count()
    //            && QGuiApplication::screens().at(screen) != this->screen()) {
    //        m_player->moveVideoToScreen(screen);
    //    }

    // no else here because open() will delete the producer if open fails
    if (!MLT.setProducer(producer)) {
        if (!play)
            m_player->setPauseAfterOpen(true);
        emit producerOpened();
        if (MLT.isProjectProducer(producer)) {
            m_filterController->motionTrackerModel()->load();
            emit profileChanged();
        } else if (!MLT.profile().is_explicit()) {
            emit profileChanged();
        }
    }
    m_player->setFocus();
    emit m_playlistDock->enableUpdate(false);

    // Needed on Windows. Upon first file open, window is deactivated, perhaps OpenGL-related.
    activateWindow();
}

bool MainWindow::isCompatibleWithGpuMode(MltXmlChecker &checker)
{
    if (checker.needsGPU() && !Settings.playerGPU()) {
        LOG_INFO() << "file uses GPU but GPU not enabled";
        QMessageBox dialog(QMessageBox::Warning,
                           qApp->applicationName(),
                           tr("The file you opened uses GPU processing, which is not enabled."),
                           QMessageBox::Ok,
                           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Ok);
        dialog.exec();
        return false;
    } else if (checker.needsCPU() && Settings.playerGPU()) {
        LOG_INFO() << "file uses GPU incompatible filters but GPU processing is enabled";
        QMessageBox dialog(
            QMessageBox::Question,
            qApp->applicationName(),
            tr("The file you opened uses CPU effects that are incompatible with GPU processing.\n"
               "Do you want to disable GPU processing and restart?"),
            QMessageBox::No | QMessageBox::Yes,
            this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes) {
            Settings.setProcessingMode(ShotcutSettings::Native8Cpu);
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
        return false;
    }
    return true;
}

bool MainWindow::saveRepairedXmlFile(MltXmlChecker &checker, QString &fileName)
{
    QFileInfo fi(fileName);
    auto filename = QStringLiteral("%1/%2 - %3.%4")
                        .arg(fi.path(), fi.completeBaseName(), tr("Repaired"), fi.suffix());
    auto caption = tr("Save Repaired XML");
    filename = QFileDialog::getSaveFileName(this,
                                            caption,
                                            filename,
                                            tr("MLT XML (*.mlt)"),
                                            nullptr,
                                            Util::getFileDialogOptions());
    if (!filename.isEmpty()) {
        QFile repaired(filename);
        repaired.open(QIODevice::WriteOnly);
        LOG_INFO() << "repaired MLT XML file name" << repaired.fileName();
        if (checker.tempFile().exists()) {
            checker.tempFile().open();
            QByteArray xml = checker.tempFile().readAll();
            checker.tempFile().close();

            if (Settings.proxyEnabled()) {
                auto s = QString::fromUtf8(xml);
                if (ProxyManager::filterXML(s, QDir::fromNativeSeparators(fi.absolutePath()))) {
                    xml = s.toUtf8();
                }
            }

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
    }
    return false;
}

bool MainWindow::isXmlRepaired(MltXmlChecker &checker, QString &fileName)
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
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        int r = dialog.exec();
        if (r == QMessageBox::Yes)
            result = saveRepairedXmlFile(checker, fileName);
    } else if (checker.unlinkedFilesModel().rowCount() > 0) {
        UnlinkedFilesDialog dialog(this);
        dialog.setModel(checker.unlinkedFilesModel());
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QDialog::Accepted) {
            if (checker.check(fileName) == QXmlStreamReader::NoError && checker.isCorrected())
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
        QMessageBox dialog(QMessageBox::Question,
                           qApp->applicationName(),
                           tr("Auto-saved files exist. Do you want to recover them now?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
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

void MainWindow::doAutosave()
{
    QMutexLocker locker(&m_autosaveMutex);
    if (m_autosaveFile) {
        bool success = false;
        if (m_autosaveFile->isOpen() || m_autosaveFile->open(QIODevice::ReadWrite)) {
            m_autosaveFile->close();
            success = saveXML(m_autosaveFile->fileName(), false /* without relative paths */);
            m_autosaveFile->open(QIODevice::ReadWrite);
        }
        if (!success) {
            LOG_ERROR() << "failed to open autosave file for writing" << m_autosaveFile->fileName();
        }
    }
}

void MainWindow::setFullScreen(bool isFullScreen)
{
    if (isFullScreen) {
        showFullScreen();
        ui->actionEnterFullScreen->setVisible(false);
    }
}

QString MainWindow::untitledFileName() const
{
    QDir dir = Settings.appDataLocation();
    if (!dir.exists())
        dir.mkpath(dir.path());
    return dir.filePath("__untitled__.mlt");
}

void MainWindow::setProfile(const QString &profile_name)
{
    LOG_DEBUG() << profile_name;
    MLT.setProfile(profile_name);
    emit profileChanged();
}

bool MainWindow::isSourceClipMyProject(QString resource, bool withDialog)
{
    if (m_player->tabIndex() == Player::ProjectTabIndex && MLT.savedProducer()
        && MLT.savedProducer()->is_valid())
        resource = QString::fromUtf8(MLT.savedProducer()->get("resource"));
    if (!resource.isEmpty() && QDir(resource) == QDir(fileName())) {
        if (withDialog) {
            QMessageBox dialog(QMessageBox::Information,
                               qApp->applicationName(),
                               tr("You cannot add a project to itself!"),
                               QMessageBox::Ok,
                               this);
            dialog.setDefaultButton(QMessageBox::Ok);
            dialog.setEscapeButton(QMessageBox::Ok);
            dialog.setWindowModality(QmlApplication::dialogModality());
            dialog.exec();
        }
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
    else if (channels == 4)
        ui->actionChannels4->setChecked(true);
    else if (channels == 6)
        ui->actionChannels6->setChecked(true);
    emit audioChannelsChanged();
}

void MainWindow::setProcessingMode(ShotcutSettings::ProcessingMode mode)
{
    LOG_DEBUG() << mode;
    if (mode != Settings.processingMode()) {
        Settings.setProcessingMode(mode);
    }
    switch (mode) {
    case ShotcutSettings::Native8Cpu:
        ui->actionNative8bitCpu->setChecked(true);
        break;
    case ShotcutSettings::Linear8Cpu:
        ui->actionLinear8bitCpu->setChecked(true);
        break;
    case ShotcutSettings::Native10Cpu:
        ui->actionNative10bitCpu->setChecked(true);
        break;
    case ShotcutSettings::Linear10Cpu:
        ui->actionLinear10bitCpu->setChecked(true);
        break;
    case ShotcutSettings::Linear10GpuCpu:
        ui->actionNative10bitGpuCpu->setChecked(true);
        break;
    }
    MLT.videoWidget()->setProperty("processing_mode", mode);
    MLT.setProcessingMode(mode);
    emit processingModeChanged();
}

void MainWindow::showSaveError()
{
    QMessageBox dialog(QMessageBox::Critical,
                       qApp->applicationName(),
                       tr("There was an error saving. Please try again."),
                       QMessageBox::Ok,
                       this);
    dialog.setDefaultButton(QMessageBox::Ok);
    dialog.setEscapeButton(QMessageBox::Ok);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.exec();
}

void MainWindow::setPreviewScale(int scale)
{
    LOG_DEBUG() << scale;
    switch (scale) {
    case 360:
        ui->actionPreview360->setChecked(true);
        break;
    case 540:
        ui->actionPreview540->setChecked(true);
        break;
    case 720:
        ui->actionPreview720->setChecked(true);
        break;
    case 1080:
        ui->actionPreview1080->setChecked(true);
        break;
    default:
        ui->actionPreviewNone->setChecked(true);
        break;
    }
    MLT.setPreviewScale(scale);
    if (!m_externalGroup->checkedAction()->data().toString().isEmpty()) {
        // DeckLink external monitor
        MLT.consumerChanged();
    } else {
        // System monitor
        MLT.refreshConsumer();
    }
}

void MainWindow::setVideoModeMenu()
{
    // Find a matching video mode
    for (const auto action : m_profileGroup->actions()) {
        auto s = action->data().toString();
        Mlt::Profile profile(s.toUtf8().constData());
        if (MLT.profile().width() == profile.width() && MLT.profile().height() == profile.height()
            && MLT.profile().sample_aspect_num() == profile.sample_aspect_num()
            && MLT.profile().sample_aspect_den() == profile.sample_aspect_den()
            && MLT.profile().frame_rate_num() == profile.frame_rate_num()
            && MLT.profile().frame_rate_den() == profile.frame_rate_den()
            && MLT.profile().colorspace() == profile.colorspace()
            && MLT.profile().progressive() == profile.progressive()) {
            // Select it
            action->setChecked(true);
            return;
        }
    }
    // Choose Automatic if nothing found
    m_profileGroup->actions().first()->setChecked(true);
}

void MainWindow::resetVideoModeMenu()
{
    // Change selected Video Mode back to Settings
    for (const auto action : m_profileGroup->actions()) {
        if (action->data().toString() == Settings.playerProfile()) {
            action->setChecked(true);
            break;
        }
    }
}

void MainWindow::resetDockCorners()
{
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
}

void MainWindow::showIncompatibleProjectMessage(const QString &shotcutVersion)
{
    LOG_INFO() << shotcutVersion;
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("This project file requires a newer version!\n\n"
                          "It was made with version ")
                           + shotcutVersion,
                       QMessageBox::Ok,
                       this);
    dialog.setDefaultButton(QMessageBox::Ok);
    dialog.setEscapeButton(QMessageBox::Ok);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.exec();
}

static void autosaveTask(MainWindow *p)
{
    LOG_DEBUG_TIME();
    p->doAutosave();
}

void MainWindow::onAutosaveTimeout()
{
    if (isWindowModified()) {
        // Automatic backup
        backupPeriodically();

        // Auto-save to recovery file
        auto result = QtConcurrent::run(autosaveTask, this);
    }
    static QMessageBox *dialog = nullptr;
    if (!dialog) {
        dialog
            = new QMessageBox(QMessageBox::Critical,
                              qApp->applicationName(),
                              tr("You are running low on available memory!\n\n"
                                 "Please close other applications or web browser tabs and retry.\n"
                                 "Or save and restart Shotcut."),
                              QMessageBox::Retry | QMessageBox::Save | QMessageBox::Ignore,
                              this);
        dialog->setDefaultButton(QMessageBox::Retry);
        dialog->setEscapeButton(QMessageBox::Ignore);
        dialog->setWindowModality(QmlApplication::dialogModality());
        connect(dialog, &QDialog::finished, this, [&](int result) {
            switch (result) {
            case QMessageBox::Save:
                on_actionBackupSave_triggered();
                m_exitCode = EXIT_RESTART;
                QApplication::closeAllWindows();
                break;
            case QMessageBox::Retry:
                onAutosaveTimeout();
                break;
            default:
                JOBS.resumeCurrent();
                break;
            }
        });
    }
    if (Settings.warnLowMemory()) {
        if (Util::isMemoryLow()) {
            MLT.pause();
            JOBS.pauseCurrent();
            dialog->show();
        } else if (dialog->isVisible()) {
            dialog->hide();
            QCoreApplication::processEvents();
            JOBS.resumeCurrent();
        }
    }
}

bool MainWindow::open(QString url, const Mlt::Properties *properties, bool play, bool skipConvert)
{
    // returns false when MLT is unable to open the file, possibly because it has percent sign in the path
    LOG_DEBUG() << url;
    bool modified = false;
    MltXmlChecker checker;
    QFileInfo info(url);

    if (info.isRelative()) {
        QDir pwd(QDir::currentPath());
        url = pwd.filePath(url);
        info.setFile(url);
    }
    if (url.endsWith(".mlt") || url.endsWith(".xml")) {
        if (url != untitledFileName()) {
            showStatusMessage(tr("Opening %1").arg(url));
            QCoreApplication::processEvents();
        }
        switch (checker.check(url)) {
        case QXmlStreamReader::NoError:
            if (!isCompatibleWithGpuMode(checker)) {
                showStatusMessage(tr("Failed to open ").append(url));
                return true;
            }
            break;
        case QXmlStreamReader::CustomError:
            showIncompatibleProjectMessage(checker.shotcutVersion());
            return true;
        default:
            showStatusMessage(tr("Failed to open ").append(url));
            return true;
        }
        // only check for a modified project when loading a project, not a simple producer
        if (!continueModified())
            return true;
        QCoreApplication::processEvents();
        // close existing project
        if (playlist()) {
            m_playlistDock->model()->close();
        }
        if (multitrack()) {
            m_timelineDock->model()->close();
        }
        MLT.purgeMemoryPool();
        if (!isXmlRepaired(checker, url))
            return true;
        modified = checkAutoSave(url);
        if (modified) {
            if (checker.check(url) == QXmlStreamReader::NoError) {
                if (!isCompatibleWithGpuMode(checker))
                    return true;
            } else {
                showStatusMessage(tr("Failed to open ").append(url));
                showIncompatibleProjectMessage(checker.shotcutVersion());
                return true;
            }
            if (!isXmlRepaired(checker, url))
                return true;
        }
        // let the new project change the profile
        if (modified || QFile::exists(url)) {
            MLT.profile().set_explicit(false);
            setWindowModified(modified);
            resetSourceUpdated();
        }
    }
    if (!playlist() && !multitrack()) {
        if (!modified && !continueModified())
            return true;
        setCurrentFile("");
        setWindowModified(modified);
        sourceUpdated();
        MLT.resetURL();
        // Return to automatic video mode if selected.
        if (m_profileGroup->checkedAction()
            && m_profileGroup->checkedAction()->data().toString().isEmpty())
            MLT.profile().set_explicit(false);
    }
    QString urlToOpen = checker.isUpdated() ? checker.tempFile().fileName() : url;
    if (!MLT.open(QDir::fromNativeSeparators(urlToOpen),
                  QDir::fromNativeSeparators(url),
                  skipConvert)
        && MLT.producer() && MLT.producer()->is_valid()) {
        Mlt::Properties *props = const_cast<Mlt::Properties *>(properties);
        if (props && props->is_valid())
            mlt_properties_inherit(MLT.producer()->get_properties(), props->get_properties());
        m_player->setPauseAfterOpen(!play || !MLT.isClip());

        setAudioChannels(MLT.audioChannels());
        Mlt::Filter filter(MLT.profile(), "color_transform");
        if (filter.is_valid())
            setProcessingMode(MLT.processingMode());
        if (url.endsWith(".mlt") || url.endsWith(".xml")) {
            if (MLT.producer()->get_int(kShotcutProjectFolder)) {
                MLT.setProjectFolder(info.absolutePath());
                ProxyManager::removePending();
            } else {
                MLT.setProjectFolder(QString());
            }
            setVideoModeMenu();
            m_notesDock->setText(MLT.producer()->get(kShotcutProjectNote));
        }

        open(MLT.producer());
        if (url.startsWith(AutoSaveFile::path())) {
            QMutexLocker locker(&m_autosaveMutex);
            if (m_autosaveFile && m_autosaveFile->managedFileName() != untitledFileName()) {
                m_recentDock->add(m_autosaveFile->managedFileName());
                LOG_INFO() << m_autosaveFile->managedFileName();
            }
        } else {
            m_recentDock->add(url);
            LOG_INFO() << url;
        }
    } else if (url != untitledFileName()) {
        showStatusMessage(tr("Failed to open ") + url);
        emit openFailed(url);
        return false;
    }
    return true;
}

// This one is invoked from the command line.
void MainWindow::openMultiple(const QStringList &paths)
{
    if (paths.size() > 1) {
        QList<QUrl> urls;
        foreach (const QString &s, paths)
            urls << s;
        openMultiple(urls);
    } else if (!paths.isEmpty()) {
        open(paths.first());
    }
}

// This one is invoked from above (command line) or drag-n-drop.
void MainWindow::openMultiple(const QList<QUrl> &urls)
{
    if (urls.size() > 1) {
        m_multipleFiles = Util::sortedFileList(Util::expandDirectories(urls));
        open(m_multipleFiles.first(), nullptr, true, true);
    } else if (urls.size() > 0) {
        QUrl url = urls.first();
        if (!open(Util::removeFileScheme(url)))
            open(Util::removeFileScheme(url, false));
    }
}

// This is one is invoked from the action.
void MainWindow::openVideo()
{
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    LOG_DEBUG() << Util::getFileDialogOptions();
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          tr("Open File"),
                                                          path,
                                                          tr("All Files (*);;MLT XML (*.mlt)"),
                                                          nullptr,
                                                          Util::getFileDialogOptions());

    if (filenames.length() > 0) {
        Settings.setOpenPath(QFileInfo(filenames.first()).path());
        activateWindow();
        if (filenames.length() > 1)
            m_multipleFiles = filenames;
        open(filenames.first(), nullptr, true, filenames.length() > 1);
    } else {
        // If file invalid, then on some platforms the dialog messes up SDL.
        MLT.onWindowResize();
        activateWindow();
    }
}

void MainWindow::openCut(Mlt::Producer *producer, bool play)
{
    m_player->setPauseAfterOpen(!play);
    open(producer);
    if (producer && producer->is_valid() && !MLT.isClosedClip(producer))
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

    QScrollArea *scrollArea = (QScrollArea *) m_propertiesDock->widget();
    delete scrollArea->widget();
    scrollArea->setWidget(nullptr);
    m_player->reset();

    QCoreApplication::processEvents();
}

void MainWindow::closeProducer()
{
    QCoreApplication::processEvents();
    hideProducer();
    m_filterController->motionTrackerModel()->load();
    MLT.close();
    MLT.setSavedProducer(nullptr);
}

void MainWindow::showStatusMessage(QAction *action, int timeoutSeconds)
{
    // This object takes ownership of the passed action.
    // This version does not currently log its message.
    m_statusBarAction.reset(action);
    action->setParent(nullptr);
    m_player->setStatusLabel(action->text(), timeoutSeconds, action);
}

void MainWindow::showStatusMessage(const QString &message,
                                   int timeoutSeconds,
                                   QPalette::ColorRole role)
{
    LOG_INFO() << message;
    auto action = new QAction;
    connect(action, SIGNAL(triggered()), this, SLOT(onStatusMessageClicked()));
    m_statusBarAction.reset(action);
    m_player->setStatusLabel(message, timeoutSeconds, action, role);
}

void MainWindow::onStatusMessageClicked()
{
    showStatusMessage(QString(), 0);
}

void MainWindow::seekPlaylist(int start)
{
    if (!playlist())
        return;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if (!MLT.producer()
        || (void *) MLT.producer()->get_producer() != (void *) playlist()->get_playlist())
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

void MainWindow::seekTimeline(int position, bool seekPlayer)
{
    if (!multitrack())
        return;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if (MLT.producer()
        && (void *) MLT.producer()->get_producer() != (void *) multitrack()->get_producer()) {
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
    }
    if (seekPlayer)
        m_player->seek(position);
    else
        m_player->pause();
}

void MainWindow::seekKeyframes(int position)
{
    m_player->seek(position);
}

void MainWindow::readPlayerSettings()
{
    LOG_DEBUG() << "begin";
    ui->actionPauseAfterSeek->setChecked(Settings.playerPauseAfterSeek());
    ui->actionRealtime->setChecked(Settings.playerRealtime());
    ui->actionProgressive->setChecked(Settings.playerProgressive());
    ui->actionScrubAudio->setChecked(Settings.playerScrubAudio());
    if (ui->actionJack)
        ui->actionJack->setChecked(Settings.playerJACK());

    QString external = Settings.playerExternal();
    bool ok = false;
    external.toInt(&ok);
    auto isExternalPeripheral = !external.isEmpty() && !ok;

    setAudioChannels(Settings.playerAudioChannels());

    if (isExternalPeripheral) {
        ui->actionPreview360->setEnabled(false);
        ui->actionPreview540->setEnabled(false);
    } else {
        ui->actionPreview360->setEnabled(true);
        ui->actionPreview540->setEnabled(true);
    }
    setPreviewScale(Settings.playerPreviewScale());

    QString deinterlacer = Settings.playerDeinterlacer();
    QString interpolation = Settings.playerInterpolation();

    if (deinterlacer == "onefield")
        ui->actionOneField->setChecked(true);
    else if (deinterlacer == "linearblend")
        ui->actionLinearBlend->setChecked(true);
    else if (deinterlacer == "yadif-nospatial")
        ui->actionYadifTemporal->setChecked(true);
    else if (deinterlacer == "yadif")
        ui->actionYadifSpatial->setChecked(true);
    else
        ui->actionBwdif->setChecked(true);

    if (interpolation == "nearest")
        ui->actionNearest->setChecked(true);
    else if (interpolation == "bilinear")
        ui->actionBilinear->setChecked(true);
    else if (interpolation == "bicubic")
        ui->actionBicubic->setChecked(true);
    else
        ui->actionHyper->setChecked(true);

    foreach (QAction *a, m_externalGroup->actions()) {
#ifndef USE_SCREENS_FOR_EXTERNAL_MONITORING
        if (isExternalPeripheral)
#endif
            if (a->data() == external) {
                a->setChecked(true);
                if (a->data().toString().startsWith("decklink")) {
                    if (m_decklinkGammaMenu)
                        m_decklinkGammaMenu->setEnabled(true);
                    if (m_keyerMenu)
                        m_keyerMenu->setEnabled(true);
                }
                break;
            }
    }

    if (m_decklinkGammaGroup) {
        auto gamma = Settings.playerDecklinkGamma();
        for (auto a : m_decklinkGammaGroup->actions()) {
            if (a->data() == gamma) {
                a->setChecked(true);
                break;
            }
        }
    }
    if (m_keyerGroup) {
        auto keyer = Settings.playerKeyerMode();
        for (auto a : m_keyerGroup->actions()) {
            if (a->data() == keyer) {
                a->setChecked(true);
                break;
            }
        }
    }

    QString profile = Settings.playerProfile();
    // Automatic not permitted for SDI/HDMI
    if (isExternalPeripheral && profile.isEmpty())
        profile = "atsc_720p_50";
    foreach (QAction *a, m_profileGroup->actions()) {
        // Automatic not permitted for SDI/HDMI
        if (a->data().toString().isEmpty() && !external.isEmpty() && !ok)
            a->setDisabled(true);
        if (a->data().toString() == profile) {
            a->setChecked(true);
            break;
        }
    }

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (!::qEnvironmentVariableIsSet("SDL_AUDIODRIVER")) {
        ::qputenv("SDL_AUDIODRIVER", Settings.playerAudioDriver().toLocal8Bit().constData());
    }
#endif

    LOG_DEBUG() << "end";
}

void MainWindow::readWindowSettings()
{
    LOG_DEBUG() << "begin";
    Settings.setWindowGeometryDefault(saveGeometry());
    Settings.setWindowStateDefault(saveState());
    Settings.sync();
    if (!Settings.windowGeometry().isEmpty()) {
        restoreState(Settings.windowState());
        restoreGeometry(Settings.windowGeometry());
    } else {
        restoreState(kLayoutEditingDefault);
    }
    LOG_DEBUG() << "end";
}

void MainWindow::setupActions()
{
    QAction *action;

    // Setup these actions as separators
    ui->actionProject->setSeparator(true);
    ui->actionUser_Interface->setSeparator(true);

    // Setup full screen action
    action = ui->actionEnterFullScreen;
    QList<QKeySequence> fullScreenShortcuts;
#ifdef Q_OS_MAC
    fullScreenShortcuts << QKeySequence(Qt::CTRL | Qt::META | Qt::Key_F);
    fullScreenShortcuts << QKeySequence(Qt::Key_F11);
    action->setShortcuts(fullScreenShortcuts);

    action = new QAction(tr("Preferences"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    connect(action, &QAction::triggered, this, &MainWindow::showSettingsMenu);
    ui->menuEdit->addAction(action);
#else
    fullScreenShortcuts << QKeySequence(Qt::Key_F11);
    fullScreenShortcuts << QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F);
    action->setShortcuts(fullScreenShortcuts);
#endif

    action = new QAction(tr("Rename Clip"), this);
    action->setShortcut(QKeySequence(Qt::Key_F2));
    connect(action, &QAction::triggered, this, [&]() {
        onPropertiesDockTriggered(true);
        emit renameRequested();
    });
    addAction(action);
    Actions.add("propertiesRenameClipAction", action, tr("Properties"));

    action = new QAction(tr("Find"), this);
    action->setShortcut(QKeySequence(Qt::Key_F3));
    connect(action, &QAction::triggered, this, [&]() {
        onRecentDockTriggered(true);
        m_recentDock->find();
    });
    addAction(action);
    Actions.add("recentFindAction", action, tr("Recent"));

    action = new QAction(tr("Reload"), this);
    action->setShortcut(QKeySequence(Qt::Key_F5));
    connect(action, &QAction::triggered, this, [&]() {
        m_timelineDock->model()->reload();
        m_keyframesDock->model().reload();
        m_filtersDock->load();
    });
    addAction(action);
    Actions.add("timelineReload", action, tr("Timeline"));

    action = new QAction(tr("Rerun Filter Analysis"), this);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        FindAnalysisFilterParser parser;
        parser.skipAnalyzed(false);
        // Look for two pass filters
        if (m_timelineDock->model()->tractor() && m_timelineDock->model()->tractor()->is_valid()) {
            parser.start(*m_timelineDock->model()->tractor());
        }
        if (m_playlistDock->model()->playlist() && m_playlistDock->model()->playlist()->is_valid()) {
            parser.start(*m_playlistDock->model()->playlist());
        }
        if (parser.filters().size() > 0) {
            QMessageBox dialog(QMessageBox::Question,
                               windowTitle(),
                               tr("This will start %n analysis job(s). Continue?",
                                  nullptr,
                                  parser.filters().size()),
                               QMessageBox::No | QMessageBox::Yes,
                               this);
            dialog.setDefaultButton(QMessageBox::Yes);
            dialog.setEscapeButton(QMessageBox::No);
            dialog.setWindowModality(QmlApplication::dialogModality());
            if (QMessageBox::Yes == dialog.exec()) {
                // If dialog accepted enqueue jobs.
                foreach (Mlt::Filter filter, parser.filters()) {
                    QScopedPointer<QmlMetadata> meta(new QmlMetadata);
                    QmlFilter qmlFilter(filter, meta.data());
                    bool isAudio = !::qstrcmp("loudness", filter.get("mlt_service"));
                    qmlFilter.analyze(isAudio, false);
                }
            }
        } else {
            emit showStatusMessage(tr("No filters to analyze."));
        }
    });
    Actions.add("analyzeFilters", action);
    ui->menuFile->insertAction(ui->actionShowProjectFolder, action);

    Actions.loadFromMenu(ui->menuFile);
    Actions.loadFromMenu(ui->menuEdit);
    Actions.loadFromMenu(ui->menuView);
    Actions.loadFromMenu(ui->menuPlayer);
    Actions.loadFromMenu(ui->menuSettings);
    Actions.loadFromMenu(ui->menuHelp);

    auto shortcuts{ui->actionKeyboardShortcuts->shortcuts()};
    shortcuts << QKeySequence(Qt::Key_Slash);
    ui->actionKeyboardShortcuts->setShortcuts(shortcuts);

    // Shortcuts for actions that are only in context menus do not work unless
    // they are added to something visible.
    addAction(Actions["timelineMergeWithNextAction"]);
    addAction(Actions["timelineDetachAudioAction"]);
    addAction(Actions["timelineAlignToReferenceAction"]);
    addAction(Actions["timelineUpdateThumbnailsAction"]);
    addAction(Actions["timelineRebuildAudioWaveformAction"]);
    addAction(Actions["keyframesTypePrevHoldAction"]);
    addAction(Actions["keyframesTypePrevLinearAction"]);
    addAction(Actions["keyframesTypePrevSmoothNaturalAction"]);
    addAction(Actions["keyframesTypePrevEaseOutSinuAction"]);
    addAction(Actions["keyframesTypePrevEaseOutQuadAction"]);
    addAction(Actions["keyframesTypePrevEaseOutCubeAction"]);
    addAction(Actions["keyframesTypePrevEaseOutQuartAction"]);
    addAction(Actions["keyframesTypePrevEaseOutQuintAction"]);
    addAction(Actions["keyframesTypePrevEaseOutExpoAction"]);
    addAction(Actions["keyframesTypePrevEaseOutCircAction"]);
    addAction(Actions["keyframesTypePrevEaseOutBackAction"]);
    addAction(Actions["keyframesTypePrevEaseOutElasAction"]);
    addAction(Actions["keyframesTypePrevEaseOutBounAction"]);
    addAction(Actions["keyframesTypeHoldAction"]);
    addAction(Actions["keyframesTypeLinearAction"]);
    addAction(Actions["keyframesTypeSmoothNaturalAction"]);
    addAction(Actions["keyframesTypeEaseInSinuAction"]);
    addAction(Actions["keyframesTypeEaseInQuadAction"]);
    addAction(Actions["keyframesTypeEaseInCubeAction"]);
    addAction(Actions["keyframesTypeEaseInQuartAction"]);
    addAction(Actions["keyframesTypeEaseInQuintAction"]);
    addAction(Actions["keyframesTypeEaseInExpoAction"]);
    addAction(Actions["keyframesTypeEaseInCircAction"]);
    addAction(Actions["keyframesTypeEaseInBackAction"]);
    addAction(Actions["keyframesTypeEaseInElasAction"]);
    addAction(Actions["keyframesTypeEaseInBounAction"]);
    addAction(Actions["keyframesTypeEaseInOutSinuAction"]);
    addAction(Actions["keyframesTypeEaseInOutQuadAction"]);
    addAction(Actions["keyframesTypeEaseInOutCubeAction"]);
    addAction(Actions["keyframesTypeEaseInOutQuartAction"]);
    addAction(Actions["keyframesTypeEaseInOutQuintAction"]);
    addAction(Actions["keyframesTypeEaseInOutExpoAction"]);
    addAction(Actions["keyframesTypeEaseInOutCircAction"]);
    addAction(Actions["keyframesTypeEaseInOutBackAction"]);
    addAction(Actions["keyframesTypeEaseInOutElasAction"]);
    addAction(Actions["keyframesTypeEaseInOutBounAction"]);
    addAction(Actions["keyframesRemoveAction"]);
    addAction(Actions["filesSearch"]);
    addAction(Actions["playlistSearch"]);

    Actions.initializeShortcuts();
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
    LOG_DEBUG() << "begin";
    if (m_profileGroup->checkedAction())
        setProfile(m_profileGroup->checkedAction()->data().toString());
    MLT.videoWidget()->setProperty("realtime", ui->actionRealtime->isChecked());
    bool ok = false;
    m_externalGroup->checkedAction()->data().toInt(&ok);
    if (!ui->menuExternal || m_externalGroup->checkedAction()->data().toString().isEmpty() || ok) {
        MLT.videoWidget()->setProperty("progressive", ui->actionProgressive->isChecked());
    } else {
        // DeckLink external monitor must strictly follow video mode
        MLT.videoWidget()->setProperty("mlt_service", m_externalGroup->checkedAction()->data());
        MLT.videoWidget()->setProperty("progressive", MLT.profile().progressive());
        ui->actionProgressive->setEnabled(false);
    }
    if (ui->actionChannels1->isChecked())
        setAudioChannels(1);
    else if (ui->actionChannels2->isChecked())
        setAudioChannels(2);
    else if (ui->actionChannels4->isChecked())
        setAudioChannels(4);
    else
        setAudioChannels(6);
    if (ui->actionOneField->isChecked())
        MLT.videoWidget()->setProperty("deinterlacer", "onefield");
    else if (ui->actionLinearBlend->isChecked())
        MLT.videoWidget()->setProperty("deinterlacer", "linearblend");
    else if (ui->actionYadifTemporal->isChecked())
        MLT.videoWidget()->setProperty("deinterlacer", "yadif-nospatial");
    else if (ui->actionYadifSpatial->isChecked())
        MLT.videoWidget()->setProperty("deinterlacer", "yadif");
    else
        MLT.videoWidget()->setProperty("deinterlacer", "bwdif");
    if (ui->actionNearest->isChecked())
        MLT.videoWidget()->setProperty("rescale", "nearest");
    else if (ui->actionBilinear->isChecked())
        MLT.videoWidget()->setProperty("rescale", "bilinear");
    else if (ui->actionBicubic->isChecked())
        MLT.videoWidget()->setProperty("rescale", "bicubic");
    else
        MLT.videoWidget()->setProperty("rescale", "hyper");
    if (m_decklinkGammaGroup && m_decklinkGammaGroup->isEnabled())
        MLT.videoWidget()->setProperty("decklinkGamma",
                                       m_decklinkGammaGroup->checkedAction()->data());
    if (m_keyerGroup)
        MLT.videoWidget()->setProperty("keyer", m_keyerGroup->checkedAction()->data());
    LOG_DEBUG() << "end";
}

void MainWindow::setCurrentFile(const QString &filename)
{
    if (filename == untitledFileName())
        m_currentFile.clear();
    else
        m_currentFile = filename;
    updateWindowTitle();
    ui->actionShowProjectFolder->setDisabled(m_currentFile.isEmpty());
}

void MainWindow::updateWindowTitle()
{
    QString shownName = tr("Untitled");
    if (!m_currentFile.isEmpty()) {
        shownName = QFileInfo(m_currentFile).filePath();
        shownName = fontMetrics().elidedText(shownName, Qt::ElideLeft, width() / 4);
    }
    QString profileText = tr("%1x%2 %3fps %4ch")
                              .arg(QString::number(MLT.profile().width(), 'f', 0),
                                   QString::number(MLT.profile().height(), 'f', 0),
                                   QString::number(MLT.profile().fps(), 'f', 2),
                                   QString::number(Settings.playerAudioChannels(), 'f', 0));
    if (!MLT.profile().is_explicit())
        profileText = tr("Automatic");
#ifdef Q_OS_MAC
    setWindowTitle(
        QStringLiteral("%1 - %2 - %3").arg(shownName).arg(profileText).arg(qApp->applicationName()));
#else
    setWindowTitle(QStringLiteral("%1[*] - %2 - %3")
                       .arg(shownName)
                       .arg(profileText)
                       .arg(qApp->applicationName()));
#endif
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    const auto copyright = QStringLiteral(
        "Copyright &copy; 2011-2025 <a href=\"https://www.meltytech.com/\">Meltytech</a>, LLC");
    const auto license = QStringLiteral(
        "<a href=\"https://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a>");
    const auto url = QStringLiteral("https://www.shotcut.org/");
    QMessageBox::about(
        this,
        tr("About %1").arg(qApp->applicationName()),
        QStringLiteral(
            "<h1>Shotcut version %2</h1>"
            "<p><a href=\"%3\">%1</a> is a free, open source, cross platform video editor.</p>"
            "<small><p>%4</p>"
            "<p>Licensed under the %5</p>"
            "<p>This program proudly uses the following projects:<ul>"
            "<li><a href=\"https://www.qt.io/\">Qt</a> application and UI framework</li>"
            "<li><a href=\"https://www.mltframework.org/\">MLT</a> multimedia authoring "
            "framework</li>"
            "<li><a href=\"https://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec "
            "libraries</li>"
            "<li><a href=\"https://www.videolan.org/developers/x264.html\">x264</a> H.264 "
            "encoder</li>"
            "<li><a href=\"https://www.videolan.org/projects/dav1d.html\">dav1d</a> AV1 "
            "decoder</li>"
            "<li><a href=\"https://gitlab.com/AOMediaCodec/SVT-AV1\">SVT-AV1</a> AV1 encoder</li>"
            "<li><a href=\"https://opus-codec.org/\">Opus</a> audio codec</li>"
            "<li><a href=\"https://www.dyne.org/software/frei0r/\">Frei0r</a> video plugins</li>"
            "<li><a href=\"https://www.ladspa.org/\">LADSPA</a> audio plugins</li>"
            "<li><a href=\"https://glaxnimate.mattbas.org/\">Glaxnimate</a> vector animation "
            "program</li>"
            "</ul></p>"
            "<p>The source code used to build this program can be downloaded from "
            "<a href=\"%3\">%3</a>.</p>"
            "This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</small>")
            .arg(qApp->applicationName(), qApp->applicationVersion(), url, copyright, license));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAccepted())
        return;

    bool handled = true;

    switch (event->key()) {
    case Qt::Key_J:
        if (m_isKKeyPressed)
            m_player->previousFrame();
        else
            m_player->rewind(false);
        break;
    case Qt::Key_K:
        m_player->pause();
        m_isKKeyPressed = true;
        break;
    case Qt::Key_L:
        if (event->modifiers() == Qt::NoModifier) {
            if (m_isKKeyPressed)
                m_player->nextFrame();
            else
                m_player->fastForward(false);
        }
        break;
    case Qt::Key_F12:
        LOG_DEBUG() << "event isAccepted:" << event->isAccepted();
        LOG_DEBUG() << "Current focusWidget:" << QApplication::focusWidget();
        LOG_DEBUG() << "Current focusObject:" << QApplication::focusObject();
        LOG_DEBUG() << "Current focusWindow:" << QApplication::focusWindow();
        handled = false;
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

void MainWindow::keyReleaseEvent(QKeyEvent *event)
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
    ui->actionAppDataSet->setVisible(false);
}

QAction *MainWindow::actionAddCustomProfile() const
{
    return ui->actionAddCustomProfile;
}

QAction *MainWindow::actionProfileRemove() const
{
    return ui->actionProfileRemove;
}

void MainWindow::buildVideoModeMenu(QMenu *topMenu,
                                    QMenu *&customMenu,
                                    QActionGroup *group,
                                    QAction *addAction,
                                    QAction *removeAction)
{
    topMenu->addAction(addProfile(group, "HD 720p 50 fps", "atsc_720p_50"));
    topMenu->addAction(addProfile(group, "HD 720p 59.94 fps", "atsc_720p_5994"));
    topMenu->addAction(addProfile(group, "HD 720p 60 fps", "atsc_720p_60"));
    topMenu->addAction(addProfile(group, "HD 1080i 25 fps", "atsc_1080i_50"));
    topMenu->addAction(addProfile(group, "HD 1080i 29.97 fps", "atsc_1080i_5994"));
    topMenu->addAction(addProfile(group, "HD 1080p 23.98 fps", "atsc_1080p_2398"));
    topMenu->addAction(addProfile(group, "HD 1080p 24 fps", "atsc_1080p_24"));
    topMenu->addAction(addProfile(group, "HD 1080p 25 fps", "atsc_1080p_25"));
    topMenu->addAction(addProfile(group, "HD 1080p 29.97 fps", "atsc_1080p_2997"));
    topMenu->addAction(addProfile(group, "HD 1080p 30 fps", "atsc_1080p_30"));
    topMenu->addAction(addProfile(group, "HD 1080p 50 fps", "atsc_1080p_50"));
    topMenu->addAction(addProfile(group, "HD 1080p 59.94 fps", "atsc_1080p_5994"));
    topMenu->addAction(addProfile(group, "HD 1080p 60 fps", "atsc_1080p_60"));
    topMenu->addAction(addProfile(group, "SD NTSC", "dv_ntsc"));
    topMenu->addAction(addProfile(group, "SD PAL", "dv_pal"));
    topMenu->addAction(addProfile(group, "UHD 2160p 23.98 fps", "uhd_2160p_2398"));
    topMenu->addAction(addProfile(group, "UHD 2160p 24 fps", "uhd_2160p_24"));
    topMenu->addAction(addProfile(group, "UHD 2160p 25 fps", "uhd_2160p_25"));
    topMenu->addAction(addProfile(group, "UHD 2160p 29.97 fps", "uhd_2160p_2997"));
    topMenu->addAction(addProfile(group, "UHD 2160p 30 fps", "uhd_2160p_30"));
    topMenu->addAction(addProfile(group, "UHD 2160p 50 fps", "uhd_2160p_50"));
    topMenu->addAction(addProfile(group, "UHD 2160p 59.94 fps", "uhd_2160p_5994"));
    topMenu->addAction(addProfile(group, "UHD 2160p 60 fps", "uhd_2160p_60"));
    QMenu *menu = topMenu->addMenu(tr("Non-Broadcast"));
    menu->addAction(addProfile(group, "640x480 4:3 NTSC", "square_ntsc"));
    menu->addAction(addProfile(group, "768x576 4:3 PAL", "square_pal"));
    menu->addAction(addProfile(group, "854x480 16:9 NTSC", "square_ntsc_wide"));
    menu->addAction(addProfile(group, "1024x576 16:9 PAL", "square_pal_wide"));
    menu->addAction(addProfile(group, tr("DVD Widescreen NTSC"), "dv_ntsc_wide"));
    menu->addAction(addProfile(group, tr("DVD Widescreen PAL"), "dv_pal_wide"));
    menu->addAction(addProfile(group, "HD 720p 23.98 fps", "atsc_720p_2398"));
    menu->addAction(addProfile(group, "HD 720p 24 fps", "atsc_720p_24"));
    menu->addAction(addProfile(group, "HD 720p 25 fps", "atsc_720p_25"));
    menu->addAction(addProfile(group, "HD 720p 29.97 fps", "atsc_720p_2997"));
    menu->addAction(addProfile(group, "HD 720p 30 fps", "atsc_720p_30"));
    menu->addAction(addProfile(group, "HD 1080i 60 fps", "atsc_1080i_60"));
    menu->addAction(addProfile(group, "HDV 1080i 25 fps", "hdv_1080_50i"));
    menu->addAction(addProfile(group, "HDV 1080i 29.97 fps", "hdv_1080_60i"));
    menu->addAction(addProfile(group, "HDV 1080p 25 fps", "hdv_1080_25p"));
    menu->addAction(addProfile(group, "HDV 1080p 29.97 fps", "hdv_1080_30p"));
    menu->addAction(addProfile(group, tr("Square 1080p 30 fps"), "square_1080p_30"));
    menu->addAction(addProfile(group, tr("Square 1080p 60 fps"), "square_1080p_60"));
    menu->addAction(addProfile(group, tr("Vertical HD 30 fps"), "vertical_hd_30"));
    menu->addAction(addProfile(group, tr("Vertical HD 60 fps"), "vertical_hd_60"));
    customMenu = topMenu->addMenu(tr("Custom"));
    customMenu->addAction(addAction);
    // Load custom profiles
    QDir dir(Settings.appDataLocation());
    if (dir.cd("profiles")) {
        QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        if (profiles.length() > 0) {
            customMenu->addAction(removeAction);
            customMenu->addSeparator();
        }
        foreach (QString name, profiles)
            customMenu->addAction(addProfile(group, name, dir.filePath(name)));
    }
}

void MainWindow::newProject(const QString &filename, bool isProjectFolder)
{
    if (isProjectFolder) {
        QFileInfo info(filename);
        MLT.setProjectFolder(info.absolutePath());
    }
    if (saveXML(filename)) {
        QMutexLocker locker(&m_autosaveMutex);
        if (m_autosaveFile)
            m_autosaveFile->changeManagedFile(filename);
        else
            m_autosaveFile.reset(new AutoSaveFile(filename));
        setCurrentFile(filename);
        setWindowModified(false);
        resetSourceUpdated();
        if (MLT.producer())
            showStatusMessage(tr("Saved %1").arg(m_currentFile));
        m_undoStack->setClean();
        m_recentDock->add(filename);
    } else {
        showSaveError();
    }
}

void MainWindow::addCustomProfile(const QString &name,
                                  QMenu *menu,
                                  QAction *action,
                                  QActionGroup *group)
{
    // Add new profile to the menu.
    QDir dir(Settings.appDataLocation());
    if (dir.cd("profiles")) {
        QStringList profiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        if (profiles.length() == 1) {
            menu->addAction(action);
            menu->addSeparator();
        }
        action = addProfile(group, name, dir.filePath(name));
        action->setChecked(true);
        menu->addAction(action);
        Settings.setPlayerProfile(dir.filePath(name));
        Settings.sync();
    }
}

void MainWindow::removeCustomProfiles(const QStringList &profiles,
                                      QDir &dir,
                                      QMenu *menu,
                                      QAction *action)
{
    foreach (const QString &profile, profiles) {
        // Remove the file.
        dir.remove(profile);
        // Locate the menu item.
        foreach (QAction *a, menu->actions()) {
            if (a->text() == profile) {
                // Remove the menu item.
                delete a;
                break;
            }
        }
    }
    // If no more custom video modes.
    if (menu->actions().size() == 3) {
        // Remove the Remove action and separator.
        menu->removeAction(action);
        foreach (QAction *a, menu->actions()) {
            if (a->isSeparator()) {
                delete a;
                break;
            }
        }
    }
}

// Drag-n-drop events

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::DragEnter && target == MLT.videoWidget()) {
        dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return true;
    } else if (event->type() == QEvent::Drop && target == MLT.videoWidget()) {
        dropEvent(static_cast<QDropEvent *>(event));
        return true;
    } else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        if (QEvent::KeyPress == event->type()) {
            // Let Shift+Escape be a global hook to defocus a widget (assign global player focus).
            auto keyEvent = static_cast<QKeyEvent *>(event);
            if (Qt::Key_Escape == keyEvent->key() && Qt::ShiftModifier == keyEvent->modifiers()) {
                Actions["playerFocus"]->trigger();
                return true;
            }
        }
        QQuickWidget *focusedQuickWidget = qobject_cast<QQuickWidget *>(qApp->focusWidget());
        if (focusedQuickWidget && focusedQuickWidget->quickWindow()->activeFocusItem()) {
            event->accept();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            qApp->sendEvent(focusedQuickWidget->quickWindow()->activeFocusItem(), event);
#else
            focusedQuickWidget->quickWindow()
                ->sendEvent(focusedQuickWidget->quickWindow()->activeFocusItem(), event);
#endif
            QWidget *w = focusedQuickWidget->parentWidget();
            if (!event->isAccepted())
                qApp->sendEvent(w, event);
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Simulate the player firing a dragStarted event to make the playlist close
    // its help text view. This lets one drop a clip directly into the playlist
    // from a fresh start.
    auto *videoWidget = (Mlt::VideoWidget *) &Mlt::Controller::singleton();
    emit videoWidget->dragStarted();

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QMap<int, QVariant> roleDataMap;
        while (!stream.atEnd()) {
            int row, col;
            stream >> row >> col >> roleDataMap;
        }
        if (roleDataMap.contains(Qt::ToolTipRole)) {
            // DisplayRole is just basename, ToolTipRole contains full path
            open(roleDataMap[Qt::ToolTipRole].toString());
            event->acceptProposedAction();
        }
    } else if (mimeData->hasUrls()) {
        // Use QTimer to workaround stupid drag from Windows Explorer bug
        const auto &urls = mimeData->urls();
        QTimer::singleShot(0, this, [=]() { openMultiple(urls); });
        event->acceptProposedAction();
    } else if (mimeData->hasFormat(Mlt::XmlMimeType)
               && MLT.XML() != mimeData->data(Mlt::XmlMimeType)) {
        m_playlistDock->onOpenActionTriggered();
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_timelineDock->stopRecording();
    if (continueJobsRunning() && continueModified()) {
        LOG_DEBUG() << "begin";
        JOBS.cleanup();
        if (m_exitCode != EXIT_RESET) {
            writeSettings();
        }
        if (m_exitCode == EXIT_SUCCESS) {
            MLT.stop();
        } else {
            if (multitrack())
                m_timelineDock->model()->close();
            if (playlist())
                m_playlistDock->model()->close();
            else
                onMultitrackClosed();
        }
        QThreadPool::globalInstance()->clear();
        AudioLevelsTask::closeAll();
        event->accept();
        emit aboutToShutDown();
        if (m_exitCode == EXIT_SUCCESS) {
            QApplication::quit();
            LOG_DEBUG() << "end";
            ::_Exit(0);
        } else {
            QApplication::exit(m_exitCode);
            LOG_DEBUG() << "end";
        }
        return;
    }
    event->ignore();
}

void MainWindow::showEvent(QShowEvent *event)
{
    // This is needed to prevent a crash on windows on startup when timeline
    // is visible and dock title bars are hidden.
    Q_UNUSED(event)
    ui->actionShowTitleBars->setChecked(Settings.showTitleBars());
    on_actionShowTitleBars_triggered(Settings.showTitleBars());
    ui->actionShowToolbar->setChecked(Settings.showToolBar());
    on_actionShowToolbar_triggered(Settings.showToolBar());
    ui->actionShowTextUnderIcons->setChecked(Settings.textUnderIcons());
    on_actionShowTextUnderIcons_toggled(Settings.textUnderIcons());
    ui->actionShowSmallIcons->setChecked(Settings.smallIcons());
    on_actionShowSmallIcons_toggled(Settings.smallIcons());

    windowHandle()->installEventFilter(this);

#ifndef SHOTCUT_NOUPGRADE
    if (!Settings.noUpgrade() && !qApp->property("noupgrade").toBool())
        QTimer::singleShot(0, this, SLOT(showUpgradePrompt()));
#endif

#if defined(Q_OS_WIN) && (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    WindowsTaskbarButton::getInstance().setParentWindow(this);
#endif
    onAutosaveTimeout();

    QTimer::singleShot(400, this, [=]() {
        Database::singleton(this);
        this->setProperty("windowOpacity", 1.0);
    });
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    setProperty("windowOpacity", 0.0);
}

void MainWindow::on_actionOpenOther_triggered()
{
    auto dialog = new OpenOtherDialog(this);
    if (MLT.producer()) {
        // open dialog with previous configuration
        dialog->load(MLT.producer());
    }
    auto result = dialog->exec();
    m_producerWidget.reset(dialog->currentWidget());
    onOpenOtherFinished(result);
}

void MainWindow::onProducerOpened(bool withReopen)
{
    QWidget *w = loadProducerWidget(MLT.producer());
    if (withReopen && w && !MLT.producer()->get(kMultitrackItemProperty)) {
        if (-1 != w->metaObject()->indexOfSignal("producerReopened(bool)"))
            connect(w, SIGNAL(producerReopened(bool)), m_player, SLOT(onProducerOpened(bool)));
    } else if (MLT.isPlaylist()) {
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
    } else if (MLT.isMultitrack()) {
        m_timelineDock->model()->load();
        if (isMultitrackValid()) {
            m_player->setIn(-1);
            m_player->setOut(-1);
            m_timelineDock->setVisible(true);
            m_timelineDock->raise();
            m_player->enableTab(Player::ProjectTabIndex);
            m_player->switchToTab(Player::ProjectTabIndex);
            m_timelineDock->selectMultitrack();
            m_timelineDock->setSelection();
        }
    }
    if (MLT.isClip()) {
        m_filterController->setProducer(MLT.producer());
        m_player->enableTab(Player::SourceTabIndex);
        m_player->switchToTab(MLT.isClosedClip() ? Player::ProjectTabIndex : Player::SourceTabIndex);
        Util::getHash(*MLT.producer());
    }
    ui->actionSave->setEnabled(true);
    QMutexLocker locker(&m_autosaveMutex);
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
        && MLT.producer()->get_int(kPlaylistIndexProperty)) {
        emit m_playlistDock->enableUpdate(true);
    }
    sourceUpdated();
}

bool MainWindow::on_actionSave_triggered()
{
    m_timelineDock->stopRecording();
    if (m_currentFile.isEmpty()) {
        return on_actionSave_As_triggered();
    } else {
        if (Util::warnIfNotWritable(m_currentFile, this, tr("Save XML")))
            return false;
        backupPeriodically();
        bool success = saveXML(m_currentFile);
        QMutexLocker locker(&m_autosaveMutex);
        m_autosaveFile.reset(new AutoSaveFile(m_currentFile));
        setCurrentFile(m_currentFile);
        setWindowModified(false);
        if (success) {
            showStatusMessage(tr("Saved %1").arg(m_currentFile));
        } else {
            showSaveError();
        }
        m_undoStack->setClean();
        return true;
    }
}

bool MainWindow::on_actionSave_As_triggered()
{
    QString path = Settings.savePath();
    if (!m_currentFile.isEmpty())
        path = m_currentFile;
    QString caption = tr("Save XML");
    QString filename = QFileDialog::getSaveFileName(this,
                                                    caption,
                                                    path,
                                                    tr("MLT XML (*.mlt)"),
                                                    nullptr,
                                                    Util::getFileDialogOptions());
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        Settings.setSavePath(fi.path());
        if (fi.suffix() != "mlt")
            filename += ".mlt";

        if (Util::warnIfNotWritable(filename, this, caption))
            return false;
        newProject(filename);
    }
    return !filename.isEmpty();
}

void MainWindow::on_actionBackupSave_triggered()
{
    m_timelineDock->stopRecording();
    if (m_currentFile.isEmpty()) {
        on_actionSave_As_triggered();
    } else {
        backup();
        if (isWindowModified())
            on_actionSave_triggered();
    }
}

void MainWindow::on_actionPauseAfterSeek_triggered(bool checked)
{
    Settings.setPlayerPauseAfterSeek(checked);
}

void MainWindow::cropSource(const QRectF &rect)
{
    filterController()->removeCurrent();

    auto model = filterController()->attachedModel();
    Mlt::Service service;
    for (int i = 0; i < model->rowCount(); i++) {
        service = model->getService(i);
        if (!qstrcmp("crop", service.get("mlt_service")))
            break;
    }
    if (!service.is_valid()) {
        auto meta = filterController()->metadata("crop");
        service = model->getService(model->add(meta));
        service.set("use_profile", 1);
    }
    service.set("left", rect.x());
    service.set("right", MLT.profile().width() - rect.x() - rect.width());
    service.set("top", rect.y());
    service.set("bottom", MLT.profile().height() - rect.y() - rect.height());

    auto newWidth = Util::coerceMultiple(rect.width());
    auto newHeight = Util::coerceMultiple(rect.height());
    QMessageBox dialog(QMessageBox::Question,
                       qApp->applicationName(),
                       tr("Do you also want to change the Video Mode to %1 x %2?")
                           .arg(newWidth)
                           .arg(newHeight),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    if (QMessageBox::Yes == dialog.exec()) {
        auto leftRatio = rect.x() / MLT.profile().width();
        auto rightRatio = 1.0 - (rect.x() + newWidth) / MLT.profile().width();
        auto topRatio = rect.y() / MLT.profile().height();
        auto bottomRatio = 1.0 - (rect.y() + newHeight) / MLT.profile().height();

        service.set("left", qRound(leftRatio * newWidth));
        service.set("right", qRound(rightRatio * newWidth));
        service.set("top", qRound(topRatio * newHeight));
        service.set("bottom", qRound(bottomRatio * newHeight));

        MLT.profile().set_width(newWidth);
        MLT.profile().set_height(newHeight);
        MLT.profile().set_display_aspect(newWidth * MLT.profile().sar(), newHeight);
        MLT.updatePreviewProfile();
        MLT.setPreviewScale(Settings.playerPreviewScale());
        auto xml = MLT.XML();
        emit profileChanged();
        MLT.restart(xml);
    }
    emit producerOpened(false);
}

void MainWindow::getMarkerRange(int position, int *start, int *end)
{
    if (!MLT.isMultitrack()) {
        showStatusMessage(tr("Timeline is not loaded"));
    } else {
        MarkersModel *model = m_timelineDock->markersModel();
        int markerIndex = model->rangeMarkerIndexForPosition(position);
        if (markerIndex >= 0) {
            Markers::Marker marker = model->getMarker(markerIndex);
            *start = marker.start;
            *end = marker.end;
            return;
        } else {
            showStatusMessage(tr("Range marker not found under the timeline cursor"));
        }
    }
    *start = -1;
    *end = -1;
}

void MainWindow::getSelectionRange(int *start, int *end)
{
    if (MLT.isMultitrack()) {
        m_timelineDock->getSelectionRange(start, end);
    } else if (MLT.isPlaylist()) {
        m_playlistDock->getSelectionRange(start, end);
    } else if (MLT.isSeekableClip()) {
        *start = MLT.producer()->get_in();
        *end = MLT.producer()->get_out();
    } else {
        *start = -1;
        *end = -1;
    }
}

Mlt::Playlist *MainWindow::binPlaylist()
{
    return m_playlistDock->binPlaylist();
}

void MainWindow::showInFiles(const QString &filePath)
{
    onFilesDockTriggered();
    m_filesDock->changeDirectory(filePath);
}

bool MainWindow::continueModified()
{
    if (isWindowModified()) {
        QMessageBox dialog(QMessageBox::Warning,
                           qApp->applicationName(),
                           tr("The project has been modified.\n"
                              "Do you want to save your changes?"),
                           QMessageBox::No | QMessageBox::Cancel | QMessageBox::Yes,
                           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::Cancel);
        int r = dialog.exec();
        if (r == QMessageBox::Yes || r == QMessageBox::No) {
            if (r == QMessageBox::Yes) {
                return on_actionSave_triggered();
            } else {
                QMutexLocker locker(&m_autosaveMutex);
                m_autosaveFile.reset();
            }
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
                              "Do you still want to exit?"),
                           QMessageBox::No | QMessageBox::Yes,
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
                              "Do you still want to exit?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        return (dialog.exec() == QMessageBox::Yes);
    }
    return true;
}

QUndoStack *MainWindow::undoStack() const
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
    if (started && MLT.resource().startsWith("avfoundation")
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

void MainWindow::onMarkersDockTriggered(bool checked)
{
    if (checked) {
        m_markersDock->show();
        m_markersDock->raise();
    }
}

void MainWindow::onNotesDockTriggered(bool checked)
{
    if (checked) {
        m_notesDock->show();
        m_notesDock->raise();
    }
}

void MainWindow::onSubtitlesDockTriggered(bool checked)
{
    if (checked) {
        m_subtitlesDock->show();
        m_subtitlesDock->raise();
    }
}

void MainWindow::onFilesDockTriggered(bool checked)
{
    if (checked) {
        m_filesDock->show();
        m_filesDock->raise();
    }
}

void MainWindow::onPlaylistCreated()
{
    if (!playlist() || playlist()->count() == 0)
        return;
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
    setProfile(Settings.playerProfile());
    resetVideoModeMenu();
    setAudioChannels(Settings.playerAudioChannels());
    setCurrentFile("");
    setWindowModified(false);
    resetSourceUpdated();
    m_undoStack->clear();
    MLT.resetURL();
    QMutexLocker locker(&m_autosaveMutex);
    m_autosaveFile.reset(new AutoSaveFile(untitledFileName()));
    if (!isMultitrackValid())
        m_player->enableTab(Player::ProjectTabIndex, false);
}

void MainWindow::onPlaylistModified()
{
    setWindowModified(true);
    if (MLT.producer() && playlist()
        && (void *) MLT.producer()->get_producer() == (void *) playlist()->get_playlist())
        m_player->onDurationChanged();
    updateMarkers();
    m_player->enableTab(Player::ProjectTabIndex, true);
}

void MainWindow::onMultitrackCreated()
{
    m_player->enableTab(Player::ProjectTabIndex, true);
    QString trackTransitionService = m_timelineDock->model()->trackTransitionService();
    m_filterController->setTrackTransitionService(trackTransitionService);
}

void MainWindow::onMultitrackClosed()
{
    setAudioChannels(Settings.playerAudioChannels());
    setProfile(Settings.playerProfile());
    resetVideoModeMenu();
    setCurrentFile("");
    setWindowModified(false);
    resetSourceUpdated();
    m_undoStack->clear();
    MLT.resetURL();
    QMutexLocker locker(&m_autosaveMutex);
    m_autosaveFile.reset(new AutoSaveFile(untitledFileName()));
    if (!playlist() || playlist()->count() == 0)
        m_player->enableTab(Player::ProjectTabIndex, false);
}

void MainWindow::onMultitrackModified()
{
    setWindowModified(true);

    // Reflect this playlist info onto the producer for keyframes dock.
    if (!m_timelineDock->selection().isEmpty()) {
        int trackIndex = m_timelineDock->selection().first().y();
        int clipIndex = m_timelineDock->selection().first().x();
        auto info = m_timelineDock->model()->getClipInfo(trackIndex, clipIndex);
        if (info && info->producer && info->producer->is_valid()) {
            int expected = info->frame_in;
            auto info2 = m_timelineDock->model()->getClipInfo(trackIndex, clipIndex - 1);
            if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
                // Factor in a transition left of the clip.
                expected -= info2->frame_count;
                info->producer->set(kPlaylistStartProperty, info2->start);
            } else {
                info->producer->set(kPlaylistStartProperty, info->start);
            }
            if (expected != info->producer->get_int(kFilterInProperty)) {
                int delta = expected - info->producer->get_int(kFilterInProperty);
                info->producer->set(kFilterInProperty, expected);
                emit m_filtersDock->producerInChanged(delta);
            }
            expected = info->frame_out;
            info2 = m_timelineDock->model()->getClipInfo(trackIndex, clipIndex + 1);
            if (info2 && info2->producer && info2->producer->is_valid()
                && info2->producer->get(kShotcutTransitionProperty)) {
                // Factor in a transition right of the clip.
                expected += info2->frame_count;
            }
            if (expected != info->producer->get_int(kFilterOutProperty)) {
                int delta = expected - info->producer->get_int(kFilterOutProperty);
                info->producer->set(kFilterOutProperty, expected);
                emit m_filtersDock->producerOutChanged(delta);
            }
        }
    }
    MLT.refreshConsumer();
}

void MainWindow::onMultitrackDurationChanged()
{
    if (MLT.producer()
        && (void *) MLT.producer()->get_producer() == (void *) multitrack()->get_producer())
        m_player->onDurationChanged();
}

void MainWindow::onNoteModified()
{
    setWindowModified(true);
}

void MainWindow::onSubtitleModified()
{
    setWindowModified(true);
}

void MainWindow::onCutModified()
{
    if (!playlist() && !multitrack()) {
        setWindowModified(true);
    }
    if (playlist()) {
        emit m_playlistDock->enableUpdate(true);
    }
    sourceUpdated();
}

void MainWindow::onProducerModified()
{
    setWindowModified(true);
    sourceUpdated();
    MLT.refreshConsumer();
}

void MainWindow::onFilterModelChanged()
{
    MLT.refreshConsumer();
    setWindowModified(true);
    sourceUpdated();
    if (playlist()) {
        emit m_playlistDock->enableUpdate(true);
    }
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

bool MainWindow::saveXML(const QString &filename, bool withRelativePaths)
{
    bool result;
    QString notes = m_notesDock->getText();
    if (m_timelineDock->model()->rowCount() > 0) {
        result = MLT.saveXML(filename, multitrack(), withRelativePaths, nullptr, false, notes);
    } else if (m_playlistDock->model()->rowCount() > 0 && MLT.producer()
               && MLT.producer()->is_valid()) {
        int in = MLT.producer()->get_in();
        int out = MLT.producer()->get_out();
        MLT.producer()->set_in_and_out(0, MLT.producer()->get_length() - 1);
        result = MLT.saveXML(filename, playlist(), withRelativePaths, nullptr, false, notes);
        MLT.producer()->set_in_and_out(in, out);
    } else if (MLT.producer() && MLT.producer()->is_valid()) {
        result = MLT.saveXML(filename,
                             (MLT.isMultitrack() || MLT.isPlaylist()) ? MLT.savedProducer() : 0,
                             withRelativePaths,
                             nullptr,
                             false,
                             notes);
    } else {
        // Save an empty playlist, which is accepted by both MLT and Shotcut.
        Mlt::Playlist playlist(MLT.profile());
        result = MLT.saveXML(filename, &playlist, withRelativePaths, nullptr, false, notes);
    }
    return result;
}

static const auto kThemeDark = QStringLiteral("dark");
static const auto kThemeLight = QStringLiteral("light");
static const auto kThemeSystem = QStringLiteral("system");
static const auto kThemeSystemFusion = QStringLiteral("system-fusion");
static const auto kStyleFusion = QStringLiteral("Fusion");
static const auto kIconsOxygen = QStringLiteral("oxygen");
static const auto kIconsDarkOxygen = QStringLiteral("oxygen-dark");

void MainWindow::changeTheme(const QString &theme)
{
    LOG_DEBUG() << "begin";
    LOG_DEBUG() << "Available styles:" << QStyleFactory::keys();
    auto mytheme = theme;

#if !defined(SHOTCUT_THEME)
    // Workaround Quick Controls not using our custom palette - temporarily?
    std::unique_ptr<QStyle> style{QStyleFactory::create("fusion")};
    auto brightness = style->standardPalette().color(QPalette::Text).lightnessF();
    LOG_DEBUG() << brightness;
    mytheme = brightness < 0.5f ? kThemeLight : kThemeDark;
    QApplication::setStyle(kStyleFusion);
    QIcon::setThemeName(mytheme);
#if defined(Q_OS_MAC)
    if (mytheme == kThemeDark) {
        auto palette = QGuiApplication::palette();
        palette.setColor(QPalette::AlternateBase, palette.color(QPalette::Base).lighter());
        QGuiApplication::setPalette(palette);
    }
#elif defined(Q_OS_WIN)
    QGuiApplication::setPalette(style->standardPalette());
#endif
#else
    if (mytheme == kThemeDark) {
        QApplication::setStyle(kStyleFusion);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(50, 50, 50));
        palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
        palette.setColor(QPalette::Base, QColor(30, 30, 30));
        palette.setColor(QPalette::AlternateBase, QColor(40, 40, 40));
        palette.setColor(QPalette::Highlight, QColor(23, 92, 118));
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
        palette.setColor(QPalette::Disabled, QPalette::Light, Qt::transparent);
        QApplication::setPalette(palette);
        QIcon::setThemeName(kThemeDark);
        if (!::qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_CONF"))
            ::qputenv("QT_QUICK_CONTROLS_CONF", ":/resources/qtquickcontrols2-dark.conf");
    } else if (mytheme == "light") {
        QApplication::setStyle(kStyleFusion);
        QPalette palette;
        palette.setColor(QPalette::Window, "#efefef");
        palette.setColor(QPalette::WindowText, "#000000");
        palette.setColor(QPalette::Base, "#ffffff");
        palette.setColor(QPalette::AlternateBase, "#f7f7f7");
        palette.setColor(QPalette::Highlight, "#308cc6");
        palette.setColor(QPalette::HighlightedText, "#ffffff");
        palette.setColor(QPalette::ToolTipBase, "#308cc6");
        palette.setColor(QPalette::ToolTipText, "#000000");
        palette.setColor(QPalette::Text, "#000000");
        palette.setColor(QPalette::BrightText, "#ffffff");
        palette.setColor(QPalette::Button, "#efefef");
        palette.setColor(QPalette::ButtonText, "#000000");
        palette.setColor(QPalette::Link, "#308cc6");
        palette.setColor(QPalette::LinkVisited, "#308cc6");
        palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
        palette.setColor(QPalette::Disabled, QPalette::Light, Qt::transparent);
        QApplication::setPalette(palette);
        QIcon::setThemeName(kThemeLight);
        if (!::qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_CONF"))
            ::qputenv("QT_QUICK_CONTROLS_CONF", ":/resources/qtquickcontrols2-light.conf");
    } else {
        // Use a macro since this can change on some OS after setStyle(Fusion)
#define isDark (QGuiApplication::palette().color(QPalette::Text).lightnessF() > 0.5f)
#if defined(Q_OS_WIN)
        if (!::qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE")) {
            // The modern Windows style adopted in Qt 6.7 changes spinboxes to have
            // larger arrow buttons side-by-side thus making the numeric area
            // smaller and incompatible with every other combination of style and OS.
            // Windows, windows11, & windowsvista styles all break the width of drop down menus!
            QApplication::setStyle(kStyleFusion);

            if (isDark) {
                QPalette palette;
                palette.setColor(QPalette::AlternateBase, palette.color(QPalette::Window));
                palette.setColor(QPalette::Button, palette.color(QPalette::Window).lighter());
                QApplication::setPalette(palette);
            }
        }
#elif defined(Q_OS_MAC)
        if (!::qEnvironmentVariableIsSet("QT_STYLE_OVERRIDE")) {
            // The macOS style is hideous in dark mode!
            QApplication::setStyle(isDark ? kStyleFusion : QStringLiteral("macOS"));
        }
        if (isDark) {
            QPalette palette;
            palette.setColor(QPalette::AlternateBase, palette.color(QPalette::Window));
            QApplication::setPalette(palette);
        }
#else
        QApplication::setStyle(qApp->property("system-style").toString());
#endif
        if (isDark)
            QIcon::setThemeName(mytheme == kThemeSystemFusion ? kThemeDark : kIconsDarkOxygen);
        else
            QIcon::setThemeName(mytheme == kThemeSystemFusion ? kThemeLight : kIconsOxygen);

        if (!::qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_CONF")) {
            if (!isDark)
                ::qputenv("QT_QUICK_CONTROLS_CONF", ":/resources/qtquickcontrols2-light.conf");
            else
                ::qputenv("QT_QUICK_CONTROLS_CONF", ":/resources/qtquickcontrols2-dark.conf");
        }
    }
#endif

    //    auto pal = QGuiApplication::palette();
    //    LOG_INFO() << "altBase" << pal.alternateBase().color().name();
    //    LOG_INFO() << "base" << pal.base().color().name();
    //    LOG_INFO() << "window" << pal.window().color().name();
    //    LOG_INFO() << "windowText" << pal.windowText().color().name();
    //    LOG_INFO() << "toolTipBase" << pal.toolTipBase().color().name();
    //    LOG_INFO() << "toolTipText" << pal.toolTipText().color().name();
    //    LOG_INFO() << "text" << pal.text().color().name();
    //    LOG_INFO() << "button" << pal.button().color().name();
    //    LOG_INFO() << "buttonText" << pal.buttonText().color().name();
    //    LOG_INFO() << "placeholderText" << pal.placeholderText().color().name();
    //    LOG_INFO() << "brightText" << pal.brightText().color().name();
    //    LOG_INFO() << "highlight" << pal.highlight().color().name();
    //    LOG_INFO() << "highlightedText" << pal.highlightedText().color().name();
    //    LOG_INFO() << "link" << pal.link().color().name();
    //    LOG_INFO() << "linkVisited" << pal.linkVisited().color().name();
    LOG_DEBUG() << "end";
}

Mlt::Playlist *MainWindow::playlist() const
{
    return m_playlistDock->model()->playlist();
}

bool MainWindow::isPlaylistValid() const
{
    return m_playlistDock->model()->playlist() && m_playlistDock->model()->rowCount() > 0;
}

Mlt::Producer *MainWindow::multitrack() const
{
    return m_timelineDock->model()->tractor();
}

bool MainWindow::isMultitrackValid() const
{
    return m_timelineDock->model()->tractor() && !m_timelineDock->model()->trackList().empty();
}

QWidget *MainWindow::loadProducerWidget(Mlt::Producer *producer)
{
    QWidget *w = 0;
    QScrollArea *scrollArea = (QScrollArea *) m_propertiesDock->widget();

    if (!producer || !producer->is_valid()) {
        if (scrollArea->widget())
            scrollArea->widget()->deleteLater();
        return w;
    } else {
        scrollArea->show();
    }

    QString service(producer->get("mlt_service"));
    QString resource = QString::fromUtf8(producer->get("resource"));
    QString shotcutProducer(producer->get(kShotcutProducerProperty));

    if (resource.startsWith("video4linux2:")
        || QString::fromUtf8(producer->get("resource1")).startsWith("video4linux2:"))
        w = new Video4LinuxWidget(this);
    else if (resource.startsWith("pulse:"))
        w = new PulseAudioWidget(this);
    else if (resource.startsWith("alsa:"))
        w = new AlsaWidget(this);
    else if (resource.startsWith("dshow:")
             || QString::fromUtf8(producer->get("resource1")).startsWith("dshow:"))
        w = new DirectShowVideoWidget(this);
    else if (resource.startsWith("avfoundation:"))
        w = new AvfoundationProducerWidget(this);
    else if (QString::fromLatin1(producer->get(kPrivateProducerProperty)) == "htmlGenerator")
        w = new HtmlGeneratorWidget(this);
    else if (service.startsWith("avformat") || shotcutProducer == "avformat")
        w = new AvformatProducerWidget(this);
    else if (MLT.isImageProducer(producer)) {
        w = new ImageProducerWidget(this);
        connect(m_player, SIGNAL(outChanged(int)), w, SLOT(updateDuration()));
    } else if (service == "decklink" || resource.contains("decklink"))
        w = new DecklinkProducerWidget(this);
    else if (service == "color")
        w = new ColorProducerWidget(this);
    else if (service == "glaxnimate")
        w = new GlaxnimateProducerWidget(this);
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
    else if (service == "tone")
        w = new ToneProducerWidget(this);
    else if (service == "count")
        w = new CountProducerWidget(this);
    else if (service == "blipflash")
        w = new BlipProducerWidget(this);
    else if (service == "xml-clip")
        w = new MltClipProducerWidget(this);
    else if (producer->parent().get(kShotcutTransitionProperty)) {
        w = new LumaMixTransition(producer->parent(), this);
        scrollArea->setWidget(w);
        if (-1 != w->metaObject()->indexOfSignal("modified()")) {
            connect(w, SIGNAL(modified()), SLOT(onProducerModified()));
        }
        if (-1 != w->metaObject()->indexOfSlot("onPlaying()")) {
            connect(MLT.videoWidget(), SIGNAL(playing()), w, SLOT(onPlaying()));
        }
        return w;
    } else if (mlt_service_playlist_type == producer->type()) {
        int trackIndex = m_timelineDock->currentTrack();
        bool isBottomVideo = m_timelineDock->model()
                                 ->data(m_timelineDock->model()->index(trackIndex),
                                        MultitrackModel::IsBottomVideoRole)
                                 .toBool();
        if (!isBottomVideo) {
            w = new TrackPropertiesWidget(*producer, this);
            scrollArea->setWidget(w);
            return w;
        }
    } else if (mlt_service_tractor_type == producer->type()) {
        w = new TimelinePropertiesWidget(*producer, this);
        scrollArea->setWidget(w);
        connect(w, SIGNAL(editProfile()), SLOT(on_actionAddCustomProfile_triggered()));
        return w;
    }
    if (w) {
        AbstractProducerWidget *pw = dynamic_cast<AbstractProducerWidget *>(w);
        if (pw) {
            pw->setProducer(producer);
        } else {
            LOG_ERROR() << "Widget is not a producer widget.";
            return w;
        }
        if (-1 != w->metaObject()->indexOfSignal("producerChanged(Mlt::Producer*)")) {
            connect(w, SIGNAL(producerChanged(Mlt::Producer *)), SLOT(onProducerChanged()));
            connect(w,
                    SIGNAL(producerChanged(Mlt::Producer *)),
                    m_filterController,
                    SLOT(setProducer(Mlt::Producer *)));
            connect(w,
                    SIGNAL(producerChanged(Mlt::Producer *)),
                    m_playlistDock,
                    SLOT(onProducerChanged(Mlt::Producer *)));
            if (producer->get(kMultitrackItemProperty))
                connect(w,
                        SIGNAL(producerChanged(Mlt::Producer *)),
                        m_timelineDock,
                        SLOT(onProducerChanged(Mlt::Producer *)));
        }
        if (-1 != w->metaObject()->indexOfSignal("modified()")) {
            connect(w, SIGNAL(modified()), SLOT(onProducerModified()));
            connect(w, SIGNAL(modified()), m_playlistDock, SLOT(onProducerModified()));
            connect(w, SIGNAL(modified()), m_timelineDock, SLOT(onProducerModified()));
            connect(w, SIGNAL(modified()), m_keyframesDock, SLOT(onProducerModified()));
            connect(w, SIGNAL(modified()), m_filterController, SLOT(onProducerChanged()));
        }
        if (-1 != w->metaObject()->indexOfSignal("showInFiles(QString)")) {
            connect(w, SIGNAL(showInFiles(QString)), this, SLOT(onFilesDockTriggered()));
            connect(w, SIGNAL(showInFiles(QString)), m_filesDock, SLOT(changeDirectory(QString)));
        }
        if (-1 != w->metaObject()->indexOfSlot("updateDuration()")) {
            connect(m_timelineDock, SIGNAL(durationChanged()), w, SLOT(updateDuration()));
        }
        if (-1 != w->metaObject()->indexOfSlot("rename()")) {
            connect(this, SIGNAL(renameRequested()), w, SLOT(rename()));
        }
        if (-1 != w->metaObject()->indexOfSlot("offerConvert(QString)")) {
            connect(m_filterController->attachedModel(),
                    SIGNAL(requestConvert(QString, bool, bool)),
                    w,
                    SLOT(offerConvert(QString, bool)),
                    Qt::QueuedConnection);
        }
        scrollArea->setWidget(w);
        onProducerChanged();
    } else if (scrollArea->widget()) {
        scrollArea->widget()->deleteLater();
    }
    return w;
}

void MainWindow::on_actionEnterFullScreen_triggered()
{
    bool isFull = isFullScreen();
    if (isFull) {
        showNormal();
        ui->actionEnterFullScreen->setText(tr("Enter Full Screen"));
    } else {
        showFullScreen();
        ui->actionEnterFullScreen->setText(tr("Exit Full Screen"));
    }
}

void MainWindow::onGpuNotSupported()
{
    if (Settings.processingMode() == ShotcutSettings::Linear10GpuCpu) {
        Settings.setProcessingMode(ShotcutSettings::Native8Cpu);
    }
    ui->actionNative10bitGpuCpu->setChecked(false);
    ui->actionNative10bitGpuCpu->setDisabled(true);
    LOG_WARNING() << "";
    QMessageBox::critical(this, qApp->applicationName(), tr("GPU processing is not supported"));
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
    if (Settings.checkUpgradeAutomatic()) {
        showStatusMessage("Checking for upgrade...");
        m_network.get(QNetworkRequest(QUrl("https://check.shotcut.org/version.json")));
    } else {
        QAction *action = new QAction(tr("Click here to check for a new version of Shotcut."), 0);
        connect(action, SIGNAL(triggered(bool)), SLOT(on_actionUpgrade_triggered()));
        showStatusMessage(action, 15 /* seconds */);
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
        MLT.updatePreviewProfile();
        MLT.restart();
    }
    Settings.setPlayerProgressive(checked);
}

void MainWindow::changeAudioChannels(bool checked, int channels)
{
    if (checked) {
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

void MainWindow::on_actionChannels4_triggered(bool checked)
{
    changeAudioChannels(checked, 4);
}

void MainWindow::on_actionChannels6_triggered(bool checked)
{
    changeAudioChannels(checked, 6);
}

void MainWindow::changeDeinterlacer(bool checked, const char *method)
{
    if (checked) {
        MLT.videoWidget()->setProperty("deinterlacer", method);
        if (MLT.consumer()) {
            MLT.consumer()->set("deinterlacer", method);
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

void MainWindow::on_actionBwdif_triggered(bool checked)
{
    changeDeinterlacer(checked, "bwdif");
}

void MainWindow::changeInterpolation(bool checked, const char *method)
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

void MainWindow::processMultipleFiles()
{
    if (m_multipleFiles.length() <= 0)
        return;
    QStringList multipleFiles = m_multipleFiles;
    m_multipleFiles.clear();
    int count = multipleFiles.length();
    if (count > 1) {
        m_multipleFilesLoading = true;
        LongUiTask longTask(tr("Open Files"));
        m_playlistDock->show();
        m_playlistDock->raise();
        ResourceDialog dialog(this);
        for (int i = 0; i < count; i++) {
            QString filename = multipleFiles.takeFirst();
            LOG_DEBUG() << filename;
            longTask.reportProgress(QFileInfo(filename).fileName(), i, count);
            Mlt::Producer p(MLT.profile(), filename.toUtf8().constData());
            if (p.is_valid()) {
                if (QDir::toNativeSeparators(filename)
                    == QDir::toNativeSeparators(MAIN.fileName())) {
                    MAIN.showStatusMessage(QObject::tr("You cannot add a project to itself!"));
                    continue;
                }
                Util::getHash(p);
                Mlt::Producer *producer = MLT.setupNewProducer(&p);
                ProxyManager::generateIfNotExists(*producer);
                producer->set(kShotcutSkipConvertProperty, true);
                undoStack()->push(
                    new Playlist::AppendCommand(*m_playlistDock->model(), MLT.XML(producer), false));
                m_recentDock->add(filename.toUtf8().constData());
                dialog.add(producer);
                delete producer;
            }
        }
        emit m_playlistDock->model()->modified();
        if (Settings.showConvertClipDialog() && dialog.hasTroubleClips()) {
            dialog.selectTroubleClips();
            dialog.setWindowTitle(tr("Opened Files"));
            dialog.exec();
        }
        m_multipleFilesLoading = false;
    }
    if (m_isPlaylistLoaded && Settings.playerGPU()) {
        updateThumbnails();
        m_isPlaylistLoaded = false;
    }
}

void MainWindow::processSingleFile()
{
    if (!m_multipleFilesLoading && Settings.showConvertClipDialog()
        && !MLT.producer()->get_int(kShotcutSkipConvertProperty)) {
        QString convertAdvice = Util::getConversionAdvice(MLT.producer());
        if (!convertAdvice.isEmpty()) {
            MLT.producer()->set(kShotcutSkipConvertProperty, true);
            LongUiTask::cancel();
            MLT.pause();
            Util::offerSingleFileConversion(convertAdvice, MLT.producer(), this);
        }
    }
}

void MainWindow::onLanguageTriggered(QAction *action)
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
        QMessageBox::warning(
            this,
            qApp->applicationName(),
            tr("Failed to connect to JACK.\nPlease verify that JACK is installed and running."));
    }
}

void MainWindow::onExternalTriggered(QAction *action)
{
    LOG_DEBUG() << action->data().toString();
    bool isExternal = !action->data().toString().isEmpty();
    QString profile = Settings.playerProfile();
    if (Settings.playerGPU() && MLT.producer() && Settings.playerExternal() != action->data()) {
        if (confirmRestartExternalMonitor()) {
            Settings.setPlayerExternal(action->data().toString());
            if (isExternal && profile.isEmpty()) {
                profile = "atsc_720p_50";
                Settings.setPlayerProfile(profile);
            }
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        } else {
            for (auto a : m_externalGroup->actions()) {
                if (a->data() == Settings.playerExternal()) {
                    a->setChecked(true);
                    if (a->data().toString().startsWith("decklink")) {
                        if (m_decklinkGammaMenu)
                            m_decklinkGammaMenu->setEnabled(true);
                        if (m_keyerMenu)
                            m_keyerMenu->setEnabled(true);
                    }
                    break;
                }
            }
        }
        return;
    }
    Settings.setPlayerExternal(action->data().toString());
    MLT.stop();
    bool ok = false;
    int screen = action->data().toInt(&ok);
    if (ok || action->data().toString().isEmpty()) {
        m_player->moveVideoToScreen(ok ? screen : -2);
        isExternal = false;
        MLT.videoWidget()->setProperty("mlt_service", QVariant());
    } else {
        m_player->moveVideoToScreen(-2);
        MLT.videoWidget()->setProperty("mlt_service", action->data());
    }

    // Automatic not permitted for SDI/HDMI
    if (isExternal && profile.isEmpty()) {
        profile = "atsc_720p_50";
        Settings.setPlayerProfile(profile);
        setProfile(profile);
        MLT.restart();
        foreach (QAction *a, m_profileGroup->actions()) {
            if (a->data() == profile) {
                a->setChecked(true);
                break;
            }
        }
    } else {
        MLT.consumerChanged();
    }
    // Automatic not permitted for SDI/HDMI
    m_profileGroup->actions().at(0)->setEnabled(!isExternal);

    // Disable progressive option when SDI/HDMI
    ui->actionProgressive->setEnabled(!isExternal);
    bool isProgressive = isExternal ? MLT.profile().progressive()
                                    : ui->actionProgressive->isChecked();
    MLT.videoWidget()->setProperty("progressive", isProgressive);
    if (MLT.consumer()) {
        MLT.consumer()->set("progressive", isProgressive);
        MLT.restart();
    }
    if (action->data().toString().startsWith("decklink")) {
        if (m_decklinkGammaMenu)
            m_decklinkGammaMenu->setEnabled(true);
        if (m_keyerMenu)
            m_keyerMenu->setEnabled(true);
    }

    // Preview scaling not permitted for SDI/HDMI
    if (isExternal) {
        ui->actionPreview360->setEnabled(false);
        ui->actionPreview540->setEnabled(false);
    } else {
        ui->actionPreview360->setEnabled(true);
        ui->actionPreview540->setEnabled(true);
    }
    setPreviewScale(Settings.playerPreviewScale());
}

void MainWindow::onDecklinkGammaTriggered(QAction *action)
{
    LOG_DEBUG() << action->data().toString();
    MLT.videoWidget()->setProperty("decklinkGamma", action->data());
    if (Settings.playerGPU() && MLT.producer()) {
        if (confirmRestartExternalMonitor()) {
            Settings.setPlayerDecklinkGamma(action->data().toInt());
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        } else {
            auto gamma = Settings.playerDecklinkGamma();
            for (auto a : m_decklinkGammaGroup->actions()) {
                if (a->data() == gamma) {
                    a->setChecked(true);
                    break;
                }
            }
        }
        return;
    }
    MLT.consumerChanged();
    Settings.setPlayerDecklinkGamma(action->data().toInt());
}

void MainWindow::onKeyerTriggered(QAction *action)
{
    LOG_DEBUG() << action->data().toString();
    MLT.videoWidget()->setProperty("keyer", action->data());
    if (Settings.playerGPU() && MLT.producer()) {
        if (confirmRestartExternalMonitor()) {
            Settings.setPlayerKeyerMode(action->data().toInt());
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        } else {
            auto keyer = Settings.playerKeyerMode();
            for (auto a : m_keyerGroup->actions()) {
                if (a->data() == keyer) {
                    a->setChecked(true);
                    break;
                }
            }
        }
        return;
    }
    MLT.consumerChanged();
    Settings.setPlayerKeyerMode(action->data().toInt());
}

void MainWindow::onProfileTriggered(QAction *action)
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        if (!confirmProfileChange())
            return;

        Settings.setPlayerProfile(action->data().toString());

        // Figure out the top-level project producer
        auto producer = MLT.producer();
        if (m_timelineDock->model()->rowCount() > 0) {
            producer = multitrack();
        } else if (m_playlistDock->model()->rowCount() > 0) {
            producer = playlist();
        } else if (MLT.isMultitrack() || MLT.isPlaylist()) {
            producer = MLT.savedProducer();
        }
        MLT.fixLengthProperties(*producer);
        // Save the XML to get correct in/out points before profile is changed.
        QString xml = MLT.XML(producer);
        setProfile(action->data().toString());
        MLT.restart(xml);
        emit producerOpened(false);
    } else {
        Settings.setPlayerProfile(action->data().toString());
        setProfile(action->data().toString());
    }
}

void MainWindow::onProfileChanged()
{
    if (multitrack() && MLT.isMultitrack()
        && (m_timelineDock->selection().isEmpty() || m_timelineDock->currentTrack() == -1)) {
        emit m_timelineDock->selected(multitrack());
    }
}

void MainWindow::on_actionAddCustomProfile_triggered()
{
    QString xml;
    if (MLT.producer() && MLT.producer()->is_valid()) {
        if (!confirmProfileChange())
            return;

        // Save the XML to get correct in/out points before profile is changed.
        xml = MLT.XML();
    }
    CustomProfileDialog dialog(this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.profileName();
        if (!name.isEmpty()) {
            addCustomProfile(name, customProfileMenu(), actionProfileRemove(), profileGroup());
        } else if (m_profileGroup->checkedAction()) {
            m_profileGroup->checkedAction()->setChecked(false);
        }
        // Use the new profile.
        emit profileChanged();
        if (!xml.isEmpty()) {
            MLT.restart(xml);
            emit producerOpened(false);
        }
    }
}

void MainWindow::restartAfterChangeTheme()
{
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("You must restart %1 to switch to the new theme.\n"
                          "Do you want to restart now?")
                           .arg(qApp->applicationName()),
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

void MainWindow::backup()
{
    QFileInfo info(m_currentFile);
    auto dateTime = info.lastModified().toString(Qt::ISODate);
    dateTime.replace(':', '-');
    auto filename = QStringLiteral("%1/%2 %3.mlt")
                        .arg(info.canonicalPath(), info.completeBaseName(), dateTime);
    if (!QFile::exists(filename) && !Util::warnIfNotWritable(filename, this, tr("Save XML"))
        && QFile::copy(m_currentFile, filename)) {
        showStatusMessage(tr("Saved backup %1").arg(filename));
    }
}

void MainWindow::backupPeriodically()
{
    auto dateTime = QFileInfo(m_currentFile).lastModified();
    if (Settings.backupPeriod() > 0 && !kBackupFileRegex.match(m_currentFile).hasMatch()
        && dateTime.secsTo(QDateTime::currentDateTime()) / 60 > Settings.backupPeriod()) {
        backup();
    }
}

bool MainWindow::confirmProfileChange()
{
    if (MLT.isClip() || !Settings.askChangeVideoMode())
        return true;

    QMessageBox dialog(QMessageBox::Warning,
                       QCoreApplication::applicationName(),
                       tr("<p>Please review your entire project after making this change.</p>"
                          "<p>Shotcut does not automatically adjust things that are sensitive to "
                          "size and position if you change resolution or aspect ratio.</p"
                          "<br>The timing of edits and keyframes may be slightly different if you "
                          "change frame rate.</p>"
                          "<p>It is a good idea to use <b>File > Backup and Save</b> before or "
                          "after this operation.</p>"
                          "<p>Do you want to change the <b>Video Mode</b> now?</p>"),
                       QMessageBox::No | QMessageBox::Yes,
                       &MAIN);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setCheckBox(
        new QCheckBox(tr("Do not show this anymore.", "Change video mode warning dialog")));
    auto result = QMessageBox::Yes == dialog.exec();
    if (dialog.checkBox()->isChecked())
        Settings.setAskChangeVideoMode(false);
    return result;
}

bool MainWindow::confirmRestartExternalMonitor()
{
    QMessageBox dialog(QMessageBox::Information,
                       qApp->applicationName(),
                       tr("Shotcut must restart to change external monitoring.\n"
                          "Do you want to restart now?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    return dialog.exec() == QMessageBox::Yes;
}

void MainWindow::resetFilterMenuIfNeeded()
{
    // Reset to Favorites if currently set to Color or Audio
    if (filterController()->metadataModel()->search() == "#color"
        || filterController()->metadataModel()->filter() == MetadataModel::AudioFilter) {
        filterController()->metadataModel()->setFilter(MetadataModel::FavoritesFilter);
        filterController()->metadataModel()->setSearch("");
    }
}

void MainWindow::on_actionSystemTheme_triggered()
{
    Settings.setTheme("system");
    restartAfterChangeTheme();
}

void MainWindow::on_actionSystemFusion_triggered()
{
    Settings.setTheme("system-fusion");
    restartAfterChangeTheme();
}

void MainWindow::on_actionFusionDark_triggered()
{
    Settings.setTheme("dark");
    restartAfterChangeTheme();
}

void MainWindow::on_actionFusionLight_triggered()
{
    Settings.setTheme("light");
    restartAfterChangeTheme();
}

void MainWindow::on_actionJobPriorityLow_triggered()
{
    Settings.setJobPriority("low");
}

void MainWindow::on_actionJobPriorityNormal_triggered()
{
    Settings.setJobPriority("normal");
}

void MainWindow::on_actionTutorials_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.shotcut.org/tutorials/"));
}

void MainWindow::on_actionRestoreLayout_triggered()
{
    auto mode = Settings.layoutMode();
    if (mode != LayoutMode::Custom) {
        // Clear the saved layout for this mode
        Settings.setLayout(QString(kReservedLayoutPrefix).arg(mode), QByteArray(), QByteArray());
        // Reset the layout mode so the current layout is saved as custom when trigger action
        Settings.setLayoutMode();
    }
    switch (mode) {
    case LayoutMode::Custom:
        ui->actionLayoutEditing->setChecked(true);
        Q_FALLTHROUGH();
    case LayoutMode::Editing:
        on_actionLayoutEditing_triggered();
        break;
    case LayoutMode::Logging:
        on_actionLayoutLogging_triggered();
        break;
    case LayoutMode::Effects:
        on_actionLayoutEffects_triggered();
        break;
    case LayoutMode::Color:
        on_actionLayoutColor_triggered();
        break;
    case LayoutMode::Audio:
        on_actionLayoutAudio_triggered();
        break;
    case LayoutMode::PlayerOnly:
        on_actionLayoutPlayer_triggered();
        break;
    }
}

void MainWindow::on_actionShowTitleBars_triggered(bool checked)
{
    QList<QDockWidget *> docks = findChildren<QDockWidget *>();
    for (int i = 0; i < docks.count(); i++) {
        QDockWidget *dock = docks.at(i);
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
#ifdef USE_SCREENS_FOR_EXTERNAL_MONITORING
    foreach (QAction *action, m_externalGroup->actions()) {
        bool ok = false;
        int i = action->data().toInt(&ok);
        if (ok && i < QGuiApplication::screens().count()) {
            if (QGuiApplication::screens().at(i) == screen()) {
                if (action->isChecked()) {
                    m_externalGroup->actions().first()->setChecked(true);
                    Settings.setPlayerExternal(QString());
                }
                action->setDisabled(true);
            } else {
                action->setEnabled(true);
            }
        }
    }
#endif
}

void MainWindow::on_actionUpgrade_triggered()
{
    if (Settings.askUpgradeAutomatic()) {
        QMessageBox dialog(QMessageBox::Question,
                           qApp->applicationName(),
                           tr("Do you want to automatically check for updates in the future?"),
                           QMessageBox::No | QMessageBox::Yes,
                           this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setCheckBox(
            new QCheckBox(tr("Do not show this anymore.", "Automatic upgrade check dialog")));
        Settings.setCheckUpgradeAutomatic(dialog.exec() == QMessageBox::Yes);
        if (dialog.checkBox()->isChecked())
            Settings.setAskUpgradeAutomatic(false);
    }
    showStatusMessage("Checking for upgrade...");
    m_network.get(QNetworkRequest(QUrl("https://check.shotcut.org/version.json")));
}

void MainWindow::on_actionOpenXML_triggered()
{
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          tr("Open File"),
                                                          path,
                                                          tr("MLT XML (*.mlt);;All Files (*)"),
                                                          nullptr,
                                                          Util::getFileDialogOptions());
    if (filenames.length() > 0) {
        QString url = filenames.first();
        MltXmlChecker checker;
        if (checker.check(url) == QXmlStreamReader::NoError) {
            if (!isCompatibleWithGpuMode(checker))
                return;
            isXmlRepaired(checker, url);
        } else {
            showStatusMessage(tr("Failed to open ").append(url));
            showIncompatibleProjectMessage(checker.shotcutVersion());
            return;
        }
        Settings.setOpenPath(QFileInfo(url).path());
        activateWindow();
        if (filenames.length() > 1)
            m_multipleFiles = filenames;
        if (!MLT.openXML(url)) {
            open(MLT.producer());
            m_recentDock->add(url);
            LOG_INFO() << url;
        } else {
            showStatusMessage(tr("Failed to open ") + url);
            emit openFailed(url);
        }
    }
}

void MainWindow::on_actionShowProjectFolder_triggered()
{
    Util::showInFolder(m_currentFile);
}

void MainWindow::onFocusChanged(QWidget *, QWidget *) const
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

#if !defined(Q_OS_MAC)
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

void MainWindow::on_actionResources_triggered()
{
    ResourceDialog dialog(this);
    dialog.search(multitrack());
    dialog.search(playlist());
    dialog.exec();
}

void MainWindow::on_actionApplicationLog_triggered()
{
    TextViewerDialog dialog(this);
    QDir dir = Settings.appDataLocation();
    QFile logFile(dir.filePath("shotcut-log.txt"));
    logFile.open(QIODevice::ReadOnly | QIODevice::Text);
    dialog.setText(logFile.readAll());
    logFile.close();
    dialog.setWindowTitle(tr("Application Log"));
    const auto previousLogName = dir.filePath("shotcut-log.bak");
    if (QFile::exists(previousLogName)) {
        auto button = dialog.buttonBox()->addButton(tr("Previous"), QDialogButtonBox::ActionRole);
        connect(button, &QAbstractButton::clicked, this, [&]() {
            QFile logFile(previousLogName);
            logFile.open(QIODevice::ReadOnly | QIODevice::Text);
            dialog.setText(logFile.readAll());
            logFile.close();
            button->setEnabled(false);
        });
    }
    dialog.exec();
}

void MainWindow::on_actionClose_triggered()
{
    m_timelineDock->stopRecording();
    if (continueModified()) {
        LOG_DEBUG() << "";
        QMutexLocker locker(&m_autosaveMutex);
        m_autosaveFile.reset();
        locker.unlock();
        setCurrentFile("");
        MLT.resetURL();
        MLT.setProjectFolder(QString());
        ui->actionSave->setEnabled(false);
        MLT.stop();
        if (MLT.consumer() && MLT.consumer()->is_valid())
            MLT.consumer()->purge();
        QCoreApplication::processEvents();
        if (multitrack())
            m_timelineDock->model()->close();
        if (playlist())
            m_playlistDock->model()->close();
        else
            onMultitrackClosed();
        closeProducer();
        m_notesDock->setText("");
        m_player->enableTab(Player::SourceTabIndex, false);
        MLT.purgeMemoryPool();
    }
}

void MainWindow::onPlayerTabIndexChanged(int index)
{
    if (Player::SourceTabIndex == index)
        m_timelineDock->saveAndClearSelection();
    else
        m_timelineDock->restoreSelection();
}

void MainWindow::onUpgradeCheckFinished(QNetworkReply *reply)
{
    if (!reply->error()) {
        QByteArray response = reply->readAll();
        LOG_DEBUG() << "response: " << response;
        QJsonDocument json = QJsonDocument::fromJson(response);
        QString current = qApp->applicationVersion();

        if (!json.isNull() && json.object().value("version_string").type() == QJsonValue::String) {
            QString latest = json.object().value("version_string").toString();
            if (current != "adhoc"
                && QVersionNumber::fromString(current) < QVersionNumber::fromString(latest)) {
                QAction *action = new QAction(
                    tr("Shotcut version %1 is available! Click here to get it.").arg(latest), 0);
                connect(action, SIGNAL(triggered(bool)), SLOT(onUpgradeTriggered()));
                if (!json.object().value("url").isUndefined())
                    m_upgradeUrl = json.object().value("url").toString();
                showStatusMessage(action, 15 /* seconds */);
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
        if (reply->error() == QNetworkReply::UnknownNetworkError) {
            m_network.get(QNetworkRequest(QUrl("http://check.shotcut.org/version.json")));
        }
    }
    QAction *action = new QAction(
        tr("Failed to read version.json when checking. Click here to go to the Web site."), 0);
    connect(action, SIGNAL(triggered(bool)), SLOT(onUpgradeTriggered()));
    showStatusMessage(action);
    reply->deleteLater();
}

void MainWindow::onUpgradeTriggered()
{
    QDesktopServices::openUrl(QUrl(m_upgradeUrl));
}

void MainWindow::onClipCopied()
{
    m_player->enableTab(Player::SourceTabIndex);
}

void MainWindow::on_actionExportEDL_triggered()
{
    // Dialog to get export file name.
    QString path = Settings.savePath();
    QString caption = tr("Export EDL");
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                                        caption,
                                                        path,
                                                        tr("EDL (*.edl);;All Files (*)"),
                                                        nullptr,
                                                        Util::getFileDialogOptions());
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
            stream.setEncoding(QStringConverter::Utf8);
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
                    f.write(result.toString().toUtf8());
                    f.close();
                }
            }
            if (result.isError()) {
                LOG_ERROR() << "Uncaught exception at line" << result.property("lineNumber").toInt()
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
    if (!MLT.producer() || !MLT.producer()->is_valid())
        return;
    filterController()->setCurrentFilter(QmlFilter::DeselectCurrentFilter);
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    connect(videoWidget, &Mlt::VideoWidget::imageReady, this, &MainWindow::onVideoWidgetImageReady);
    MLT.setPreviewScale(0);
    videoWidget->requestImage();
    MLT.refreshConsumer();
}

void MainWindow::onVideoWidgetImageReady()
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    QImage image = videoWidget->image();
    disconnect(videoWidget, SIGNAL(imageReady()), this, nullptr);
    if (Settings.playerGPU() || Settings.playerPreviewScale()) {
        MLT.setPreviewScale(Settings.playerPreviewScale());
    }
    if (!image.isNull()
        && (videoWidget->imageIsProxy() || (MLT.isMultitrack() && Settings.proxyEnabled()))

    ) {
        QMessageBox dialog(
            QMessageBox::Question,
            tr("Export frame from proxy?"),
            tr("This frame may be from a lower resolution proxy instead of the original source.\n\n"
               "Do you still want to continue?"),
            QMessageBox::No | QMessageBox::Yes,
            this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() != QMessageBox::Yes) {
            return;
        }
    }
    if (!image.isNull()) {
        SaveImageDialog dialog(this, tr("Export Frame"), image);
        dialog.exec();
        if (!dialog.saveFile().isEmpty()) {
            m_recentDock->add(dialog.saveFile());
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
    if (dialog.exec() != QMessageBox::Yes)
        return;

    QString dirName = QFileDialog::getExistingDirectory(this,
                                                        tr("Data Directory"),
                                                        Settings.appDataLocation(),
                                                        Util::getFileDialogOptions());
    if (!dirName.isEmpty()) {
        // Move the data files.
        QDirIterator it(Settings.appDataLocation());
        while (it.hasNext()) {
            if (!it.filePath().isEmpty() && it.fileName() != "." && it.fileName() != "..") {
                if (!QFile::exists(dirName + "/" + it.fileName())) {
                    if (it.fileInfo().isDir()) {
                        if (!QFile::rename(it.filePath(), dirName + "/" + it.fileName()))
                            LOG_WARNING() << "Failed to move" << it.filePath() << "to"
                                          << dirName + "/" + it.fileName();
                    } else {
                        if (!QFile::copy(it.filePath(), dirName + "/" + it.fileName()))
                            LOG_WARNING() << "Failed to copy" << it.filePath() << "to"
                                          << dirName + "/" + it.fileName();
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
    // Make the transitions sub-folder if it does not exist
    QmlApplication::wipes();
    Util::showInFolder(Settings.appDataLocation());
}

void MainWindow::on_actionNew_triggered()
{
    on_actionClose_triggered();
}

void MainWindow::on_actionScreenSnapshot_triggered()
{
    auto fileName = QmlApplication::getNextProjectFile("screenshot-.png");
    if (fileName.isEmpty()) {
        fileName = Settings.savePath() + "/%1.png";
        fileName = QFileDialog::getSaveFileName(this,
                                                tr("Screen Snapshot"),
                                                fileName.arg(tr("screenshot")),
                                                tr("PNG Files (*.png)"));
        if (fileName.isEmpty())
            return;

        // Ensure the filename ends with .png
        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
            fileName += ".png";
        }
    }
    bool maximized = windowState() & Qt::WindowMaximized;
#ifdef Q_OS_MAC
    // Run screencapture command directly
    QStringList args;
    args << "-i"
         << "-t"
         << "png" << fileName;

    QProcess *process = new QProcess(this);
    connect(process, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0 && QFileInfo::exists(fileName)) {
            // Automatically open the captured file
            QTimer::singleShot(500, this, [this, fileName]() { open(fileName); });
        }
        process->deleteLater();
        if (maximized)
            showMaximized();
        else
            showNormal();
    });

    showMinimized();
    process->start("screencapture", args);
#else
    const auto mode = ScreenCapture::isWayland() ? ScreenCapture::Fullscreen
                                                 : ScreenCapture::Interactive;
    m_screenCapture = new ScreenCapture(fileName, mode, this);
    connect(m_screenCapture, &ScreenCapture::minimizeShotcut, this, [this]() { showMinimized(); });
    connect(m_screenCapture, &ScreenCapture::finished, this, [=](bool success) {
        if (success)
            // Automatically open the captured file
            QTimer::singleShot(500, this, [this, fileName]() { open(fileName); });
        if (maximized)
            showMaximized();
        else
            showNormal();
    });
    m_screenCapture->startSnapshot();
#endif
}

void MainWindow::on_actionScreenRecording_triggered()
{
    QString filenameExtension;
    auto mode = ScreenCapture::Interactive;
#ifdef Q_OS_WIN
    QDesktopServices::openUrl({"ms-screenclip:?type=recording", QUrl::TolerantMode});
    return;
#elif defined(Q_OS_MAC)
    filenameExtension = ".mov";
#else
    bool isGNOMEorKDEonWayland = false;
    // GNOME and KDE have built-in screen recording compatible with Wayland
    if (ScreenCapture::isWayland()) {
        const auto desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
        isGNOMEorKDEonWayland = desktop.contains("gnome") || desktop.contains("kde")
                                || desktop.contains("plasma");
        if (isGNOMEorKDEonWayland) {
            filenameExtension = ".webm";
            mode = ScreenCapture::Fullscreen;
        }
    } else {
        filenameExtension = ".mp4";
    }
#endif
    QString fileName;
    if (!filenameExtension.isEmpty()) {
        fileName = QmlApplication::getNextProjectFile("screen-" + filenameExtension);
        if (fileName.isEmpty()) {
            fileName = Settings.savePath() + "/%1" + filenameExtension;
            fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Screen Recording"),
                                                    fileName.arg(tr("screen")),
                                                    QString("%1 %2 (*%3)")
                                                        .arg(filenameExtension.toUpper(),
                                                             tr("Files"),
                                                             filenameExtension));
            if (fileName.isEmpty())
                return;

            // Ensure the filename ends with extension
            if (!fileName.endsWith(filenameExtension, Qt::CaseInsensitive)) {
                fileName += filenameExtension;
            }
        }
    }
#ifdef Q_OS_MAC
    ScreenCaptureJob *job = new ScreenCaptureJob(tr("Screen Recording"), fileName, QRect(), true);
    JOBS.add(job);
    return;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // On Linux with Wayland and not GNOME or KDE
    if (ScreenCapture::isWayland() && !isGNOMEorKDEonWayland) {
        // Let user pick a program to use on Wayland and not GNOME or KDE.
        // OBS Studio (default) supports Wayland.
        QString exePath = Settings.screenRecorderPath();

        // Check if saved path is still valid
        if (!exePath.isEmpty() && !QFile::exists(exePath)) {
            exePath.clear();
        }

        // If not found, prompt user
        if (exePath.isEmpty()) {
            exePath = Util::getExecutable(this);
            if (exePath.isEmpty()) {
                return; // User cancelled
            }
            Settings.setScreenRecorderPath(exePath);
        }

        // Launch the screen recorder
        if (Util::startDetached(exePath, QStringList())) {
            showStatusMessage(tr("Screen recorder launched"));
        } else {
            showStatusMessage(tr("Failed to launch screen recorder"));
        }
        return;
    }
#endif
    m_screenCapture = new ScreenCapture(fileName, mode, this);
    connect(m_screenCapture, &ScreenCapture::minimizeShotcut, this, [this]() { showMinimized(); });
    connect(m_screenCapture,
            &ScreenCapture::beginRecording,
            this,
            [=](const QRect &rect, bool recordAudio) {
                ScreenCaptureJob *job
                    = new ScreenCaptureJob(tr("Screen Recording"), fileName, rect, recordAudio);
                JOBS.add(job);
            });
    m_screenCapture->startRecording();
}

void MainWindow::on_actionKeyboardShortcuts_triggered()
{
    auto name = QString::fromLatin1("actionsDialog");
    auto dialog = QObject::findChild<ActionsDialog *>(name, Qt::FindDirectChildrenOnly);
    if (!dialog) {
        dialog = new ActionsDialog(this);
        dialog->setObjectName(name);
    }
    dialog->show();
    dialog->activateWindow();
    dialog->raise();
}

void MainWindow::on_actionLayoutLogging_triggered()
{
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::Logging);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::Logging));
    if (state.isEmpty()) {
        restoreState(kLayoutLoggingDefault);
        //        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        //        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        //        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        //        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
        //        resizeDocks({m_playlistDock, m_propertiesDock},
        //            {qFloor(width() * 0.25), qFloor(width() * 0.25)}, Qt::Horizontal);
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
}

void MainWindow::on_actionLayoutEditing_triggered()
{
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::Editing);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::Editing));
    if (state.isEmpty()) {
        restoreState(kLayoutEditingDefault);
        //        resetDockCorners();
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
}

void MainWindow::on_actionLayoutEffects_triggered()
{
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::Effects);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::Effects));
    if (state.isEmpty()) {
        restoreState(kLayoutEffectsDefault);
        //        resetDockCorners();
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
}

void MainWindow::on_actionLayoutColor_triggered()
{
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::Color);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::Color));
    if (state.isEmpty()) {
        restoreState(kLayoutColorDefault);
        //        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        //        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        //        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        //        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    // Set filter menu to Video filters for Color layout
    filterController()->metadataModel()->setFilter(MetadataModel::VideoFilter);
    filterController()->metadataModel()->setSearch("#color");
}

void MainWindow::on_actionLayoutAudio_triggered()
{
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::Audio);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::Audio));
    if (state.isEmpty()) {
        restoreState(kLayoutAudioDefault);
        //        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        //        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        //        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        //        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    // Set filter menu to Audio filters for Audio layout
    filterController()->metadataModel()->setFilter(MetadataModel::AudioFilter);
    filterController()->metadataModel()->setSearch("");
}

void MainWindow::on_actionLayoutPlayer_triggered()
{
    if (isMultitrackValid()) {
        // Clearing selection causes Filters to clear, which prevents showing a filter's VUI
        m_timelineDock->setSelection();
    }
    Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                       QByteArray(),
                       saveState());
    Settings.setLayoutMode(LayoutMode::PlayerOnly);
    auto state = Settings.layoutState(QString(kReservedLayoutPrefix).arg(LayoutMode::PlayerOnly));
    if (state.isEmpty()) {
        restoreState(kLayoutPlayerDefault);
        //        resetDockCorners();
    } else {
        //        LOG_DEBUG() << state.toBase64();
        restoreState(state);
    }
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
}

void MainWindow::on_actionLayoutPlaylist_triggered()
{
    if (Settings.layoutMode() != LayoutMode::Custom) {
        Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                           QByteArray(),
                           saveState());
        Settings.setLayoutMode(LayoutMode::Custom);
    }
    clearCurrentLayout();
    restoreState(Settings.windowStateDefault());
    m_recentDock->show();
    m_recentDock->raise();
    m_playlistDock->show();
    m_playlistDock->raise();
    Settings.setWindowState(saveState());
}

void MainWindow::on_actionLayoutClip_triggered()
{
    if (Settings.layoutMode() != LayoutMode::Custom) {
        Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                           QByteArray(),
                           saveState());
        Settings.setLayoutMode(LayoutMode::Custom);
    }
    clearCurrentLayout();
    restoreState(Settings.windowStateDefault());
    m_recentDock->show();
    m_recentDock->raise();
    m_filtersDock->show();
    m_filtersDock->raise();
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
}

void MainWindow::on_actionLayoutAdd_triggered()
{
    QInputDialog dialog(this);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Add Custom Layout"));
    dialog.setLabelText(tr("Name"));
    dialog.setWindowModality(QmlApplication::dialogModality());
    auto result = dialog.exec();
    auto name = dialog.textValue();
    if (result == QDialog::Accepted && !name.isEmpty()) {
        if (Settings.setLayout(name, saveGeometry(), saveState())) {
            Settings.setLayoutMode();
            clearCurrentLayout();
            Settings.sync();
            if (Settings.layouts().size() == 1) {
                ui->menuLayout->addAction(ui->actionLayoutRemove);
                ui->menuLayout->addSeparator();
            }
            ui->menuLayout->addAction(addLayout(m_layoutGroup, name));
        }
    }
}

void MainWindow::onLayoutTriggered(QAction *action)
{
    if (Settings.layoutMode() != LayoutMode::Custom) {
        Settings.setLayout(QString(kReservedLayoutPrefix).arg(Settings.layoutMode()),
                           QByteArray(),
                           saveState());
        Settings.setLayoutMode(LayoutMode::Custom);
    }
    clearCurrentLayout();
    restoreState(Settings.layoutState(action->text()));
    Settings.setWindowState(saveState());
    resetFilterMenuIfNeeded();
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
            removeCustomProfiles(dialog.selection(),
                                 dir,
                                 customProfileMenu(),
                                 actionProfileRemove());
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
        foreach (const QString &layout, dialog.selection()) {
            // Update the configuration.
            if (Settings.removeLayout(layout))
                Settings.sync();
            // Locate the menu item.
            foreach (QAction *action, ui->menuLayout->actions()) {
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
            foreach (QAction *action, ui->menuLayout->actions()) {
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

void MainWindow::on_actionOpenOther2_triggered()
{
    const auto widget = ui->mainToolBar->widgetForAction(ui->actionOpenOther2);
    ui->actionOpenOther2->menu()->popup(widget->mapToGlobal(QPoint(0, widget->height())));
}

void MainWindow::onOpenOtherTriggered(QWidget *widget)
{
    m_producerWidget.reset(widget);
    auto dialog = new QDialog(this);
    dialog->resize(426, 288);
    dialog->setWindowModality(QmlApplication::dialogModality());
    auto vlayout = new QVBoxLayout(dialog);
    vlayout->addWidget(widget);
    auto buttonBox = new QDialogButtonBox(dialog);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
    if (!AbstractProducerWidget::isDevice(widget)) {
        auto button = buttonBox->addButton(tr("Add To Timeline"), QDialogButtonBox::ApplyRole);
        connect(button, &QPushButton::clicked, this, [=]() { dialog->done(-1); });
    }
    vlayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::finished, this, &MainWindow::onOpenOtherFinished);
    dialog->show();
}

void MainWindow::onOpenOtherFinished(int result)
{
    // Handle screen capture custom result codes
    if (result == 42) { // Screen Snapshot
        on_actionScreenSnapshot_triggered();
        return;
    } else if (result == 43) { // Screen Recording
        on_actionScreenRecording_triggered();
        return;
    }

    if (QDialog::Rejected == result || !m_producerWidget)
        return;
    if (AbstractProducerWidget::isDevice(m_producerWidget.get()) && !Settings.playerGPU())
        closeProducer();

    const auto name = m_producerWidget->objectName();
    auto dialog = dynamic_cast<AbstractProducerWidget *>(m_producerWidget.get());
    if (QLatin1String("GlaxnimateProducerWidget") == name) {
        auto glax = qobject_cast<GlaxnimateProducerWidget *>(m_producerWidget.get());
        glax->setLaunchOnNew(false);
    }

    auto &profile = MLT.profile();
    auto producer = dialog->newProducer(profile);
    if (!(producer && producer->is_valid())) {
        delete producer;
        m_producerWidget.reset();
        return;
    }
    if (!profile.is_explicit()) {
        profile.from_producer(*producer);
        profile.set_width(Util::coerceMultiple(profile.width()));
        profile.set_height(Util::coerceMultiple(profile.height()));
    }
    MLT.updatePreviewProfile();
    setPreviewScale(Settings.playerPreviewScale());
    if (QDialog::Accepted == result) {
        // open in the source player
        open(producer);
        // Mlt::Controller owns the producer now
    } else {
        m_player->switchToTab(Player::ProjectTabIndex);
        auto trackType = (QLatin1String("ToneProducerWidget") == name) ? AudioTrackType
                                                                       : VideoTrackType;
        auto trackIndex = m_timelineDock->addTrackIfNeeded(trackType);
        m_timelineDock->overwrite(trackIndex, -1, MLT.XML(producer), true);
        delete producer;
    }
    if (QLatin1String("TextProducerWidget") == name) {
        m_filtersDock->show();
        m_filtersDock->raise();
    } else {
        m_propertiesDock->show();
        m_propertiesDock->raise();
    }
    m_producerWidget.reset();
}

void MainWindow::onOpenOtherTriggered()
{
    if (sender()->objectName() == "color")
        onOpenOtherTriggered(new ColorProducerWidget(this));
    else if (sender()->objectName() == "text")
        onOpenOtherTriggered(new TextProducerWidget(this));
    else if (sender()->objectName() == "glaxnimate")
        onOpenOtherTriggered(new GlaxnimateProducerWidget(this));
    else if (sender()->objectName() == "noise")
        onOpenOtherTriggered(new NoiseWidget(this));
    else if (sender()->objectName() == "ising0r")
        onOpenOtherTriggered(new IsingWidget(this));
    else if (sender()->objectName() == "lissajous0r")
        onOpenOtherTriggered(new LissajousWidget(this));
    else if (sender()->objectName() == "plasma")
        onOpenOtherTriggered(new PlasmaWidget(this));
    else if (sender()->objectName() == "test_pat_B")
        onOpenOtherTriggered(new ColorBarsWidget(this));
    else if (sender()->objectName() == "tone")
        onOpenOtherTriggered(new ToneProducerWidget(this));
    else if (sender()->objectName() == "count")
        onOpenOtherTriggered(new CountProducerWidget(this));
    else if (sender()->objectName() == "blipflash")
        onOpenOtherTriggered(new BlipProducerWidget(this));
}

void MainWindow::onHtmlGeneratorTriggered()
{
    if (!Util::isChromiumAvailable()) {
        QMessageBox
            dialog(QMessageBox::Warning,
                   QApplication::applicationName(),
                   this->tr(
                           "<p>This feature requires Google Chrome or a Chromium-based browser.</p>"
                           "<p>If you already installed one it could not be "
                           "found at the expected location: <tt>%1</tt></p><p>Click <b>OK</b> to "
                           "continue and locate the program on your system.</p>")
                       .arg(Settings.chromiumPath()),
                   QMessageBox::Cancel | QMessageBox::Ok,
                   this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Cancel);
        if (QMessageBox::Cancel == dialog.exec())
            return;

        const auto executable = Util::getExecutable(this);
        if (executable.isEmpty())
            return;
        Settings.setChromiumPath(executable);
    }
    // Create a color producer with special property and open it
    auto producer = new Mlt::Producer(MLT.profile(), "color", "#00000000");
    producer->set(kPrivateProducerProperty, "htmlGenerator");
    open(producer, false);
    m_propertiesDock->show();
    m_propertiesDock->raise();
    m_producerWidget.reset();
}

void MainWindow::on_actionClearRecentOnExit_toggled(bool arg1)
{
    Settings.setClearRecent(arg1);
    if (arg1) {
        Settings.setRecent(QStringList());
        Settings.setProjects(QStringList());
    }
}

void MainWindow::onSceneGraphInitialized()
{
    if (Settings.playerGPU() && Settings.playerWarnGPU()) {
        QMessageBox
            dialog(QMessageBox::Warning,
                   qApp->applicationName(),
                   tr("GPU processing is EXPERIMENTAL, UNSTABLE and UNSUPPORTED! Unsupported "
                      "means do not report bugs about it.\n\n"
                      "Do you want to disable GPU processing and restart Shotcut?"),
                   QMessageBox::No | QMessageBox::Yes,
                   this);
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Yes) {
            Settings.setProcessingMode(ShotcutSettings::Native8Cpu);
            m_exitCode = EXIT_RESTART;
            QApplication::closeAllWindows();
        }
    }
    auto videoWidget = (Mlt::VideoWidget *) &(MLT);
    videoWidget->setBlankScene();
}

void MainWindow::on_actionShowTextUnderIcons_toggled(bool b)
{
    ui->mainToolBar->setToolButtonStyle(b ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
    Settings.setTextUnderIcons(b);
    updateLayoutSwitcher();
    if (b && this->width() < 1900) {
        ui->mainToolBar->removeAction(ui->actionFiles);
        ui->mainToolBar->removeAction(ui->actionMarkers);
        ui->mainToolBar->removeAction(ui->actionNotes);
    }
}

void MainWindow::on_actionShowSmallIcons_toggled(bool b)
{
    ui->mainToolBar->setIconSize(b ? QSize(16, 16) : QSize());
    Settings.setSmallIcons(b);
    updateLayoutSwitcher();
}

void MainWindow::onPlaylistInChanged(int in)
{
    m_player->blockSignals(true);
    m_player->setIn(in);
    m_player->blockSignals(false);
}

void MainWindow::onPlaylistOutChanged(int out)
{
    m_player->blockSignals(true);
    m_player->setOut(out);
    m_player->blockSignals(false);
}

void MainWindow::on_actionPreviewNone_triggered(bool checked)
{
    if (checked) {
        Settings.setPlayerPreviewScale(0);
        setPreviewScale(0);
        m_player->showIdleStatus();
    }
}

void MainWindow::on_actionPreview360_triggered(bool checked)
{
    if (checked) {
        Settings.setPlayerPreviewScale(360);
        setPreviewScale(360);
        m_player->showIdleStatus();
    }
}

void MainWindow::on_actionPreview540_triggered(bool checked)
{
    if (checked) {
        Settings.setPlayerPreviewScale(540);
        setPreviewScale(540);
        m_player->showIdleStatus();
    }
}

void MainWindow::on_actionPreview720_triggered(bool checked)
{
    if (checked) {
        Settings.setPlayerPreviewScale(720);
        setPreviewScale(720);
        m_player->showIdleStatus();
    }
}

void MainWindow::on_actionPreview1080_triggered(bool checked)
{
    if (checked) {
        Settings.setPlayerPreviewScale(1080);
        setPreviewScale(1080);
        m_player->showIdleStatus();
    }
}

QUuid MainWindow::timelineClipUuid(int trackIndex, int clipIndex)
{
    auto info = m_timelineDock->model()->getClipInfo(trackIndex, clipIndex);
    if (info && info->producer && info->producer->is_valid())
        return MLT.ensureHasUuid(*info->producer);
    return QUuid();
}

void MainWindow::replaceInTimeline(const QUuid &uuid, Mlt::Producer &producer)
{
    int trackIndex = -1;
    int clipIndex = -1;
    // lookup the current track and clip index by UUID
    auto info = m_timelineDock->model()->findClipByUuid(uuid, trackIndex, clipIndex);

    if (info && trackIndex >= 0 && clipIndex >= 0) {
        Util::getHash(producer);
        Util::applyCustomProperties(producer,
                                    *info->producer,
                                    producer.get_in(),
                                    producer.get_out());
        m_timelineDock->replace(trackIndex, clipIndex, MLT.XML(&producer));
    }
}

void MainWindow::replaceAllByHash(const QString &hash, Mlt::Producer &producer, bool isProxy)
{
    Util::getHash(producer);
    if (!isProxy)
        m_recentDock->add(producer.get("resource"));
    if (MLT.isClip() && Util::getHash(*MLT.producer()) == hash) {
        Util::applyCustomProperties(producer,
                                    *MLT.producer(),
                                    MLT.producer()->get_in(),
                                    MLT.producer()->get_out());
        MLT.copyFilters(*MLT.producer(), producer);
        MLT.close();
        m_player->setPauseAfterOpen(true);
        open(
            new Mlt::Producer(MLT.profile(), "xml-string", MLT.XML(&producer).toUtf8().constData()));
    } else if (MLT.savedProducer() && Util::getHash(*MLT.savedProducer()) == hash) {
        Util::applyCustomProperties(producer,
                                    *MLT.savedProducer(),
                                    MLT.savedProducer()->get_in(),
                                    MLT.savedProducer()->get_out());
        MLT.copyFilters(*MLT.savedProducer(), producer);
        MLT.setSavedProducer(&producer);
    }
    if (playlist()) {
        if (isProxy) {
            m_playlistDock->replaceClipsWithHash(hash, producer);
        } else {
            // Append to playlist
            producer.set(kPlaylistIndexProperty, playlist()->count());
            MAIN.undoStack()->push(
                new Playlist::AppendCommand(*m_playlistDock->model(), MLT.XML(&producer)));
        }
    }
    if (isMultitrackValid()) {
        m_timelineDock->replaceClipsWithHash(hash, producer);
    }
}

int MainWindow::mltIndexForTrack(int trackIndex) const
{
    return m_timelineDock->model()->mltIndexForTrack(trackIndex);
}

int MainWindow::bottomVideoTrackIndex() const
{
    return m_timelineDock->model()->bottomVideoTrackIndex();
}

void MainWindow::on_actionTopics_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.shotcut.org/howtos/"));
}

void MainWindow::on_actionSync_triggered()
{
    auto dialog = new SystemSyncDialog(this);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::on_actionUseProxy_triggered(bool checked)
{
    if (MLT.producer()) {
        QDir dir(m_currentFile.isEmpty() ? QDir::tempPath() : QFileInfo(m_currentFile).dir());
        QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile(dir.filePath("shotcut-XXXXXX.mlt")));
        tmp->open();
        tmp->close();
        QString fileName = tmp->fileName();
        tmp->remove();
        tmp.reset();
        LOG_DEBUG() << fileName;

        if (saveXML(fileName)) {
            MltXmlChecker checker;

            Settings.setProxyEnabled(checked);
            checker.check(fileName);
            if (!isXmlRepaired(checker, fileName)) {
                QFile::remove(fileName);
                return;
            }
            if (checker.isUpdated()) {
                QFile::remove(fileName);
                fileName = checker.tempFile().fileName();
            }

            // Open the temporary file
            int result = 0;
            {
                LongUiTask longTask(checked ? tr("Turn Proxy On") : tr("Turn Proxy Off"));
                QFuture<int> future = QtConcurrent::run([=]() {
                    return MLT.open(QDir::fromNativeSeparators(fileName),
                                    QDir::fromNativeSeparators(m_currentFile));
                });
                result = longTask.wait<int>(tr("Converting"), future);
            }
            if (!result) {
                auto position = m_player->position();
                m_undoStack->clear();
                m_player->stop();
                m_player->setPauseAfterOpen(true);
                open(MLT.producer());
                MLT.seek(m_player->position());
                m_player->seek(position);

                if (checked && (isPlaylistValid() || isMultitrackValid())) {
                    // Prompt user if they want to create missing proxies
                    QMessageBox dialog(
                        QMessageBox::Question,
                        qApp->applicationName(),
                        tr("Do you want to create missing proxies for every file in this "
                           "project?\n\n"
                           "You must reopen your project after all proxy jobs are finished."),
                        QMessageBox::No | QMessageBox::Yes,
                        this);
                    dialog.setWindowModality(QmlApplication::dialogModality());
                    dialog.setDefaultButton(QMessageBox::Yes);
                    dialog.setEscapeButton(QMessageBox::No);
                    if (dialog.exec() == QMessageBox::Yes) {
                        Mlt::Producer producer(playlist());
                        if (producer.is_valid()) {
                            ProxyManager::generateIfNotExistsAll(producer);
                        }
                        producer = multitrack();
                        if (producer.is_valid()) {
                            ProxyManager::generateIfNotExistsAll(producer);
                        }
                    }
                }
            } else if (fileName != untitledFileName()) {
                showStatusMessage(tr("Failed to open ") + fileName);
                emit openFailed(fileName);
            }
        } else {
            ui->actionUseProxy->setChecked(!checked);
            showSaveError();
        }
        QFile::remove(fileName);
    } else {
        Settings.setProxyEnabled(checked);
    }
    m_player->showIdleStatus();
}

void MainWindow::on_actionProxyStorageSet_triggered()
{
    // Present folder dialog just like App Data Directory
    QString dirName = QFileDialog::getExistingDirectory(this,
                                                        tr("Proxy Folder"),
                                                        Settings.proxyFolder(),
                                                        Util::getFileDialogOptions());
    if (!dirName.isEmpty() && dirName != Settings.proxyFolder()) {
        auto oldFolder = Settings.proxyFolder();
        Settings.setProxyFolder(dirName);
        Settings.sync();

        // Get a count for the progress dialog
        auto oldDir = QDir(oldFolder);
        auto dirList = oldDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        auto count = dirList.size();

        if (count > 0) {
            // Prompt user if they want to create missing proxies
            QMessageBox
                dialog(QMessageBox::Question,
                       qApp->applicationName(),
                       tr("Do you want to move all files from the old folder to the new folder?"),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
            dialog.setWindowModality(QmlApplication::dialogModality());
            dialog.setDefaultButton(QMessageBox::Yes);
            dialog.setEscapeButton(QMessageBox::No);
            if (dialog.exec() == QMessageBox::Yes) {
                // Move the existing files
                LongUiTask longTask(tr("Moving Files"));
                int i = 0;
                for (const auto &fileName : dirList) {
                    if (!fileName.isEmpty() && !QFile::exists(dirName + "/" + fileName)) {
                        LOG_DEBUG() << "moving" << oldDir.filePath(fileName) << "to"
                                    << dirName + "/" + fileName;
                        longTask.reportProgress(fileName, i++, count);
                        if (!QFile::rename(oldDir.filePath(fileName), dirName + "/" + fileName))
                            LOG_WARNING() << "Failed to move" << oldDir.filePath(fileName);
                    }
                }
            }
        }
    }
}

void MainWindow::on_actionProxyStorageShow_triggered()
{
    Util::showInFolder(ProxyManager::dir().path());
}

void MainWindow::on_actionProxyUseProjectFolder_triggered(bool checked)
{
    Settings.setProxyUseProjectFolder(checked);
}

void MainWindow::on_actionProxyUseHardware_triggered(bool checked)
{
    if (checked && Settings.encodeHardware().isEmpty()) {
        if (!m_encodeDock->detectHardwareEncoders())
            ui->actionProxyUseHardware->setChecked(false);
    }
    Settings.setProxyUseHardware(ui->actionProxyUseHardware->isChecked());
}

void MainWindow::on_actionProxyConfigureHardware_triggered()
{
    m_encodeDock->on_hwencodeButton_clicked();
    if (Settings.encodeHardware().isEmpty()) {
        ui->actionProxyUseHardware->setChecked(false);
        Settings.setProxyUseHardware(false);
    }
}

void MainWindow::updateLayoutSwitcher()
{
    if (Settings.textUnderIcons() && !Settings.smallIcons()) {
        auto layoutSwitcher = findChild<QWidget *>(kLayoutSwitcherName);
        if (layoutSwitcher) {
            layoutSwitcher->show();
            for (const auto &child : layoutSwitcher->findChildren<QToolButton *>()) {
                child->show();
            }
        } else {
            layoutSwitcher = new QWidget;
            layoutSwitcher->setObjectName(kLayoutSwitcherName);
            auto layoutGrid = new QGridLayout(layoutSwitcher);
            layoutGrid->setContentsMargins(0, 0, 0, 0);
            ui->mainToolBar->insertWidget(ui->dummyAction, layoutSwitcher);
            auto button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutLogging);
            layoutGrid->addWidget(button, 0, 0, Qt::AlignCenter);
            button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutEditing);
            layoutGrid->addWidget(button, 0, 1, Qt::AlignCenter);
            button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutEffects);
            layoutGrid->addWidget(button, 0, 2, Qt::AlignCenter);
            button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutColor);
            layoutGrid->addWidget(button, 1, 0, Qt::AlignCenter);
            button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutAudio);
            layoutGrid->addWidget(button, 1, 1, Qt::AlignCenter);
            button = new QToolButton;
            button->setAutoRaise(true);
            button->setDefaultAction(ui->actionLayoutPlayer);
            layoutGrid->addWidget(button, 1, 2, Qt::AlignCenter);
        }
        ui->mainToolBar->removeAction(ui->actionLayoutLogging);
        ui->mainToolBar->removeAction(ui->actionLayoutEditing);
        ui->mainToolBar->removeAction(ui->actionLayoutEffects);
        ui->mainToolBar->removeAction(ui->actionLayoutColor);
        ui->mainToolBar->removeAction(ui->actionLayoutAudio);
        ui->mainToolBar->removeAction(ui->actionLayoutPlayer);
    } else {
        auto layoutSwitcher = findChild<QWidget *>(kLayoutSwitcherName);
        if (layoutSwitcher) {
            layoutSwitcher->hide();
            for (const auto &child : layoutSwitcher->findChildren<QToolButton *>()) {
                child->hide();
            }
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutLogging);
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutEditing);
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutEffects);
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutColor);
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutAudio);
            ui->mainToolBar->insertAction(ui->dummyAction, ui->actionLayoutPlayer);
        }
    }
}

void MainWindow::clearCurrentLayout()
{
    auto currentLayout = ui->actionLayoutLogging->actionGroup()->checkedAction();
    if (currentLayout) {
        currentLayout->setChecked(false);
    }
}

void MainWindow::onClipboardChanged()
{
    auto s = QGuiApplication::clipboard()->text();
    if (MLT.isMltXml(s) && !s.contains(kShotcutFiltersClipboard)) {
        m_clipboardUpdatedAt = QDateTime::currentDateTime();
        LOG_DEBUG() << m_clipboardUpdatedAt;
    }
}

void MainWindow::sourceUpdated()
{
    if (MLT.isClip()) {
        m_sourceUpdatedAt = QDateTime::currentDateTime();
    }
}

void MainWindow::resetSourceUpdated()
{
    m_sourceUpdatedAt.setSecsSinceEpoch(0);
}

void MainWindow::on_actionExportChapters_triggered()
{
    // Options dialog
    auto uniqueColors = m_timelineDock->markersModel()->allColors();
    if (uniqueColors.isEmpty()) {
        return;
    }
    std::sort(uniqueColors.begin(), uniqueColors.end(), [=](const QColor &a, const QColor &b) {
        if (a.hue() == b.hue()) {
            if (a.saturation() == b.saturation()) {
                return a.value() <= b.value();
            }
            return a.saturation() <= b.saturation();
        }
        return a.hue() <= b.hue();
    });
    QStringList colors;
    for (auto &color : uniqueColors) {
        colors << color.name();
    }
    const auto rangesOption = tr("Include ranges (Duration > 1 frame)?");
    QStringList initialOptions;
    for (auto &m : m_timelineDock->markersModel()->getMarkers()) {
        if (m.end != m.start) {
            initialOptions << rangesOption;
            break;
        }
    }

    ListSelectionDialog dialog(initialOptions, this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setWindowTitle(tr("Choose Markers"));
    if (Settings.exportRangeMarkers()) {
        dialog.setSelection({rangesOption});
    }
    dialog.setColors(colors);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto selection = dialog.selection();
    Settings.setExportRangeMarkers(selection.contains(rangesOption));

    // Dialog to get export file name
    QString path = Settings.savePath();
    QString caption = tr("Export Chapters");
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                                        caption,
                                                        path,
                                                        tr("Text (*.txt);;All Files (*)"),
                                                        nullptr,
                                                        Util::getFileDialogOptions());
    if (!saveFileName.isEmpty()) {
        QFileInfo fi(saveFileName);
        if (fi.suffix() != "txt")
            saveFileName += ".txt";

        if (Util::warnIfNotWritable(saveFileName, this, caption))
            return;

        // Locate the JavaScript file in the filesystem.
        QDir qmlDir = QmlUtilities::qmlDir();
        qmlDir.cd("export-chapters");
        QString jsFileName = qmlDir.absoluteFilePath("export-chapters.js");
        QFile scriptFile(jsFileName);
        if (scriptFile.open(QIODevice::ReadOnly)) {
            // Read JavaScript into a string.
            QTextStream stream(&scriptFile);
            stream.setEncoding(QStringConverter::Utf8);
            stream.setAutoDetectUnicode(true);
            QString contents = stream.readAll();
            scriptFile.close();

            // Evaluate JavaScript.
            QJSEngine jsEngine;
            QJSValue result = jsEngine.evaluate(contents, jsFileName);
            if (!result.isError()) {
                // Call the JavaScript main function.
                QJSValue options = jsEngine.newObject();
                if (selection.contains(rangesOption)) {
                    options.setProperty("includeRanges", true);
                    selection.removeOne(rangesOption);
                }
                QJSValue array = jsEngine.newArray(selection.size());
                for (int i = 0; i < selection.size(); ++i)
                    array.setProperty(i, selection[i].toUpper());
                options.setProperty("colors", array);
                QJSValueList args;
                args << MLT.XML(0, true, true) << options;
                result = result.call(args);
                if (!result.isError()) {
                    // Save the result with the export file name.
                    QFile f(saveFileName);
                    f.open(QIODevice::WriteOnly | QIODevice::Text);
                    f.write(result.toString().toUtf8());
                    f.close();
                }
            }
            if (result.isError()) {
                LOG_ERROR() << "Uncaught exception at line" << result.property("lineNumber").toInt()
                            << ":" << result.toString();
                showStatusMessage(tr("A JavaScript error occurred during export."));
            }
        } else {
            showStatusMessage(tr("Failed to open export-chapters.js"));
        }
    }
}

void MainWindow::on_actionAudioVideoDevice_triggered()
{
    QDialog dialog(this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    auto layout = new QVBoxLayout(&dialog);
    QWidget *widget = nullptr;

#if defined(Q_OS_LINUX)
    widget = new Video4LinuxWidget;
    dialog.resize(500, 400);
    dialog.setSizeGripEnabled(true);
#elif defined(Q_OS_MAC)
    widget = new AvfoundationProducerWidget;
    dialog.resize(1, 1);
#elif defined(Q_OS_WIN)
    widget = new DirectShowVideoWidget;
    dialog.resize(1, 1);
#endif

    layout->addWidget(widget);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    if (dialog.exec() == QDialog::Accepted) {
        Mlt::Profile profile;
        profile.set_explicit(1);
        delete dynamic_cast<AbstractProducerWidget *>(widget)->newProducer(profile);
    }
}

void MainWindow::on_actionReset_triggered()
{
    QMessageBox
        dialog(QMessageBox::Question,
               qApp->applicationName(),
               tr("This will reset <b>all</b> settings, and Shotcut must restart afterwards.\n"
                  "Do you want to reset and restart now?"),
               QMessageBox::No | QMessageBox::Yes,
               this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        Settings.reset();
        m_exitCode = EXIT_RESET;
        QApplication::closeAllWindows();
    }
}

void MainWindow::onCreateOrEditFilterOnOutput(Mlt::Filter *filter, const QStringList &key_properties)
{
    m_timelineDock->selectMultitrack();
    onFiltersDockTriggered();
    m_timelineDock->emitSelectedFromSelection();
    m_filterController->addOrEditFilter(filter, key_properties);
}

void MainWindow::showSettingsMenu() const
{
    QPoint point(140 * devicePixelRatioF(), ui->menuBar->height());
#if !defined(Q_OS_MAC)
    point = ui->menuBar->mapToGlobal(point);
#endif
    ui->menuSettings->popup(point, ui->menuSettings->defaultAction());
}
