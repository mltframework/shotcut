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
#include <QtGui>
#include <Mlt.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings("Meltytech", "Shotcut")
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

    readSettings();

    // Create MLT video widget and connect its signals.
    mltWidget = Mlt::Controller::createWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    ui->centralWidget->setLayout(layout);

    mltWidget->qwidget()->setContentsMargins(0, 0, 0, 0);
    mltWidget->qwidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(mltWidget->qwidget(), 10);
    layout->addStretch();

    m_scrubber = new ScrubBar(this);
    m_scrubber->hide();
    layout->addWidget(m_scrubber);

    connect(mltWidget->qwidget(), SIGNAL(frameReceived(Mlt::QFrame, unsigned)), this, SLOT(onShowFrame(Mlt::QFrame, unsigned)));
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(onSeek(int)));
}

MainWindow::~MainWindow()
{
    delete mltWidget;
    delete ui;
}

void MainWindow::open(const QString& url)
{
    if (!mltWidget->open(url.toUtf8().constData())) {
        m_scrubber->setFramerate(mltWidget->profile()->fps());
        m_scrubber->setScale(mltWidget->producer()->get_length());
        if (mltWidget->producer()->get_int("seekable"))
            m_scrubber->show();
        else
            m_scrubber->hide();
        play();
    }
}

void MainWindow::openVideo()
{
    QString settingKey("openPath");
    QString directory(m_settings.value(settingKey, QDir::homePath()).toString());
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), directory);

    if (!filename.isNull()) {
        m_settings.setValue(settingKey, QFileInfo(filename).path());
        open(filename);
    }
    else {
        // If file invalid, then on some platforms the dialog messes up SDL.
        mltWidget->onWindowResize();
    }
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
    return;
    // XXX: this is a hack to force video container to resize
    int width = ui->centralWidget->width();
    int height = ui->centralWidget->height();
    ui->centralWidget->resize(width - 1, height - 1);
    ui->centralWidget->resize(width, height);
}

void MainWindow::readSettings()
{
    QRect rect = m_settings.value("geometry", QRect(200, 200, 852, 555)).toRect();
    rect.setTop(rect.top() - ui->menuBar->height());
    rect.setHeight(rect.height() - ui->menuBar->height());
    move(rect.topLeft());
    resize(rect.size());
}

void MainWindow::writeSettings()
{
    m_settings.setValue("geometry", geometry());
}

void MainWindow::onShowFrame(Mlt::QFrame, unsigned position)
{
    ui->statusBar->showMessage(QString().sprintf("%.3f", position / mltWidget->profile()->fps()));
    m_scrubber->onSeek(position);
}

void MainWindow::on_actionAbout_Shotcut_triggered()
{
    QMessageBox::about(this, tr("About Shotcut"),
             tr("<h1>Shotcut version 0.5.0</h1>"
                "<p>Shotcut is an open source cross platform video editor.</p>"
                "<p>Copyright &copy; 2011 Meltytech, LLC</p>"
                "<p>Licensed under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU General Public License v3.0</a></p>"
                "<p>This program is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
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

void MainWindow::closeEvent(QCloseEvent* event)
{
    writeSettings();
    event->accept();
}

void MainWindow::on_actionOpenURL_triggered()
{
    bool ok;
    QString url = QInputDialog::getText (this, QString(), tr("Enter a media URL"), QLineEdit::Normal, "decklink:", &ok);
    if (ok && !url.isEmpty())
        open(url);
}

void MainWindow::onSeek(int position)
{
    pause();
    mltWidget->seek(position);
}
