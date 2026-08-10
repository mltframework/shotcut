/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#include "Logger.h"
#include "mltcontroller.h"
#include "models/audiolevelstask.h"
#include "shotcut_mlt_properties.h"

#include <QScopedPointer>
#include <QUuid>

#ifdef UNDOHELPER_DEBUG
#define UNDOLOG LOG_DEBUG()
#else
#define UNDOLOG \
    if (false) \
    LOG_DEBUG()
#endif

UndoHelper::UndoHelper(MultitrackModel &model)
    : m_model(model)
{}

void UndoHelper::recordBeforeState(const QSet<int> &trackScope)
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before state");
#endif
    qint64 xmlCallCountBefore = MLT.xmlCallCount();
    m_trackScope = trackScope;
    m_beforeXml.clear();
    m_affectedTracks.clear();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        if (!m_trackScope.isEmpty() && !m_trackScope.contains(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        promoteUuids(playlist);
        m_beforeXml[i] = MLT.XML(trackProducer.data());
        demoteUuids(playlist);
    }
    UNDOLOG << "recordBeforeState() called Controller::XML()"
            << (MLT.xmlCallCount() - xmlCallCountBefore) << "times";
}

void UndoHelper::recordAfterState()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After state");
#endif
    qint64 xmlCallCountBefore = MLT.xmlCallCount();
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        if (!m_trackScope.isEmpty() && !m_trackScope.contains(i))
            continue;
        int mltIndex = m_model.trackList()[i].mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        promoteUuids(playlist);
        QString afterXml = MLT.XML(trackProducer.data());
        demoteUuids(playlist);
        if (!m_beforeXml.contains(i) || m_beforeXml.value(i) != afterXml) {
            UNDOLOG << "Track" << i << "was modified";
            m_affectedTracks << i;
        }
    }
    UNDOLOG << "recordAfterState() called Controller::XML()"
            << (MLT.xmlCallCount() - xmlCallCountBefore) << "times";
}

void UndoHelper::undoChanges()
{
#ifdef UNDOHELPER_DEBUG
    debugPrintState("Before undo");
#endif
    for (const auto &trackIndex : std::as_const(m_affectedTracks)) {
        if (trackIndex < 0 || trackIndex >= m_model.trackList().size()
            || !m_beforeXml.contains(trackIndex))
            continue;
        int mltIndex = m_model.trackList().at(trackIndex).mlt_index;
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        if (!trackProducer || !trackProducer->is_valid())
            continue;
        Mlt::Playlist playlist(*trackProducer);
        QModelIndex modelIndex = m_model.index(trackIndex);

        // Parse the "before" snapshot into a self-contained shadow playlist so that any
        // transitions among its entries are wired to each other rather than to anything
        // currently on the live track.
        Mlt::Producer restoredTrack(MLT.profile(),
                                    "xml-string",
                                    m_beforeXml.value(trackIndex).toUtf8().constData());
        Mlt::Playlist restoredPlaylist(restoredTrack);

        int oldCount = playlist.count();
        if (oldCount > 0) {
            m_model.beginRemoveRows(modelIndex, 0, oldCount - 1);
            UNDOLOG << "clearing track" << trackIndex;
            playlist.clear();
            m_model.endRemoveRows();
        }

        int newCount = restoredPlaylist.count();
        if (newCount > 0) {
            m_model.beginInsertRows(modelIndex, 0, newCount - 1);
            for (int j = 0; j < newCount; ++j) {
                Mlt::ClipInfo clipInfo;
                restoredPlaylist.clip_info(j, &clipInfo);
                if (restoredPlaylist.is_blank(j)) {
                    playlist.insert_blank(j, clipInfo.frame_out - clipInfo.frame_in);
                } else {
                    QScopedPointer<Mlt::Producer> entry(restoredPlaylist.get_clip(j));
                    playlist.insert(*entry, j, clipInfo.frame_in, clipInfo.frame_out);
                }
            }
            m_model.endInsertRows();
        }

        // Restore uuids that were demoted to a temporary serializable property before the
        // snapshot was taken, and kick off audio levels rebuilds for the restored clips.
        for (int j = 0; j < playlist.count(); ++j) {
            QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
            if (!clip || !clip->is_valid())
                continue;
            if (playlist.is_blank(j)) {
                if (clip->get(kUuidPropertyTemp)) {
                    clip->set(kUuidProperty, clip->get(kUuidPropertyTemp));
                    clip->set(kUuidPropertyTemp, nullptr, 0);
                }
            } else {
                Mlt::Producer &parent = clip->parent();
                if (parent.get(kUuidPropertyTemp)) {
                    parent.set(kUuidProperty, parent.get(kUuidPropertyTemp));
                    parent.set(kUuidPropertyTemp, nullptr, 0);
                }
                AudioLevelsTask::start(parent, &m_model, m_model.createIndex(j, 0, trackIndex));
            }
        }
    }

    emit m_model.modified();
#ifdef UNDOHELPER_DEBUG
    debugPrintState("After undo");
#endif
}

void UndoHelper::debugPrintState(const QString &title)
{
    LOG_DEBUG() << "timeline state:" << title << "{";
    for (int i = 0; i < m_model.trackList().count(); ++i) {
        int mltIndex = m_model.trackList()[i].mlt_index;
        QString trackStr = QStringLiteral("   track %1 (mlt-idx %2):").arg(i).arg(mltIndex);
        QScopedPointer<Mlt::Producer> trackProducer(m_model.tractor()->track(mltIndex));
        Mlt::Playlist playlist(*trackProducer);

        for (int j = 0; j < playlist.count(); ++j) {
            Mlt::ClipInfo info;
            playlist.clip_info(j, &info);
            QUuid uid = MLT.uuid(*info.producer);
            if (info.producer->is_blank() && info.cut) {
                uid = MLT.uuid(*info.cut);
            }
            trackStr += QStringLiteral(" [ %5 %1 -> %2 (%3 frames) %4]")
                            .arg(info.frame_in)
                            .arg(info.frame_out)
                            .arg(info.frame_count)
                            .arg(info.cut->is_blank() ? "blank " : "clip")
                            .arg(uid.toString());
        }
        LOG_DEBUG() << qPrintable(trackStr);
    }
    LOG_DEBUG() << "}";
}

void UndoHelper::promoteUuids(Mlt::Playlist &playlist)
{
    // Copy the private, non-serialized uuid property to a temporary serializable property so
    // that it survives a round trip through MLT.XML().
    for (int j = 0; j < playlist.count(); ++j) {
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
        if (!clip || !clip->is_valid())
            continue;
        if (playlist.is_blank(j)) {
            QUuid uid = MLT.ensureHasUuid(*clip);
            clip->set(kUuidPropertyTemp, uid.toString().toUtf8().constData());
        } else {
            Mlt::Producer &parent = clip->parent();
            QUuid uid = MLT.ensureHasUuid(parent);
            parent.set(kUuidPropertyTemp, uid.toString().toUtf8().constData());
        }
    }
}

void UndoHelper::demoteUuids(Mlt::Playlist &playlist)
{
    // Clear the temporary serializable uuid property from the live objects so that it is
    // never left behind on anything other than a stored snapshot.
    for (int j = 0; j < playlist.count(); ++j) {
        QScopedPointer<Mlt::Producer> clip(playlist.get_clip(j));
        if (!clip || !clip->is_valid())
            continue;
        if (playlist.is_blank(j)) {
            clip->set(kUuidPropertyTemp, nullptr, 0);
        } else {
            clip->parent().set(kUuidPropertyTemp, nullptr, 0);
        }
    }
}
