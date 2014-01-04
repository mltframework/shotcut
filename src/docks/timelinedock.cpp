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
#include "commands/timelinecommands.h"
#include "docks/filtersdock.h"

#include <QtQml>
#include <QtQuick>

TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::TimelineDock),
    m_position(-1)
{
    ui->setupUi(this);
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

    QWidget* container = QWidget::createWindowContainer(&m_quickView, this);
    container->setFocusPolicy(Qt::TabFocus);
    delete ui->scrollAreaWidgetContents;
    ui->scrollArea->setWidget(container);

    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onShowFrame(Mlt::QFrame)));
#ifdef Q_OS_WIN
    onVisibilityChanged(true);
#else
    connect(this, &QDockWidget::visibilityChanged, this, &TimelineDock::onVisibilityChanged);
#endif
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

Mlt::ClipInfo *TimelineDock::getClipInfo(int trackIndex, int clipIndex)
{
    Mlt::ClipInfo* result = 0;
    if (clipIndex >= 0 && trackIndex >= 0) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.clip_info(clipIndex);
        }
    }
    return result;
}

Mlt::Producer *TimelineDock::getClip(int trackIndex, int clipIndex)
{
    Mlt::Producer* result = 0;
    Mlt::ClipInfo* info = getClipInfo(trackIndex, clipIndex);
    if (info) {
        result = new Mlt::Producer(info->producer);
        result->set_in_and_out(info->frame_in, info->frame_out);
        delete info;
    }
    return result;
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
        MAIN.undoStack()->push(
            new Timeline::AppendCommand(m_model, trackIndex, MLT.saveXML("string")));
    }
}

void TimelineDock::remove(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    Mlt::Producer* clip = getClip(trackIndex, clipIndex);
    if (clip) {
        QString xml = MLT.saveXML("string", clip);
        delete clip;
        QModelIndex idx = m_model.index(clipIndex, 0, m_model.index(trackIndex));
        int position = m_model.data(idx, MultitrackModel::StartRole).toInt();
        MAIN.undoStack()->push(
            new Timeline::RemoveCommand(m_model, trackIndex, clipIndex, position, xml));
    }
}

void TimelineDock::lift(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    Mlt::Producer* clip = getClip(trackIndex, clipIndex);
    if (clip) {
        QString xml = MLT.saveXML("string", clip);
        delete clip;
        QModelIndex idx = m_model.index(clipIndex, 0, m_model.index(trackIndex));
        int position = m_model.data(idx, MultitrackModel::StartRole).toInt();
        MAIN.undoStack()->push(
            new Timeline::LiftCommand(m_model, trackIndex, clipIndex, position, xml));
    }
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

void TimelineDock::openClip(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info) {
        QString xml = MLT.saveXML("string", info->producer);
        Mlt::Producer* p = new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
        emit clipOpened(p, info->frame_in, info->frame_out);
    }
}

void TimelineDock::selectClip(int trackIndex, int clipIndex)
{
    Mlt::ClipInfo* info = getClipInfo(trackIndex, clipIndex);
    if (info && info->producer && info->producer->is_valid()) {
        MAIN.filtersDock()->model()->reset(info->producer);
        delete info;
    }
}

void TimelineDock::setTrackName(int trackIndex, const QString &value)
{
    MAIN.undoStack()->push(
        new Timeline::NameTrackCommand(m_model, trackIndex, value));
}

void TimelineDock::toggleTrackMute(int trackIndex)
{
    MAIN.undoStack()->push(
        new Timeline::MuteTrackCommand(m_model, trackIndex));
}

void TimelineDock::toggleTrackHidden(int trackIndex)
{
    MAIN.undoStack()->push(
        new Timeline::HideTrackCommand(m_model, trackIndex));
}

void TimelineDock::setTrackComposite(int trackIndex, Qt::CheckState composite)
{
    MAIN.undoStack()->push(
        new Timeline::CompositeTrackCommand(m_model, trackIndex, composite));
}

bool TimelineDock::moveClip(int fromTrack, int toTrack, int clipIndex, int position)
{
    if (m_model.moveClipValid(fromTrack, toTrack, clipIndex, position)) {
        MAIN.undoStack()->push(
            new Timeline::MoveClipCommand(m_model, fromTrack, toTrack, clipIndex, position));
        return true;
    } else {
        return false;
    }
}

void TimelineDock::trimClipIn(int trackIndex, int clipIndex, int delta)
{
    MAIN.undoStack()->push(
        new Timeline::TrimClipInCommand(m_model, trackIndex, clipIndex, delta));
}

void TimelineDock::trimClipOut(int trackIndex, int clipIndex, int delta)
{
    MAIN.undoStack()->push(
                new Timeline::TrimClipOutCommand(m_model, trackIndex, clipIndex, delta));
}

void TimelineDock::insert(int trackIndex)
{
    if (MLT.isSeekableClip()) {
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        MAIN.undoStack()->push(
            new Timeline::InsertCommand(m_model, trackIndex, m_position, MLT.saveXML("string")));
    }
}

void TimelineDock::overwrite(int trackIndex)
{
    if (MLT.isSeekableClip()) {
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        MAIN.undoStack()->push(
            new Timeline::OverwriteCommand(m_model, trackIndex, m_position, MLT.saveXML("string")));
    }
}

void TimelineDock::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(Mlt::XmlMimeType)) {
        event->acceptProposedAction();
    }
}

void TimelineDock::dragMoveEvent(QDragMoveEvent *event)
{
    emit dragging(event->posF(), event->mimeData()->text().toInt());
}

void TimelineDock::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    emit dropped();
}

void TimelineDock::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(Mlt::XmlMimeType)) {
        int trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        if (trackIndex >= 0) {
            MAIN.undoStack()->push(new Timeline::AppendCommand(m_model, trackIndex, event->mimeData()->data(Mlt::XmlMimeType)));
            event->acceptProposedAction();
        }
    }
    emit dropped();
}

void TimelineDock::onVisibilityChanged(bool visible)
{
    if (visible) {
        QDir sourcePath = QmlUtilities::qmlDir();
        sourcePath.cd("timeline");
        m_quickView.setSource(QUrl::fromLocalFile(sourcePath.filePath("timeline.qml")));
        disconnect(this, &QDockWidget::visibilityChanged, this, &TimelineDock::onVisibilityChanged);
    }
}

