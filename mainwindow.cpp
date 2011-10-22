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
#include <QFileDialog>
#include <QVBoxLayout>

MainWindow::MainWindow (QWidget *parent)
    : QMainWindow (parent)
    , ui (new Ui::MainWindow)
{
    // Create the UI.
    ui->setupUi (this);

    // This is required for SDL embeddding.
    ui->centralWidget->setAttribute (Qt::WA_NativeWindow);

    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    ui->actionOpen->setIcon (QIcon::fromTheme ("document-open", ui->actionOpen->icon ()));
    ui->actionPlay->setIcon (QIcon::fromTheme ("media-playback-start", ui->actionPlay->icon ()));
    ui->actionPause->setIcon (QIcon::fromTheme ("media-playback-pause", ui->actionPause->icon ()));

    // Connect UI signals.
    connect (ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect (ui->actionPlay, SIGNAL(triggered()), this, SLOT(play()));
    connect (ui->actionPause, SIGNAL(triggered()), this, SLOT(pause()));

    // Create MLT controller and connect its signals.
    mlt = new MltController (ui->centralWidget);
    connect (mlt, SIGNAL(frameReceived (void*, unsigned)), this, SLOT(onShowFrame (void*, unsigned)));
#ifdef Q_WS_MAC
    gl = new GLWidget (this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget (gl);
    layout->setMargin (0);
    ui->centralWidget->setLayout (layout);
    connect (this, SIGNAL (showImageSignal (QImage)), gl, SLOT (showImage(QImage)));
#endif
}

MainWindow::~MainWindow ()
{
    delete mlt;
#ifdef Q_WS_MAC
    delete gl;
#endif
    delete ui;
}

void MainWindow::initializeMlt ()
{
    ui->statusBar->showMessage (tr("Loading plugins..."));

    mlt->init ();
    // Load a color producer to clear the video region with black.
    mlt->open ("color:");
    pause ();

    ui->statusBar->showMessage (tr("Ready"));
}

void MainWindow::openVideo ()
{
    QString filename = QFileDialog::getOpenFileName (this);
    if (!filename.isNull())
    {
        if (!mlt->open (filename.toUtf8().constData())) {
#ifdef Q_WS_MAC
            gl->setImageAspectRatio (mlt->profile()->dar());
#endif
            play();
        }
    }
    // If file invalid, then on some platforms the dialog messes up SDL.
    mlt->onWindowResize ();
}

void MainWindow::play ()
{
    mlt->play ();
    forceResize ();
    ui->statusBar->showMessage (tr("Playing"));
}

void MainWindow::pause ()
{
    mlt->pause ();
    forceResize ();
    ui->statusBar->showMessage (tr("Paused"));
}

void MainWindow::resizeEvent (QResizeEvent*)
{
    mlt->onWindowResize();
}

void MainWindow::forceResize()
{
    // XXX: this is a hack to force video container to resize
    int width = ui->centralWidget->width();
    int height = ui->centralWidget->height();
    ui->centralWidget->resize (width - 1, height - 1);
    ui->centralWidget->resize (width, height);
}

void MainWindow::onShowFrame (void* frame, unsigned position)
{
#ifdef Q_WS_MAC
    emit showImageSignal (mlt->getImage (frame));
#endif
    ui->statusBar->showMessage (QString().sprintf ("%.3f", position / mlt->profile()->fps()));
}
