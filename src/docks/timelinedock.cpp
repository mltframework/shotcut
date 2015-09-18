/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
#include "models/multitrackmodel.h"
#include "qmltypes/thumbnailprovider.h"
#include "mainwindow.h"
#include "commands/timelinecommands.h"
#include "qmltypes/qmlutilities.h"
#include "shotcut_mlt_properties.h"

#include <QtQml>
#include <QtQuick>
#include <QDebug>


TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::TimelineDock),
    m_quickView(QmlUtilities::sharedEngine(), this),
    m_position(-1)
{
    qDebug() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());

    qmlRegisterType<MultitrackModel>("Shotcut.Models", 1, 0, "MultitrackModel");

    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    m_quickView.engine()->addImportPath(importPath.path());
    m_quickView.engine()->addImageProvider(QString("thumbnail"), new ThumbnailProvider);
    QmlUtilities::setCommonProperties(&m_quickView);
    m_quickView.rootContext()->setContextProperty("timeline", this);
    m_quickView.rootContext()->setContextProperty("multitrack", &m_model);
    m_quickView.setResizeMode(QQuickView::SizeRootObjectToView);
    m_quickView.setColor(palette().window().color());

    connect(&m_model, &MultitrackModel::modified, this, &TimelineDock::clearSelectionIfInvalid);

    QWidget* container = QWidget::createWindowContainer(&m_quickView, this);
    container->setFocusPolicy(Qt::TabFocus);
    delete ui->scrollAreaWidgetContents;
    ui->scrollArea->setWidget(container);

    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onShowFrame(const SharedFrame&)));
#ifdef Q_OS_WIN
    onVisibilityChanged(true);
#else
    connect(this, &QDockWidget::visibilityChanged, this, &TimelineDock::onVisibilityChanged);
#endif
    qDebug() << "end";
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

Mlt::Producer *TimelineDock::producerForClip(int trackIndex, int clipIndex)
{
    Mlt::Producer* result = 0;
    Mlt::ClipInfo* info = getClipInfo(trackIndex, clipIndex);
    if (info) {
        result = new Mlt::Producer(info->producer);
        delete info;
    }
    return result;
}

int TimelineDock::clipIndexAtPlayhead(int trackIndex)
{
    return clipIndexAtPosition(trackIndex, m_position);
}

int TimelineDock::clipIndexAtPosition(int trackIndex, int position)
{
    int result = -1;
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            result = playlist.get_clip_index_at(position);
        }
    }
    return result;
}

bool TimelineDock::isBlank(int trackIndex, int clipIndex)
{
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    return m_model.index(clipIndex, 0, m_model.index(trackIndex))
        .data(MultitrackModel::IsBlankRole).toBool();
}

void TimelineDock::pulseLockButtonOnTrack(int trackIndex)
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "pulseLockButtonOnTrack",
            Qt::DirectConnection, Q_ARG(QVariant, trackIndex));
}

void TimelineDock::chooseClipAtPosition(int position, int * trackIndex, int * clipIndex)
{
    QScopedPointer<Mlt::Producer> clip;

    // Start by checking for a hit at the specified track
    if (*trackIndex != -1 && !isTrackLocked(*trackIndex)) {
        *clipIndex = clipIndexAtPosition(*trackIndex, position);
        if (*clipIndex != -1 && !isBlank(*trackIndex, *clipIndex))
            return;
    }

    // Next we try the current track
    *trackIndex = currentTrack();
    *clipIndex = clipIndexAtPosition(*trackIndex, position);

    if (!isTrackLocked(*trackIndex) && *clipIndex != -1 && !isBlank(*trackIndex, *clipIndex)) {
        return;
    }

    // if there was no hit, look through the other tracks
    for (*trackIndex = 0; *trackIndex < m_model.trackList().size(); (*trackIndex)++) {
        if (*trackIndex == currentTrack())
            continue;
        if (isTrackLocked(*trackIndex))
            continue;
        *clipIndex = clipIndexAtPosition(*trackIndex, position);
        if (*clipIndex != -1 && !isBlank(*trackIndex, *clipIndex))
            return;
    }
    *trackIndex = -1;
    *clipIndex = -1;
}

int TimelineDock::clipCount(int trackIndex) const
{
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (trackIndex >= 0 && trackIndex < m_model.trackList().size()) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            return playlist.count();
        }
    }
    return 0;
}

int TimelineDock::dockYOffset() const
{
    // XXX This is a workaround for menus appearing in wrong location in a Quick
    // view used in a DockWidget.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
#  if defined(Q_OS_MAC)
    return 0;
#  else
    return mapToParent(QPoint(0, 0)).y();
#  endif
#else
#  if defined(Q_OS_MAC)
    return mapToParent(QPoint(0, 0)).y();
#  else
    return 0;
#  endif
#endif

}

void TimelineDock::setCurrentTrack(int currentTrack)
{
    if (!m_quickView.rootObject())
        return;
    m_quickView.rootObject()->setProperty("currentTrack", currentTrack);
}

int TimelineDock::currentTrack() const
{
    if (!m_quickView.rootObject())
        return 0;
    return m_quickView.rootObject()->property("currentTrack").toInt();
}

void TimelineDock::zoomIn()
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "zoomIn");
}

void TimelineDock::zoomOut()
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "zoomOut");
}

void TimelineDock::resetZoom()
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "resetZoom");
}

void TimelineDock::makeTracksShorter()
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "makeTracksShorter");
}

void TimelineDock::makeTracksTaller()
{
    QMetaObject::invokeMethod(m_quickView.rootObject(), "makeTracksTaller");
}

void TimelineDock::clearSelection()
{
    setSelection(QVariantList());
}

void TimelineDock::setSelection(const QVariantList & newSelection)
{
    qDebug() << "Setting selection to" << newSelection;
    if (newSelection == selection())
        return;

    m_quickView.rootObject()->setProperty("selection", newSelection);
}

void TimelineDock::setSelection(int trackIndex, int clipIndex)
{
    setSelection(QList<ClipIndex>() << ClipIndex(trackIndex, clipIndex));
}

void TimelineDock::setSelection(const ClipIndex& selection)
{
    setSelection(QList<ClipIndex>() << selection);
}

void TimelineDock::setSelection(const QList<ClipIndex> & newSelection)
{
    setSelection(reinterpret_cast<const QVariantList&>(newSelection));
}

QVariantList TimelineDock::selection() const
{
    if (!m_quickView.rootObject())
        return QVariantList();

    return m_quickView.rootObject()->property("selection").toList();
}

struct ClipIndexSorter {
    bool operator()(const QVariant& a, const QVariant& b) {
        ClipIndex clipA(a);
        ClipIndex clipB(b);
        if (clipA.trackIndex() == clipB.trackIndex())
            return clipA.clipIndex() < clipB.clipIndex();
        else
            return clipA.trackIndex() < clipB.trackIndex();
    }
};

QVariantList TimelineDock::sortedSelection() const
{
    QVariantList sorted = selection();
    std::sort(sorted.begin(), sorted.end(), ClipIndexSorter());
    return sorted;
}

void TimelineDock::selectClipUnderPlayhead()
{
    int track = -1, clip = -1;
    chooseClipAtPosition(m_position, &track, &clip);
    if (clip == -1) {
        if (isTrackLocked(currentTrack())) {
            pulseLockButtonOnTrack(currentTrack());
            return;
        }
        int idx = clipIndexAtPlayhead(-1);
        if (idx == -1)
            clearSelection();
        else
            setSelection(currentTrack(), idx);
        return;
    }

    setCurrentTrack(track);
    setSelection(currentTrack(), clip);
}

int TimelineDock::centerOfClip(int trackIndex, int clipIndex)
{
    Mlt::ClipInfo * clip = getClipInfo(trackIndex, clipIndex);
    Q_ASSERT(clip);
    int centerOfClip = clip->start + clip->frame_count / 2;
    delete clip;
    clip = 0;
    return centerOfClip;
}

bool TimelineDock::isTrackLocked(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= m_model.trackList().size())
        return false;
    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    return track->get_int(kTrackLockProperty);
}

void TimelineDock::clearSelectionIfInvalid()
{
    QVariantList newSelection;
    foreach (QVariant clip, selection()) {
        ClipIndex clipMap(clip);
        if (clipMap.clipIndex() >= clipCount(clipMap.trackIndex()))
            continue;

        newSelection << clip;
    }
    setSelection(newSelection);
}

void TimelineDock::insertTrack()
{
    MAIN.undoStack()->push(
                new Timeline::InsertTrackCommand(m_model, currentTrack()));
}

void TimelineDock::removeTrack()
{
    if (m_model.trackList().size() > 0)
        MAIN.undoStack()->push(
                new Timeline::RemoveTrackCommand(m_model, currentTrack()));
}

void TimelineDock::addAudioTrack()
{
    MAIN.undoStack()->push(
        new Timeline::AddTrackCommand(m_model, false));
}

void TimelineDock::addVideoTrack()
{
    MAIN.undoStack()->push(
        new Timeline::AddTrackCommand(m_model, true));
}

void TimelineDock::close()
{
    if (MAIN.continueModified())
        m_model.close();
}

void TimelineDock::onShowFrame(const SharedFrame& frame)
{
    if (MLT.isMultitrack()) {
        m_position = frame.get_position();
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
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    if (MLT.isSeekableClip() || MLT.savedProducer()) {
        MAIN.undoStack()->push(
            new Timeline::AppendCommand(m_model, trackIndex,
                MLT.XML(MLT.isClip()? 0 : MLT.savedProducer())));
    }
}

void TimelineDock::remove(int trackIndex, int clipIndex)
{
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    Mlt::Producer* clip = producerForClip(trackIndex, clipIndex);
    if (clip) {
        QString xml = MLT.XML(clip);
        delete clip;
        MAIN.undoStack()->push(
            new Timeline::RemoveCommand(m_model, trackIndex, clipIndex, xml));
    }
}

void TimelineDock::lift(int trackIndex, int clipIndex)
{
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    QScopedPointer<Mlt::Producer> clip(producerForClip(trackIndex, clipIndex));
    if (clip) {
        if (clip->is_blank())
            return;
        QString xml = MLT.XML(clip.data());
        MAIN.undoStack()->push(
            new Timeline::LiftCommand(m_model, trackIndex, clipIndex, xml));
    }
}

void TimelineDock::removeSelection()
{
    if (isTrackLocked(currentTrack())) {
        pulseLockButtonOnTrack(currentTrack());
        return;
    }
    if (selection().isEmpty())
        selectClipUnderPlayhead();
    if (selection().isEmpty())
        return;
    QVariantList sorted = sortedSelection();
    for (int i = sorted.size() - 1; i >= 0; --i)
    {
        ClipIndex clip(sorted[i]);
        remove(clip.trackIndex(), clip.clipIndex());
    }
}

void TimelineDock::liftSelection()
{
    if (isTrackLocked(currentTrack())) {
        pulseLockButtonOnTrack(currentTrack());
        return;
    }
    if (selection().isEmpty())
        selectClipUnderPlayhead();
    if (selection().isEmpty())
        return;
    QVariantList sorted = sortedSelection();
    for (int i = sorted.size() - 1; i >= 0; --i)
    {
        ClipIndex clip(sorted[i]);
        lift(clip.trackIndex(), clip.clipIndex());
    }
}

void TimelineDock::selectTrack(int by)
{
    int newTrack = currentTrack();
    if (by < 0)
        newTrack = qMax(0, newTrack + by);
    else
        newTrack = qMin(m_model.trackList().size() - 1, newTrack + by);
    setCurrentTrack(newTrack);
}

void TimelineDock::selectTrackHead(int trackIndex)
{
    if (trackIndex >= 0) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        Mlt::Producer* producer = m_model.tractor()->track(i);
        if (producer && producer->is_valid())
            emit trackSelected(producer);
        delete producer;
    }
}

void TimelineDock::openClip(int trackIndex, int clipIndex)
{
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info) {
        QString xml = MLT.XML(info->producer);
        Mlt::Producer* p = new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
        p->set_in_and_out(info->frame_in, info->frame_out);
        emit clipOpened(p);
    }
}

void TimelineDock::emitClipSelectedFromSelection()
{
    if (selection().isEmpty() || currentTrack() == -1) {
        emit clipSelected(0);
        return;
    }

    ClipIndex clip(selection().first());
    Mlt::ClipInfo* info = getClipInfo(clip.trackIndex(), clip.clipIndex());
    if (info && info->producer && info->producer->is_valid()) {
        // We need to set these special properties so time-based filters
        // can get information about the cut while still applying filters
        // to the cut parent.
        info->producer->set(kFilterInProperty, info->frame_in);
        info->producer->set(kFilterOutProperty, info->frame_out);
        emit clipSelected(info->producer);
        MAIN.loadProducerWidget(info->producer);
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

void TimelineDock::setTrackLock(int trackIndex, bool lock)
{
    MAIN.undoStack()->push(
        new Timeline::LockTrackCommand(m_model, trackIndex, lock));
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
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.XML(MLT.isClip()? 0 : MLT.savedProducer());
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::InsertCommand(m_model, trackIndex, position, xmlToUse));
    }
}

void TimelineDock::overwrite(int trackIndex, int position, const QString &xml)
{
    if (trackIndex < 0)
        trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.XML(MLT.isClip()? 0 : MLT.savedProducer());
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::OverwriteCommand(m_model, trackIndex, position, xmlToUse));
    }
}

void TimelineDock::appendFromPlaylist(Mlt::Playlist *playlist)
{
    int trackIndex = currentTrack();
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    m_model.appendFromPlaylist(playlist, trackIndex);
}

void TimelineDock::splitClip(int trackIndex, int clipIndex)
{
    if (trackIndex < 0 || clipIndex < 0)
        chooseClipAtPosition(m_position, &trackIndex, &clipIndex);
    if (trackIndex < 0 || clipIndex < 0)
        return;
    setCurrentTrack(trackIndex);
    if (clipIndex >= 0 && trackIndex >= 0) {
        int i = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            if (!m_model.isTransition(playlist, clipIndex)) {
                QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
                if (info && m_position >= info->start && m_position < info->start + info->frame_count - 1) {
                    MAIN.undoStack()->push(
                        new Timeline::SplitCommand(m_model, trackIndex, clipIndex, m_position));
                }
            } else {
                MAIN.showStatusMessage(tr("You cannot split a transition."));
            }
        }
    }
}

void TimelineDock::fadeIn(int trackIndex, int clipIndex, int duration)
{
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    if (duration < 0) return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    MAIN.undoStack()->push(
        new Timeline::FadeInCommand(m_model, trackIndex, clipIndex, duration));
    emit fadeInChanged(duration);
}

void TimelineDock::fadeOut(int trackIndex, int clipIndex, int duration)
{
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    if (duration < 0) return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
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
            if (clipIndex >= 0)
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
            if (clipIndex < playlist.count())
                newPosition = qMin(newPosition, playlist.clip_start(clipIndex));
            else if (clipIndex == playlist.count())
                newPosition = qMin(newPosition, playlist.clip_start(clipIndex) + playlist.clip_length(clipIndex));
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
        int trackIndex = currentTrack();
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
        disconnect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(onVisibilityChanged(bool)));
        connect(m_quickView.rootObject(), SIGNAL(currentTrackChanged()),
                this, SIGNAL(currentTrackChanged()));
        connect(m_quickView.rootObject(), SIGNAL(selectionChanged()),
                this, SIGNAL(selectionChanged()));
        connect(m_quickView.rootObject(), SIGNAL(selectionChanged()),
                this, SLOT(emitClipSelectedFromSelection()));
        connect(m_quickView.rootObject(), SIGNAL(clipClicked()),
                this, SIGNAL(clipClicked()));
    }
}

