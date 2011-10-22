/*
 * Copyright (c) 2011 Dan Dennedy <dan@dennedy.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
