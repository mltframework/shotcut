/*
 * Copyright (c) 2013-2014 Meltytech, LLC
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
#include "settings.h"
#include "mltproperties.h"

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
    m_quickView.rootContext()->setContextProperty("settings", &ShotcutSettings::singleton());
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
    if (!m_model.tractor()) return;
    if (position <= m_model.tractor()->get_length()) {
        emit seeked(position);
    } else {
        m_position = m_model.tractor()->get_length();
        emit positionChanged();
    }
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
        if (result->is_blank())
            result->set("blank_length", info->frame_count);
        else
            result->set_in_and_out(info->frame_in, info->frame_out);
        delete info;
    }
    return result;
}

int TimelineDock::clipIndexAtPlayhead(int trackIndex)
{
    int result = -1;
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.get_clip_index_at(m_position);
        }
    }
    return result;
}

int TimelineDock::dockYOffset() const
{
    // XXX This is a workaround for menus appearing in wrong location in a Quick
    // view used in a DockWidget on OS X.
#ifdef Q_OS_MAC
    return mapToParent(QPoint(0, 0)).y();
#else
    return 0;
#endif
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
    if (MLT.isSeekableClip() || MLT.savedProducer()) {
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        MAIN.undoStack()->push(
            new Timeline::AppendCommand(m_model, trackIndex,
                MLT.saveXML("string", MLT.isClip()? 0 : MLT.savedProducer())));
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
        // We need to set these special properties so time-based filters
        // can get information about the cut while still applying filters
        // to the cut parent.
        info->producer->set(FilterInProperty, info->frame_in);
        info->producer->set(FilterOutProperty, info->frame_out);
        MAIN.filtersDock()->model()->reset(info->producer);
        MAIN.loadProducerWidget(info->producer);
        delete info;
        emit MAIN.filtersDock()->model()->changed();
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
    } else if (m_model.addTransitionValid(fromTrack, toTrack, clipIndex, position)) {
        MAIN.undoStack()->push(
            new Timeline::AddTransitionCommand(m_model, fromTrack, clipIndex, position));
        return true;
    } else {
        return false;
    }
}

bool TimelineDock::trimClipIn(int trackIndex, int clipIndex, int delta)
{
    if (m_model.addTransitionByTrimInValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
            new Timeline::AddTransitionByTrimInCommand(m_model, trackIndex, clipIndex, delta));
    }
    else if (m_model.trimTransitionOutValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
            new Timeline::TrimTransitionOutCommand(m_model, trackIndex, clipIndex, delta));
    }
    else if (m_model.trimClipInValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
            new Timeline::TrimClipInCommand(m_model, trackIndex, clipIndex, delta));
    }
    else return false;
    return true;
}

bool TimelineDock::trimClipOut(int trackIndex, int clipIndex, int delta)
{
    if (m_model.addTransitionByTrimOutValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
            new Timeline::AddTransitionByTrimOutCommand(m_model, trackIndex, clipIndex, delta));
    }
    else if (m_model.trimTransitionInValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
            new Timeline::TrimTransitionInCommand(m_model, trackIndex, clipIndex, delta));
    }
    else if (m_model.trimClipOutValid(trackIndex, clipIndex, delta)) {
        MAIN.undoStack()->push(
                new Timeline::TrimClipOutCommand(m_model, trackIndex, clipIndex, delta));
    }
    else return false;
    return true;
}

void TimelineDock::insert(int trackIndex, int position, const QString &xml)
{
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.saveXML("string", MLT.isClip()? 0 : MLT.savedProducer());
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::InsertCommand(m_model, trackIndex, position, xmlToUse));
    }
}

void TimelineDock::overwrite(int trackIndex, int position, const QString &xml)
{
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.saveXML("string", MLT.isClip()? 0 : MLT.savedProducer());
        if (trackIndex < 0)
            trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::OverwriteCommand(m_model, trackIndex, position, xmlToUse));
    }
}

void TimelineDock::appendFromPlaylist(Mlt::Playlist *playlist)
{
    int trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    m_model.appendFromPlaylist(playlist, trackIndex);
}

void TimelineDock::splitClip(int trackIndex, int clipIndex)
{
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = clipIndexAtPlayhead(trackIndex);
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && m_position >= info->start && m_position < info->start + info->frame_count - 1) {
        MAIN.undoStack()->push(
            new Timeline::SplitCommand(m_model, trackIndex, clipIndex, m_position));
    }
}

void TimelineDock::fadeIn(int trackIndex, int clipIndex, int duration)
{
    if (duration < 0) return;
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    MAIN.undoStack()->push(
        new Timeline::FadeInCommand(m_model, trackIndex, clipIndex, duration));
    emit fadeInChanged(duration);
}

void TimelineDock::fadeOut(int trackIndex, int clipIndex, int duration)
{
    if (duration < 0) return;
    if (trackIndex < 0)
        trackIndex = m_quickView.rootObject()->property("currentTrack").toInt();
    if (clipIndex < 0)
        clipIndex = m_quickView.rootObject()->property("currentClip").toInt();
    MAIN.undoStack()->push(
        new Timeline::FadeOutCommand(m_model, trackIndex, clipIndex, duration));
    emit fadeOutChanged(duration);
}

void TimelineDock::seekPreviousEdit()
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;

    int newPosition = -1;
    int n = m_model.tractor()->count();
    for (int i = 0; i < n; i++) {
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            int clipIndex = playlist.get_clip_index_at(m_position);
            if (clipIndex >= 0 && m_position == playlist.clip_start(clipIndex))
                --clipIndex;
            while (clipIndex > 0 && playlist.is_blank(clipIndex))
                --clipIndex;
            if (!playlist.is_blank(clipIndex) && clipIndex >= 0)
                newPosition = qMax(newPosition, playlist.clip_start(clipIndex));
        }
    }
    if (newPosition != m_position)
        setPosition(newPosition);
}

void TimelineDock::seekNextEdit()
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;

    int newPosition = std::numeric_limits<int>::max();
    int n = m_model.tractor()->count();
    for (int i = 0; i < n; i++) {
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            int clipIndex = playlist.get_clip_index_at(m_position) + 1;
            while (clipIndex < playlist.count() && playlist.is_blank(clipIndex))
                ++clipIndex;
            if (!playlist.is_blank(clipIndex) && clipIndex < playlist.count())
                newPosition = qMin(newPosition, playlist.clip_start(clipIndex));
            else if (clipIndex == playlist.count())
                newPosition = playlist.get_playtime();
        }
    }
    if (newPosition != m_position)
        setPosition(newPosition);
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
            emit dropAccepted(QString::fromUtf8(event->mimeData()->data(Mlt::XmlMimeType)));
            event->acceptProposedAction();
        }
    }
    emit dropped();
}

bool TimelineDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        onVisibilityChanged(true);
    return result;
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

