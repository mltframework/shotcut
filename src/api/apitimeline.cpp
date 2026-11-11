/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "api/apitimeline.h"

#include "Logger.h"
#include "api/apiutil.h"
#include "api/transaction.h"
#include "commands/timelinecommands.h"
#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/markersmodel.h"
#include "models/multitrackmodel.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <Mlt.h>

#include <QUndoStack>
#include <QUuid>

#include <limits>

namespace Shotcut {
namespace Api {

ApiTimeline::ApiTimeline(QObject *parent)
    : ApiObject(parent)
{
    auto dock = MAIN.timelineDock();
    auto m = dock->model();
    connect(m, &MultitrackModel::created, this, &ApiTimeline::changed);
    connect(m, &MultitrackModel::closed, this, &ApiTimeline::changed);
    connect(m, &MultitrackModel::modified, this, &ApiTimeline::changed);
    connect(m, &MultitrackModel::durationChanged, this, &ApiTimeline::changed);
    connect(m, &QAbstractItemModel::modelReset, this, &ApiTimeline::changed);
    connect(dock, &TimelineDock::positionChanged, this, &ApiTimeline::positionChanged);
    connect(dock, &TimelineDock::currentTrackChanged, this, &ApiTimeline::currentTrackChanged);
    connect(dock, &TimelineDock::selectionChanged, this, &ApiTimeline::selectionChanged);
}

MultitrackModel *ApiTimeline::model() const
{
    return MAIN.timelineDock()->model();
}

bool ApiTimeline::exists() const
{
    return model()->tractor() != nullptr;
}

int ApiTimeline::trackCount() const
{
    return exists() ? model()->trackList().size() : 0;
}

int ApiTimeline::duration() const
{
    return exists() ? model()->tractor()->get_playtime() : 0;
}

int ApiTimeline::position() const
{
    return MAIN.timelineDock()->position();
}

int ApiTimeline::currentTrack() const
{
    return exists() ? MAIN.timelineDock()->currentTrack() : -1;
}

int ApiTimeline::selectedTrack() const
{
    return exists() ? MAIN.timelineDock()->selectedTrack() : -1;
}

QVariantList ApiTimeline::tracks() const
{
    QVariantList result;
    for (int i = 0; i < trackCount(); ++i)
        result.append(trackSnapshot(i));
    return result;
}

QVariant ApiTimeline::track(int trackIndex) const
{
    if (!isValidTrack(trackIndex))
        return fail(ErrorCode::NotFound, tr("Track %1 does not exist.").arg(trackIndex));
    return trackSnapshot(trackIndex);
}

QVariantList ApiTimeline::clips(const QVariantMap &options) const
{
    QVariantList result;
    if (!checkOptions("clips", options))
        return result;
    for (const auto &key : {"trackIndex", "start", "end"}) {
        if (options.contains(key)) {
            const double value = options.value(key).toDouble();
            if (value < 0 || value > std::numeric_limits<int>::max()) {
                fail(ErrorCode::InvalidArgument,
                     tr("%1 must be a nonnegative frame or index within integer range.")
                         .arg(QString::fromLatin1(key)));
                return result;
            }
        }
    }
    if (options.contains("start") && options.contains("end")
        && options.value("end").toInt() <= options.value("start").toInt()) {
        fail(ErrorCode::InvalidArgument, tr("Range end must be greater than start."));
        return result;
    }
    const int trackIndex = options.value("trackIndex", -1).toInt();
    const bool includeBlanks = options.value("includeBlanks", false).toBool();
    const bool includeTransitions = options.value("includeTransitions", true).toBool();
    const bool selectedOnly = options.value("selectedOnly", false).toBool();
    const auto selected = MAIN.timelineDock()->selection();
    const int start = options.value("start", std::numeric_limits<int>::min()).toInt();
    const int end = options.value("end", std::numeric_limits<int>::max()).toInt();
    if (options.contains("trackIndex") && !isValidTrack(trackIndex)) {
        fail(ErrorCode::NotFound, tr("Track %1 does not exist.").arg(trackIndex));
        return result;
    }
    const int first = trackIndex == -1 ? 0 : trackIndex;
    const int last = trackIndex == -1 ? trackCount() - 1 : trackIndex;
    for (int t = first; t <= last; ++t) {
        const int count = clipCount(t);
        for (int c = 0; c < count; ++c) {
            const auto index = model()->makeIndex(t, c);
            if (!includeBlanks && model()->data(index, MultitrackModel::IsBlankRole).toBool())
                continue;
            if (!includeTransitions
                && model()->data(index, MultitrackModel::IsTransitionRole).toBool())
                continue;
            if (selectedOnly && !selected.contains(QPoint(c, t)))
                continue;
            const int clipStart = model()->data(index, MultitrackModel::StartRole).toInt();
            const qint64 clipEnd = qint64(clipStart)
                                   + model()->data(index, MultitrackModel::DurationRole).toInt();
            if (clipEnd <= start || clipStart >= end)
                continue;
            result.append(clipSnapshot(t, c));
        }
    }
    return result;
}

QStringList ApiTimeline::clipIds(const QVariantMap &options) const
{
    QStringList result;
    if (!checkOptions("clipIds", options))
        return result;
    auto query = options;
    query.insert("includeBlanks", false);
    query.insert("includeTransitions", false);
    for (const auto &clip : clips(query))
        result.append(clip.toMap().value("id").toString());
    return result;
}

QVariant ApiTimeline::clip(int trackIndex, int clipIndex) const
{
    if (!isValidTrack(trackIndex))
        return fail(ErrorCode::NotFound, tr("Track %1 does not exist.").arg(trackIndex));
    if (clipIndex < 0 || clipIndex >= clipCount(trackIndex))
        return fail(ErrorCode::NotFound,
                    tr("Clip %1 does not exist on track %2.").arg(clipIndex).arg(trackIndex));
    return clipSnapshot(trackIndex, clipIndex);
}

QVariant ApiTimeline::clipAt(int trackIndex, int position) const
{
    if (!isValidTrack(trackIndex))
        return fail(ErrorCode::NotFound, tr("Track %1 does not exist.").arg(trackIndex));
    const int clipIndex = model()->clipIndex(trackIndex, position);
    if (clipIndex < 0 || clipIndex >= clipCount(trackIndex))
        return QVariant();
    return clipSnapshot(trackIndex, clipIndex);
}

QVariant ApiTimeline::findClip(const QString &id) const
{
    const QUuid uuid = QUuid::fromString(id);
    if (uuid.isNull())
        return fail(ErrorCode::InvalidArgument, tr("Invalid clip id: %1").arg(id));
    if (!exists())
        return QVariant();
    int trackIndex = -1;
    int clipIndex = -1;
    if (!model()->findClipByUuid(uuid, trackIndex, clipIndex))
        return QVariant();
    return clipSnapshot(trackIndex, clipIndex);
}

QVariantList ApiTimeline::selection() const
{
    QVariantList result;
    if (!exists())
        return result;
    for (const auto &point : MAIN.timelineDock()->selection()) {
        if (isValidTrack(point.y()) && point.x() >= 0 && point.x() < clipCount(point.y()))
            result.append(clipSnapshot(point.y(), point.x()));
    }
    return result;
}

QVariant ApiTimeline::appendClip(const QVariantMap &options)
{
    return addClip("appendClip", options);
}

QVariant ApiTimeline::insertClip(const QVariantMap &options)
{
    return addClip("insertClip", options);
}

QVariant ApiTimeline::overwriteClip(const QVariantMap &options)
{
    return addClip("overwriteClip", options);
}

QVariant ApiTimeline::addClip(const char *method, const QVariantMap &options)
{
    ApiCallGuard callGuard;
    if (!checkOptions(method, options))
        return QVariant();
    const int t = options.value("trackIndex").toInt();
    if (!checkTrack(t) || !checkUnlocked(t))
        return QVariant();
    const bool append = QByteArray(method) == "appendClip";
    const bool insert = QByteArray(method) == "insertClip";
    const int trackDuration = trackSnapshot(t).value("duration").toInt();
    const bool placeholder = clipCount(t) == 1 && blankDuration(t, 0) == 1;
    const int position = append ? (placeholder ? 0 : trackDuration)
                                : options.value("position").toInt();
    if (position < 0)
        return fail(ErrorCode::InvalidArgument, tr("Position cannot be negative."));
    QString errorCode, errorMessage;
    auto producer = prepareMedia(options, errorCode, errorMessage);
    if (!producer)
        return fail(errorCode, errorMessage);
    const int in = producer->get_in();
    const int out = producer->get_out();
    const int length = producer->get_playtime();
    if (length <= 0 || position > std::numeric_limits<int>::max() - length
        || (insert && trackDuration > std::numeric_limits<int>::max() - length))
        return fail(ErrorCode::InvalidArgument, tr("The resulting timeline duration is too large."));
    const bool allTracks = options.value("rippleAllTracks", false).toBool();
    if (insert && options.value("rippleMarkers", false).toBool()) {
        for (const auto &marker : MAIN.timelineDock()->markersModel()->getMarkers()) {
            if (marker.start >= position
                && (marker.start > std::numeric_limits<int>::max() - length
                    || marker.end > std::numeric_limits<int>::max() - length))
                return fail(ErrorCode::InvalidArgument,
                            tr("The resulting marker position is too large."));
        }
    }
    const auto isTransition = [&](int track, int clip) {
        return clip >= 0 && clip < clipCount(track)
               && model()
                      ->data(model()->makeIndex(track, clip), MultitrackModel::IsTransitionRole)
                      .toBool();
    };
    if (!append) {
        for (int track = 0; track < trackCount(); ++track) {
            if (track != t && (!insert || !allTracks || MAIN.timelineDock()->isTrackLocked(track)))
                continue;
            if (insert
                && trackSnapshot(track).value("duration").toInt()
                       > std::numeric_limits<int>::max() - length)
                return fail(ErrorCode::InvalidArgument,
                            tr("The resulting timeline duration is too large."));
            for (int c = 0; c < clipCount(track); ++c) {
                const auto clip = clipSnapshot(track, c);
                const int start = clip.value("start").toInt();
                const int end = start + clip.value("duration").toInt();
                const bool affected = insert ? position >= start && position < end
                                             : position < end && position + length > start;
                if (affected
                    && (isTransition(track, c) || isTransition(track, c - 1)
                        || ((!insert || position > start) && isTransition(track, c + 1))))
                    return fail(ErrorCode::Unavailable,
                                tr("This edit would split or overwrite a transition or a clip "
                                   "joined to one."));
            }
        }
    }
    QSet<QString> existingIds;
    for (const auto &value : clips())
        existingIds.insert(value.toMap().value("id").toString());
    const QString xml = MLT.XML(producer.get());
    Transaction::singleton().touch();
    MAIN.timelineDock()->setSelection();
    if (append)
        MAIN.undoStack()->push(new Timeline::AppendCommand(*model(), t, xml, true, false));
    else if (insert)
        MAIN.undoStack()->push(
            new Timeline::InsertCommand(*model(),
                                        *MAIN.timelineDock()->markersModel(),
                                        t,
                                        position,
                                        xml,
                                        false,
                                        allTracks,
                                        options.value("rippleMarkers", false).toBool()));
    else
        MAIN.undoStack()->push(new Timeline::OverwriteCommand(*model(), t, position, xml, false));
    MAIN.timelineDock()->setSelection();
    const int resultIndex = model()->clipIndex(t, position);
    const auto added = resultIndex >= 0 && resultIndex < clipCount(t) ? clipSnapshot(t, resultIndex)
                                                                      : QVariantMap();
    if (added.value("id").toString().isEmpty() || existingIds.contains(added.value("id").toString())
        || added.value("isBlank").toBool() || added.value("isTransition").toBool()
        || added.value("start").toInt() != position || added.value("duration").toInt() != length
        || added.value("in").toInt() != in || added.value("out").toInt() != out) {
        const QString error = tr(
            "The clip could not be added as requested; the edits are rolled back.");
        LOG_ERROR() << method << "expected" << t << position << in << out << "actual" << added;
        Transaction::singleton().fail(error);
        return fail(ErrorCode::Unavailable, error);
    }
    return added;
}

QVariant ApiTimeline::removeClip(const QString &clipId, const QVariantMap &options)
{
    if (!checkOptions("removeClip", options))
        return QVariant();
    int t, c;
    if (!findEditableClip(clipId, &t, &c))
        return QVariant();
    const QVariantMap removed = clipSnapshot(t, c);
    auto dock = MAIN.timelineDock();
    Transaction::singleton().touch();
    dock->setSelection();
    if (options.value("ripple", false).toBool())
        dock->removeWithRipplePolicy(t, c, false, false, false);
    else
        dock->liftWithRipplePolicy(t, c, false, false, false);
    return removed;
}

QVariant ApiTimeline::splitClip(const QString &clipId, int position)
{
    int t, c;
    if (!findEditableClip(clipId, &t, &c))
        return QVariant();
    const auto index = model()->makeIndex(t, c);
    if (model()->data(index, MultitrackModel::IsTransitionRole).toBool())
        return fail(ErrorCode::InvalidArgument, tr("Transitions cannot be split."));
    const int start = model()->data(index, MultitrackModel::StartRole).toInt();
    const int duration = model()->data(index, MultitrackModel::DurationRole).toInt();
    if (position <= start || position >= start + duration)
        return fail(ErrorCode::InvalidArgument,
                    tr("Position %1 is not inside the clip (frames %2 to %3).")
                        .arg(position)
                        .arg(start + 1)
                        .arg(start + duration - 1));
    Transaction::singleton().touch();
    if (!MAIN.timelineDock()->split(t, c, position))
        return fail(ErrorCode::Unavailable, tr("The clip could not be split."));
    QVariantMap result;
    result["left"] = clipSnapshot(t, c);
    result["right"] = clipSnapshot(t, c + 1);
    return result;
}

QVariant ApiTimeline::moveClip(const QString &clipId, const QVariantMap &options)
{
    if (!checkOptions("moveClip", options))
        return QVariant();
    if (!options.contains("position") && !options.contains("trackIndex"))
        return fail(ErrorCode::InvalidArgument, tr("Give a position, a trackIndex, or both."));
    int t, c;
    if (!findEditableClip(clipId, &t, &c))
        return QVariant();
    const auto m = model();
    const auto index = m->makeIndex(t, c);
    if (m->data(index, MultitrackModel::IsTransitionRole).toBool())
        return fail(ErrorCode::InvalidArgument, tr("Transitions cannot be moved."));
    const auto isTransitionAt = [&](int trackIndex, int clipIndex) {
        return clipIndex >= 0 && clipIndex < clipCount(trackIndex)
               && m->data(m->makeIndex(trackIndex, clipIndex), MultitrackModel::IsTransitionRole)
                      .toBool();
    };
    if (isTransitionAt(t, c - 1) || isTransitionAt(t, c + 1))
        return fail(ErrorCode::Unavailable,
                    tr("Clips joined to a transition cannot be moved by the API yet."));

    const int start = m->data(index, MultitrackModel::StartRole).toInt();
    const int duration = m->data(index, MultitrackModel::DurationRole).toInt();
    const int toTrack = options.value("trackIndex", t).toInt();
    const int position = options.value("position", start).toInt();
    const bool ripple = options.value("ripple", false).toBool();
    if (!checkTrack(toTrack) || !checkUnlocked(toTrack))
        return QVariant();
    if (m->trackList().at(toTrack).type != m->trackList().at(t).type)
        return fail(ErrorCode::InvalidArgument,
                    tr("Clips can only move between tracks of the same type."));
    if (position < 0)
        return fail(ErrorCode::InvalidArgument, tr("Position cannot be negative."));
    if (toTrack == t && position == start)
        return clipSnapshot(t, c);
    if (ripple) {
        const int at = m->clipIndex(toTrack, position);
        if (isTransitionAt(toTrack, at) && !(toTrack == t && at == c))
            return fail(ErrorCode::InvalidArgument, tr("The destination is inside a transition."));
    } else if (!checkClipFits(toTrack, position, duration, toTrack == t ? c : -1)) {
        return QVariant();
    }

    auto dock = MAIN.timelineDock();
    Transaction::singleton().touch();
    dock->setSelection();
    auto command
        = new Timeline::MoveClipCommand(*dock, toTrack - t, position - start, ripple, false, false);
    command->addClip(t, c);
    MAIN.undoStack()->push(command);
    return clipSnapshotById(clipId);
}

QVariant ApiTimeline::trimClip(const QString &clipId, const QVariantMap &options)
{
    if (!checkOptions("trimClip", options))
        return QVariant();
    const int startDelta = options.value("start", 0).toInt();
    const int endDelta = options.value("end", 0).toInt();
    const bool ripple = options.value("ripple", false).toBool();
    if (!startDelta && !endDelta)
        return fail(ErrorCode::InvalidArgument, tr("Give a nonzero start or end."));
    int t, c;
    if (!findEditableClip(clipId, &t, &c))
        return QVariant();
    const auto m = model();
    if (m->data(m->makeIndex(t, c), MultitrackModel::IsTransitionRole).toBool())
        return fail(ErrorCode::InvalidArgument, tr("Transitions cannot be trimmed."));
    auto info = m->getClipInfo(t, c);
    if (!info)
        return fail(ErrorCode::NotFound, tr("Clip %1 was not found.").arg(clipId));
    if (startDelta + endDelta >= info->frame_count)
        return fail(ErrorCode::InvalidArgument,
                    tr("The clip is %1 frames long; at least 1 frame must remain.")
                        .arg(info->frame_count));
    if (startDelta < 0 && info->frame_in + startDelta < 0)
        return fail(ErrorCode::InvalidArgument,
                    tr("Only %1 unused frames are available before the clip.").arg(info->frame_in));
    if (endDelta < 0 && info->frame_out - endDelta >= info->length)
        return fail(ErrorCode::InvalidArgument,
                    tr("Only %1 unused frames are available after the clip.")
                        .arg(info->length - 1 - info->frame_out));
    if (endDelta && !m->trimClipOutValid(t, c, endDelta, ripple))
        return fail(ErrorCode::InvalidArgument,
                    tr("The end cannot be trimmed there; without ripple, lengthening needs a gap "
                       "after the clip."));
    if (startDelta && !m->trimClipInValid(t, c, startDelta, ripple))
        return fail(ErrorCode::InvalidArgument,
                    tr("The start cannot be trimmed there; without ripple, lengthening needs a "
                       "gap before the clip."));
    if (!ripple && endDelta < 0 && c + 1 < clipCount(t) && blankDuration(t, c + 1) < -endDelta)
        return fail(ErrorCode::InvalidArgument,
                    tr("The gap after the clip is only %1 frames.").arg(blankDuration(t, c + 1)));
    if (!ripple && startDelta < 0 && blankDuration(t, c - 1) < -startDelta)
        return fail(ErrorCode::InvalidArgument,
                    tr("The gap before the clip is only %1 frames.").arg(blankDuration(t, c - 1)));

    auto dock = MAIN.timelineDock();
    Transaction::singleton().touch();
    dock->setSelection();
    if (endDelta)
        MAIN.undoStack()->push(new Timeline::TrimClipOutCommand(
            *m, *dock->markersModel(), t, c, endDelta, ripple, false, false));
    if (startDelta) {
        if (!m->findClipByUuid(QUuid::fromString(clipId), t, c))
            return fail(ErrorCode::NotFound, tr("Clip %1 was lost while trimming.").arg(clipId));
        MAIN.undoStack()->push(new Timeline::TrimClipInCommand(
            *m, *dock->markersModel(), t, c, startDelta, ripple, false, false));
    }
    return clipSnapshotById(clipId);
}

QVariant ApiTimeline::updateClip(const QString &clipId, const QVariantMap &options)
{
    if (!checkOptions("updateClip", options))
        return QVariant();
    if (options.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Nothing to change."));
    int t, c;
    if (!findEditableClip(clipId, &t, &c))
        return QVariant();
    const auto index = model()->makeIndex(t, c);
    if (model()->data(index, MultitrackModel::IsTransitionRole).toBool())
        return fail(ErrorCode::InvalidArgument, tr("Transitions do not have fades."));
    const int duration = model()->data(index, MultitrackModel::DurationRole).toInt();
    for (const char *key : {"fadeIn", "fadeOut"}) {
        if (!options.contains(key))
            continue;
        const int frames = options.value(key).toInt();
        if (frames < 0 || frames > duration)
            return fail(ErrorCode::InvalidArgument,
                        tr("%1 must be from 0 to the clip duration (%2).")
                            .arg(QLatin1String(key))
                            .arg(duration));
    }
    Transaction::singleton().touch();
    if (options.contains("fadeIn"))
        MAIN.undoStack()->push(
            new Timeline::FadeInCommand(*model(), t, c, options.value("fadeIn").toInt()));
    if (options.contains("fadeOut"))
        MAIN.undoStack()->push(
            new Timeline::FadeOutCommand(*model(), t, c, options.value("fadeOut").toInt()));
    return clipSnapshot(t, c);
}

QVariant ApiTimeline::addTrack(const QVariantMap &options)
{
    if (!checkOptions("addTrack", options))
        return QVariant();
    if (!exists())
        return fail(ErrorCode::Unavailable, tr("There is no timeline."));
    const QString type = options.value("type").toString();
    if (type != "video" && type != "audio")
        return fail(ErrorCode::InvalidArgument, tr("type must be \"video\" or \"audio\"."));
    auto dock = MAIN.timelineDock();
    Transaction::singleton().touch();
    const int trackIndex = type == "video" ? dock->addVideoTrack() : dock->addAudioTrack();
    if (options.contains("name"))
        MAIN.undoStack()->push(
            new Timeline::NameTrackCommand(*model(), trackIndex, options.value("name").toString()));
    return trackSnapshot(trackIndex);
}

QVariant ApiTimeline::updateTrack(int trackIndex, const QVariantMap &options)
{
    if (!checkOptions("updateTrack", options))
        return QVariant();
    if (options.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Nothing to change."));
    if (!checkTrack(trackIndex))
        return QVariant();
    const auto m = model();
    const auto index = m->index(trackIndex);
    const bool isAudio = m->data(index, MultitrackModel::IsAudioRole).toBool();
    if (isAudio && (options.contains("hidden") || options.contains("composite")))
        return fail(ErrorCode::InvalidArgument,
                    tr("hidden and composite apply only to video tracks."));

    Transaction::singleton().touch();
    if (options.contains("name"))
        MAIN.undoStack()->push(
            new Timeline::NameTrackCommand(*m, trackIndex, options.value("name").toString()));
    if (options.contains("mute")
        && options.value("mute").toBool() != m->data(index, MultitrackModel::IsMuteRole).toBool())
        MAIN.undoStack()->push(new Timeline::MuteTrackCommand(*m, trackIndex));
    if (options.contains("hidden")
        && options.value("hidden").toBool()
               != m->data(index, MultitrackModel::IsHiddenRole).toBool())
        MAIN.undoStack()->push(new Timeline::HideTrackCommand(*m, trackIndex));
    if (options.contains("composite")
        && options.value("composite").toBool()
               != m->data(index, MultitrackModel::IsCompositeRole).toBool())
        MAIN.undoStack()->push(
            new Timeline::CompositeTrackCommand(*m,
                                                trackIndex,
                                                options.value("composite").toBool()));
    if (options.contains("locked")
        && options.value("locked").toBool()
               != m->data(index, MultitrackModel::IsLockedRole).toBool())
        MAIN.undoStack()->push(
            new Timeline::LockTrackCommand(*m, trackIndex, options.value("locked").toBool()));
    return trackSnapshot(trackIndex);
}

QVariant ApiTimeline::removeTrack(int trackIndex)
{
    if (!checkTrack(trackIndex) || !checkUnlocked(trackIndex))
        return QVariant();
    const QVariantMap removed = trackSnapshot(trackIndex);
    auto dock = MAIN.timelineDock();
    Transaction::singleton().touch();
    dock->setSelection();
    MAIN.undoStack()->push(new Timeline::RemoveTrackCommand(*model(), trackIndex));
    if (dock->currentTrack() >= trackCount())
        dock->setCurrentTrack(trackCount() - 1);
    return removed;
}

QVariant ApiTimeline::seek(int position)
{
    if (!exists())
        return fail(ErrorCode::Unavailable, tr("There is no timeline."));
    if (position < 0)
        return fail(ErrorCode::InvalidArgument, tr("Position cannot be negative."));
    position = qMin(position, duration());
    MAIN.seekTimeline(position);
    if (!MLT.isMultitrack())
        return fail(ErrorCode::Unavailable, tr("The timeline is not loaded in the player."));
    return position;
}

QVariant ApiTimeline::setSelection(const QStringList &clipIds)
{
    if (!exists())
        return fail(ErrorCode::Unavailable, tr("There is no timeline."));
    QList<QPoint> points;
    for (const auto &id : clipIds) {
        const QUuid uuid = QUuid::fromString(id);
        if (uuid.isNull())
            return fail(ErrorCode::InvalidArgument, tr("Invalid clip id: %1").arg(id));
        int t, c;
        if (!model()->findClipByUuid(uuid, t, c))
            return fail(ErrorCode::NotFound, tr("Clip %1 is not in the timeline.").arg(id));
        if (model()->data(model()->makeIndex(t, c), MultitrackModel::IsBlankRole).toBool())
            return fail(ErrorCode::InvalidArgument, tr("Clip %1 is a gap, not a clip.").arg(id));
        const QPoint point(c, t);
        if (!points.contains(point))
            points.append(point);
    }
    MAIN.timelineDock()->setSelection(points);
    return selection();
}

QVariant ApiTimeline::addTransitions(const QStringList &clipIds, const QVariantMap &options)
{
    ApiCallGuard callGuard;
    if (!checkOptions("addTransitions", options))
        return QVariant();
    const int duration = options.value("duration").toInt();
    const QString mode = options.value("mode", "overlap").toString();
    const bool preserve = mode == QLatin1String("preserve");
    const bool ripple = options.value("ripple", false).toBool();
    const bool allTracks = options.value("rippleAllTracks", false).toBool();
    if (duration <= 0 || (mode != QLatin1String("overlap") && !preserve))
        return fail(ErrorCode::InvalidArgument,
                    tr("Use a positive duration and overlap or preserve mode."));
    if (allTracks && !ripple)
        return fail(ErrorCode::InvalidArgument, tr("Ripple All Tracks requires Ripple."));
    if (!exists())
        return fail(ErrorCode::Unavailable, tr("There is no timeline."));

    QVariantList created, skipped;
    QSet<QUuid> selected;
    for (const auto &id : clipIds) {
        const QUuid uuid(id);
        if (uuid.isNull())
            return fail(ErrorCode::InvalidArgument, tr("Invalid clip id: %1").arg(id));
        selected.insert(uuid);
    }
    struct Pair
    {
        QString left;
        QString right;
    };
    QList<Pair> pairs;
    for (int t = 0; t < trackCount(); ++t) {
        QString previous;
        for (int c = 0; c < clipCount(t); ++c) {
            const auto clip = clipSnapshot(t, c);
            if (selected.remove(QUuid(clip.value("id").toString()))) {
                const QString id = clip.value("id").toString();
                if (!previous.isEmpty())
                    pairs.append({previous, id});
                previous = id;
            }
        }
    }
    for (const auto &id : selected) {
        const QString reason = tr("Selected clip is no longer in the timeline.");
        skipped.append(
            QVariantMap{{"clipId", id.toString(QUuid::WithoutBraces)}, {"reason", reason}});
    }
    for (const auto &pair : pairs) {
        int t = -1, l = -1, rt = -1, r = -1;
        QString reason;
        const auto m = model();
        if (!m->findClipByUuid(QUuid(pair.left), t, l)
            || !m->findClipByUuid(QUuid(pair.right), rt, r) || t != rt || r <= l) {
            reason = tr("The pair is no longer on the same track in order.");
        } else if (MAIN.timelineDock()->isTrackLocked(t)) {
            reason = tr("The track is locked.");
        } else {
            for (int c = l; c <= r; ++c) {
                const auto index = m->makeIndex(t, c);
                if ((c == l || c == r)
                        ? (m->data(index, MultitrackModel::IsBlankRole).toBool()
                           || m->data(index, MultitrackModel::IsTransitionRole).toBool()
                           || m->data(index, MultitrackModel::IsAdjustmentRole).toBool())
                        : !m->data(index, MultitrackModel::IsBlankRole).toBool()) {
                    reason = tr("Only ordinary clips with no intervening clip or transition can be "
                                "joined.");
                    break;
                }
            }
            const auto left = clipSnapshot(t, l);
            const auto right = clipSnapshot(t, r);
            if (reason.isEmpty() && r > l + 1 && !options.value("closeGaps", true).toBool())
                reason = tr("The pair has a gap; enable Close Gaps.");
            if (reason.isEmpty()
                && (duration >= left.value("duration").toInt()
                    || (!preserve && duration >= right.value("duration").toInt())))
                reason = tr("The transition must leave at least one frame in each affected clip.");
            if (reason.isEmpty() && preserve && right.value("in").toInt() < duration)
                reason = tr(
                    "The right clip has insufficient spare source frames before its in point.");
            if (reason.isEmpty() && !preserve
                && !m->addTransitionValid(t,
                                          t,
                                          r,
                                          left.value("start").toInt()
                                              + left.value("duration").toInt() - duration,
                                          ripple))
                reason = tr("Shotcut cannot create this overlap.");
        }
        if (!reason.isEmpty()) {
            skipped.append(
                QVariantMap{{"leftId", pair.left}, {"rightId", pair.right}, {"reason", reason}});
            continue;
        }
        const auto leftBefore = clipSnapshot(t, l);
        const auto rightBefore = clipSnapshot(t, r);
        const int boundary = leftBefore.value("start").toInt()
                             + leftBefore.value("duration").toInt();
        Transaction::singleton().touch(tr("Add transitions"));
        MAIN.timelineDock()->setSelection();
        auto command = new Timeline::CreateTransitionCommand(*m,
                                                             t,
                                                             l,
                                                             r,
                                                             duration,
                                                             preserve,
                                                             ripple,
                                                             allTracks);
        MAIN.undoStack()->push(command);
        const int index = command->transitionIndex();
        int leftTrack = -1, leftIndex = -1, rightTrack = -1, rightIndex = -1;
        const auto transition = index >= 0 ? clipSnapshot(t, index) : QVariantMap();
        if (index < 0 || !transition.value("isTransition").toBool()
            || transition.value("duration").toInt() != duration
            || transition.value("start").toInt() != boundary - duration
            || !m->findClipByUuid(QUuid(pair.left), leftTrack, leftIndex)
            || !m->findClipByUuid(QUuid(pair.right), rightTrack, rightIndex) || leftTrack != t
            || rightTrack != t || leftIndex + 1 != index || rightIndex != index + 1
            || clipSnapshot(t, leftIndex).value("duration").toInt()
                   != leftBefore.value("duration").toInt() - duration
            || clipSnapshot(t, rightIndex).value("duration").toInt()
                   != rightBefore.value("duration").toInt() - (preserve ? 0 : duration)
            || clipSnapshot(t, leftIndex).value("out").toInt()
                   != leftBefore.value("out").toInt() - duration
            || clipSnapshot(t, rightIndex).value("in").toInt()
                   != rightBefore.value("in").toInt() + (preserve ? 0 : duration)) {
            const QString error = tr(
                "Unexpected failure creating a transition; the batch is rolled back.");
            Transaction::singleton().fail(error);
            return fail(ErrorCode::Unavailable, error);
        }
        created.append(
            QVariantMap{{"leftId", pair.left}, {"rightId", pair.right}, {"transition", transition}});
    }
    if (!skipped.isEmpty())
        LOG_WARNING() << "addTransitions: skipped" << skipped.size()
                      << "clip(s) or pair(s); see the returned skipped array for details.";
    return QVariantMap{{"created", created}, {"skipped", skipped}};
}

QVariant ApiTimeline::setCurrentTrack(int trackIndex)
{
    if (!checkTrack(trackIndex))
        return QVariant();
    MAIN.timelineDock()->setCurrentTrack(trackIndex);
    return trackSnapshot(trackIndex);
}

bool ApiTimeline::findEditableClip(const QString &clipId, int *trackIndex, int *clipIndex) const
{
    if (!exists()) {
        fail(ErrorCode::Unavailable, tr("There is no timeline."));
        return false;
    }
    const QUuid uuid = QUuid::fromString(clipId);
    if (uuid.isNull()) {
        fail(ErrorCode::InvalidArgument, tr("Invalid clip id: %1").arg(clipId));
        return false;
    }
    if (!model()->findClipByUuid(uuid, *trackIndex, *clipIndex)) {
        fail(ErrorCode::NotFound, tr("Clip %1 is not in the timeline.").arg(clipId));
        return false;
    }
    if (model()
            ->data(model()->makeIndex(*trackIndex, *clipIndex), MultitrackModel::IsBlankRole)
            .toBool()) {
        fail(ErrorCode::InvalidArgument, tr("Clip %1 is a gap, not a clip.").arg(clipId));
        return false;
    }
    return checkUnlocked(*trackIndex);
}

bool ApiTimeline::checkTrack(int trackIndex) const
{
    if (!exists()) {
        fail(ErrorCode::Unavailable, tr("There is no timeline."));
        return false;
    }
    if (!isValidTrack(trackIndex)) {
        fail(ErrorCode::NotFound, tr("Track %1 does not exist.").arg(trackIndex));
        return false;
    }
    return true;
}

bool ApiTimeline::checkUnlocked(int trackIndex) const
{
    if (MAIN.timelineDock()->isTrackLocked(trackIndex)) {
        fail(ErrorCode::Unavailable, tr("Track %1 is locked.").arg(trackIndex));
        return false;
    }
    return true;
}

bool ApiTimeline::checkClipFits(int trackIndex, int position, int duration, int ignoreClipIndex) const
{
    const auto m = model();
    const int count = clipCount(trackIndex);
    for (int i = 0; i < count; ++i) {
        if (i == ignoreClipIndex)
            continue;
        const auto index = m->makeIndex(trackIndex, i);
        if (m->data(index, MultitrackModel::IsBlankRole).toBool())
            continue;
        const int start = m->data(index, MultitrackModel::StartRole).toInt();
        const int end = start + m->data(index, MultitrackModel::DurationRole).toInt();
        if (start < position + duration && end > position) {
            fail(ErrorCode::InvalidArgument,
                 tr("Frames %1 to %2 on track %3 are not empty; use ripple to insert.")
                     .arg(position)
                     .arg(position + duration - 1)
                     .arg(trackIndex));
            return false;
        }
    }
    return true;
}

int ApiTimeline::blankDuration(int trackIndex, int clipIndex) const
{
    if (clipIndex < 0 || clipIndex >= clipCount(trackIndex))
        return 0;
    const auto index = model()->makeIndex(trackIndex, clipIndex);
    if (!model()->data(index, MultitrackModel::IsBlankRole).toBool())
        return 0;
    return model()->data(index, MultitrackModel::DurationRole).toInt();
}

QVariant ApiTimeline::clipSnapshotById(const QString &clipId) const
{
    int t, c;
    if (!model()->findClipByUuid(QUuid::fromString(clipId), t, c))
        return QVariant();
    return clipSnapshot(t, c);
}

bool ApiTimeline::isValidTrack(int trackIndex) const
{
    return trackIndex >= 0 && trackIndex < trackCount();
}

int ApiTimeline::clipCount(int trackIndex) const
{
    return model()->rowCount(model()->index(trackIndex));
}

QVariantMap ApiTimeline::trackSnapshot(int trackIndex) const
{
    const auto m = model();
    const QModelIndex index = m->index(trackIndex);
    const bool isAudio = m->data(index, MultitrackModel::IsAudioRole).toBool();
    QVariantMap result;
    result["index"] = trackIndex;
    result["name"] = m->data(index, MultitrackModel::NameRole).toString();
    result["type"] = isAudio ? QStringLiteral("audio") : QStringLiteral("video");
    result["duration"] = m->data(index, MultitrackModel::DurationRole).toInt();
    result["clipCount"] = clipCount(trackIndex);
    result["mute"] = m->data(index, MultitrackModel::IsMuteRole).toBool();
    result["locked"] = m->data(index, MultitrackModel::IsLockedRole).toBool();
    result["gain"] = m->data(index, MultitrackModel::GainRole).toDouble();
    if (!isAudio) {
        result["hidden"] = m->data(index, MultitrackModel::IsHiddenRole).toBool();
        result["composite"] = m->data(index, MultitrackModel::IsCompositeRole).toBool();
    }
    return result;
}

QVariantMap ApiTimeline::clipSnapshot(int trackIndex, int clipIndex) const
{
    const auto m = model();
    const QModelIndex index = m->makeIndex(trackIndex, clipIndex);
    const bool isBlank = m->data(index, MultitrackModel::IsBlankRole).toBool();
    QVariantMap result;
    auto info = m->getClipInfo(trackIndex, clipIndex);
    if (info)
        result["id"] = clipId(*info);
    result["trackIndex"] = trackIndex;
    result["clipIndex"] = clipIndex;
    result["start"] = m->data(index, MultitrackModel::StartRole).toInt();
    result["duration"] = m->data(index, MultitrackModel::DurationRole).toInt();
    result["isBlank"] = isBlank;
    if (isBlank)
        return result;

    result["in"] = m->data(index, MultitrackModel::InPointRole).toInt();
    result["out"] = m->data(index, MultitrackModel::OutPointRole).toInt();
    if (info && info->producer && info->producer->is_valid()) {
        result["name"] = Util::clipDisplayName(info->producer);
        result["length"] = info->producer->get_length();
        result["isProxy"] = info->producer->get_int(kIsProxyProperty) != 0;
    }
    result["service"] = m->data(index, MultitrackModel::ServiceRole).toString();
    result["resource"] = m->data(index, MultitrackModel::ResourceRole).toString();
    result["comment"] = m->data(index, MultitrackModel::CommentRole).toString();
    result["speed"] = m->data(index, MultitrackModel::SpeedRole).toDouble();
    result["fadeIn"] = m->data(index, MultitrackModel::FadeInRole).toInt();
    result["fadeOut"] = m->data(index, MultitrackModel::FadeOutRole).toInt();
    result["group"] = m->data(index, MultitrackModel::GroupRole).toInt();
    result["isTransition"] = m->data(index, MultitrackModel::IsTransitionRole).toBool();
    result["isFiltered"] = m->data(index, MultitrackModel::IsFilteredRole).toBool();
    return result;
}

} // namespace Api
} // namespace Shotcut
