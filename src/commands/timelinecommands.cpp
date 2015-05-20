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

#include "timelinecommands.h"
#include "mltcontroller.h"
#include <QtDebug>

#ifdef UNDOHELPER_DEBUG
#define UNDOLOG qDebug()
#else
#define UNDOLOG if (false) qDebug()
#endif

static const char* kUndoIdProperty = "temporary_undo_id";

class UndoHelper
{
public:
    UndoHelper(MultitrackModel & model)
        : m_model(model)
    {
    }

    void debugPrintState()
    {
        qDebug("timeline state: {");
        for (int i = 0; i < m_model.trackList().count(); ++i)
        {
            int mltIndex = m_model.trackList()[i].mlt_index;
            QString trackStr = QString("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
            Mlt::Producer * trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);

            for (int j = 0; j < playlist.count(); ++j) {
                Mlt::Producer * clip = playlist.get_clip(j);
                Mlt::ClipInfo info;
                playlist.clip_info(j, &info);
                trackStr += QString(" [ %1 -> %2 (%3 frames) %4]").arg(info.frame_in).arg(info.frame_out).arg(info.frame_count).arg(clip->is_blank() ? "blank " : "");
            }
            qDebug() << qPrintable(trackStr);
        }
        qDebug("}");
    }

    void recordBeforeState()
    {
#ifdef UNDOHELPER_DEBUG
        debugPrintState();
#endif
        m_state.clear();
        m_clipsAdded.clear();
        int uid = 1;
        for (int i = 0; i < m_model.trackList().count(); ++i)
        {
            int mltIndex = m_model.trackList()[i].mlt_index;
            Mlt::Producer * trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);

            for (int j = 0; j < playlist.count(); ++j) {
                Mlt::Producer * clip = playlist.get_clip(j);
                Info& info = m_state[uid];
                info.xml = MLT.XML(clip);
                playlist.clip_info(j, &info.clipInfo);
                info.oldTrackIndex = i;
                info.oldClipIndex = j;
                info.isBlank = playlist.is_blank(j);
                clip->set(kUndoIdProperty, uid);
                uid++;
            }
        }
    }

    void recordAfterState()
    {
#ifdef UNDOHELPER_DEBUG
        debugPrintState();
#endif
        QList<int> clipsRemoved = m_state.keys();
        for (int i = 0; i < m_model.trackList().count(); ++i)
        {
            int mltIndex = m_model.trackList()[i].mlt_index;
            Mlt::Producer * trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);

            for (int j = 0; j < playlist.count(); ++j) {
                Mlt::Producer * clip = playlist.get_clip(j);
                int uid = clip->get_int(kUndoIdProperty);

                /* Clips that do not have the undo id set are new */
                if (!uid) {
                    UNDOLOG << "New clip!" << clip;
                    m_clipsAdded << qMakePair(i, j);
                }
                else {
                    Q_ASSERT(m_state.contains(uid));
                    Info &info = m_state[uid];
                    info.newTrackIndex = i;
                    info.newClipIndex = j;

                    /* Indices have changed; these are moved */
                    if (info.oldTrackIndex != info.newTrackIndex || info.oldClipIndex != info.newClipIndex) {
                        UNDOLOG << "Clip" << uid << "moved from"
                            << info.oldTrackIndex << info.oldClipIndex
                            << "to"
                            << info.newTrackIndex << info.newClipIndex;
                        info.changes |= Moved;
                    }

                    QString newXml = MLT.XML(clip);
                    if (info.xml != newXml) {
                        UNDOLOG << "Modified xml:" << uid;
                        info.changes |= XMLModified;
                    }

                    Mlt::ClipInfo newInfo;
                    playlist.clip_info(j, &newInfo);
                    /* Only in/out point changes are handled at the point of writing, but the whole
                     * clipinfo struct is available */
                    if (info.clipInfo.frame_in != newInfo.frame_in
                            || info.clipInfo.frame_out != newInfo.frame_out) {
                        UNDOLOG << "In/out changed:" << uid;
                        info.changes |= ClipInfoModified;
                    }
                }
                clipsRemoved.removeOne(uid);
            }
        }

        /* Clips that did not show up are removed from the timeline */
        foreach (int uid, clipsRemoved) {
            UNDOLOG << "Clip removed:" << uid;
            m_state[uid].changes |= Removed;
        }
    }

    void undoChanges()
    {
#ifdef UNDOHELPER_DEBUG
        debugPrintState();
#endif
        /* Clips that were added do not have an id. They will be removed as part of the operation,
         * but because clips are moved around, we simply use a negative id to signal that it is due
         * for removal once done */
        typedef QPair<int,int> ClipLoc;
        foreach (const ClipLoc& loc, m_clipsAdded) {
            int mltIndex = m_model.trackList()[loc.first].mlt_index;
            Mlt::Producer* trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);
            playlist.get_clip(loc.second)->set(kUndoIdProperty, -1);
        }

        /* We need the uid as it was before on some operations, so we locate the clip using its
         * index here and once more set the uid property */
        foreach (int uid, m_state.keys()) {
            /* Skip removed clips since they are of course not present in the timeline */
            if (m_state[uid].changes & Removed)
                continue;

            int mltIndex = m_model.trackList()[m_state[uid].newTrackIndex].mlt_index;
            Mlt::Producer* trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);
            playlist.get_clip(m_state[uid].newClipIndex)->set(kUndoIdProperty, uid);
        }

        /* We're walking through the list in the order of uids, which is the order in which the
         * clips were laid out originally. As we go through the clips we make sure the clips behind
         * the current index are as they were originally before we move on to the next one */
        foreach (int uid, m_state.keys()) {
            const Info& info = m_state[uid];
            UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index" << info.oldClipIndex;

            /* This is the index in the track we're currently restoring */
            int currentIndex = info.oldClipIndex;

            int mltIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
            Mlt::Producer* trackProducer = m_model.tractor()->track(mltIndex);
            Mlt::Playlist playlist(*trackProducer);

            /* Clips that were moved are simply searched for using the uid, and moved in place. We
             * do not use the indices directly because they become invalid once the playlist is
             * modified. */
            if (info.changes & Moved) {
                Q_ASSERT(info.newTrackIndex == info.oldTrackIndex && "cross-track moves are unsupported so far");
                int clipCurrentlyAt = -1;
                for (int i = 0; i < playlist.count(); ++i) {
                    if (playlist.get_clip(i)->get_int(kUndoIdProperty) == uid) {
                        clipCurrentlyAt = i;
                        break;
                    }
                }
                Q_ASSERT(clipCurrentlyAt != -1 && "Moved clip could not be found");
                UNDOLOG << "Found clip with uid" << uid << "at index" << clipCurrentlyAt;

                if (clipCurrentlyAt != info.oldClipIndex) {
                    UNDOLOG << "moving from" << clipCurrentlyAt << "to" << currentIndex;
                    QModelIndex modelIndex = m_model.createIndex(clipCurrentlyAt, 0, info.oldTrackIndex);
                    m_model.beginMoveRows(modelIndex.parent(), clipCurrentlyAt, clipCurrentlyAt, modelIndex.parent(), currentIndex);
                    playlist.move(clipCurrentlyAt, currentIndex);
                    m_model.endMoveRows();
                }
            }

            /* Removed clips are reinserted using their stored XML */
            if (info.changes & Removed) {
                QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
                m_model.beginInsertRows(modelIndex.parent(), currentIndex, currentIndex);
                if (info.isBlank) {
                    playlist.insert_blank(currentIndex, info.clipInfo.frame_out);
                    UNDOLOG << "inserting isBlank at " << currentIndex;
                } else {
                    UNDOLOG << "inserting clip at " << currentIndex;
                    Mlt::Producer restoredClip(MLT.profile(), "xml-string", info.xml.toUtf8().constData());
                    playlist.insert(restoredClip, currentIndex, info.clipInfo.frame_in, info.clipInfo.frame_out);
                }
                m_model.endInsertRows();

                /* just in case we might need the uid later in the procedure */
                playlist.get_clip(currentIndex)->set(kUndoIdProperty, uid);
            }

            /* Only in/out points handled so far */
            if (info.changes & ClipInfoModified) {
                UNDOLOG << "resizing clip at" << currentIndex;
                playlist.resize_clip(currentIndex, info.clipInfo.frame_in, info.clipInfo.frame_out);
                /* TODO: What other clipinfo changes might we want to cover? */

                QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
                QVector<int> roles;
                roles << MultitrackModel::DurationRole;
                emit m_model.dataChanged(modelIndex, modelIndex, roles);
            }
        }

        /* Finally we walk through the tracks once more, removing clips that
         * were added, and clearing the temporarily used uid property */
        int trackIndex = 0;
        foreach (const Track & track, m_model.trackList()) {
            Mlt::Producer* trackProducer = m_model.tractor()->track(track.mlt_index);
            Mlt::Playlist playlist(*trackProducer);
            for (int i = playlist.count() - 1; i >= 0; --i) {
                Mlt::Producer * clip = playlist.get_clip(i);
                if (clip->get_int(kUndoIdProperty) < 0) {
                    UNDOLOG << "Removing clip at" << i;
                    m_model.beginRemoveRows(m_model.index(trackIndex), i, i);
                    playlist.remove(i);
                    m_model.endRemoveRows();
                }
                else {
                    /* reset the uid property now that we're finished undoing */
                    clip->set(kUndoIdProperty, 0);
                }
            }
            trackIndex++;
        }
        emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
        debugPrintState();
#endif
    }
private:
    enum ChangeFlags {
        NoChange = 0x0,
        ClipInfoModified = 0x1,
        XMLModified = 0x2,
        Moved = 0x4,
        Removed = 0x8
    };

    struct Info {
        int oldTrackIndex;
        int oldClipIndex;
        int newTrackIndex;
        int newClipIndex;
        bool isBlank;
        QString xml;
        Mlt::ClipInfo clipInfo;

        int changes;
        Info()
            : oldTrackIndex(-1)
            , oldClipIndex(-1)
            , newTrackIndex(-1)
            , newClipIndex(-1)
            , isBlank(false)
            , changes(NoChange)
        {}
    };
    QMap<int,Info> m_state;
    QList<QPair<int,int> > m_clipsAdded;
    MultitrackModel & m_model;
};

namespace Timeline {

AppendCommand::AppendCommand(MultitrackModel &model, int trackIndex, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_xml(xml)
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Append to track"));
}


void AppendCommand::redo()
{
    m_undoHelper->recordBeforeState();
    Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.appendClip(m_trackIndex, producer);
    m_undoHelper->recordAfterState();
}

void AppendCommand::undo()
{
    m_undoHelper->undoChanges();
}

InsertCommand::InsertCommand(MultitrackModel &model, int trackIndex,
    int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Insert into track"));
}

void InsertCommand::redo()
{
    m_undoHelper->recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_model.insertClip(m_trackIndex, clip, m_position);
    m_undoHelper->recordAfterState();
}

void InsertCommand::undo()
{
    m_undoHelper->undoChanges();
}

OverwriteCommand::OverwriteCommand(MultitrackModel &model, int trackIndex,
    int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Overwrite onto track"));
}

void OverwriteCommand::redo()
{
    m_undoHelper->recordBeforeState();
    Mlt::Producer clip(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
    m_playlistXml = m_model.overwrite(m_trackIndex, clip, m_position);
    m_undoHelper->recordAfterState();
}

void OverwriteCommand::undo()
{
    m_undoHelper->undoChanges();
}

LiftCommand::LiftCommand(MultitrackModel &model, int trackIndex,
    int clipIndex, int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Lift from track"));
}

void LiftCommand::redo()
{
    m_undoHelper->recordBeforeState();
    m_model.liftClip(m_trackIndex, m_clipIndex);
    m_undoHelper->recordAfterState();
}

void LiftCommand::undo()
{
    m_undoHelper->undoChanges();
}

RemoveCommand::RemoveCommand(MultitrackModel &model, int trackIndex,
    int clipIndex, int position, const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_position(position)
    , m_xml(xml)
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Remove from track"));
}

void RemoveCommand::redo()
{
    m_undoHelper->recordBeforeState();
    m_model.removeClip(m_trackIndex, m_clipIndex);
    m_undoHelper->recordAfterState();
}

void RemoveCommand::undo()
{
    m_undoHelper->undoChanges();
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

LockTrackCommand::LockTrackCommand(MultitrackModel &model, int trackIndex, Qt::CheckState value, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_value(value)
    , m_oldValue(Qt::CheckState(model.data(m_model.index(trackIndex), MultitrackModel::IsLockedRole).toInt()))
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
    , m_undoHelper(new UndoHelper(m_model))
{
    setText(QObject::tr("Move clip"));
}

void MoveClipCommand::redo()
{
    m_undoHelper->recordBeforeState();
    m_model.moveClip(m_fromTrackIndex, m_toTrackIndex, m_fromClipIndex, m_toStart);
    m_undoHelper->recordAfterState();
}

void MoveClipCommand::undo()
{
    m_undoHelper->undoChanges();
}

TrimClipInCommand::TrimClipInCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Trim clip in point"));
}

void TrimClipInCommand::redo()
{
    m_clipIndex = m_model.trimClipIn(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify && m_clipIndex >= 0)
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
}

void TrimClipInCommand::undo()
{
    if (m_clipIndex >= 0) {
        m_clipIndex = m_model.trimClipIn(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipIn(m_trackIndex, m_clipIndex);
        m_notify = true;
    }
}

bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand* that = static_cast<const TrimClipInCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += static_cast<const TrimClipInCommand*>(other)->m_delta;
    return true;
}

TrimClipOutCommand::TrimClipOutCommand(MultitrackModel &model, int trackIndex, int clipIndex, int delta, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(trackIndex)
    , m_clipIndex(clipIndex)
    , m_delta(delta)
    , m_notify(false)
{
    setText(QObject::tr("Trim clip out point"));
}

void TrimClipOutCommand::redo()
{
    m_clipIndex = m_model.trimClipOut(m_trackIndex, m_clipIndex, m_delta);
    if (m_notify && m_clipIndex >= 0)
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
}

void TrimClipOutCommand::undo()
{
    if (m_clipIndex >= 0) {
        m_model.trimClipOut(m_trackIndex, m_clipIndex, -m_delta);
        m_model.notifyClipOut(m_trackIndex, m_clipIndex);
        m_notify = true;
    }
}

bool TrimClipOutCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipOutCommand* that = static_cast<const TrimClipOutCommand*>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex || that->m_clipIndex != m_clipIndex)
        return false;
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
        if (m_transitionIndex == m_clipIndex) { // dragged left
            m_model.removeClip(m_trackIndex, m_clipIndex + 1);
        } else {
            m_model.removeClip(m_trackIndex, m_clipIndex);
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
        m_model.trimClipOut(m_trackIndex, m_clipIndex - 1, m_delta);
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
        m_model.trimClipIn(m_trackIndex, m_clipIndex + 2, m_delta);
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

} // namespace
