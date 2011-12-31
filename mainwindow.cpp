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
    , mltWidget(0)
    , m_settings("Meltytech", "Shotcut")
{
    // Create the UI.
    ui->setupUi(this);

    // These use the icon theme on Linux, with fallbacks to the icons specified in QtDesigner for other platforms.
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open", ui->actionOpen->icon()));
    ui->actionPlay->setIcon(QIcon::fromTheme("media-playback-start", ui->actionPlay->icon()));
    ui->actionPause->setIcon(QIcon::fromTheme("media-playback-pause", ui->actionPause->icon()));
    ui->actionSkipNext->setIcon(QIcon::fromTheme("media-skip-forward", ui->actionSkipNext->icon()));
    ui->actionSkipPrevious->setIcon(QIcon::fromTheme("media-skip-backward", ui->actionSkipPrevious->icon()));
    ui->actionRewind->setIcon(QIcon::fromTheme("media-seek-backward", ui->actionRewind->icon()));
    ui->actionFastForward->setIcon(QIcon::fromTheme("media-seek-forward", ui->actionFastForward->icon()));
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
    mltWidget->qwidget()->setProperty("progressive", ui->actionProgressive->isChecked());
    if (m_settings.value("quality", "medium").toString() == "high") {
        mltWidget->qwidget()->setProperty("rescale", "bicubic");
        mltWidget->qwidget()->setProperty("deinterlace_method", "yadif");
    }
    else if (m_settings.value("quality", "medium").toString() == "medium") {
        mltWidget->qwidget()->setProperty("rescale", "bilinear");
        mltWidget->qwidget()->setProperty("deinterlace_method", "yadif-nospatial");
    }
    else {
        mltWidget->qwidget()->setProperty("rescale", "nearest");
        mltWidget->qwidget()->setProperty("deinterlace_method", "onefield");
    }
    mltWidget->qwidget()->setContentsMargins(0, 0, 0, 0);
    mltWidget->qwidget()->setContextMenuPolicy(Qt::CustomContextMenu);
    mltWidget->qwidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(mltWidget->qwidget(), 10);
    layout->addStretch();
    ui->centralWidget->setLayout(layout);
    connect(mltWidget->qwidget(), SIGNAL(frameReceived(Mlt::QFrame, unsigned)), this, SLOT(onShowFrame(Mlt::QFrame, unsigned)));
    connect(mltWidget->qwidget(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onVideoWidgetContextMenu(QPoint)));

    // Add the scrub bar.
    m_scrubber = new ScrubBar(this);
    m_scrubber->hide();
    layout->addSpacing(4);
    layout->addWidget(m_scrubber);
    layout->addSpacing(4);
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(onSeek(int)));
    connect(m_scrubber, SIGNAL(inChanged(int)), this, SLOT(onInChanged(int)));
    connect(m_scrubber, SIGNAL(outChanged(int)), this, SLOT(onOutChanged(int)));

    // Add toolbar for transport controls.
    QToolBar* toolbar = new QToolBar(tr("Transport Controls"), this);
    int s = style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolbar->setIconSize(QSize(s, s));
    toolbar->setContentsMargins(0, 0, 5, 0);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_positionSpinner = new QSpinBox(this);
    m_positionSpinner->setToolTip(tr("Position in frames"));
    m_positionSpinner->setAlignment(Qt::AlignRight);
    m_positionSpinner->setRange(0, INT_MAX);
    m_positionSpinner->setValue(0);
    m_positionSpinner->setEnabled(false);
    m_durationLabel = new QLabel(this);
    m_durationLabel->setToolTip(tr("Duration in seconds"));
    m_durationLabel->setText("0.000");
    m_durationLabel->setAlignment(Qt::AlignRight);
    m_durationLabel->setContentsMargins(0, 5, 0, 0);
    m_durationLabel->setFixedWidth(m_positionSpinner->width());
    connect(m_positionSpinner, SIGNAL(valueChanged(int)), this, SLOT(onSeek(int)));
    connect(m_positionSpinner, SIGNAL(editingFinished()), this, SLOT(setFocus()));

    ui->actionPlay->setEnabled(false);
    ui->actionSkipPrevious->setEnabled(false);
    ui->actionSkipNext->setEnabled(false);
    ui->actionRewind->setEnabled(false);
    ui->actionFastForward->setEnabled(false);
    toolbar->addWidget(m_positionSpinner);
    toolbar->addWidget(spacer);
    toolbar->addAction(ui->actionSkipPrevious);
    toolbar->addAction(ui->actionRewind);
    toolbar->addAction(ui->actionPlay);
    toolbar->addAction(ui->actionFastForward);
    toolbar->addAction(ui->actionSkipNext);
    spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    toolbar->addWidget(m_durationLabel);
    layout->addWidget(toolbar);
    setFocus();
}

MainWindow::~MainWindow()
{
    delete mltWidget;
    delete ui;
}

void MainWindow::open(const QString& url)
{
    if (!mltWidget->open(url.toUtf8().constData())) {
        int len = mltWidget->producer()->get_length();
        bool seekable = mltWidget->producer()->get_int("seekable");

        mltWidget->producer()->set("ignore_points", 1);
        m_scrubber->setFramerate(mltWidget->profile()->fps());
        m_scrubber->setScale(len);
        if (seekable) {
            m_durationLabel->setText(QString().sprintf("%.03f", len / mltWidget->profile()->fps()));
            m_scrubber->setInPoint(mltWidget->producer()->get_in());
            m_scrubber->setOutPoint(mltWidget->producer()->get_out());
            m_scrubber->show();
        }
        else {
            m_durationLabel->setText(tr("Live"));
            m_scrubber->hide();
        }
        m_positionSpinner->setEnabled(seekable);
        ui->actionPlay->setEnabled(true);
        ui->actionSkipPrevious->setEnabled(seekable);
        ui->actionSkipNext->setEnabled(seekable);
        ui->actionRewind->setEnabled(seekable);
        ui->actionFastForward->setEnabled(seekable);
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
        activateWindow();
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

void MainWindow::play(double speed)
{
    mltWidget->play(speed);
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
    ui->actionProgressive->setChecked(m_settings.value("progressive", true).toBool());
    if (m_settings.value("quality", "medium").toString() == "high")
        ui->actionHighQuality->setChecked(true);
    else if (m_settings.value("quality", "medium").toString() == "medium")
        ui->actionMediumQuality->setChecked(true);
    else
        ui->actionLowQuality->setChecked(true);
}

void MainWindow::writeSettings()
{
    m_settings.setValue("geometry", geometry());
    m_settings.setValue("progressive", ui->actionProgressive->isChecked());
    if (ui->actionLowQuality->isChecked())
        m_settings.setValue("quality", "low");
    else if (ui->actionMediumQuality->isChecked())
        m_settings.setValue("quality", "medium");
    else if (ui->actionHighQuality->isChecked())
        m_settings.setValue("quality", "high");
}

void MainWindow::onShowFrame(Mlt::QFrame, unsigned position)
{
    m_positionSpinner->blockSignals(true);
    m_positionSpinner->setValue((int) position);
    m_positionSpinner->blockSignals(false);
    m_scrubber->onSeek(position);
    if ((int) position >= mltWidget->producer()->get_length() - 1)
        pause();
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


void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Home:
        onSeek(0);
        break;
    case Qt::Key_End:
        onSeek(mltWidget->producer()->get_length() - 1);
        break;
    case Qt::Key_Left:
        onSeek(m_positionSpinner->value() - 1);
        break;
    case Qt::Key_Right:
        onSeek(m_positionSpinner->value() + 1);
        break;
    case Qt::Key_PageUp:
        onSeek(m_positionSpinner->value() - 10);
        break;
    case Qt::Key_PageDown:
        onSeek(m_positionSpinner->value() + 10);
        break;
    case Qt::Key_K:
        togglePlayPause();
        break;
    case Qt::Key_I:
        onInChanged(m_positionSpinner->value());
        m_scrubber->setInPoint(m_positionSpinner->value());
        break;
    case Qt::Key_O:
        onOutChanged(m_positionSpinner->value());
        m_scrubber->setOutPoint(m_positionSpinner->value());
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

void MainWindow::on_actionOpenURL_triggered()
{
    bool ok;
    QString url = QInputDialog::getText (this, QString(), tr("Enter a media URL"), QLineEdit::Normal, "decklink:", &ok);
    if (ok && !url.isEmpty())
        open(url);
}

void MainWindow::onSeek(int position)
{
    if (mltWidget->producer()->get_int("seekable")) {
        pause();
        if (position >= 0)
            mltWidget->seek(qMin(position, mltWidget->producer()->get_length() - 1));
    }
}

void MainWindow::onInChanged(int in)
{
    mltWidget->producer()->set("in", in);
}

void MainWindow::onOutChanged(int out)
{
    mltWidget->producer()->set("out", out);
}

void MainWindow::onVideoWidgetContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    menu.addAction(ui->actionProgressive);
    QMenu* sub = menu.addMenu(tr("Quality"));
    QActionGroup group(sub);
    group.addAction(ui->actionLowQuality);
    group.addAction(ui->actionMediumQuality);
    group.addAction(ui->actionHighQuality);
    sub->addActions(group.actions());
    menu.exec(ui->centralWidget->mapToGlobal(pos));
}

void MainWindow::on_actionSkipNext_triggered()
{
    int pos = m_positionSpinner->value();
    if (pos < mltWidget->producer()->get_in())
        mltWidget->seek(mltWidget->producer()->get_in());
    else if (pos >= mltWidget->producer()->get_out())
        mltWidget->seek(mltWidget->producer()->get_length() - 1);
    else
        mltWidget->seek(mltWidget->producer()->get_out());
    ui->statusBar->showMessage(ui->actionSkipNext->toolTip(), 3000);
}

void MainWindow::on_actionSkipPrevious_triggered()
{
    int pos = m_positionSpinner->value();
    if (pos > mltWidget->producer()->get_out())
        mltWidget->seek(mltWidget->producer()->get_out());
    else if (pos <= mltWidget->producer()->get_in())
        mltWidget->seek(0);
    else
        mltWidget->seek(mltWidget->producer()->get_in());
    ui->statusBar->showMessage(ui->actionSkipPrevious->toolTip(), 3000);
}

void MainWindow::on_actionProgressive_triggered(bool checked)
{
    mltWidget->qwidget()->setProperty("progressive", checked);
    if (mltWidget->consumer()) {
        mltWidget->consumer()->stop();
        mltWidget->consumer()->set("progressive", checked);
        mltWidget->consumer()->start();
    }
}

void MainWindow::on_actionLowQuality_triggered(bool checked)
{
    if (checked) {
        mltWidget->qwidget()->setProperty("rescale", "nearest");
        mltWidget->qwidget()->setProperty("deinterlace_method", "onefield");
        if (mltWidget->consumer()) {
            mltWidget->consumer()->stop();
            mltWidget->consumer()->set("rescale", "nearest");
            mltWidget->consumer()->set("deinterlace_method", "onefield");
            mltWidget->consumer()->start();
        }
    }
}

void MainWindow::on_actionMediumQuality_triggered(bool checked)
{
    if (checked) {
        mltWidget->qwidget()->setProperty("rescale", "bilinear");
        mltWidget->qwidget()->setProperty("deinterlace_method", "yadif-nospatial");
        if (mltWidget->consumer()) {
            mltWidget->consumer()->stop();
            mltWidget->consumer()->set("rescale", "bilinear");
            mltWidget->consumer()->set("deinterlace_method", "yadif-nospatial");
            mltWidget->consumer()->start();
        }
    }
}

void MainWindow::on_actionHighQuality_triggered(bool checked)
{
    if (checked) {
        mltWidget->qwidget()->setProperty("rescale", "bicubic");
        mltWidget->qwidget()->setProperty("deinterlace_method", "yadif");
        if (mltWidget->consumer()) {
            mltWidget->consumer()->stop();
            mltWidget->consumer()->set("rescale", "bicubic");
            mltWidget->consumer()->set("deinterlace_method", "yadif");
            mltWidget->consumer()->start();
        }
    }
}

void MainWindow::on_actionRewind_triggered()
{
    if (mltWidget->producer()->get_int("seekable")) {
        if (mltWidget->producer()->get_speed() >= 0)
            play(-1.0);
        else
            mltWidget->producer()->set_speed(mltWidget->producer()->get_speed() * 2);
    }
}

void MainWindow::on_actionFastForward_triggered()
{
    if (mltWidget->producer()->get_int("seekable")) {
        if (mltWidget->producer()->get_speed() <= 0)
            play();
        else
            mltWidget->producer()->set_speed(mltWidget->producer()->get_speed() * 2);
    }
}
