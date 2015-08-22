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

#include "timelinecommands.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include <QtDebug>

namespace Timeline {

AppendCommand::AppendCommand(MultitrackModel &model, int trackIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_xml(xml)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Append to track"));
}


void AppendCommand::redo()
{
    m_undoHelper.recordBeforeState();
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.appendClip(m_trackIndex, producer);
    m_undoHelper.recordAfterState();
}

void AppendCommand::undo()
{
    m_undoHelper.undoChanges();
}

InsertCommand::InsertCommand(MultitrackModel &model, int trackIndex,
    int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Insert into track"));
}

void InsertCommand::redo()
{
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insertClip(m_trackIndex, clip, m_position);
    m_undoHelper.recordAfterState();
}

void InsertCommand::undo()
{
    m_undoHelper.undoChanges();
}

OverwriteCommand::OverwriteCommand(MultitrackModel &model, int trackIndex,
    int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Overwrite onto track"));
}

void OverwriteCommand::redo()
{
    m_undoHelper.recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_playlistXml = m_model.overwrite(m_trackIndex, clip, m_position);
    m_undoHelper.recordAfterState();
}

void OverwriteCommand::undo()
{
    m_undoHelper.undoChanges();
}

LiftCommand::LiftCommand(MultitrackModel &model, int trackIndex,
    int clipIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_xml(xml)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Lift from track"));
}

void LiftCommand::redo()
{
    m_undoHelper.recordBeforeState();
    m_model.liftClip(m_trackIndex, m_clipIndex);
    m_undoHelper.recordAfterState();
}

void LiftCommand::undo()
{
    m_undoHelper.undoChanges();
}

RemoveCommand::RemoveCommand(MultitrackModel &model, int trackIndex,
    int clipIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_xml(xml)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Remove from track"));
}

void RemoveCommand::redo()
{
    m_undoHelper.recordBeforeState();
    m_model.removeClip(m_trackIndex, m_clipIndex);
    m_undoHelper.recordAfterState();
}

void RemoveCommand::undo()
{
    m_undoHelper.undoChanges();
}


NameTrackCommand::NameTrackCommand(MultitrackModel &model, int trackIndex,
    const QString &name, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_name(name)
    , m_oldName(model.data(m_model.index(trackIndex), MultitrackModel::NameRole).toString())
{
    setText(QObject::tr("Change track name"));
}

void NameTrackCommand::redo()
{
    m_model.setTrackName(m_trackIndex, m_name);
}

void NameTrackCommand::undo()
{
    m_model.setTrackName(m_trackIndex, m_oldName);
}

MuteTrackCommand::MuteTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsMuteRole).toBool())
{
    setText(QObject::tr("Toggle track mute"));
}

void MuteTrackCommand::redo()
{
    m_model.setTrackMute(m_trackIndex, !m_oldValue);
}

void MuteTrackCommand::undo()
{
    m_model.setTrackMute(m_trackIndex, m_oldValue);
}

HideTrackCommand::HideTrackCommand(MultitrackModel &model, int trackIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsHiddenRole).toBool())
{
    setText(QObject::tr("Toggle track hidden"));
}

void HideTrackCommand::redo()
{
    m_model.setTrackHidden(m_trackIndex, !m_oldValue);
}

void HideTrackCommand::undo()
{
    m_model.setTrackHidden(m_trackIndex, m_oldValue);
}

CompositeTrackCommand::CompositeTrackCommand(MultitrackModel &model, int trackIndex, Qt::CheckState value, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_value(value)
    , m_oldValue(Qt::CheckState(model.data(m_model.index(trackIndex), MultitrackModel::IsCompositeRole).toInt()))
{
    setText(QObject::tr("Change track compositing"));
}

void CompositeTrackCommand::redo()
{
    m_model.setTrackComposite(m_trackIndex, m_value);
}

void CompositeTrackCommand::undo()
{
    m_model.setTrackComposite(m_trackIndex, m_oldValue);
}

LockTrackCommand::LockTrackCommand(MultitrackModel &model, int trackIndex, bool value, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_value(value)
    , m_oldValue(model.data(m_model.index(trackIndex), MultitrackModel::IsLockedRole).toBool())
{
    setText(QObject::tr("Lock track"));
}

void LockTrackCommand::redo()
{
    m_model.setTrackLock(m_trackIndex, m_value);
}

void LockTrackCommand::undo()
{
    m_model.setTrackLock(m_trackIndex, m_oldValue);
}

MoveClipCommand::MoveClipCommand(MultitrackModel &model, int fromTrackIndex, int toTrackIndex, int clipIndex, int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_fromTrackIndex(fromTrackIndex)
    , m_toTrackIndex(toTrackIndex)
    , m_fromClipIndex(clipIndex)
    , m_fromStart(model.data(
        m_model.index(clipIndex, 0, m_model.index(fromTrackIndex)),
            MultitrackModel::StartRole).toInt())
    , m_toStart(position)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Move clip"));
}

void MoveClipCommand::redo()
{
    m_undoHelper.recordBeforeState();
    m_model.moveClip(m_fromTrackIndex, m_toTrackIndex, m_fromClipIndex, m_toStart);
    m_undoHelper.recordAfterState();
}

void MoveClipCommand::undo()
{
    m_undoHelper.undoChanges();
}

TrimClipInCommand::TrimClipInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, bool ripple, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_ripple(ripple)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Trim clip in point"));
}

void TrimClipInCommand::redo()
{
    m_undoHelper.recordBeforeState();
    m_clipIndex = m_model.trimClipIn(m_trackIndex, m_clipIndex, m_delta, m_ripple);
    m_undoHelper.recordAfterState();
}

void TrimClipInCommand::undo()
{
    m_undoHelper.undoChanges();
}

bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand* that = static_cast<const TrimClipInCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_undoHelper.recordAfterState();
    m_delta += static_cast<const TrimClipInCommand*>(other)->m_delta;
    return true;
}

TrimClipOutCommand::TrimClipOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, bool ripple, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_ripple(ripple)
    , m_undoHelper(m_model)
{
    setText(QObject::tr("Trim clip out point"));
}

void TrimClipOutCommand::redo()
{
    m_undoHelper.recordBeforeState();
    m_clipIndex = m_model.trimClipOut(m_trackIndex, m_clipIndex, m_delta, m_ripple);
    m_undoHelper.recordAfterState();
}

void TrimClipOutCommand::undo()
{
    m_undoHelper.undoChanges();
}

bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand* that = static_cast<const TrimClipOutCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_undoHelper.recordAfterState();
    m_delta += static_cast<const TrimClipOutCommand*>(other)->m_delta;
    return true;
}

SplitCommand::SplitCommand(MultitrackModel &model, int trackIndex,
    int clipIndex, int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
{
    setText(QObject::tr("Split clip"));
}

void SplitCommand::redo()
{
    m_model.splitClip(m_trackIndex, m_clipIndex, m_position);
}

void SplitCommand::undo()
{
    m_model.joinClips(m_trackIndex, m_clipIndex);
}

FadeInCommand::FadeInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int duration, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_duration(duration)
{
    QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
    m_previous = model.data(modelIndex, MultitrackModel::FadeInRole).toInt();
    setText(QObject::tr("Adjust fade in"));
}

void FadeInCommand::redo()
{
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_duration);
}

void FadeInCommand::undo()
{
    m_model.fadeIn(m_trackIndex, m_clipIndex, m_previous);
}

bool FadeInCommand::mergeWith(const QUndoCommand *other)
{
    const FadeInCommand* that = static_cast<const FadeInCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_duration = static_cast<const FadeInCommand*>(other)->m_duration;
    return true;
}

FadeOutCommand::FadeOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int duration, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_duration(duration)
{
    QModelIndex modelIndex = m_model.index(clipIndex, 0, m_model.index(trackIndex));
    m_previous = model.data(modelIndex, MultitrackModel::FadeOutRole).toInt();
    setText(QObject::tr("Adjust fade out"));
}

void FadeOutCommand::redo()
{
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_duration);
}

void FadeOutCommand::undo()
{
    m_model.fadeOut(m_trackIndex, m_clipIndex, m_previous);
}

bool FadeOutCommand::mergeWith(const QUndoCommand *other)
{
    const FadeOutCommand* that = static_cast<const FadeOutCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_duration = static_cast<const FadeOutCommand*>(other)->m_duration;
    return true;
}

AddTransitionCommand::AddTransitionCommand(MultitrackModel &model, int trackIndex, int clipIndex, int position, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_transitionIndex(-1)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionCommand::redo()
{
    m_transitionIndex = m_model.addTransition(m_trackIndex, m_clipIndex, m_position);
}

void AddTransitionCommand::undo()
{
    if (m_transitionIndex >= 0) {
        m_model.removeTransition(m_trackIndex, m_transitionIndex);
        // Delete the blank that was inserted.
        int i = m_model.trackList().at(m_trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> track(m_model.tractor()->track(i));
        if (track) {
            Mlt::Playlist playlist(*track);
            if (playlist.is_blank(m_clipIndex + 1) && m_transitionIndex == m_clipIndex) { // dragged left
                m_model.removeClip(m_trackIndex, m_clipIndex + 1);
            } else if (playlist.is_blank(m_clipIndex)) {
                m_model.removeClip(m_trackIndex, m_clipIndex);
            }
        }
    } else {
        qWarning() << "Failed to undo the transition!";
    }
}

TrimTransitionInCommand::TrimTransitionInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Trim transition in point"));
}

void TrimTransitionInCommand::redo()
{
    m_model.trimTransitionIn(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify && m_clipIndex >= 0)
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
}

void TrimTransitionInCommand::undo()
{
    if (m_clipIndex >= 0) {
        m_model.trimTransitionIn(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
        m_notify = true;
    }
}

bool TrimTransitionInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionInCommand* that = static_cast<const TrimTransitionInCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += static_cast<const TrimTransitionInCommand*>(other)->m_delta;
    return true;
}

TrimTransitionOutCommand::TrimTransitionOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Trim transition out point"));
}

void TrimTransitionOutCommand::redo()
{
    m_model.trimTransitionOut(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify && m_clipIndex >= 0)
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
}

void TrimTransitionOutCommand::undo()
{
    if (m_clipIndex >= 0) {
        m_model.trimTransitionOut(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
        m_notify = true;
    }
}

bool TrimTransitionOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimTransitionOutCommand* that = static_cast<const TrimTransitionOutCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += static_cast<const TrimTransitionOutCommand*>(other)->m_delta;
    return true;
}

AddTransitionByTrimInCommand::AddTransitionByTrimInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionByTrimInCommand::redo()
{
    m_model.addTransitionByTrimIn(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify && m_clipIndex > 0)
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
}

void AddTransitionByTrimInCommand::undo()
{
    if (m_clipIndex > 0) {
        QModelIndex modelIndex = m_model.index(m_clipIndex, 0, m_model.index(m_trackIndex));
        m_delta = -m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(m_trackIndex, m_clipIndex);
        m_model.trimClipOut(m_trackIndex, m_clipIndex - 1, m_delta, false);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex - 1);
        m_notify = true;
    }
}

bool AddTransitionByTrimInCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimInCommand* that = static_cast<const AddTransitionByTrimInCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex ||
        (that->m_clipIndex != m_clipIndex && m_clipIndex != that->m_clipIndex - 1))
        return false;
    return true;
}

AddTransitionByTrimOutCommand::AddTransitionByTrimOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Add transition"));
}

void AddTransitionByTrimOutCommand::redo()
{
    m_model.addTransitionByTrimOut(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify)
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 2);
}

void AddTransitionByTrimOutCommand::undo()
{
    if (m_clipIndex + 2 < m_model.rowCount(m_model.index(m_trackIndex))) {
        QModelIndex modelIndex = m_model.index(m_clipIndex + 1, 0, m_model.index(m_trackIndex));
        m_delta = -m_model.data(modelIndex, MultitrackModel::DurationRole).toInt();
        m_model.liftClip(m_trackIndex, m_clipIndex + 1);
        m_model.trimClipIn(m_trackIndex, m_clipIndex + 2, m_delta, false);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex + 1);
        m_notify = true;
    }
}

bool AddTransitionByTrimOutCommand::mergeWith(const QUndoCommand *other)
{
    const AddTransitionByTrimOutCommand* that = static_cast<const AddTransitionByTrimOutCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    return true;
}

AddTrackCommand::AddTrackCommand(MultitrackModel& model, bool isVideo, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_isVideo(isVideo)
{
    if (isVideo)
        setText(QObject::tr("Add video track"));
    else
        setText(QObject::tr("Add audio track"));
}

void AddTrackCommand::redo()
{
    if (m_isVideo)
        m_trackIndex = m_model.addVideoTrack();
    else
        m_trackIndex = m_model.addAudioTrack();
}

void AddTrackCommand::undo()
{
    m_model.removeTrack(m_trackIndex);
}

InsertTrackCommand::InsertTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_trackType(model.trackList().size() > 0 ? model.trackList().at(trackIndex).type : VideoTrackType)
{
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Insert audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Insert video track"));
}

void InsertTrackCommand::redo()
{
    m_model.insertTrack(m_trackIndex, m_trackType);
}

void InsertTrackCommand::undo()
{
    m_model.removeTrack(m_trackIndex);
}

RemoveTrackCommand::RemoveTrackCommand(MultitrackModel& model, int trackIndex, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_trackType(model.trackList().at(trackIndex).type)
{
    if (m_trackType == AudioTrackType)
        setText(QObject::tr("Remove audio track"));
    else if (m_trackType == VideoTrackType)
        setText(QObject::tr("Remove video track"));

    // Save track XML.
    int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
    QScopedPointer<Mlt::Producer> producer(m_model.tractor()->multitrack()->track(mlt_index));
    if (producer && producer->is_valid()) {
        m_xml = MLT.XML(producer.data());
        m_trackName = QString::fromUtf8(producer->get(kTrackNameProperty));
        qDebug() << m_xml;
    }
}

void RemoveTrackCommand::redo()
{
    m_model.removeTrack(m_trackIndex);
}

void RemoveTrackCommand::undo()
{
    m_model.insertTrack(m_trackIndex, m_trackType);
    m_model.setTrackName(m_trackIndex, m_trackName);

    // Restore track from XML.
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    Mlt::Playlist playlist(producer);
    m_model.appendFromPlaylist(&playlist, m_trackIndex);

    // Re-attach filters.
    int n = playlist.filter_count();
    if (n > 0) {
        int mlt_index = m_model.trackList().at(m_trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> producer(m_model.tractor()->multitrack()->track(mlt_index));
        for (int i = 0; i < n; ++i) {
            QScopedPointer<Mlt::Filter> filter(playlist.filter(i));
            if (filter && filter->is_valid())
                producer->attach(*filter);
        }
    }
}

} // namespace
