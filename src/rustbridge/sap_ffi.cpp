/*
 * SAP (Snapshot App Protocol) FFI shim implementation. See sap_ffi.h for the
 * overall design note. Every function here that touches Qt/MLT state
 * crosses to the Qt main thread via
 * QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection) -- this is
 * the load-bearing threading rule from
 * memory/head/gen/rust-fork/02-rust-embedding.md, not a style choice.
 */

#include "sap_ffi.h"

#include "commands/timelinecommands.h"
#include "docks/timelinedock.h"
#include "docks/playlistdock.h"
#include "models/playlistmodel.h"
#include "models/markersmodel.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"
#include "player.h"

#include <Mlt.h>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QScopedPointer>
#include <QThread>
#include <QString>
#include <QUndoStack>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace {

MainWindow *mainWindowFromHandle(void *handle)
{
    return reinterpret_cast<MainWindow *>(handle);
}

QString trackKindString(TrackType type)
{
    switch (type) {
    case VideoTrackType:
        return QStringLiteral("video");
    case AudioTrackType:
        return QStringLiteral("audio");
    default:
        return QStringLiteral("other");
    }
}

char *newCString(const QByteArray &bytes)
{
    char *buffer = static_cast<char *>(std::malloc(static_cast<size_t>(bytes.size()) + 1));
    if (!buffer)
        return nullptr;
    std::memcpy(buffer, bytes.constData(), static_cast<size_t>(bytes.size()));
    buffer[bytes.size()] = '\0';
    return buffer;
}

// Duplicates TrackPropertiesWidget::getTransition()'s transition-chain walk
// (trackpropertieswidget.cpp) rather than calling MultitrackModel's private
// getVideoBlendTransition() of the same shape -- per sap_ffi.h/.cpp's
// "only new files" constraint, this file may not add a friend declaration
// or new public method to multitrackmodel.h. `trackProducer` is the
// specific track's own Mlt::Producer (model->tractor()->track(mltIndex)),
// exactly like TrackPropertiesWidget's m_track.
Mlt::Transition *findTrackBlendTransition(Mlt::Producer &trackProducer, const QString &name)
{
    QScopedPointer<Mlt::Service> service(trackProducer.consumer());
    if (service && service->is_valid()) {
        Mlt::Multitrack multi(*service);
        int trackIndex;
        for (trackIndex = 0; trackIndex < multi.count(); ++trackIndex) {
            QScopedPointer<Mlt::Producer> producer(multi.track(trackIndex));
            if (producer->get_producer() == trackProducer.get_producer())
                break;
        }
        while (service && service->is_valid() && mlt_service_tractor_type != service->type()) {
            if (service->type() == mlt_service_transition_type) {
                Mlt::Transition t((mlt_transition) service->get_service());
                if (name == t.get("mlt_service") && t.get_b_track() == trackIndex)
                    return new Mlt::Transition(t);
            }
            service.reset(service->consumer());
        }
    }
    return nullptr;
}

// Same qtblend -> movit.overlay -> cairoblend fallback order as
// TrackPropertiesWidget's constructor. Returns the transition (caller
// owns it) and, via `isCairoblend`, which property name/value space
// applies (BLEND_PROPERTY_QTBLEND's numeric "compositing" property vs
// BLEND_PROPERTY_CAIROBLEND's named property "1").
Mlt::Transition *findAnyTrackBlendTransition(Mlt::Producer &trackProducer, bool *isCairoblend)
{
    Mlt::Transition *transition = findTrackBlendTransition(trackProducer, QStringLiteral("qtblend"));
    if (!transition)
        transition = findTrackBlendTransition(trackProducer, QStringLiteral("movit.overlay"));
    if (transition) {
        if (isCairoblend)
            *isCairoblend = false;
        return transition;
    }
    transition = findTrackBlendTransition(trackProducer, QStringLiteral("frei0r.cairoblend"));
    if (isCairoblend)
        *isCairoblend = (transition != nullptr);
    return transition;
}

} // namespace

extern "C" {

int sap_add_video_track(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result]() { result = mw->timelineDock()->addVideoTrack(); },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_add_audio_track(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result]() { result = mw->timelineDock()->addAudioTrack(); },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_remove_track(void *mainWindowHandle, int trackIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, &result]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            // TimelineDock::removeTrack() operates on the "current" track,
            // there is no indexed overload -- this is the real primitive,
            // just called after pointing "current" at the requested index.
            dock->setCurrentTrack(trackIndex);
            dock->removeTrack();
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_track_muted(void *mainWindowHandle, int trackIndex, int muted)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, muted, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            model->setTrackMute(trackIndex, muted != 0);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_track_hidden(void *mainWindowHandle, int trackIndex, int hidden)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, hidden, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            model->setTrackHidden(trackIndex, hidden != 0);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_track_locked(void *mainWindowHandle, int trackIndex, int locked)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, locked, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            model->setTrackLock(trackIndex, locked != 0);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_reorder_track(void *mainWindowHandle, int fromTrackIndex, int toTrackIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, fromTrackIndex, toTrackIndex, &result]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model)
                return;
            const int count = model->trackList().size();
            if (fromTrackIndex < 0 || fromTrackIndex >= count || toTrackIndex < 0
                || toTrackIndex >= count)
                return;
            if (model->trackList().at(fromTrackIndex).type
                != model->trackList().at(toTrackIndex).type)
                return;
            // The real, undoable TimelineDock::moveTrack() primitive (the
            // same one the Track panel's up/down move buttons call).
            dock->moveTrack(fromTrackIndex, toTrackIndex);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_remove_clip(void *mainWindowHandle, int trackIndex, int clipIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, &result]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;
            if (clipIndex < 0 || clipIndex >= dock->clipCount(trackIndex))
                return;
            // The real, undoable TimelineDock::remove() primitive (the
            // same one the timeline's Delete/Ripple-Delete action calls).
            dock->remove(trackIndex, clipIndex, false);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_move_clip(void *mainWindowHandle,
                    int fromTrackIndex,
                    int fromClipIndex,
                    int toTrackIndex,
                    int toClipIndex,
                    int ripple)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, fromTrackIndex, fromClipIndex, toTrackIndex, toClipIndex, ripple, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model)
                return;
            const int trackCount = model->trackList().size();
            if (fromTrackIndex < 0 || fromTrackIndex >= trackCount || toTrackIndex < 0
                || toTrackIndex >= trackCount)
                return;
            if (dock->isTrackLocked(fromTrackIndex) || dock->isTrackLocked(toTrackIndex))
                return;
            if (fromClipIndex < 0 || fromClipIndex >= dock->clipCount(fromTrackIndex))
                return;

            // Compute the absolute destination frame position from the
            // requested clip-slot index -- moveClip() itself (and its
            // onClipMoved() handler) wants an absolute timeline frame, the
            // same as a drag-and-drop cursor position would supply.
            const int destClipCount = dock->clipCount(toTrackIndex);
            if (toClipIndex < 0 || toClipIndex > destClipCount)
                return;
            int targetPosition = 0;
            if (toClipIndex >= destClipCount) {
                if (destClipCount > 0) {
                    auto lastInfo = model->getClipInfo(toTrackIndex, destClipCount - 1);
                    if (lastInfo)
                        targetPosition = lastInfo->start + lastInfo->frame_count;
                }
            } else {
                auto destInfo = model->getClipInfo(toTrackIndex, toClipIndex);
                if (!destInfo)
                    return;
                targetPosition = destInfo->start;
            }

            // NOT calling TimelineDock::moveClip() here: that's the
            // drag-and-drop entry point, and it (a) can silently take a
            // completely different code path -- emitting transitionAdded
            // instead of clipMoved when Settings.timelineAllowTransitions()
            // is on and addTransitionValid() agrees, which is desirable for
            // a mouse drag but not for an explicit programmatic "move this
            // clip" call -- and (b) only *emits* clipMoved; the actual
            // model mutation happens in onClipMoved(), connected via
            // Qt::QueuedConnection (timelinedock.cpp's constructor), which
            // does not reliably drain in this headless/offscreen host even
            // with an explicit processEvents() pump (confirmed live: the
            // undo stack depth did not change afterward).
            //
            // Instead, construct and push the same real, undoable
            // Timeline::MoveClipCommand that onClipMoved() itself builds
            // (timelinedock.cpp), replicating its exact delta math
            // (position delta = target absolute frame minus the source
            // clip's current start) -- deterministic, synchronous, and
            // unambiguous for an API-driven move.
            auto sourceInfo = model->getClipInfo(fromTrackIndex, fromClipIndex);
            if (!sourceInfo)
                return;
            const int positionDelta = targetPosition - sourceInfo->start;
            const int trackDelta = toTrackIndex - fromTrackIndex;
            auto *command = new Timeline::MoveClipCommand(*dock, trackDelta, positionDelta, ripple != 0);
            command->addClip(fromTrackIndex, fromClipIndex);
            mw->undoStack()->push(command);

            // Re-read the real destination playlist to report where the
            // clip actually landed (not an echo of the request) -- the
            // same get_clip_index_at() lookup moveClip() itself uses
            // internally for its own overlap validation.
            const int mltIndex = model->trackList().at(toTrackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(model->tractor()->track(mltIndex));
            if (!track || !track->is_valid())
                return;
            Mlt::Playlist playlist(*track.data());
            const int finalIndex = playlist.get_clip_index_at(targetPosition);
            if (finalIndex < 0)
                return;
            auto finalInfo = model->getClipInfo(toTrackIndex, finalIndex);
            if (!finalInfo)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(toTrackIndex).arg(finalIndex);
            result["index"] = finalIndex;
            result["inFrame"] = finalInfo->frame_in;
            result["outFrame"] = finalInfo->frame_out;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_get_track_blend_mode(void *mainWindowHandle, int trackIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QByteArray value;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, &value, &ok]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> trackProducer(model->tractor()->track(mltIndex));
            if (!trackProducer || !trackProducer->is_valid())
                return;
            bool isCairoblend = false;
            QScopedPointer<Mlt::Transition> transition(
                findAnyTrackBlendTransition(*trackProducer, &isCairoblend));
            if (!transition || !transition->is_valid())
                return;
            QString mode = transition->get(isCairoblend ? "1" : "compositing");
            if (transition->get_int("disable"))
                mode.clear();
            else if (mode.isEmpty())
                mode = isCairoblend ? QStringLiteral("normal") : QStringLiteral("0");
            value = mode.toUtf8();
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(value);
}

int sap_set_track_blend_mode(void *mainWindowHandle, int trackIndex, const char *mode)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mode)
        return -1;
    const QString modeStr = QString::fromUtf8(mode);
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, modeStr, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> trackProducer(model->tractor()->track(mltIndex));
            if (!trackProducer || !trackProducer->is_valid())
                return;
            bool isCairoblend = false;
            QScopedPointer<Mlt::Transition> transition(
                findAnyTrackBlendTransition(*trackProducer, &isCairoblend));
            if (!transition || !transition->is_valid())
                return;
            // The real, undoable Timeline::ChangeBlendModeCommand -- the
            // same primitive TrackPropertiesWidget's blend mode combo box
            // pushes (trackpropertieswidget.cpp).
            auto *command = new Timeline::ChangeBlendModeCommand(*transition,
                                                                  isCairoblend ? QStringLiteral("1")
                                                                              : QStringLiteral(
                                                                                    "compositing"),
                                                                  modeStr);
            mw->undoStack()->push(command);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_track_height(void *mainWindowHandle, int height)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, height, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model)
                return;
            // Real MultitrackModel::setTrackHeight() clamps to [10, 150]
            // and stores it as the single project-wide `shotcut:trackHeight`
            // tractor property (not per-track), matching the Timeline
            // panel's row-height control (see multitrackmodel.cpp).
            model->setTrackHeight(height);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

namespace {

// Resolves the real per-instance "cut" producer for (trackIndex, clipIndex)
// (see Mlt::ClipInfo::cut in framework/mlt_playlist.h -- the instance
// producer, as opposed to the shared parent/source producer, so filters
// attached here never leak onto other clips sharing the same source
// file). Must be called on the Qt main thread (from inside an
// invokeMethod lambda). Returns a null ClipInfo unique_ptr AND a null
// Producer* on any failure (invalid track/clip/model).
std::unique_ptr<Mlt::ClipInfo> resolveClipInfo(MultitrackModel *model,
                                               int trackIndex,
                                               int clipIndex,
                                               Mlt::Producer **cutOut)
{
    *cutOut = nullptr;
    if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
        return nullptr;
    auto info = model->getClipInfo(trackIndex, clipIndex);
    if (!info || !info->cut || !info->cut->is_valid())
        return nullptr;
    *cutOut = info->cut;
    return info;
}

// Applies a flat JSON object of scalar (string/number/bool) values onto an
// MLT filter's property set, matching how filter properties are typically
// supplied by the filter panel's QML metadata bindings (string properties
// for combo/text values, numeric for sliders).
void applyJsonPropertiesToFilter(Mlt::Filter &filter, const QJsonObject &props)
{
    for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
        const QByteArray key = it.key().toUtf8();
        const QJsonValue v = it.value();
        if (v.isString())
            filter.set(key.constData(), v.toString().toUtf8().constData());
        else if (v.isBool())
            filter.set(key.constData(), v.toBool() ? 1 : 0);
        else if (v.isDouble())
            filter.set(key.constData(), v.toDouble());
    }
}

// Builds the standard playlist-entry JSON object for row `index` from a
// live Mlt::ClipInfo, matching PlaylistModel::data()'s COLUMN_RESOURCE
// display logic (prefer the real shotcut:caption property, else the
// resource's file basename) -- see playlistmodel.cpp.
QJsonObject playlistEntryToJson(int index, Mlt::ClipInfo *info)
{
    QJsonObject entry;
    entry["index"] = index;
    QString name;
    if (info->producer && info->producer->is_valid()) {
        name = QString::fromUtf8(info->producer->get("shotcut:caption"));
    }
    const QString resource = QString::fromUtf8(info->resource ? info->resource : "");
    if (name.isEmpty())
        name = QFileInfo(resource).fileName();
    entry["name"] = name;
    entry["path"] = resource;
    entry["durationFrames"] = info->frame_count;
    return entry;
}

// Builds the standard `markers.*` JSON marker object from a real
// Markers::Marker, matching sap-rust's `Marker` wire shape (backend.rs):
// `endFrame` is present only when the marker has a non-degenerate range
// (end != start), same convention MockBackend/MltBackend use.
QJsonObject markerToJson(int index, const Markers::Marker &marker)
{
    QJsonObject entry;
    entry["index"] = index;
    entry["frame"] = marker.start;
    if (marker.end != marker.start)
        entry["endFrame"] = marker.end;
    entry["text"] = marker.text;
    entry["color"] = marker.color.name(QColor::HexRgb);
    return entry;
}

} // namespace

char *sap_filter_add(void *mainWindowHandle,
                     int trackIndex,
                     int clipIndex,
                     const char *mltService,
                     const char *propertiesJson)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mltService)
        return nullptr;
    const QString serviceStr = QString::fromUtf8(mltService);
    QJsonObject props;
    if (propertiesJson && *propertiesJson) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(propertiesJson), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            props = doc.object();
    }
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, serviceStr, props, &result, &ok]() {
            auto *model = mw->timelineDock()->model();
            Mlt::Producer *cut = nullptr;
            auto info = resolveClipInfo(model, trackIndex, clipIndex, &cut);
            if (!cut)
                return;
            Mlt::Filter filter(MLT.profile(), serviceStr.toUtf8().constData());
            if (!filter.is_valid())
                return;
            applyJsonPropertiesToFilter(filter, props);
            const int filterIndex = cut->filter_count();
            if (cut->attach(filter) != 0)
                return;
            result["filterIndex"] = filterIndex;
            result["mltService"] = serviceStr;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_filter_set_property(void *mainWindowHandle,
                            int trackIndex,
                            int clipIndex,
                            int filterIndex,
                            const char *property,
                            const char *valueJson)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !property || !valueJson)
        return -1;
    const QString propertyStr = QString::fromUtf8(property);
    QJsonParseError err;
    const QJsonDocument valueDoc = QJsonDocument::fromJson(
        QByteArray("[") + valueJson + "]", &err);
    if (err.error != QJsonParseError::NoError || !valueDoc.isArray()
        || valueDoc.array().size() != 1)
        return -1;
    const QJsonValue value = valueDoc.array().at(0);
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, filterIndex, propertyStr, value, &result]() {
            auto *model = mw->timelineDock()->model();
            Mlt::Producer *cut = nullptr;
            auto info = resolveClipInfo(model, trackIndex, clipIndex, &cut);
            if (!cut)
                return;
            if (filterIndex < 0 || filterIndex >= cut->filter_count())
                return;
            QScopedPointer<Mlt::Filter> filter(cut->filter(filterIndex));
            if (!filter || !filter->is_valid())
                return;
            const QByteArray key = propertyStr.toUtf8();
            if (value.isString())
                filter->set(key.constData(), value.toString().toUtf8().constData());
            else if (value.isBool())
                filter->set(key.constData(), value.toBool() ? 1 : 0);
            else if (value.isDouble())
                filter->set(key.constData(), value.toDouble());
            else
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_filter_list(void *mainWindowHandle, int trackIndex, int clipIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonArray result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, &result, &ok]() {
            auto *model = mw->timelineDock()->model();
            Mlt::Producer *cut = nullptr;
            auto info = resolveClipInfo(model, trackIndex, clipIndex, &cut);
            if (!cut)
                return;
            for (int i = 0; i < cut->filter_count(); ++i) {
                QScopedPointer<Mlt::Filter> filter(cut->filter(i));
                if (!filter || !filter->is_valid())
                    continue;
                QJsonObject entry;
                entry["filterIndex"] = i;
                entry["mltService"] = QString::fromUtf8(filter->get("mlt_service"));
                result.append(entry);
            }
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_list_clips(void *mainWindowHandle, int trackIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonArray result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            const int count = dock->clipCount(trackIndex);
            for (int clipIndex = 0; clipIndex < count; ++clipIndex) {
                auto info = model->getClipInfo(trackIndex, clipIndex);
                if (!info)
                    continue;
                QJsonObject entry;
                entry["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(clipIndex);
                entry["index"] = clipIndex;
                entry["path"] = QString::fromUtf8(info->resource ? info->resource : "");
                entry["inFrame"] = info->frame_in;
                entry["outFrame"] = info->frame_out;
                result.append(entry);
            }
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_trim_clip_in(void *mainWindowHandle, int trackIndex, int clipIndex, long long newInFrame)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, newInFrame, &result]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;
            auto info = model->getClipInfo(trackIndex, clipIndex);
            if (!info)
                return;
            const int delta = static_cast<int>(newInFrame) - info->frame_in;
            if (delta != 0) {
                if (!model->trimClipInValid(trackIndex, clipIndex, delta, false))
                    return;
                // Real, undoable Timeline::TrimClipInCommand -- constructed
                // and pushed directly (its redo() performs the actual
                // mutation, same as the other real primitives in this
                // file), skipping TimelineDock::trimClipIn()'s stateful
                // drag-gesture machinery (m_trimCommand/commitTrimCommand,
                // transition auto-add/remove) which isn't meaningful for a
                // one-shot programmatic call.
                auto *command = new Timeline::TrimClipInCommand(*model,
                                                                 *dock->markersModel(),
                                                                 trackIndex,
                                                                 clipIndex,
                                                                 delta,
                                                                 /*ripple=*/false);
                mw->undoStack()->push(command);
            }
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_trim_clip_out(void *mainWindowHandle, int trackIndex, int clipIndex, long long newOutFrame)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, newOutFrame, &result]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;
            auto info = model->getClipInfo(trackIndex, clipIndex);
            if (!info)
                return;
            // Note the inverted sign convention vs. trim-in: the real
            // MultitrackModel::trimClipOutValid()/trimClipOut() compute the
            // new out-point as (frame_out - delta), not (frame_out + delta).
            const int delta = info->frame_out - static_cast<int>(newOutFrame);
            if (delta != 0) {
                if (!model->trimClipOutValid(trackIndex, clipIndex, delta, false))
                    return;
                auto *command = new Timeline::TrimClipOutCommand(*model,
                                                                  *dock->markersModel(),
                                                                  trackIndex,
                                                                  clipIndex,
                                                                  delta,
                                                                  /*ripple=*/false);
                mw->undoStack()->push(command);
            }
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_split_clip(void *mainWindowHandle, int trackIndex, int clipIndex, long long position)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, position, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;
            auto info = model->getClipInfo(trackIndex, clipIndex);
            if (!info)
                return;
            const int clipStart = info->start;
            const int clipEnd = clipStart + info->frame_count;
            const int splitPosition = static_cast<int>(position);
            if (splitPosition <= clipStart || splitPosition >= clipEnd)
                return;
            // Real, undoable Timeline::SplitCommand -- the same primitive
            // the "Split At Playhead" action uses (constructed directly
            // with a single-clip vector, same pattern as its multi-clip
            // "split all selected clips" caller).
            std::vector<int> trackIndices{trackIndex};
            std::vector<int> clipIndices{clipIndex};
            auto *command = new Timeline::SplitCommand(*model, trackIndices, clipIndices, splitPosition);
            mw->undoStack()->push(command);
            // After the split, clipIndex is the left half and clipIndex+1
            // is the newly-inserted right half (real MultitrackModel::
            // splitClip() inserts the new clip right after the original).
            result["leftClipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(clipIndex);
            result["rightClipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(clipIndex + 1);
            result["leftIndex"] = clipIndex;
            result["rightIndex"] = clipIndex + 1;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

long long sap_clip_length_frames(void *mainWindowHandle, int trackIndex, int clipIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    long long result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            auto info = model->getClipInfo(trackIndex, clipIndex);
            if (!info)
                return;
            result = info->frame_count;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_filter_remove(void *mainWindowHandle, int trackIndex, int clipIndex, int filterIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, filterIndex, &result]() {
            auto *model = mw->timelineDock()->model();
            Mlt::Producer *cut = nullptr;
            auto info = resolveClipInfo(model, trackIndex, clipIndex, &cut);
            if (!cut)
                return;
            if (filterIndex < 0 || filterIndex >= cut->filter_count())
                return;
            QScopedPointer<Mlt::Filter> filter(cut->filter(filterIndex));
            if (!filter || !filter->is_valid())
                return;
            if (cut->detach(*filter) != 0)
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_filter_reorder(void *mainWindowHandle, int trackIndex, int clipIndex, int fromIndex, int toIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, fromIndex, toIndex, &result]() {
            auto *model = mw->timelineDock()->model();
            Mlt::Producer *cut = nullptr;
            auto info = resolveClipInfo(model, trackIndex, clipIndex, &cut);
            if (!cut)
                return;
            const int count = cut->filter_count();
            if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex >= count)
                return;
            if (cut->move_filter(fromIndex, toIndex) != 0)
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_list_tracks(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonArray tracks;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &tracks]() {
            auto *model = mw->timelineDock()->model();
            if (!model)
                return;
            const TrackList &list = model->trackList();
            for (int i = 0; i < list.size(); ++i) {
                const Track &t = list.at(i);
                if (t.type != AudioTrackType && t.type != VideoTrackType)
                    continue;
                QJsonObject obj;
                obj["index"] = i;
                obj["kind"] = trackKindString(t.type);
                // Real read-back via MultitrackModel's own public
                // Is{Mute,Hidden,Locked}Role -- the same roles the QML
                // timeline UI reads to draw the mute/hide/lock icons
                // (multitrackmodel.cpp's data()), so this is genuine
                // current Qt/MLT state, not an echo of whatever was last
                // requested.
                const QModelIndex modelIndex = model->index(i, 0);
                obj["muted"] = model->data(modelIndex, MultitrackModel::IsMuteRole).toBool();
                obj["hidden"] = model->data(modelIndex, MultitrackModel::IsHiddenRole).toBool();
                obj["locked"] = model->data(modelIndex, MultitrackModel::IsLockedRole).toBool();
                // Real read-back of the qtblend/movit.overlay/cairoblend
                // transition's mode property, via the same transition-chain
                // walk sap_get_track_blend_mode()/findAnyTrackBlendTransition()
                // use (duplicated from TrackPropertiesWidget::getTransition()
                // since MultitrackModel's own lookup is private).
                QString blendMode = QStringLiteral("0");
                if (t.type == VideoTrackType) {
                    const int mltIndex = t.mlt_index;
                    QScopedPointer<Mlt::Producer> trackProducer(model->tractor()->track(mltIndex));
                    if (trackProducer && trackProducer->is_valid()) {
                        bool isCairoblend = false;
                        QScopedPointer<Mlt::Transition> transition(
                            findAnyTrackBlendTransition(*trackProducer, &isCairoblend));
                        if (transition && transition->is_valid()) {
                            QString mode = transition->get(isCairoblend ? "1" : "compositing");
                            if (transition->get_int("disable"))
                                mode.clear();
                            else if (mode.isEmpty())
                                mode = isCairoblend ? QStringLiteral("normal") : QStringLiteral("0");
                            blendMode = mode;
                        }
                    }
                }
                obj["blendMode"] = blendMode;
                tracks.append(obj);
            }
        },
        Qt::BlockingQueuedConnection);
    QJsonDocument doc(tracks);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_save_project(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, &result]() {
            QString filename = mw->fileName();
            if (filename.isEmpty())
                filename = mw->untitledFileName();
            result = mw->saveXML(filename) ? 0 : -1;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_export_project_xml(void *mainWindowHandle, const char *outputXmlPath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !outputXmlPath)
        return -1;
    const QString path = QString::fromUtf8(outputXmlPath);
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, path, &result]() { result = mw->saveXML(path, /*withRelativePaths=*/false) ? 0 : -1; },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_get_undo_depth(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->undoStack())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, &result]() { result = mw->undoStack()->index(); },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_get_redo_depth(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->undoStack())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, &result]() {
            auto *stack = mw->undoStack();
            result = stack->count() - stack->index();
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_project_undo(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->undoStack())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, &result]() {
            auto *stack = mw->undoStack();
            if (!stack->canUndo())
                return;
            stack->undo();
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_project_redo(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->undoStack())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, &result]() {
            auto *stack = mw->undoStack();
            if (!stack->canRedo())
                return;
            stack->redo();
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_playback_seek(void *mainWindowHandle, long long frame)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player || frame < 0 || frame > std::numeric_limits<int>::max())
        return -1;

    const int position = static_cast<int>(frame);
    int result = -1;
    auto seek = [player, position, &result]() {
        player->seek(position);
        result = 0;
    };

    if (QThread::currentThread() == mw->thread()) {
        seek();
        return result;
    }

    if (!QMetaObject::invokeMethod(mw, seek, Qt::BlockingQueuedConnection))
        return -1;
    return result;
}

char *sap_append_clip(void *mainWindowHandle, int trackIndex, const char *sourcePath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !sourcePath || !*sourcePath)
        return nullptr;
    const QString path = QString::fromUtf8(sourcePath);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, path, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;

            // Open the source as a real MLT producer, exactly like
            // MainWindow::open()/Controller::open() do for a user-selected
            // file -- this is what lets appendClip take a path directly
            // instead of being limited to whatever happens to be on the
            // clipboard/"current source" (the prior stub's documented gap).
            Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            producer.set_in_and_out(0, producer.get_length() - 1);

            const QString xml = MLT.XML(&producer);
            if (xml.isEmpty())
                return;

            // The real, undoable primitive TimelineDock::appendFromPlaylist()
            // pushes internally (timelinedock.cpp) -- lands on MAIN's
            // QUndoStack, so this is a genuine user-equivalent append, not a
            // bypass of undo/redo.
            //
            // Note: an empty track starts with a single blank placeholder
            // clip that gets consumed/replaced by the real append, so clip
            // count does not necessarily increase by 1 (it can even stay
            // the same) -- do not infer success from a before/after count
            // delta. The last clip on the track after the push is always
            // the one just appended.
            mw->undoStack()->push(new Timeline::AppendCommand(*model, trackIndex, xml, false));
            const int index = dock->clipCount(trackIndex) - 1;
            if (index < 0)
                return;

            auto info = model->getClipInfo(trackIndex, index);
            if (!info)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(index);
            result["index"] = index;
            result["inFrame"] = info->frame_in;
            result["outFrame"] = info->frame_out;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

unsigned char *sap_get_frame(void *mainWindowHandle,
                             long long frame,
                             const char *format,
                             int *outLen)
{
    if (outLen)
        *outLen = 0;
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !outLen || frame < 0 || frame > std::numeric_limits<int>::max())
        return nullptr;

    QByteArray formatUpper = format ? QByteArray(format).toUpper() : QByteArray("JPEG");
    if (formatUpper == "JPG")
        formatUpper = "JPEG";
    if (formatUpper != "JPEG" && formatUpper != "PNG")
        formatUpper = "JPEG";

    QByteArray encoded;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw,
        [mw, frame, formatUpper, &encoded, &ok]() {
            // FFI-driven edits (sap_add_track/sap_append_clip) mutate the
            // MultitrackModel/tractor directly and do not go through
            // MainWindow::seekTimeline(), which is the normal UI path that
            // points the live Controller::producer() at the timeline
            // tractor (mainwindow.cpp). Without this, MLT.producer() can
            // still be the original blank/untitled producer even after
            // real appends. Mirror seekTimeline()'s own sync check here so
            // getFrame renders the actual composited timeline, using the
            // same real primitive (MLT.setProducer over the multitrack
            // tractor), not a workaround.
            if (mw->isMultitrackValid()
                && (!MLT.producer() || !MLT.producer()->is_valid()
                    || (void *) MLT.producer()->get_producer()
                           != (void *) mw->multitrack()->get_producer())) {
                MLT.setProducer(new Mlt::Producer(*mw->multitrack()));
            }
            // Renders off the same live Mlt::Producer/tractor that drives
            // the app's own preview (Controller::producer()), via the same
            // Controller::image() primitive Shotcut's own timeline/property
            // thumbnails already use (mltcontroller.cpp) -- a real decode
            // of the actual composited project, not a mock.
            Mlt::Producer *producer = MLT.producer();
            if (!producer || !producer->is_valid())
                return;
            const int width = MLT.profile().width();
            const int height = MLT.profile().height();
            if (width <= 0 || height <= 0)
                return;
            QImage image = MLT.image(*producer, static_cast<int>(frame), width, height);
            if (image.isNull())
                return;
            QBuffer buffer(&encoded);
            if (!buffer.open(QIODevice::WriteOnly))
                return;
            ok = image.save(&buffer, formatUpper.constData());
        },
        Qt::BlockingQueuedConnection);

    if (!ok || encoded.isEmpty())
        return nullptr;

    auto *buffer = static_cast<unsigned char *>(std::malloc(static_cast<size_t>(encoded.size())));
    if (!buffer)
        return nullptr;
    std::memcpy(buffer, encoded.constData(), static_cast<size_t>(encoded.size()));
    *outLen = encoded.size();
    return buffer;
}

void sap_free_bytes(unsigned char *buf)
{
    std::free(buf);
}

char *sap_playlist_append(void *mainWindowHandle, const char *sourcePath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock() || !sourcePath || !*sourcePath)
        return nullptr;
    const QString path = QString::fromUtf8(sourcePath);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, path, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            if (!model)
                return;
            Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            producer.set_in_and_out(0, producer.get_length() - 1);
            model->append(producer);
            auto *playlist = model->playlist();
            if (!playlist)
                return;
            const int index = playlist->count() - 1;
            if (index < 0)
                return;
            QScopedPointer<Mlt::ClipInfo> info(playlist->clip_info(index));
            if (!info)
                return;
            result = playlistEntryToJson(index, info.data());
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_playlist_insert(void *mainWindowHandle, int index, const char *sourcePath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock() || !sourcePath || !*sourcePath)
        return nullptr;
    const QString path = QString::fromUtf8(sourcePath);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, index, path, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            if (!model)
                return;
            // PlaylistModel::createIfNeeded() (called inside append/insert)
            // means the playlist may still be null before the very first
            // entry -- only index 0 is valid on an empty playlist.
            auto *playlist = model->playlist();
            const int count = playlist ? playlist->count() : 0;
            if (index < 0 || index > count)
                return;
            Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            producer.set_in_and_out(0, producer.get_length() - 1);
            model->insert(producer, index);
            playlist = model->playlist();
            if (!playlist)
                return;
            QScopedPointer<Mlt::ClipInfo> info(playlist->clip_info(index));
            if (!info)
                return;
            result = playlistEntryToJson(index, info.data());
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_playlist_remove(void *mainWindowHandle, int index)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, index, &result]() {
            auto *model = mw->playlistDock()->model();
            auto *playlist = model ? model->playlist() : nullptr;
            if (!playlist || index < 0 || index >= playlist->count())
                return;
            model->remove(index);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_playlist_move(void *mainWindowHandle, int fromIndex, int toIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, fromIndex, toIndex, &result]() {
            auto *model = mw->playlistDock()->model();
            auto *playlist = model ? model->playlist() : nullptr;
            if (!playlist || fromIndex < 0 || fromIndex >= playlist->count() || toIndex < 0
                || toIndex >= playlist->count())
                return;
            model->move(fromIndex, toIndex);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_playlist_get(void *mainWindowHandle, int index)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, index, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            auto *playlist = model ? model->playlist() : nullptr;
            if (!playlist || index < 0 || index >= playlist->count())
                return;
            QScopedPointer<Mlt::ClipInfo> info(playlist->clip_info(index));
            if (!info)
                return;
            result = playlistEntryToJson(index, info.data());
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_playlist_list(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock())
        return nullptr;
    QJsonArray result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            auto *playlist = model ? model->playlist() : nullptr;
            const int count = playlist ? playlist->count() : 0;
            for (int i = 0; i < count; ++i) {
                QScopedPointer<Mlt::ClipInfo> info(playlist->clip_info(i));
                if (!info)
                    continue;
                result.append(playlistEntryToJson(i, info.data()));
            }
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_markers_append(void *mainWindowHandle, long long frame, const char *text, const char *color)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, frame, text, color, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            Markers::Marker marker;
            marker.text = QString::fromUtf8(text ? text : "");
            marker.start = static_cast<int>(frame);
            marker.end = static_cast<int>(frame);
            marker.color = QColor(QString::fromUtf8(color && *color ? color : "#FF0000"));
            const int index = model->getMarkers().size();
            model->append(marker);
            result = markerToJson(index, model->getMarker(index));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_markers_remove(void *mainWindowHandle, int markerIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, markerIndex, &result]() {
            auto *model = mw->timelineDock()->markersModel();
            if (markerIndex < 0 || markerIndex >= model->getMarkers().size())
                return;
            model->remove(markerIndex);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_markers_update(void *mainWindowHandle,
                         int markerIndex,
                         long long frame,
                         long long endFrame,
                         const char *text,
                         const char *color)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, markerIndex, frame, endFrame, text, color, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            if (markerIndex < 0 || markerIndex >= model->getMarkers().size())
                return;
            Markers::Marker marker;
            marker.text = QString::fromUtf8(text ? text : "");
            marker.start = static_cast<int>(frame);
            marker.end = static_cast<int>(endFrame);
            marker.color = QColor(QString::fromUtf8(color && *color ? color : "#FF0000"));
            model->update(markerIndex, marker);
            result = markerToJson(markerIndex, model->getMarker(markerIndex));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_markers_move(void *mainWindowHandle, int markerIndex, long long start, long long end)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, markerIndex, start, end, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            if (markerIndex < 0 || markerIndex >= model->getMarkers().size())
                return;
            model->move(markerIndex, static_cast<int>(start), static_cast<int>(end));
            result = markerToJson(markerIndex, model->getMarker(markerIndex));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_markers_set_color(void *mainWindowHandle, int markerIndex, const char *color)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel() || !color)
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, markerIndex, color, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            if (markerIndex < 0 || markerIndex >= model->getMarkers().size())
                return;
            model->setColor(markerIndex, QColor(QString::fromUtf8(color)));
            result = markerToJson(markerIndex, model->getMarker(markerIndex));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_markers_clear(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result]() {
            mw->timelineDock()->markersModel()->clear();
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_markers_list(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return nullptr;
    QJsonArray result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            const auto markers = model->getMarkers();
            for (int i = 0; i < markers.size(); ++i)
                result.append(markerToJson(i, markers.at(i)));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_markers_get(void *mainWindowHandle, int markerIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, markerIndex, &result, &ok]() {
            auto *model = mw->timelineDock()->markersModel();
            if (markerIndex < 0 || markerIndex >= model->getMarkers().size())
                return;
            result = markerToJson(markerIndex, model->getMarker(markerIndex));
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

long long sap_markers_next(void *mainWindowHandle, long long fromFrame)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return -1;
    long long result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, fromFrame, &result]() {
            result = mw->timelineDock()->markersModel()->nextMarkerPosition(static_cast<int>(fromFrame));
        },
        Qt::BlockingQueuedConnection);
    return result;
}

long long sap_markers_prev(void *mainWindowHandle, long long fromFrame)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->markersModel())
        return -1;
    long long result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, fromFrame, &result]() {
            result = mw->timelineDock()->markersModel()->prevMarkerPosition(static_cast<int>(fromFrame));
        },
        Qt::BlockingQueuedConnection);
    return result;
}

void sap_free_string(char *s)
{
    std::free(s);
}

void sap_emit_event(const char *jsonPayload)
{
    // Stub: proves the symbol is real and linkable, and that the signal
    // path below actually fires it. Full bridging into sap-rust's
    // broadcast/notification channel (so this reaches connected JSON-RPC
    // clients as an unsolicited edit.changed notification) is flagged as
    // follow-up work in the README's "Real FFI" section.
    if (jsonPayload)
        std::fprintf(stderr, "[sap_ffi] event: %s\n", jsonPayload);
}

void sap_install_notification_bridge(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->model())
        return;
    // Called on the Qt main thread by main.cpp, so this connect() itself
    // needs no thread hop -- it's the *emission* of modified() later that
    // will already be happening on the main thread too, so sap_emit_event
    // runs there. If/when sap_emit_event is wired to push into a Rust
    // channel, that push must stay non-blocking (e.g. a bounded/unbounded
    // mpsc `send`), not another BlockingQueuedConnection back out.
    QObject::connect(mw->timelineDock()->model(),
                      &MultitrackModel::modified,
                      mw,
                      []() { sap_emit_event("{\"type\":\"edit.changed\"}"); });
}

} // extern "C"
