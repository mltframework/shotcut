/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Harald Hvaal <harald.hvaal@gmail.com>
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

#include "undohelper.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"
#include <QtDebug>
#include <QScopedPointer>
#include <QUuid>

#ifdef UNDOHELPER_DEBUG
#define UNDOLOG qDebug()
#else
#define UNDOLOG if (false) qDebug()
#endif

UndoHelper::UndoHelper(MultitrackModel& model)
    : m_model(model)
    , m_hints(NoHints)
{
}

void UndoHelper::recordBeforeState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState();
#endif
    m_state.clear();
    m_clipsAdded.clear();
    m_insertedOrder.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i)
    {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(*clip);
            m_insertedOrder << uid;
            Info& info = m_state[uid];
            if (!(m_hints & SkipXML))
                info.xml = MLT.XML(&clip->parent());
            Mlt::ClipInfo clipInfo;
            playlist.clip_info(j, &clipInfo);
            info.frame_in = clipInfo.frame_in;
            info.frame_out = clipInfo.frame_out;
            info.oldTrackIndex = i;
            info.oldClipIndex = j;
            info.isBlank = playlist.is_blank(j);
        }
    }
}

void UndoHelper::recordAfterState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState();
#endif
    QList<QUuid> clipsRemoved = m_state.keys();
    m_clipsAdded.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i)
    {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            QUuid uid = MLT.ensureHasUuid(*clip);

            /* Clips not previously in m_state are new */
            if (!m_state.contains(uid)) {
                UNDOLOG << "New clip!" << clip;
                m_clipsAdded << uid;
            }
            else {
                Info &info = m_state[uid];
                info.changes = 0;
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

                if (!(m_hints & SkipXML)) {
                    QString newXml = MLT.XML(&clip->parent());
                    if (info.xml != newXml) {
                        UNDOLOG << "Modified xml:" << uid;
                        info.changes |= XMLModified;
                    }
                }

                Mlt::ClipInfo newInfo;
                playlist.clip_info(j, &newInfo);
                /* Only in/out point changes are handled at this time. */
                if (info.frame_in != newInfo.frame_in || info.frame_out != newInfo.frame_out) {
                    UNDOLOG << "In/out changed:" << uid;
                    info.changes |= ClipInfoModified;
                }
            }
            clipsRemoved.removeOne(uid);
        }
    }

    /* Clips that did not show up are removed from the timeline */
    foreach (QUuid uid, clipsRemoved) {
        UNDOLOG << "Clip removed:" << uid;
        m_state[uid].changes = Removed;
    }
}

void UndoHelper::undoChanges()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState();
#endif

    /* We're walking through the list in the order of uids, which is the order in which the
     * clips were laid out originally. As we go through the clips we make sure the clips behind
     * the current index are as they were originally before we move on to the next one */
    foreach (QUuid uid, m_insertedOrder) {
        const Info& info = m_state[uid];
        UNDOLOG << "Handling uid" << uid << "on track" << info.oldTrackIndex << "index" << info.oldClipIndex;

        /* This is the index in the track we're currently restoring */
        int currentIndex = info.oldClipIndex;

        int mltIndex = m_model.trackList()[info.oldTrackIndex].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        /* Clips that were moved are simply searched for using the uid, and moved in place. We
         * do not use the indices directly because they become invalid once the playlist is
         * modified. */
        if (info.changes & Moved) {
            Q_ASSERT(info.newTrackIndex == info.oldTrackIndex && "cross-track moves are unsupported so far");
            int clipCurrentlyAt = -1;
            for (int i = 0; i < playlist.count(); ++i) {
                QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
                if (MLT.uuid(*clip) == uid) {
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
                playlist.insert_blank(currentIndex, info.frame_out);
                UNDOLOG << "inserting isBlank at " << currentIndex;
            } else {
                UNDOLOG << "inserting clip at " << currentIndex;
                qDebug() << m_hints;
                Q_ASSERT(!(m_hints & SkipXML) && "Cannot restore clip without stored XML");
                Q_ASSERT(!info.xml.isEmpty());
                Mlt::Producer restoredClip(MLT.profile(), "xml-string", info.xml.toUtf8().constData());
                playlist.insert(restoredClip, currentIndex, info.frame_in, info.frame_out);
            }
            m_model.endInsertRows();

            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            Q_ASSERT(currentIndex < playlist.count());
            Q_ASSERT(!clip.isNull());
            MLT.setUuid(*clip, uid);
            AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
        }

        /* Only in/out points handled so far */
        if (info.changes & ClipInfoModified) {
            UNDOLOG << "resizing clip at" << currentIndex;
            playlist.resize_clip(currentIndex, info.frame_in, info.frame_out);

            QModelIndex modelIndex = m_model.createIndex(currentIndex, 0, info.oldTrackIndex);
            QVector<int> roles;
            roles << MultitrackModel::DurationRole;
            emit m_model.dataChanged(modelIndex, modelIndex, roles);
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(currentIndex));
            AudioLevelsTask::start(clip->parent(), &m_model, modelIndex);
        }
    }

    /* Finally we walk through the tracks once more, removing clips that
     * were added, and clearing the temporarily used uid property */
    int trackIndex = 0;
    foreach (const Track & track, m_model.trackList()) {
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(track.mlt_index));
        Mlt::Playlist playlist(*trackProducer);
        for (int i = playlist.count() - 1; i >= 0; --i) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(i));
            QUuid uid = MLT.uuid(*clip);
            Q_ASSERT(!uid.isNull());
            if (m_clipsAdded.removeOne(uid)) {
                UNDOLOG << "Removing clip at" << i;
                m_model.beginRemoveRows(m_model.index(trackIndex), i, i);
                playlist.remove(i);
                m_model.endRemoveRows();
            }
        }
        trackIndex++;
    }
    emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
    debugPrintState();
#endif
}

void UndoHelper::setHints(OptimizationHints hints)
{
    m_hints = hints;
}

void UndoHelper::debugPrintState()
{
    qDebug("timeline state: {");
    for (int i = 0; i < m_model.trackList().count(); ++i)
    {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QString trackStr = QString("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            Mlt::ClipInfo info;
            playlist.clip_info(j, &info);
            trackStr += QString(" [ %5 %1 -> %2 (%3 frames) %4]").arg(info.frame_in).arg(info.frame_out).arg(info.frame_count).arg(clip->is_blank() ? "blank " : "").arg(MLT.uuid(*clip).toString());
        }
        qDebug() << qPrintable(trackStr);
    }
    qDebug("}");
}
