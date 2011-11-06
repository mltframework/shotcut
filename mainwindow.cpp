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
#include <QtGui>
#include <MltProfile.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Create the UI.
    ui->setupUi(this);

    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open", ui->actionOpen->icon()));
    ui->actionPlay->setIcon(QIcon::fromTheme("media-playback-start", ui->actionPlay->icon()));
    ui->actionPause->setIcon(QIcon::fromTheme("media-playback-pause", ui->actionPause->icon()));
    m_playIcon = ui->actionPlay->icon();
    m_pauseIcon = ui->actionPause->icon();

    // Connect UI signals.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openVideo()));
    connect(ui->actionPlay, SIGNAL(triggered()), this, SLOT(togglePlayPause()));
    connect(ui->actionPause, SIGNAL(triggered()), this, SLOT(pause()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    // Accept drag-n-drop of files.
    this->setAcceptDrops(true);

    // Create MLT video widget and connect its signals.
    mltWidget = Mlt::Controller::createWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mltWidget->qwidget());
    ui->centralWidget->setLayout(layout);
    connect(mltWidget->qwidget(), SIGNAL(frameReceived(Mlt::QFrame, unsigned)), this, SLOT(onShowFrame(Mlt::QFrame, unsigned)));
}

MainWindow::~MainWindow()
{
    delete mltWidget;
    delete ui;
}

void MainWindow::open(const QString& url)
{
    if (!mltWidget->open(url.toUtf8().constData()))
        play();
}

void MainWindow::openVideo()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath());
    if (!filename.isNull())
        open(filename);
    else
        // If file invalid, then on some platforms the dialog messes up SDL.
        mltWidget->onWindowResize();
}

void MainWindow::togglePlayPause()
{
    if (ui->actionPlay->icon().cacheKey() == m_playIcon.cacheKey())
        play();
    else
        pause();
}

void MainWindow::play()
{
    mltWidget->play();
    ui->actionPlay->setIcon(m_pauseIcon);
    ui->actionPlay->setText(tr("Pause"));
    ui->actionPlay->setToolTip(tr("Pause playback"));
    forceResize();
}

void MainWindow::pause()
{
    mltWidget->pause();
    ui->actionPlay->setIcon(m_playIcon);
    ui->actionPlay->setText(tr("Play"));
    ui->actionPlay->setToolTip(tr("Start playback"));
    forceResize();
}

void MainWindow::resizeEvent(QResizeEvent*)
{
    mltWidget->onWindowResize();
}

void MainWindow::forceResize()
{
    // XXX: this is a hack to force video container to resize
    int width = ui->centralWidget->width();
    int height = ui->centralWidget->height();
    ui->centralWidget->resize(width - 1, height - 1);
    ui->centralWidget->resize(width, height);
}

void MainWindow::onShowFrame(Mlt::QFrame, unsigned position)
{
    ui->statusBar->showMessage(QString().sprintf("%.3f", position / mltWidget->profile()->fps()));
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version 0.5.0</h1>"
                "<p>Shotcut is an open source cross platform video editor.</p>"
                "<p><small>Copyright 2011 Meltytech, LLC</small></p>"
                "<p><small>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></small></p>"
                "<p><small>This program is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</small></p>"
                ));
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

void MainWindow::on_actionOpenURL_triggered()
{
    bool ok;
    QString url = QInputDialog::getText (this, QString(), tr("Enter a media URL"), QLineEdit::Normal, "decklink:", &ok);
    if (ok && !url.isEmpty())
        open(url);
}
