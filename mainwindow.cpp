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

#include <QtGui>

static const int STATUS_TIMEOUT_MS = 3000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Create the UI.
    ui->setupUi(this);

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

    m_propertiesDock = new QDockWidget(tr("Properties"));
    m_propertiesDock->setObjectName("propertiesDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
    connect(this, SIGNAL(producerOpened()), this, SLOT(onProducerOpened()));
    connect(m_propertiesDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onPropertiesVisibilityChanged(bool)));

    readSettings();
    setFocus();
}

MainWindow::~MainWindow()
{
    delete m_propertiesDock;
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
    restoreState(m_settings.value("windowState").toByteArray());
}

void MainWindow::writeSettings()
{
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("windowState", saveState());
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version %1</h1>"
                "<p>Shotcut is an open source cross platform video editor.</p>"
                "<p>Copyright &copy; 2011 Meltytech, LLC</p>"
                "<p>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
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
    writeSettings();
    event->accept();
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
    QString resource(MLT.producer()->get("resource"));
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
        connect(avw, SIGNAL(producerChanged()), m_player, SLOT(onProducerOpened()));
    }
    else if (service == "decklink")
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
}

void MainWindow::onProducerChanged()
{
    MLT.refreshConsumer();
}

void MainWindow::on_actionViewProperties_triggered(bool checked)
{
    if (checked)
        m_propertiesDock->show();
    else
        m_propertiesDock->hide();
}

void MainWindow::onPropertiesVisibilityChanged(bool visible)
{
    ui->actionViewProperties->setChecked(visible);
}
