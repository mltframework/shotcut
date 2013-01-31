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

#include <QtGui>
#include <QDebug>

static const int STATUS_TIMEOUT_MS = 3000;

MainWindow::MainWindow()
    : QMainWindow(0)
    , ui(new Ui::MainWindow)
    , m_isKKeyPressed(false)
{
    // Create the UI.
    ui->setupUi(this);
#ifndef Q_WS_X11
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setDockNestingEnabled(true);

    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open", ui->actionOpen->icon()));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save", ui->actionSave->icon()));
    ui->actionEncode->setIcon(QIcon::fromTheme("media-record", ui->actionEncode->icon()));

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Setup the undo stack.
    m_undoStack = new QUndoStack(this);
    QAction *undoAction = m_undoStack->createUndoAction(this);
    QAction *redoAction = m_undoStack->createRedoAction(this);
    undoAction->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/icons/edit-undo.png")));
    redoAction->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/icons/edit-redo.png")));
    undoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Z", 0, QApplication::UnicodeUTF8));
    redoAction->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+Z", 0, QApplication::UnicodeUTF8));
    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);
    ui->actionUndo->setIcon(undoAction->icon());
    ui->actionRedo->setIcon(redoAction->icon());
    ui->actionUndo->setToolTip(undoAction->toolTip());
    ui->actionRedo->setToolTip(redoAction->toolTip());
    connect(m_undoStack, SIGNAL(canUndoChanged(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect(m_undoStack, SIGNAL(canRedoChanged(bool)), ui->actionRedo, SLOT(setEnabled(bool)));

    // Add the player widget.
    QLayout* layout = new QVBoxLayout(ui->playerPage);
    layout->setObjectName("centralWidgetLayout");
    layout->setMargin(0);
    m_player = new Player(this);
    layout->addWidget(m_player);
    connect(this, SIGNAL(producerOpened()), m_player, SLOT(onProducerOpened()));
    connect(m_player, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_player, SIGNAL(inChanged(int)), this, SLOT(onCutModified()));
    connect(m_player, SIGNAL(outChanged(int)), this, SLOT(onCutModified()));

    // Add the docks.
    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->hide();
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setWindowIcon(QIcon((":/icons/icons/view-form.png")));
    m_propertiesDock->toggleViewAction()->setIcon(QIcon::fromTheme("view-form", m_propertiesDock->windowIcon()));
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    ui->menuView->addAction(m_propertiesDock->toggleViewAction());
    ui->mainToolBar->addAction(m_propertiesDock->toggleViewAction());
    connect(m_propertiesDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPropertiesDockTriggered(bool)));

    m_recentDock = new RecentDock(this);
    m_recentDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_recentDock);
    ui->menuView->addAction(m_recentDock->toggleViewAction());
    ui->mainToolBar->addAction(m_recentDock->toggleViewAction());
    connect(m_recentDock, SIGNAL(itemActivated(QString)), this, SLOT(open(QString)));
    connect(m_recentDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onRecentDockTriggered(bool)));

    m_playlistDock = new PlaylistDock(this);
    m_playlistDock->hide();
    addDockWidget(Qt::LeftDockWidgetArea, m_playlistDock);
    ui->menuView->addAction(m_playlistDock->toggleViewAction());
    ui->mainToolBar->addAction(m_playlistDock->toggleViewAction());
    connect(m_playlistDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPlaylistDockTriggered(bool)));
    connect(m_playlistDock, SIGNAL(clipOpened(void*,int,int)), this, SLOT(openCut(void*, int, int)));
    connect(m_playlistDock, SIGNAL(itemActivated(int)), this, SLOT(seekPlaylist(int)));
    connect(m_playlistDock, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));
    connect(m_playlistDock->model(), SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(m_playlistDock->model(), SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(m_playlistDock->model(), SIGNAL(closed()), this, SLOT(onPlaylistClosed()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(m_playlistDock->model(), SIGNAL(loaded()), this, SLOT(updateMarkers()));
    connect(m_playlistDock->model(), SIGNAL(modified()), this, SLOT(updateMarkers()));

    m_historyDock = new QDockWidget(tr("History"), this);
    m_historyDock->hide();
    m_historyDock->setObjectName("historyDock");
    m_historyDock->setWindowIcon(QIcon((":/icons/icons/view-history.png")));
    m_historyDock->toggleViewAction()->setIcon(QIcon::fromTheme("view-history", m_historyDock->windowIcon()));
    addDockWidget(Qt::LeftDockWidgetArea, m_historyDock);
    ui->menuView->addAction(m_historyDock->toggleViewAction());
    ui->mainToolBar->addAction(m_historyDock->toggleViewAction());
    connect(m_historyDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onHistoryDockTriggered(bool)));
    QUndoView* undoView = new QUndoView(m_undoStack, m_historyDock);
    undoView->setObjectName("historyView");
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
    ui->mainToolBar->addAction(ui->actionEncode);
    connect(this, SIGNAL(producerOpened()), m_encodeDock, SLOT(onProducerOpened()));
    connect(m_encodeDock, SIGNAL(visibilityChanged(bool)), ui->actionEncode, SLOT(setChecked(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_player, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_propertiesDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_recentDock, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpen, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionOpenOther, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), ui->actionExit, SLOT(setDisabled(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), this, SLOT(onCaptureStateChanged(bool)));
    connect(m_encodeDock, SIGNAL(captureStateChanged(bool)), m_historyDock, SLOT(setDisabled(bool)));

    m_jobsDock = new JobsDock(this);
    m_jobsDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_jobsDock);
    tabifyDockWidget(m_encodeDock, m_jobsDock);
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(show()));
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(raise()));
    connect(m_jobsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onJobsVisibilityChanged(bool)));

    m_meltedServerDock = new MeltedServerDock(this);
    m_meltedServerDock->hide();
    addDockWidget(Qt::TopDockWidgetArea, m_meltedServerDock);
    ui->menuView->addAction(m_meltedServerDock->toggleViewAction());

    m_meltedPlaylistDock = new MeltedPlaylistDock(this);
    m_meltedPlaylistDock->hide();
    addDockWidget(Qt::TopDockWidgetArea, m_meltedPlaylistDock);
    splitDockWidget(m_meltedServerDock, m_meltedPlaylistDock, Qt::Horizontal);
    ui->menuView->addAction(m_meltedPlaylistDock->toggleViewAction());
    connect(m_meltedServerDock, SIGNAL(connected(QString, quint16)), m_meltedPlaylistDock, SLOT(onConnected(QString,quint16)));
    connect(m_meltedServerDock, SIGNAL(disconnected()), m_meltedPlaylistDock, SLOT(onDisconnected()));
    connect(m_meltedServerDock, SIGNAL(unitActivated(quint8)), m_meltedPlaylistDock, SLOT(onUnitChanged(quint8)));
    connect(m_meltedServerDock, SIGNAL(unitActivated(quint8)), this, SLOT(onMeltedUnitActivated()));
    connect(m_meltedPlaylistDock, SIGNAL(appendRequested()), m_meltedServerDock, SLOT(onAppendRequested()));
    connect(m_meltedServerDock, SIGNAL(append(QString)), m_meltedPlaylistDock, SLOT(onAppend(QString)));
    connect(m_meltedPlaylistDock, SIGNAL(insertRequested(int)), m_meltedServerDock, SLOT(onInsertRequested(int)));
    connect(m_meltedServerDock, SIGNAL(insert(QString,int)), m_meltedPlaylistDock, SLOT(onInsert(QString,int)));
    connect(m_meltedServerDock, SIGNAL(unitOpened(quint8)), this, SLOT(onMeltedUnitOpened()));
    connect(m_meltedServerDock, SIGNAL(unitOpened(quint8)), m_player, SLOT(onMeltedUnitOpened()));
    connect(m_meltedServerDock->actionFastForward(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(fastForward()));
    connect(m_meltedServerDock->actionPause(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(pause()));
    connect(m_meltedServerDock->actionPlay(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(play()));
    connect(m_meltedServerDock->actionRewind(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(rewind()));
    connect(m_meltedServerDock->actionStop(), SIGNAL(triggered()), m_meltedPlaylistDock->transportControl(), SLOT(stop()));

    MeltedUnitsModel* unitsModel = (MeltedUnitsModel*) m_meltedServerDock->unitsModel();
    MeltedPlaylistModel* playlistModel = (MeltedPlaylistModel*) m_meltedPlaylistDock->model();
    connect(m_meltedServerDock, SIGNAL(connected(QString,quint16)), unitsModel, SLOT(onConnected(QString,quint16)));
    connect(unitsModel, SIGNAL(clipIndexChanged(quint8, int)), playlistModel, SLOT(onClipIndexChanged(quint8, int)));
    connect(unitsModel, SIGNAL(generationChanged(quint8)), playlistModel, SLOT(onGenerationChanged(quint8)));

    // connect video widget signals
#if defined(Q_WS_MAC) || defined(Q_WS_WIN)
    Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
    connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
    connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
#else
    if (m_settings.value("player/opengl", true).toBool()) {
        Mlt::GLWidget* videoWidget = (Mlt::GLWidget*) &(MLT);
        connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
        connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
    }
    else {
        Mlt::SDLWidget* videoWidget = (Mlt::SDLWidget*) &(MLT);
        connect(videoWidget, SIGNAL(dragStarted()), m_playlistDock, SLOT(onPlayerDragStarted()));
        connect(videoWidget, SIGNAL(seekTo(int)), m_player, SLOT(seek(int)));
    }
#endif

    // Add fullscreen action to toolbar
//    QWidget* spacer = new QWidget(this);
//    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
//    ui->mainToolBar->addWidget(spacer);
    ui->mainToolBar->addAction(ui->actionEnter_Full_Screen);

    readSettings();
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
    }
}

void MainWindow::openVideo()
{
    QString settingKey("openPath");
    QString directory(m_settings.value(settingKey,
        QDesktopServices::storageLocation(QDesktopServices::MoviesLocation)).toString());
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), directory);

    if (!filename.isNull()) {
        m_settings.setValue(settingKey, QFileInfo(filename).path());
        activateWindow();
        open(filename);
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
    double speed = MLT.producer()? MLT.producer()->get_speed(): 0;
    // we bypass this->open() to prevent sending producerOpened signal to self, which causes to reload playlist
    if ((void*) MLT.producer()->get_producer() != (void*) m_playlistDock->model()->playlist()->get_playlist())
        MLT.open(new Mlt::Producer(*(m_playlistDock->model()->playlist())));
    m_player->setIn(-1);
    m_player->setOut(-1);
    // since we do not emit producerOpened, these components need updating
    m_player->onProducerOpened();
    m_encodeDock->onProducerOpened();
    updateMarkers();
    if (speed == 0)
        m_player->pause();
    else
        m_player->play(speed);
    MLT.seek(start);
    m_player->setFocus();
}

void MainWindow::readSettings()
{
    restoreGeometry(m_settings.value("geometry").toByteArray());
#if defined(Q_WS_MAC)
    QSize s = size();
    s.setHeight(s.height() + 38);
    resize(s);
#endif
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

void MainWindow::setCurrentFile(const QString &filename)
{
    QString shownName = "Untitled";
    m_currentFile = filename;
    setWindowModified(false);
    if (!m_currentFile.isEmpty())
        shownName = QFileInfo(m_currentFile).fileName();
    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(qApp->applicationName()));
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version %1</h1>"
                "<p><a href=\"http://www.shotcut.org/\">Shotcut</a> is a free, open source, cross platform video editor.</p>"
                "<p>Copyright &copy; 2011-2012 <a href=\"http://www.meltytech.com/\">Meltytech</a>, LLC</p>"
                "<p>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program proudly uses the following projects:<ul>"
                "<li><a href=\"http://qt.nokia.com/\">Qt</a> application and UI framework</li>"
                "<li><a href=\"http://www.mltframework.org/\">MLT</a> multimedia authoring framework</li>"
                "<li><a href=\"http://www.ffmpeg.org/\">FFmpeg</a> multimedia format and codec libraries</li>"
                "<li><a href=\"http://www.videolan.org/developers/x264.html\">x264</a> H.264 encoder</li>"
                "<li><a href=\"http://www.webmproject.org/\">WebM</a> VP8 encoder</li>"
                "<li><a href=\"http://lame.sourceforge.net/\">LAME</a> MP3 encoder</li>"
                "<li><a href=\"http://www.dyne.org/software/frei0r/\">Frei0r</a> video plugins</li>"
                "<li><a href=\"http://www.ladspa.org/\">LADSPA</a> audio plugins</li>"
                "</ul></p>"
                "<p>The source code used to build this program can be downloaded from "
                "<a href=\"http://www.shotcut.org/\">shotcut.org</a>.</p>"
                "<small>This program is distributed in the hope that it will be useful, "
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
        m_player->seek(m_player->position() - 10);
        break;
    case Qt::Key_PageDown:
        m_player->seek(m_player->position() + 10);
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
        m_playlistDock->show();
        m_playlistDock->raise();
        if (event->modifiers() == Qt::ControlModifier)
            m_playlistDock->moveClipUp();
        m_playlistDock->decrementIndex();
        break;
    case Qt::Key_Down:
        m_playlistDock->show();
        m_playlistDock->raise();
        if (event->modifiers() == Qt::ControlModifier)
            m_playlistDock->moveClipDown();
        m_playlistDock->incrementIndex();
        break;
    case Qt::Key_1:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(0);
        break;
    case Qt::Key_2:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(1);
        break;
    case Qt::Key_3:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(2);
        break;
    case Qt::Key_4:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(3);
        break;
    case Qt::Key_5:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(4);
        break;
    case Qt::Key_6:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(5);
        break;
    case Qt::Key_7:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(6);
        break;
    case Qt::Key_8:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(7);
        break;
    case Qt::Key_9:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(8);
        break;
    case Qt::Key_0:
        m_playlistDock->show();
        m_playlistDock->raise();
        m_playlistDock->setIndex(9);
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
        open(mimeData->urls().at(0).path());
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
    ui->stackedWidget->setCurrentIndex(1);
    delete m_propertiesDock->widget();

    // TODO: make a producer widget for avformat file sources
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
        QScrollArea* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setWidget(w);
        m_propertiesDock->setWidget(scroll);
    }
    onProducerChanged();
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
        QDesktopServices::storageLocation(QDesktopServices::MoviesLocation)).toString());
    QString filename = QFileDialog::getSaveFileName(this, tr("Save XML"), directory, tr("MLT XML (*.mlt)"));
    if (!filename.isEmpty()) {
        saveXML(filename);
        setCurrentFile(filename);
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

void MainWindow::on_actionEncode_triggered(bool checked)
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

void MainWindow::onJobsVisibilityChanged(bool checked)
{
    m_jobsVisible = checked;
}

void MainWindow::onRecentDockTriggered(bool checked)
{
    if (checked)
        m_recentDock->raise();
}

void MainWindow::onPropertiesDockTriggered(bool checked)
{
    if (checked)
        m_propertiesDock->raise();
}

void MainWindow::onPlaylistDockTriggered(bool checked)
{
    if (checked)
        m_playlistDock->raise();
}

void MainWindow::onHistoryDockTriggered(bool checked)
{
    if (checked)
        m_historyDock->raise();
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

void MainWindow::onMeltedUnitOpened()
{
    Mlt::Producer* producer = new Mlt::Producer(MLT.profile(), "color:");
    MLT.open(producer);
    MLT.play(0);
    ui->stackedWidget->setCurrentIndex(1);
    delete m_propertiesDock->widget();
    m_player->connectTransport(m_meltedPlaylistDock->transportControl());
    connect(m_meltedServerDock, SIGNAL(positionUpdated(int,double,int,int,int,bool)), m_player, SLOT(onShowFrame(int,double,int,int,int,bool)));
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
