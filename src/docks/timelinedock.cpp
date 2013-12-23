/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "timelinedock.h"
#include "ui_timelinedock.h"
#include "qmltypes/qmlutilities.h"
#include "models/multitrackmodel.h"
#include "qmltypes/thumbnailprovider.h"
#include "mainwindow.h"

#include <QtQml>
#include <QtQuick>

TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::TimelineDock),
    m_position(-1)
{
    ui->setupUi(this);
    delete ui->treeView;
    toggleViewAction()->setIcon(windowIcon());

    qmlRegisterType<MultitrackModel>("Shotcut.Models", 1, 0, "MultitrackModel");

    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    m_quickView.engine()->addImportPath(importPath.path());
    m_quickView.engine()->addImageProvider(QString("thumbnail"), new ThumbnailProvider);
    m_quickView.rootContext()->setContextProperty("timeline", this);
    m_quickView.rootContext()->setContextProperty("multitrack", &m_model);
    m_quickView.setResizeMode(QQuickView::SizeRootObjectToView);
    m_quickView.setColor(palette().window().color());

    importPath = QmlUtilities::qmlDir();
    importPath.cd("timeline");
    m_quickView.setSource(QUrl::fromLocalFile(importPath.filePath("timeline.qml")));
    QWidget* container = QWidget::createWindowContainer(&m_quickView);
    container->setFocusPolicy(Qt::TabFocus);

    ui->verticalLayout->addWidget(container);
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onShowFrame(Mlt::QFrame)));
}

TimelineDock::~TimelineDock()
{
    delete ui;
}

void TimelineDock::setPosition(int position)
{
    emit seeked(position);
}

QString TimelineDock::timecode(int frames)
{
    return MLT.producer()->frames_to_time(frames, mlt_time_smpte);
}

void TimelineDock::addAudioTrack()
{
    m_model.addAudioTrack();
}

void TimelineDock::addVideoTrack()
{
    m_model.addVideoTrack();
}

void TimelineDock::close()
{
    m_model.close();
    hide();
}

void TimelineDock::onShowFrame(Mlt::QFrame frame)
{
    if (MLT.isMultitrack()) {
        m_position = frame.frame()->get_position();
        emit positionChanged();
    }
}

void TimelineDock::onSeeked(int position)
{
    if (MLT.isMultitrack()) {
        m_position = position;
        emit positionChanged();
    }
}

void TimelineDock::append(int trackIndex)
{
    if (MLT.isSeekableClip()) {
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        m_model.appendClip(trackIndex, *MLT.producer());
    }
}

void TimelineDock::remove(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    if (clipIndex >= 0 && trackIndex >= 0)
        m_model.removeClip(trackIndex, clipIndex);
}

void TimelineDock::lift(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    if (clipIndex >= 0 && trackIndex >= 0)
        m_model.liftClip(trackIndex, clipIndex);
}

void TimelineDock::pressKey(int key, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    MAIN.keyPressEvent(&event);
}

void TimelineDock::releaseKey(int key, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent event(QEvent::KeyRelease, key, modifiers);
    MAIN.keyReleaseEvent(&event);
}

void TimelineDock::selectTrack(int by)
{
    int currentTrack = m_quickView.rootObject()->property("currentTrack").toInt();
    if (by < 0)
        currentTrack = qMax(0, currentTrack + by);
    else
        currentTrack = qMin(m_model.trackList().size() - 1, currentTrack + by);
    m_quickView.rootObject()->setProperty("currentTrack", currentTrack);
}
