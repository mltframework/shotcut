/*
 * Copyright (c) 2013-2018 Meltytech, LLC
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
#include "models/audiolevelstask.h"
#include "models/multitrackmodel.h"
#include "qmltypes/thumbnailprovider.h"
#include "mainwindow.h"
#include "commands/timelinecommands.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "shotcut_mlt_properties.h"
#include "settings.h"
#include "util.h"

#include <QAction>
#include <QtQml>
#include <QtQuick>
#include <Logger.h>


TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::TimelineDock),
    m_quickView(QmlUtilities::sharedEngine(), this),
    m_position(-1),
    m_updateCommand(0),
    m_ignoreNextPositionChange(false),
    m_trimDelta(0),
    m_transitionDelta(0)
{
    LOG_DEBUG() << "begin";
    m_selection.selectedTrack = -1;
    m_selection.isMultitrackSelected = false;

    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());

    qmlRegisterType<MultitrackModel>("Shotcut.Models", 1, 0, "MultitrackModel");

    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    m_quickView.engine()->addImportPath(importPath.path());
    m_quickView.engine()->addImageProvider(QString("thumbnail"), new ThumbnailProvider);
    QmlUtilities::setCommonProperties(m_quickView.rootContext());
    m_quickView.rootContext()->setContextProperty("view", new QmlView(&m_quickView));
    m_quickView.rootContext()->setContextProperty("timeline", this);
    m_quickView.rootContext()->setContextProperty("multitrack", &m_model);
    m_quickView.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickView.setClearColor(palette().window().color());

    connect(&m_model, SIGNAL(modified()), this, SLOT(clearSelectionIfInvalid()));
    connect(&m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(onRowsInserted(QModelIndex,int,int)));
    connect(&m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(onRowsRemoved(QModelIndex,int,int)));

    setWidget(&m_quickView);

    connect(this, SIGNAL(clipMoved(int,int,int,int,bool)), SLOT(onClipMoved(int,int,int,int,bool)), Qt::QueuedConnection);
    connect(this, SIGNAL(transitionAdded(int,int,int,bool)), SLOT(onTransitionAdded(int,int,int,bool)), Qt::QueuedConnection);
    connect(MLT.videoWidget(), SIGNAL(frameDisplayed(const SharedFrame&)), this, SLOT(onShowFrame(const SharedFrame&)));
    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(load()));
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(onTopLevelChanged(bool)));
    LOG_DEBUG() << "end";
}

TimelineDock::~TimelineDock()
{
    delete m_updateCommand;
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
    emit showStatusMessage(tr("This track is locked"));
}

void TimelineDock::chooseClipAtPosition(int position, int& trackIndex, int& clipIndex)
{
    QScopedPointer<Mlt::Producer> clip;

    // Start by checking for a hit at the specified track
    if (trackIndex != -1 && !isTrackLocked(trackIndex)) {
        clipIndex = clipIndexAtPosition(trackIndex, position);
        if (clipIndex != -1 && !isBlank(trackIndex, clipIndex))
            return;
    }

    // Next we try the current track
    trackIndex = currentTrack();
    clipIndex = qMin(clipIndexAtPosition(trackIndex, position), clipCount(trackIndex) - 1);

    if (!isTrackLocked(trackIndex) && clipIndex != -1 && !isBlank(trackIndex, clipIndex)) {
        return;
    }

    // if there was no hit, look through the other tracks
    for (trackIndex = 0; trackIndex < m_model.trackList().size(); (trackIndex)++) {
        if (trackIndex == currentTrack())
            continue;
        if (isTrackLocked(trackIndex))
            continue;
        clipIndex = clipIndexAtPosition(trackIndex, position);
        if (clipIndex != -1 && !isBlank(trackIndex, clipIndex))
            return;
    }
    trackIndex = -1;
    clipIndex = -1;
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

void TimelineDock::setCurrentTrack(int currentTrack)
{
    if (!m_quickView.rootObject())
        return;
    m_quickView.rootObject()->setProperty("currentTrack", qBound(0, currentTrack, m_model.trackList().size() - 1));
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

void TimelineDock::setSelection(QList<int> newSelection, int trackIndex, bool isMultitrack)
{
    if (newSelection != selection()
            || trackIndex != m_selection.selectedTrack
            || isMultitrack != m_selection.isMultitrackSelected) {
        LOG_DEBUG() << "Changing selection to" << newSelection << " trackIndex" << trackIndex << "isMultitrack" << isMultitrack;
        m_selection.selectedClips = newSelection;
        m_selection.selectedTrack = trackIndex;
        m_selection.isMultitrackSelected = isMultitrack;
        emit selectionChanged();

        if (!m_selection.selectedClips.isEmpty())
            emitSelectedFromSelection();
        else
            emit selected(0);
    }
}

QList<int> TimelineDock::selection() const
{
    if (!m_quickView.rootObject())
        return QList<int>();
    return m_selection.selectedClips;
}

void TimelineDock::saveAndClearSelection()
{
    m_savedSelection = m_selection;
    m_selection.selectedClips = QList<int>();
    m_selection.selectedTrack = -1;
    m_selection.isMultitrackSelected = false;
    emit selectionChanged();
}

void TimelineDock::restoreSelection()
{
    m_selection = m_savedSelection;
    emit selectionChanged();
    emitSelectedFromSelection();
}

void TimelineDock::selectClipUnderPlayhead()
{
    int track = -1, clip = -1;
    chooseClipAtPosition(m_position, track, clip);
    if (clip == -1) {
        if (isTrackLocked(currentTrack())) {
            pulseLockButtonOnTrack(currentTrack());
            return;
        }
        int idx = clipIndexAtPlayhead(-1);
        if (idx == -1)
            setSelection();
        else
            setSelection(QList<int>() << idx);
        return;
    }

    setCurrentTrack(track);
    setSelection(QList<int>() << clip);
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

void TimelineDock::trimClipAtPlayhead(TrimLocation location, bool ripple)
{
    int trackIndex = currentTrack(), clipIndex = -1;
    chooseClipAtPosition(m_position, trackIndex, clipIndex);
    if (trackIndex < 0 || clipIndex < 0)
        return;
    setCurrentTrack(trackIndex);

    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    if (!track)
        return;

    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (!info)
        return;

    if (location == TrimInPoint) {
        MAIN.undoStack()->push(
            new Timeline::TrimClipInCommand(m_model, trackIndex, clipIndex, m_position - info->start, ripple));
        if (ripple)
            setPosition(info->start);
    } else {
        MAIN.undoStack()->push(
            new Timeline::TrimClipOutCommand(m_model, trackIndex, clipIndex, info->start + info->frame_count - m_position, ripple));
    }
}

bool TimelineDock::isRipple() const
{
    return m_quickView.rootObject()->property("ripple").toBool();
}

void TimelineDock::copyToSource()
{
    if (model()->tractor() && model()->tractor()->is_valid()) {
        if (MAIN.on_actionSave_triggered()) {
            if (!MLT.openXML(MAIN.fileName())) {
                MLT.producer()->set(kExportFromProperty, 1);
                MAIN.open(MLT.producer());
            } else {
                emit showStatusMessage(tr("Failed to open ") + MAIN.fileName());
            }
        } else {
            emit showStatusMessage(tr("You must save to Copy Timeline to Source."));
        }
    }
}

void TimelineDock::openProperties()
{
    MAIN.onPropertiesDockTriggered(true);
}

void TimelineDock::clearSelectionIfInvalid()
{
    int count = clipCount(currentTrack());

    QList<int> newSelection;
    foreach (int index, selection()) {
        if (index >= count)
            continue;

        newSelection << index;
    }
    setSelection(newSelection);
    emit selectionChanged();
}

void TimelineDock::insertTrack()
{
    MAIN.undoStack()->push(
                new Timeline::InsertTrackCommand(m_model, currentTrack()));
}

void TimelineDock::removeTrack()
{
    if (m_model.trackList().size() > 0) {
        int trackIndex = currentTrack();
        MAIN.undoStack()->push(
                new Timeline::RemoveTrackCommand(m_model, trackIndex));
        if (trackIndex >= m_model.trackList().count())
            setCurrentTrack(m_model.trackList().count() - 1);
    }
}

bool TimelineDock::mergeClipWithNext(int trackIndex, int clipIndex, bool dryrun)
{
    if (dryrun)
        return m_model.mergeClipWithNext(trackIndex, clipIndex, true);

    MAIN.undoStack()->push(
        new Timeline::MergeCommand(m_model, trackIndex, clipIndex));

    return true;
}

void TimelineDock::onProducerChanged(Mlt::Producer* after)
{
    int trackIndex = currentTrack();
    if (trackIndex < 0 || selection().isEmpty() || !m_updateCommand || !after || !after->is_valid())
        return;
    if (isTrackLocked(trackIndex)) {
        pulseLockButtonOnTrack(trackIndex);
        return;
    }
    int i = m_model.trackList().at(trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
    if (track) {
        // Ensure the new XML has same in/out point as selected clip by making
        // a copy of the changed producer and copying the in/out from timeline.
        Mlt::Playlist playlist(*track);
        int clipIndex = selection().first();
        QScopedPointer<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
        if (info) {
            double oldSpeed = qstrcmp("timewarp", info->producer->get("mlt_service")) ? 1.0 : info->producer->get_double("warp_speed");
            double newSpeed = qstrcmp("timewarp", after->get("mlt_service")) ? 1.0 : after->get_double("warp_speed");
            double speedRatio = oldSpeed / newSpeed;

            int length = qRound(info->length * speedRatio);
            int in = qMin(qRound(info->frame_in * speedRatio), length - 1);
            int out = qMin(qRound(info->frame_out * speedRatio), length - 1);
            after->set("length", length);
            after->set_in_and_out(in, out);

            // Adjust filters.
            int n = after->filter_count();
            for (int j = 0; j < n; j++) {
                QScopedPointer<Mlt::Filter> filter(after->filter(j));
                if (filter && filter->is_valid() && !filter->get_int("_loader")) {
                    in = qMin(qRound(filter->get_in() * speedRatio), length - 1);
                    out = qMin(qRound(filter->get_out() * speedRatio), length - 1);
                    filter->set_in_and_out(in, out);
                    //TODO: keyframes
                }
            }
        }
    }
    QString xmlAfter = MLT.XML(after);
    m_updateCommand->setXmlAfter(xmlAfter);
    setSelection(); // clearing selection prevents a crash
    Timeline::UpdateCommand* command = m_updateCommand;
    m_updateCommand = 0;
    MAIN.undoStack()->push(command);
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

void TimelineDock::onShowFrame(const SharedFrame& frame)
{
    if (m_ignoreNextPositionChange) {
        m_ignoreNextPositionChange = false;
    } else if (MLT.isMultitrack()) {
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
    if (MAIN.isSourceClipMyProject()) return;
    if (MLT.isSeekableClip() || MLT.savedProducer()) {
        MAIN.undoStack()->push(
            new Timeline::AppendCommand(m_model, trackIndex,
                MLT.XML(MLT.isClip()? 0 : MLT.savedProducer())));
        selectClipUnderPlayhead();
    }
}

void TimelineDock::remove(int trackIndex, int clipIndex)
{
    if (!m_model.trackList().count())
        return;
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
    if (!m_model.trackList().count())
        return;
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
        setSelection();
    }
}

void TimelineDock::removeSelection(bool withCopy)
{
    if (isTrackLocked(currentTrack())) {
        pulseLockButtonOnTrack(currentTrack());
        return;
    }
    if (selection().isEmpty())
        selectClipUnderPlayhead();
    if (selection().isEmpty() || currentTrack() < 0)
        return;

    if (withCopy)
        copyClip(currentTrack(), selection().first());
    foreach (int index, selection())
        remove(currentTrack(), index);
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
    foreach (int index, selection())
        lift(currentTrack(), index);
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
        setSelection(QList<int>(), trackIndex);
        int i = m_model.trackList().at(trackIndex).mlt_index;
        Mlt::Producer* producer = m_model.tractor()->track(i);
        if (producer && producer->is_valid())
            emit selected(producer);
        delete producer;
    }
}

void TimelineDock::selectMultitrack()
{
    setSelection(QList<int>(), -1, true);
    QMetaObject::invokeMethod(m_quickView.rootObject(), "selectMultitrack");
    emit selected(m_model.tractor());
}

void TimelineDock::copyClip(int trackIndex, int clipIndex)
{
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info) {
        QString xml = MLT.XML(info->producer);
        Mlt::Producer p(MLT.profile(), "xml-string", xml.toUtf8().constData());
        p.set_speed(0);
        p.seek(info->frame_in);
        p.set_in_and_out(info->frame_in, info->frame_out);
        MLT.setSavedProducer(&p);
        emit clipCopied();
    }
}

void TimelineDock::emitSelectedFromSelection()
{
    if (!m_model.trackList().count()) {
        if (m_model.tractor())
            selectMultitrack();
        else
            emit selected(0);
        return;
    }

    int trackIndex = currentTrack();
    int clipIndex = selection().isEmpty()? 0 : selection().first();
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && info->producer && info->producer->is_valid()) {
        delete m_updateCommand;
        m_updateCommand = new Timeline::UpdateCommand(*this, trackIndex, clipIndex, info->start);
        // We need to set these special properties so time-based filters
        // can get information about the cut while still applying filters
        // to the cut parent.
        info->producer->set(kFilterInProperty, info->frame_in);
        info->producer->set(kFilterOutProperty, info->frame_out);
        info->producer->set(kPlaylistStartProperty, info->start);
        info->producer->set(kMultitrackItemProperty, QString("%1:%2").arg(clipIndex).arg(trackIndex).toLatin1().constData());
        m_ignoreNextPositionChange = true;
        emit selected(info->producer);
    }
    m_model.tractor()->set(kFilterInProperty, 0);
    m_model.tractor()->set(kFilterOutProperty, m_model.tractor()->get_length() - 1);
}

void TimelineDock::remakeAudioLevels(int trackIndex, int clipIndex, bool force)
{
    if (Settings.timelineShowWaveforms()) {
        QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
        AudioLevelsTask::start(*info->producer, &m_model, modelIndex, force);
    }
}

void TimelineDock::commitTrimCommand()
{
    if (m_trimCommand && (m_trimDelta || m_transitionDelta)) {
        if (m_undoHelper) m_trimCommand->setUndoHelper(m_undoHelper.take());
        MAIN.undoStack()->push(m_trimCommand.take());
    }
    m_trimDelta = 0;
    m_transitionDelta = 0;
}

void TimelineDock::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    // Adjust selected clips for changed indices.
    if (-1 == m_selection.selectedTrack) {
        QList<int> newSelection;
        int n = last - first + 1;
        foreach (int i, m_selection.selectedClips) {
            if (i < first)
                newSelection << i;
            else
                newSelection << (i + n);
        }
        setSelection(newSelection);
    }
}

void TimelineDock::onRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    // Adjust selected clips for changed indices.
    if (-1 == m_selection.selectedTrack) {
        QList<int> newSelection;
        int n = last - first + 1;
        foreach (int i, m_selection.selectedClips) {
            if (i < first)
                newSelection << i;
            else if (i > last)
                newSelection << (i - n);
        }
        setSelection(newSelection);
    }
}

void TimelineDock::detachAudio(int trackIndex, int clipIndex)
{
    if (!m_model.trackList().count())
        return;
    Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
    QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
    if (info && info->producer && info->producer->is_valid() && !info->producer->is_blank()
             && info->producer->get("audio_index") && info->producer->get_int("audio_index") >= 0) {
        Mlt::Producer clip(MLT.profile(), "xml-string", MLT.XML(info->producer).toUtf8().constData());
        clip.set_in_and_out(info->frame_in, info->frame_out);
        MAIN.undoStack()->push(
            new Timeline::DetachAudioCommand(m_model, trackIndex, clipIndex, info->start, MLT.XML(&clip)));
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

void TimelineDock::setTrackComposite(int trackIndex, bool composite)
{
    MAIN.undoStack()->push(
        new Timeline::CompositeTrackCommand(m_model, trackIndex, composite));
}

void TimelineDock::setTrackLock(int trackIndex, bool lock)
{
    MAIN.undoStack()->push(
        new Timeline::LockTrackCommand(m_model, trackIndex, lock));
}

bool TimelineDock::moveClip(int fromTrack, int toTrack, int clipIndex, int position, bool ripple)
{
    if (m_model.moveClipValid(fromTrack, toTrack, clipIndex, position, ripple)) {
        // Workaround bug #326 moving clips between tracks stops allowing drag-n-drop
        // into Timeline, which appeared with Qt 5.6 upgrade.
        emit clipMoved(fromTrack, toTrack, clipIndex, position, ripple);
        return true;
    } else if (m_model.addTransitionValid(fromTrack, toTrack, clipIndex, position)) {
        emit transitionAdded(fromTrack, clipIndex, position, ripple);
        return true;
    } else {
        return false;
    }
}

void TimelineDock::onClipMoved(int fromTrack, int toTrack, int clipIndex, int position, bool ripple)
{
    MAIN.undoStack()->push(
        new Timeline::MoveClipCommand(m_model, fromTrack, toTrack, clipIndex, position, ripple));
}

bool TimelineDock::trimClipIn(int trackIndex, int clipIndex, int oldClipIndex, int delta, bool ripple)
{
    if (!ripple && m_model.addTransitionByTrimInValid(trackIndex, clipIndex, delta)) {
        m_model.addTransitionByTrimIn(trackIndex, clipIndex, delta);
        m_transitionDelta += delta;
        m_trimCommand.reset(new Timeline::AddTransitionByTrimInCommand(m_model, trackIndex, clipIndex - 1, m_transitionDelta, m_trimDelta, false));
    }
    else if (!ripple && m_model.removeTransitionByTrimInValid(trackIndex, clipIndex, delta)) {
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QModelIndex modelIndex = m_model.makeIndex(trackIndex, clipIndex - 1);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(trackIndex, clipIndex - 1);
        m_model.trimClipOut(trackIndex, clipIndex - 2, -n, false);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::RemoveTransitionByTrimInCommand(m_model, trackIndex, clipIndex - 1, m_trimDelta, false));
    }
    else if (m_model.trimTransitionOutValid(trackIndex, clipIndex, delta)) {
        m_model.trimTransitionOut(trackIndex, clipIndex, delta);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimTransitionOutCommand(m_model, trackIndex, clipIndex, m_trimDelta, false));
    }
    else if (m_model.trimClipInValid(trackIndex, clipIndex, delta, ripple)) {
        if (!m_undoHelper) {
            m_undoHelper.reset(new UndoHelper(m_model));
            if (ripple) m_undoHelper->setHints(UndoHelper::SkipXML);
            m_undoHelper->recordBeforeState();
        }
        clipIndex = m_model.trimClipIn(trackIndex, clipIndex, delta, ripple);

        // Update duration in properties for image clip.
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
        if (info && MLT.isImageProducer(info->producer) && !info->producer->get_int(kShotcutSequenceProperty))
            emit imageDurationChanged();

        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimClipInCommand(m_model, trackIndex, oldClipIndex, m_trimDelta, ripple, false));
    }
    else return false;
    return true;
}

bool TimelineDock::trimClipOut(int trackIndex, int clipIndex, int delta, bool ripple)
{
    if (!ripple && m_model.addTransitionByTrimOutValid(trackIndex, clipIndex, delta)) {
        m_model.addTransitionByTrimOut(trackIndex, clipIndex, delta);
        m_transitionDelta += delta;
        m_trimCommand.reset(new Timeline::AddTransitionByTrimOutCommand(m_model, trackIndex, clipIndex, m_transitionDelta, m_trimDelta, false));
    }
    else if (!ripple && m_model.removeTransitionByTrimOutValid(trackIndex, clipIndex, delta)) {
        Q_ASSERT(trackIndex >= 0 && clipIndex >= 0);
        QModelIndex modelIndex = m_model.makeIndex(trackIndex, clipIndex + 1);
        int n = m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.trimClipIn(trackIndex, clipIndex + 2, -n, true);
        m_model.removeTransition(trackIndex, clipIndex + 1);
        m_model.trimClipOut(trackIndex, clipIndex, delta - n, false);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::RemoveTransitionByTrimOutCommand(m_model, trackIndex, clipIndex + 1, m_trimDelta, false));
    }
    else if (m_model.trimTransitionInValid(trackIndex, clipIndex, delta)) {
        m_model.trimTransitionIn(trackIndex, clipIndex, delta);
        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimTransitionInCommand(m_model, trackIndex, clipIndex, m_trimDelta, false));
    }
    else if (m_model.trimClipOutValid(trackIndex, clipIndex, delta, ripple)) {
        if (!m_undoHelper) {
            m_undoHelper.reset(new UndoHelper(m_model));
            if (ripple) m_undoHelper->setHints(UndoHelper::SkipXML);
            m_undoHelper->recordBeforeState();
        }
        m_model.trimClipOut(trackIndex, clipIndex, delta, ripple);

        // Update duration in properties for image clip.
        QScopedPointer<Mlt::ClipInfo> info(getClipInfo(trackIndex, clipIndex));
        if (info && MLT.isImageProducer(info->producer) && !info->producer->get_int(kShotcutSequenceProperty))
            emit imageDurationChanged();

        m_trimDelta += delta;
        m_trimCommand.reset(new Timeline::TrimClipOutCommand(m_model, trackIndex, clipIndex, m_trimDelta, ripple, false));
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
    if (MAIN.isSourceClipMyProject()) return;
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.XML(MLT.isClip()? 0 : MLT.savedProducer());
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::InsertCommand(m_model, trackIndex, position, xmlToUse));
        selectClipUnderPlayhead();
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
    if (MAIN.isSourceClipMyProject()) return;
    if (MLT.isSeekableClip() || MLT.savedProducer() || !xml.isEmpty()) {
        QString xmlToUse = !xml.isEmpty()? xml
            : MLT.XML(MLT.isClip()? 0 : MLT.savedProducer());
        if (position < 0)
            position = m_position;
        MAIN.undoStack()->push(
            new Timeline::OverwriteCommand(m_model, trackIndex, position, xmlToUse));
        selectClipUnderPlayhead();
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
    selectClipUnderPlayhead();
}

void TimelineDock::splitClip(int trackIndex, int clipIndex)
{
    if (trackIndex < 0 || clipIndex < 0)
        chooseClipAtPosition(m_position, trackIndex, clipIndex);
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
                if (info && m_position > info->start && m_position < info->start + info->frame_count) {
                    MAIN.undoStack()->push(
                        new Timeline::SplitCommand(m_model, trackIndex, clipIndex, m_position));
                }
            } else {
                emit showStatusMessage(tr("You cannot split a transition."));
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

void TimelineDock::seekInPoint(int clipIndex)
{
    if (!MLT.isMultitrack()) return;
    if (!m_model.tractor()) return;
    if (clipIndex < 0) return;

    int mltTrackIndex = m_model.trackList().at(currentTrack()).mlt_index;
    QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(mltTrackIndex));
    if (track) {
        Mlt::Playlist playlist(*track);
        if (m_position != playlist.clip_start(clipIndex))
            setPosition(playlist.clip_start(clipIndex));
    }
}

void TimelineDock::dragEnterEvent(QDragEnterEvent *event)
{
    LOG_DEBUG() << event->mimeData()->hasFormat(Mlt::XmlMimeType);
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
        load(true);
    return result;
}

void TimelineDock::keyPressEvent(QKeyEvent* event)
{
    QDockWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void TimelineDock::keyReleaseEvent(QKeyEvent* event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void TimelineDock::load(bool force)
{
    if (m_quickView.source().isEmpty() || force) {
        QDir sourcePath = QmlUtilities::qmlDir();
        sourcePath.cd("timeline");
        m_quickView.setFocusPolicy(isFloating()? Qt::NoFocus : Qt::StrongFocus);
        m_quickView.setSource(QUrl::fromLocalFile(sourcePath.filePath("timeline.qml")));
        connect(m_quickView.rootObject(), SIGNAL(currentTrackChanged()),
                this, SIGNAL(currentTrackChanged()));
        connect(m_quickView.rootObject(), SIGNAL(clipClicked()),
                this, SIGNAL(clipClicked()));
        if (force && Settings.timelineShowWaveforms())
            m_model.reload();
    } else if (Settings.timelineShowWaveforms()) {
        m_model.reload();
    }
}

void TimelineDock::onTopLevelChanged(bool floating)
{
    m_quickView.setFocusPolicy(floating? Qt::NoFocus : Qt::StrongFocus);
}

void TimelineDock::onTransitionAdded(int trackIndex, int clipIndex, int position, bool ripple)
{
    setSelection(); // cleared
    Timeline::AddTransitionCommand* command = new Timeline::AddTransitionCommand(m_model, trackIndex, clipIndex, position, ripple);
    MAIN.undoStack()->push(command);
    // Select the transition.
    setSelection(QList<int>() << command->getTransitionIndex());
}
