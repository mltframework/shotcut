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

#include <QtGui>

static const int STATUS_TIMEOUT_MS = 3000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Add the player widget.
    QLayout* layout = new QVBoxLayout(ui->centralWidget);
    layout->setObjectName("centralWidgetLayout");
    layout->setMargin(0);
    m_player = new Player(this);
    layout->addWidget(m_player);
    connect(this, SIGNAL(producerOpened()), m_player, SLOT(onProducerOpened()));
    connect(m_player, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));

    // Add the docks.
    m_propertiesDock = new QDockWidget(tr("Properties"));
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setWindowIcon(QIcon((":/icons/icons/view-form.png")));
    m_propertiesDock->toggleViewAction()->setIcon(QIcon::fromTheme("view-form", m_propertiesDock->windowIcon()));
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    ui->menuView->addAction(m_propertiesDock->toggleViewAction());
    ui->mainToolBar->addAction(m_propertiesDock->toggleViewAction());
    connect(m_propertiesDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onPropertiesDockTriggered(bool)));

    m_recentDock = new RecentDock(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_recentDock);
    tabifyDockWidget(m_recentDock, m_propertiesDock);
    ui->menuView->addAction(m_recentDock->toggleViewAction());
    ui->mainToolBar->addAction(m_recentDock->toggleViewAction());
    connect(m_recentDock, SIGNAL(itemActivated(QString)), this, SLOT(open(QString)));
    connect(m_recentDock->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onRecentDockTriggered(bool)));

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

    m_jobsDock = new JobsDock(this);
    m_jobsDock->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_jobsDock);
    tabifyDockWidget(m_jobsDock, m_encodeDock);
    ui->menuView->addAction(m_jobsDock->toggleViewAction());
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(show()));
    connect(&JOBS, SIGNAL(jobAdded()), m_jobsDock, SLOT(raise()));
    connect(m_jobsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onJobsVisibilityChanged(bool)));

    // Connect signals.
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));

    readSettings();
    setFocus();
    setCurrentFile("");
}

MainWindow::~MainWindow()
{
    delete m_propertiesDock;
    delete m_recentDock;
    delete m_encodeDock;
    delete m_jobsDock;
    delete m_player;
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
    if (!MLT.open(url.toUtf8().constData())) {
        Mlt::Properties* props = const_cast<Mlt::Properties*>(properties);
        if (props && props->is_valid())
            mlt_properties_inherit(MLT.producer()->get_properties(), props->get_properties());
        open(MLT.producer());
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

void MainWindow::showStatusMessage(QString message)
{
    ui->statusBar->showMessage(message, STATUS_TIMEOUT_MS);
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
    case Qt::Key_K:
        m_player->togglePlayPaused();
        break;
    case Qt::Key_I:
        m_player->setIn(m_player->position());
        break;
    case Qt::Key_O:
        m_player->setOut(m_player->position());
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

// Drag-n-drop events

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        open(mimeData->urls().at(0).path());
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
        m_recentDock->add(resource);
    }
    else if (service == "pixbuf" || service == "qimage") {
        ImageProducerWidget* avw = new ImageProducerWidget(this);
        w = avw;
        connect(avw, SIGNAL(producerReopened()), m_player, SLOT(onProducerOpened()));
        m_recentDock->add(resource);
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
    // TODO: disable this until we have playlist support
    // setWindowModified(true);

    MLT.refreshConsumer();
}

bool MainWindow::on_actionSave_triggered()
{
    if (m_currentFile.isEmpty()) {
        return on_actionSave_As_triggered();
    } else {
        MLT.saveXML(m_currentFile);
        setCurrentFile(m_currentFile);
        showStatusMessage(tr("Saved %1").arg(m_currentFile));
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
        MLT.saveXML(filename);
        setCurrentFile(filename);
        showStatusMessage(tr("Saved %1").arg(m_currentFile));
    }
    return filename.isEmpty();
}

bool MainWindow::continueModified()
{
    if (isWindowModified()) {
        int r = QMessageBox::warning(this, qApp->applicationName(),
                                     tr("The project has been modified.\n"
                                        "Do you want to save your changes?"),
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No,
                                     QMessageBox::Cancel | QMessageBox::Escape);
        if (r == QMessageBox::Yes) {
            return on_actionSave_triggered();
        } else if (r == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
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
