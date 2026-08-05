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
#include "docks/notesdock.h"
#include "docks/recentdock.h"
#include "models/playlistmodel.h"
#include "models/markersmodel.h"
#include "models/subtitlesmodel.h"
#include "models/subtitles.h"
#include "snapflow_mlt_properties.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"
#include "settings.h"
#include "player.h"
#include "util.h"

#include <Mlt.h>
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QScopedPointer>
#include <QSet>
#include <QThread>
#include <QString>
#include <QUndoStack>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace {

// See sap_ffi.h's sap_ffi_begin_project_teardown()/sap_ffi_end_project_teardown()
// doc comment for the full rationale. Plain atomic, not a mutex: readers
// (mainWindowFromHandle(), called from any ACP tokio worker thread) only
// need a fast, non-blocking, eventually-consistent view of "is Close/New
// Project's MLT teardown running right now" -- it does not gate access to
// any data the atomic itself owns.
std::atomic<bool> g_projectTeardownInProgress{false};

MainWindow *mainWindowFromHandle(void *handle)
{
    // During Close/New Project's MLT teardown (mainwindow.cpp), reject any
    // sap_* call that has not yet been dispatched rather than let it queue
    // against partially-torn-down Mlt::Producer/Consumer/model state; see
    // sap_ffi.h.
    if (g_projectTeardownInProgress.load(std::memory_order_acquire))
        return nullptr;
    return reinterpret_cast<MainWindow *>(handle);
}

// Re-points the live `Controller::producer()` at the timeline tractor after an
// FFI-driven edit.
//
// Why this exists: the FFI edit verbs below mutate through TimelineDock's own
// real, undoable primitives (so the timeline VIEW and undo stack are correct),
// but they do not go through `MainWindow::seekTimeline()`, which is the normal
// UI path that also re-points `Controller::producer()` at the multitrack
// tractor. Without that, `MLT.producer()` can still be the original
// blank/untitled producer after a real append/add-track, so the app's PLAYER /
// PREVIEW keeps showing pre-edit content even though the edit really landed --
// i.e. part of what the user sees silently lags the project state.
//
// This check was previously inlined in `sap_get_frame_png` only, which fixed
// the screenshot path while leaving the on-screen preview stale (and made a
// getFrame-based screenshot test pass while the user's own preview was wrong).
// Factored out here so every mutating verb calls the exact same primitive
// (`MLT.setProducer` over the multitrack tractor) rather than a third copy of
// the condition. Must be called on the Qt GUI thread -- every call site below
// invokes it from inside its own QMetaObject::invokeMethod lambda.
void syncTimelineProducer(MainWindow *mw)
{
    if (!mw || !mw->isMultitrackValid())
        return;
    if (!MLT.producer() || !MLT.producer()->is_valid()
        || (void *) MLT.producer()->get_producer() != (void *) mw->multitrack()->get_producer()) {
        MLT.setProducer(new Mlt::Producer(*mw->multitrack()));
    }
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

// C++-linkage, not part of the extern "C" Rust ABI below -- see sap_ffi.h.
void sap_ffi_begin_project_teardown()
{
    g_projectTeardownInProgress.store(true, std::memory_order_release);
}

void sap_ffi_end_project_teardown()
{
    g_projectTeardownInProgress.store(false, std::memory_order_release);
}

extern "C" {

int sap_add_video_track(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result]() {
            result = mw->timelineDock()->addVideoTrack();
            syncTimelineProducer(mw);
        },
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
        [mw, &result]() {
            result = mw->timelineDock()->addAudioTrack();
            syncTimelineProducer(mw);
        },
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
            syncTimelineProducer(mw);
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
            syncTimelineProducer(mw);
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
            syncTimelineProducer(mw);

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

int sap_set_track_composite(void *mainWindowHandle, int trackIndex, int composite)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, composite, &result]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            // Real MultitrackModel::setTrackComposite -- Track Properties
            // "Composite" toggle (qtblend transition disable bit).
            model->setTrackComposite(trackIndex, composite != 0);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_profile(void *mainWindowHandle,
                    const char *profileName,
                    int width,
                    int height,
                    int frameRateNum,
                    int frameRateDen)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return -1;
    const QString name = profileName ? QString::fromUtf8(profileName) : QString();
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, name, width, height, frameRateNum, frameRateDen, &result]() {
            if (!name.isEmpty()) {
                mw->setProfile(name);
                result = 0;
                return;
            }
            if (width <= 0 || height <= 0)
                return;
            const int fpsNum = frameRateNum > 0 ? frameRateNum : 25;
            const int fpsDen = frameRateDen > 0 ? frameRateDen : 1;
            auto &profile = MLT.profile();
            profile.set_width(Util::coerceMultiple(width));
            profile.set_height(Util::coerceMultiple(height));
            auto gcd = Util::greatestCommonDivisor(fpsNum, fpsDen);
            profile.set_frame_rate(fpsNum / gcd, fpsDen / gcd);
            profile.set_explicit(true);
            MLT.updatePreviewProfile();
            MLT.consumerChanged();
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_get_profile(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return nullptr;
    QJsonObject obj;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw,
        [&obj, &ok]() {
            auto &profile = MLT.profile();
            obj["width"] = profile.width();
            obj["height"] = profile.height();
            obj["frameRateNum"] = profile.frame_rate_num();
            obj["frameRateDen"] = profile.frame_rate_den();
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
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
            // and stores it as the single project-wide `snapflow:trackHeight`
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
// Maps 01-jsonrpc-spec.md's filter.addKeyframe interpolation strings onto
// the real MLT keyframe-type enum -- "" (default)/"smooth"/"discrete"|
// "hold", matching MltBackend's own tag mapping (mlt_backend.rs) for
// consistency between the two backends' accepted spellings.
mlt_keyframe_type interpolationFromString(const QString &s)
{
    if (s == QLatin1String("smooth"))
        return mlt_keyframe_smooth;
    if (s == QLatin1String("discrete") || s == QLatin1String("hold"))
        return mlt_keyframe_discrete;
    return mlt_keyframe_linear;
}

QString keyframeTypeToString(mlt_keyframe_type type)
{
    switch (type) {
    case mlt_keyframe_discrete:
        return QStringLiteral("discrete");
    case mlt_keyframe_smooth:
    case mlt_keyframe_smooth_natural:
    case mlt_keyframe_smooth_tight:
        return QStringLiteral("smooth");
    case mlt_keyframe_linear:
    default:
        return QStringLiteral("linear");
    }
}

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

// Enumerates a real, attached Mlt::Filter's user-visible properties for
// filter.list, live off the actual MLT property store (not an echo of
// whatever filter.add/filter.setProperty happened to be called with) --
// skips MLT/Snapflow's own internal bookkeeping keys (mlt_type,
// mlt_service, in/out/disable/version, anything "_"-prefixed like
// _unique_id, "snapflow:"-prefixed, or "meta."-prefixed metadata) since
// those are never a filter's own tunable parameters. Values come back as
// real MLT property strings; each is smart-typed to a JSON number when it
// parses cleanly as one (matching how filter.setProperty/anim_set already
// accept numeric values), otherwise left as a JSON string.
QJsonObject filterUserPropertiesToJson(Mlt::Filter &filter)
{
    static const QSet<QString> kReservedKeys{
        QStringLiteral("mlt_type"),
        QStringLiteral("mlt_service"),
        QStringLiteral("in"),
        QStringLiteral("out"),
        QStringLiteral("disable"),
        QStringLiteral("version"),
    };
    QJsonObject props;
    const int count = filter.count();
    for (int i = 0; i < count; ++i) {
        const char *rawName = filter.get_name(i);
        if (!rawName || !*rawName)
            continue;
        const QString name = QString::fromUtf8(rawName);
        if (kReservedKeys.contains(name) || name.startsWith(QLatin1Char('_'))
            || name.startsWith(QLatin1String("snapflow:")) || name.startsWith(QLatin1String("meta.")))
            continue;
        const char *rawValue = filter.get(rawName);
        if (!rawValue)
            continue;
        const QString value = QString::fromUtf8(rawValue);
        bool numOk = false;
        const double asDouble = value.toDouble(&numOk);
        if (numOk)
            props[name] = asDouble;
        else
            props[name] = value;
    }
    return props;
}

// Builds the standard playlist-entry JSON object for row `index` from a
// live Mlt::ClipInfo, matching PlaylistModel::data()'s COLUMN_RESOURCE
// display logic (prefer the real snapflow:caption property, else the
// resource's file basename) -- see playlistmodel.cpp.
QJsonObject playlistEntryToJson(int index, Mlt::ClipInfo *info)
{
    QJsonObject entry;
    entry["index"] = index;
    QString name;
    if (info->producer && info->producer->is_valid()) {
        name = QString::fromUtf8(info->producer->get("snapflow:caption"));
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
                            const char *valueJson,
                            long long position)
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
        [mw, trackIndex, clipIndex, filterIndex, propertyStr, value, position, &result]() {
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
            if (position >= 0) {
                // Real MLT keyframe at `position` (linear interpolation --
                // sap_filter_add_keyframe is the entry point for other
                // interpolation types), via the same anim_set() primitive.
                int rc = -1;
                if (value.isBool())
                    rc = filter->anim_set(key.constData(), value.toBool() ? 1.0 : 0.0, static_cast<int>(position));
                else if (value.isDouble())
                    rc = filter->anim_set(key.constData(), value.toDouble(), static_cast<int>(position));
                else if (value.isString()) {
                    bool numOk = false;
                    const double asDouble = value.toString().toDouble(&numOk);
                    rc = numOk
                             ? filter->anim_set(key.constData(), asDouble, static_cast<int>(position))
                             : filter->anim_set(key.constData(),
                                                value.toString().toUtf8().constData(),
                                                static_cast<int>(position));
                }
                if (rc == 0)
                    result = 0;
                return;
            }
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
                entry["properties"] = filterUserPropertiesToJson(*filter);
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
                entry["speed"] = Util::GetSpeedFromProducer(info->producer);
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

int sap_trim_clip_in(
    void *mainWindowHandle, int trackIndex, int clipIndex, long long newInFrame, int ripple)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    const bool doRipple = ripple != 0;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, newInFrame, doRipple, &result]() {
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
                if (!model->trimClipInValid(trackIndex, clipIndex, delta, doRipple))
                    return;
                // Real, undoable Timeline::TrimClipInCommand -- constructed
                // and pushed directly (its redo() performs the actual
                // mutation, same as the other real primitives in this
                // file), skipping TimelineDock::trimClipIn()'s stateful
                // drag-gesture machinery (m_trimCommand/commitTrimCommand,
                // transition auto-add/remove) which isn't meaningful for a
                // one-shot programmatic call. ripple mirrors the real
                // Ripple Trim toggle -- closes/opens the gap on this track
                // by shifting every downstream clip instead of leaving a
                // blank (see sap_ffi.h's non-ripple caveat for the
                // ripple==0 case).
                auto *command = new Timeline::TrimClipInCommand(*model,
                                                                 *dock->markersModel(),
                                                                 trackIndex,
                                                                 clipIndex,
                                                                 delta,
                                                                 doRipple);
                mw->undoStack()->push(command);
            }
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_trim_clip_out(
    void *mainWindowHandle, int trackIndex, int clipIndex, long long newOutFrame, int ripple)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return -1;
    const bool doRipple = ripple != 0;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, newOutFrame, doRipple, &result]() {
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
                if (!model->trimClipOutValid(trackIndex, clipIndex, delta, doRipple))
                    return;
                auto *command = new Timeline::TrimClipOutCommand(*model,
                                                                  *dock->markersModel(),
                                                                  trackIndex,
                                                                  clipIndex,
                                                                  delta,
                                                                  doRipple);
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

char *sap_transitions_add_crossfade(void *mainWindowHandle,
                                    int trackIndex,
                                    int firstClipIndex,
                                    int secondClipIndex,
                                    long long durationFrames)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, firstClipIndex, secondClipIndex, durationFrames, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;
            if (secondClipIndex != firstClipIndex + 1)
                return;
            auto firstInfo = model->getClipInfo(trackIndex, firstClipIndex);
            auto secondInfo = model->getClipInfo(trackIndex, secondClipIndex);
            if (!firstInfo || !secondInfo)
                return;
            if (durationFrames <= 0 || durationFrames >= firstInfo->frame_count
                || durationFrames >= secondInfo->frame_count)
            {
                qWarning("[sap_ffi] transitions.addCrossfade rejected: track=%d clips=%d/%d "
                         "durationFrames=%lld firstLen=%d secondLen=%d",
                         trackIndex,
                         firstClipIndex,
                         secondClipIndex,
                         durationFrames,
                         firstInfo->frame_count,
                         secondInfo->frame_count);
                return;
            }
            const int position = secondInfo->start - static_cast<int>(durationFrames);
            // ripple=true: without this, MultitrackModel::addTransition's
            // moveClipInBlank() only slides secondClipIndex left to build the
            // overlap and leaves everything downstream at its original
            // absolute position, backfilling the vacated durationFrames as an
            // explicit blank/gap clip -- confirmed live (repro:
            // scripts/debug-crossfade-repro.py) via edit.listClips showing a
            // spurious {"source":{"path":"blank"}} entry right after the
            // transition, which then makes a *second*, later crossfade
            // targeting the (now-shifted) downstream clip indices reject
            // with "durationFrames >= either clip's length" because the
            // blank's own length happened to equal durationFrames. Rippling
            // (matching Settings.timelineRippleAllTracks()-independent
            // downstream-clip-shift semantics real drag-to-crossfade already
            // gets from the timeline gesture path, timelinedock.cpp's
            // onTransitionAdded) closes that gap instead of leaving it, which
            // is what a one-shot programmatic "add this crossfade" call
            // should do -- there is no drag gesture here to preserve a
            // deliberate downstream gap for.
            auto *command = new Timeline::AddTransitionCommand(*dock,
                                                                trackIndex,
                                                                secondClipIndex,
                                                                position,
                                                                /*ripple=*/true);
            mw->undoStack()->push(command);
            const int transitionIndex = command->getTransitionIndex();
            if (transitionIndex < 0)
                return;
            result["trackIndex"] = trackIndex;
            result["transitionIndex"] = transitionIndex;
            result["betweenClips"] = QJsonArray{firstClipIndex, secondClipIndex};
            result["durationFrames"] = static_cast<double>(durationFrames);
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

int sap_filter_add_keyframe(void *mainWindowHandle,
                            int trackIndex,
                            int clipIndex,
                            int filterIndex,
                            const char *property,
                            long long position,
                            const char *valueJson,
                            const char *interpolation)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !property || !valueJson)
        return -1;
    const QString propertyStr = QString::fromUtf8(property);
    const QString interpStr = QString::fromUtf8(interpolation ? interpolation : "linear");
    QJsonParseError err;
    const QJsonDocument valueDoc = QJsonDocument::fromJson(QByteArray("[") + valueJson + "]", &err);
    if (err.error != QJsonParseError::NoError || !valueDoc.isArray() || valueDoc.array().size() != 1)
        return -1;
    const QJsonValue value = valueDoc.array().at(0);
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, filterIndex, propertyStr, position, value, interpStr, &result]() {
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
            const mlt_keyframe_type kfType = interpolationFromString(interpStr);
            int rc = -1;
            if (value.isBool()) {
                rc = filter->anim_set(key.constData(), value.toBool() ? 1.0 : 0.0, static_cast<int>(position), 0, kfType);
            } else if (value.isDouble()) {
                rc = filter->anim_set(key.constData(), value.toDouble(), static_cast<int>(position), 0, kfType);
            } else if (value.isString()) {
                bool numOk = false;
                const double asDouble = value.toString().toDouble(&numOk);
                rc = numOk ? filter->anim_set(key.constData(), asDouble, static_cast<int>(position), 0, kfType)
                           : filter->anim_set(key.constData(),
                                              value.toString().toUtf8().constData(),
                                              static_cast<int>(position));
                if (rc == 0 && !numOk) {
                    // mlt++'s only const-char*-value anim_set() overload
                    // (MltProperties.h) has no keyframe_type parameter at
                    // all -- it silently always creates a discrete/hold
                    // keyframe regardless of what the caller asked for,
                    // which for a non-numeric animated property (e.g. this
                    // filter's own "transition.rect" geometry string, "0%
                    // 0% 100% 100% 1") meant every requested "linear"/
                    // "smooth" keyframe silently downgraded to "discrete":
                    // the value stayed pinned at the previous keyframe and
                    // only snapped to the new one exactly at its frame,
                    // instead of interpolating -- confirmed live (a
                    // "linear" zoom rect keyframe pair rendered as a flat
                    // corner color for every sampled frame up to, but not
                    // including, the final keyframe's own exact frame; see
                    // scripts/debug-zoom-repro.py). Since Animation::
                    // key_set_type() can retroactively fix the type of the
                    // keyframe anim_set() just created, look it up by its
                    // now-known frame and correct it rather than avoiding
                    // the string overload (which is otherwise the only
                    // API that accepts a raw geometry/rect string as-is).
                    auto *anim = filter->get_anim(key.constData());
                    if (anim && anim->is_valid()) {
                        const int count = anim->key_count();
                        for (int i = 0; i < count; ++i) {
                            int frame = 0;
                            mlt_keyframe_type existingType = mlt_keyframe_linear;
                            if (anim->key_get(i, frame, existingType) == 0
                                && frame == static_cast<int>(position)) {
                                anim->key_set_type(i, kfType);
                                break;
                            }
                        }
                    }
                }
            }
            if (rc == 0)
                result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_filter_list_keyframes(void *mainWindowHandle,
                                int trackIndex,
                                int clipIndex,
                                int filterIndex,
                                const char *property)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !property)
        return nullptr;
    const QString propertyStr = QString::fromUtf8(property);
    QJsonArray result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, filterIndex, propertyStr, &result, &ok]() {
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
            // Not owned by the caller -- see qmlfilter.cpp/encodedock.cpp's
            // own get_anim() call sites, neither of which deletes it
            // either; the underlying mlt_animation is cached inside the
            // property itself.
            auto *anim = filter->get_anim(key.constData());
            if (!anim || !anim->is_valid()) {
                ok = true; // Never keyframed -- empty list, not an error.
                return;
            }
            const int count = anim->key_count();
            for (int i = 0; i < count; ++i) {
                int frame = 0;
                mlt_keyframe_type type = mlt_keyframe_linear;
                if (anim->key_get(i, frame, type) != 0)
                    continue;
                QJsonObject entry;
                entry["position"] = frame;
                entry["value"] = filter->anim_get_double(key.constData(), frame);
                entry["interpolation"] = keyframeTypeToString(type);
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

int sap_filter_remove_keyframe(void *mainWindowHandle,
                               int trackIndex,
                               int clipIndex,
                               int filterIndex,
                               const char *property,
                               long long position)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !property)
        return -1;
    const QString propertyStr = QString::fromUtf8(property);
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, filterIndex, propertyStr, position, &result]() {
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
            auto *anim = filter->get_anim(key.constData());
            if (!anim || !anim->is_valid() || !anim->is_key(static_cast<int>(position)))
                return;
            if (anim->remove(static_cast<int>(position)) != 0)
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
                // IsCompositeRole -- Track Properties "Composite" toggle
                // (false when qtblend disable=1, e.g. bottom V-track).
                obj["composite"] = model->data(modelIndex, MultitrackModel::IsCompositeRole).toBool();
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

int sap_set_project_file(void *mainWindowHandle, const char *filename)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !filename || !*filename)
        return -1;
    const QString path = QString::fromUtf8(filename);
    QMetaObject::invokeMethod(
        mw, [mw, path]() { mw->setSapProjectFile(path); }, Qt::BlockingQueuedConnection);
    return 0;
}

int sap_open_project(void *mainWindowHandle, const char *path)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !path || !*path)
        return -1;
    const QString url = QString::fromUtf8(path);
    int result = -1;
    QMetaObject::invokeMethod(
        mw,
        [mw, url, &result]() { result = mw->openForSap(url) ? 0 : -1; },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_close_project(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return -1;
    QMetaObject::invokeMethod(
        mw,
        [mw]() {
            // Same teardown open() performs before loading a new file:
            // reset to untitled without quitting the Qt process.
            mw->openForSap(mw->untitledFileName());
        },
        Qt::BlockingQueuedConnection);
    return 0;
}

int sap_is_project_folder(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return -1;
    int result = 0;
    QMetaObject::invokeMethod(
        mw,
        [&result]() { result = MLT.projectFolder().isEmpty() ? 0 : 1; },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_set_project_folder(void *mainWindowHandle, const char *folder)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return -1;
    const QString path = folder ? QString::fromUtf8(folder) : QString();
    QMetaObject::invokeMethod(
        mw, [path]() { MLT.setProjectFolder(path); }, Qt::BlockingQueuedConnection);
    return 0;
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

int sap_playback_fast_forward(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player)
        return -1;
    int result = -1;
    auto fastForward = [player, &result]() {
        player->fastForward(true);
        result = 0;
    };
    if (QThread::currentThread() == mw->thread()) {
        fastForward();
        return result;
    }
    if (!QMetaObject::invokeMethod(mw, fastForward, Qt::BlockingQueuedConnection))
        return -1;
    return result;
}

char *sap_set_clip_speed(void *mainWindowHandle, int trackIndex, int clipIndex, double speed)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !std::isfinite(speed) || speed <= 0.0)
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw,
        [mw, trackIndex, clipIndex, speed, &result, &ok]() {
            auto *model = mw->timelineDock()->model();
            if (!model || trackIndex < 0 || clipIndex < 0
                || trackIndex >= model->trackList().size())
                return;
            if (mw->timelineDock()->isTrackLocked(trackIndex))
                return;
            auto info = model->getClipInfo(trackIndex, clipIndex);
            if (!info || !info->producer || !info->producer->is_valid() || !info->cut
                || !info->cut->is_valid())
                return;
            const QString filename = Util::GetFilenameFromProducer(info->producer, false);
            Mlt::Producer replacement;
            if (qFuzzyCompare(speed, 1.0)) {
                replacement = Mlt::Producer(info->cut);
            } else {
                const QString resource = QStringLiteral("timewarp:%1:%2").arg(speed).arg(filename);
                replacement = Mlt::Producer(MLT.profile(), resource.toUtf8().constData());
                if (!replacement.is_valid())
                    return;
                Util::passProducerProperties(info->producer, &replacement);
                Util::updateCaption(&replacement);
                const int length = qRound(info->producer->get_length() / speed);
                const int in = qRound(info->cut->get_in() / speed);
                const int out = qRound(info->cut->get_out() / speed);
                replacement.set("length", replacement.frames_to_time(length, mlt_time_clock));
                replacement.set_in_and_out(in, out);
                MLT.copyFilters(*info->producer, replacement);
            }
            const QString xml = MLT.XML(&replacement);
            mw->undoStack()->push(new Timeline::ReplaceCommand(*model, trackIndex, clipIndex, xml));
            syncTimelineProducer(mw);
            auto updated = model->getClipInfo(trackIndex, clipIndex);
            if (!updated)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(clipIndex);
            result["index"] = clipIndex;
            result["inFrame"] = updated->frame_in;
            result["outFrame"] = updated->frame_out;
            result["speed"] = Util::GetSpeedFromProducer(updated->producer);
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

int sap_playback_play(void *mainWindowHandle, double speed)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player)
        return -1;
    int result = -1;
    auto play = [player, speed, &result]() {
        player->play(speed);
        result = 0;
    };
    if (QThread::currentThread() == mw->thread()) {
        play();
        return result;
    }
    if (!QMetaObject::invokeMethod(mw, play, Qt::BlockingQueuedConnection))
        return -1;
    return result;
}

int sap_playback_pause(void *mainWindowHandle, long long position)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player)
        return -1;
    const int pos = (position < 0 || position > std::numeric_limits<int>::max())
                        ? -1
                        : static_cast<int>(position);
    int result = -1;
    auto pause = [player, pos, &result]() {
        player->pause(pos);
        result = 0;
    };
    if (QThread::currentThread() == mw->thread()) {
        pause();
        return result;
    }
    if (!QMetaObject::invokeMethod(mw, pause, Qt::BlockingQueuedConnection))
        return -1;
    return result;
}

int sap_playback_stop(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player)
        return -1;
    int result = -1;
    auto stop = [player, &result]() {
        player->stop();
        result = 0;
    };
    if (QThread::currentThread() == mw->thread()) {
        stop();
        return result;
    }
    if (!QMetaObject::invokeMethod(mw, stop, Qt::BlockingQueuedConnection))
        return -1;
    return result;
}

char *sap_playback_get_state(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    auto *player = mw ? mw->player() : nullptr;
    if (!player)
        return nullptr;
    QJsonObject obj;
    bool ok = false;
    auto readState = [player, &obj, &ok]() {
        // isPaused() is the Controller transport state; Player::position is
        // the scrubber/playhead frame.
        obj["playing"] = !MLT.isPaused();
        obj["position"] = player->position();
        // Duration via player position max: onDurationChanged updates
        // m_duration privately; use producer length when available.
        int duration = 0;
        if (MLT.producer() && MLT.producer()->is_valid())
            duration = MLT.producer()->get_length();
        obj["duration"] = duration;
        ok = true;
    };
    if (QThread::currentThread() == mw->thread()) {
        readState();
    } else if (!QMetaObject::invokeMethod(mw, readState, Qt::BlockingQueuedConnection)) {
        return nullptr;
    }
    if (!ok)
        return nullptr;
    return newCString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
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
            syncTimelineProducer(mw);
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

char *sap_insert_clip(void *mainWindowHandle, int trackIndex, int clipIndex, const char *sourcePath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !sourcePath || !*sourcePath)
        return nullptr;
    const QString path = QString::fromUtf8(sourcePath);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, path, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;

            // clipIndex is a clip-SLOT index (insert before this slot),
            // matching sap_move_clip's toClipIndex convention rather than
            // an absolute frame -- computed into an absolute frame below
            // exactly like sap_move_clip does for its own toClipIndex,
            // since Timeline::InsertCommand itself wants an absolute
            // frame position.
            const int destClipCount = dock->clipCount(trackIndex);
            if (clipIndex < 0 || clipIndex > destClipCount)
                return;

            // Open the source as a real MLT producer, exactly like
            // sap_append_clip does -- lets insertClip take a path
            // directly instead of being limited to the clipboard/"current
            // source" TimelineDock::insert() itself reads from.
            Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            producer.set_in_and_out(0, producer.get_length() - 1);

            const QString xml = MLT.XML(&producer);
            if (xml.isEmpty())
                return;

            int insertPosition = 0;
            if (clipIndex >= destClipCount) {
                if (destClipCount > 0) {
                    auto lastInfo = model->getClipInfo(trackIndex, destClipCount - 1);
                    if (lastInfo)
                        insertPosition = lastInfo->start + lastInfo->frame_count;
                }
            } else {
                auto destInfo = model->getClipInfo(trackIndex, clipIndex);
                if (!destInfo)
                    return;
                insertPosition = destInfo->start;
            }

            // The real, undoable primitive: the exact same
            // Timeline::InsertCommand TimelineDock::insert() itself
            // pushes (timelinedock.cpp), called directly here rather
            // than through insert() so a filesystem path can be taken
            // without going via the system clipboard. seek=false so this
            // FFI call does not move the playhead as a side effect.
            mw->undoStack()->push(new Timeline::InsertCommand(*model,
                                                                *dock->markersModel(),
                                                                trackIndex,
                                                                insertPosition,
                                                                xml,
                                                                false));

            // Re-read the real destination playlist to report where the
            // clip actually landed -- same get_clip_index_at() lookup
            // sap_move_clip uses, since insert() only takes an absolute
            // frame position, not a clip-slot index.
            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(model->tractor()->track(mltIndex));
            if (!track || !track->is_valid())
                return;
            Mlt::Playlist playlist(*track.data());
            const int finalIndex = playlist.get_clip_index_at(insertPosition);
            if (finalIndex < 0)
                return;
            auto info = model->getClipInfo(trackIndex, finalIndex);
            if (!info)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(finalIndex);
            result["index"] = finalIndex;
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

char *sap_overwrite_clip(void *mainWindowHandle, int trackIndex, int clipIndex, const char *sourcePath)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !sourcePath || !*sourcePath)
        return nullptr;
    const QString path = QString::fromUtf8(sourcePath);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, path, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;

            // clipIndex is a clip-SLOT index (the slot to replace), matching
            // sap_insert_clip's convention -- computed into an absolute frame
            // below exactly like sap_insert_clip does for its own clipIndex,
            // since Timeline::OverwriteCommand itself wants an absolute
            // frame position.
            const int destClipCount = dock->clipCount(trackIndex);
            if (clipIndex < 0 || clipIndex > destClipCount)
                return;

            // Open the source as a real MLT producer, exactly like
            // sap_insert_clip/sap_append_clip do -- lets overwriteClip take
            // a path directly instead of being limited to the
            // clipboard/"current source" TimelineDock::overwrite() itself
            // reads from.
            Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            producer.set_in_and_out(0, producer.get_length() - 1);

            const QString xml = MLT.XML(&producer);
            if (xml.isEmpty())
                return;

            int overwritePosition = 0;
            if (clipIndex >= destClipCount) {
                if (destClipCount > 0) {
                    auto lastInfo = model->getClipInfo(trackIndex, destClipCount - 1);
                    if (lastInfo)
                        overwritePosition = lastInfo->start + lastInfo->frame_count;
                }
            } else {
                auto destInfo = model->getClipInfo(trackIndex, clipIndex);
                if (!destInfo)
                    return;
                overwritePosition = destInfo->start;
            }

            // The real, undoable primitive: the exact same
            // Timeline::OverwriteCommand TimelineDock::overwrite() itself
            // pushes (timelinedock.cpp), called directly here rather than
            // through overwrite() so a filesystem path can be taken without
            // going via the system clipboard. Unlike InsertCommand, this
            // does NOT ripple downstream clips -- it drops and replaces
            // whatever occupies the target frame range. seek=false so this
            // FFI call does not move the playhead as a side effect.
            mw->undoStack()->push(new Timeline::OverwriteCommand(*model,
                                                                   trackIndex,
                                                                   overwritePosition,
                                                                   xml,
                                                                   false));

            // Re-read the real destination playlist to report where the
            // clip actually landed -- same get_clip_index_at() lookup
            // sap_insert_clip/sap_move_clip use, since overwrite() only
            // takes an absolute frame position, not a clip-slot index.
            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(model->tractor()->track(mltIndex));
            if (!track || !track->is_valid())
                return;
            Mlt::Playlist playlist(*track.data());
            const int finalIndex = playlist.get_clip_index_at(overwritePosition);
            if (finalIndex < 0)
                return;
            auto info = model->getClipInfo(trackIndex, finalIndex);
            if (!info)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(finalIndex);
            result["index"] = finalIndex;
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

// Shared tail for sap_append_clip_xml/sap_insert_clip_xml/
// sap_overwrite_clip_xml/sap_playlist_get_xml's *_clip_xml siblings:
// resolves a playlist entry's *live* Mlt::Producer (with any attached
// filters intact, e.g. the dynamictext/qtext filter a
// sap_generator_create_title clip carries) to its full MLT XML
// serialization. Distinct from playlistEntryToJson's plain "path" field,
// which is only the raw resource string (e.g. "color:#00000000" for a
// title) and loses any attached filter chain -- reopening that resource
// as a fresh producer instead of reusing this XML would silently drop
// the title text/colour filter. Reused by both `{source:{xml}}` (caller
// already has XML) and `{source:{playlistIndex}}` (resolved here first)
// per 01-jsonrpc-spec.md's shared source union across
// appendClip/insertClip/overwriteClip.
char *sap_playlist_get_xml(void *mainWindowHandle, int index)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock())
        return nullptr;
    QString xml;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, index, &xml, &ok]() {
            auto *model = mw->playlistDock()->model();
            auto *playlist = model ? model->playlist() : nullptr;
            if (!playlist || index < 0 || index >= playlist->count())
                return;
            QScopedPointer<Mlt::ClipInfo> info(playlist->clip_info(index));
            if (!info || !info->producer || !info->producer->is_valid())
                return;
            // ClipInfo::frame_in/frame_out (not info->producer's OWN in/out,
            // which for e.g. a title/color producer defaults to its full
            // multi-hour underlying `length`) is this specific bin entry's
            // real trim -- PlaylistModel::append() sets it via
            // producer.set_in_and_out(0, producer.get_length() - 1) at
            // append time, e.g. 100 frames for a generator.createTitle
            // clip, but that trim lives on the playlist ENTRY (a distinct
            // MLT "cut" of the producer), not the raw producer object
            // itself. Applying it here before serializing is required --
            // skipping this step silently exports/appends the untrimmed
            // full-length producer instead of the real bin entry.
            info->producer->set_in_and_out(info->frame_in, info->frame_out);
            xml = MLT.XML(info->producer);
            ok = !xml.isEmpty();
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(xml.toUtf8());
}

char *sap_append_clip_xml(void *mainWindowHandle, int trackIndex, const char *xmlStr)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !xmlStr || !*xmlStr)
        return nullptr;
    const QString xml = QString::fromUtf8(xmlStr);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, xml, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;

            // Same real, undoable Timeline::AppendCommand sap_append_clip
            // pushes -- the only difference is the caller already has a
            // ready-made MLT XML producer (from `{source:{xml}}` or a
            // resolved `{source:{playlistIndex}}` via
            // sap_playlist_get_xml) instead of a filesystem path to open.
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

char *sap_insert_clip_xml(void *mainWindowHandle, int trackIndex, int clipIndex, const char *xmlStr)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !xmlStr || !*xmlStr)
        return nullptr;
    const QString xml = QString::fromUtf8(xmlStr);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, xml, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;

            const int destClipCount = dock->clipCount(trackIndex);
            if (clipIndex < 0 || clipIndex > destClipCount)
                return;

            int insertPosition = 0;
            if (clipIndex >= destClipCount) {
                if (destClipCount > 0) {
                    auto lastInfo = model->getClipInfo(trackIndex, destClipCount - 1);
                    if (lastInfo)
                        insertPosition = lastInfo->start + lastInfo->frame_count;
                }
            } else {
                auto destInfo = model->getClipInfo(trackIndex, clipIndex);
                if (!destInfo)
                    return;
                insertPosition = destInfo->start;
            }

            // Same real, undoable Timeline::InsertCommand sap_insert_clip
            // pushes -- see sap_append_clip_xml for why xml is taken
            // ready-made here instead of a path.
            mw->undoStack()->push(new Timeline::InsertCommand(*model,
                                                                *dock->markersModel(),
                                                                trackIndex,
                                                                insertPosition,
                                                                xml,
                                                                false));

            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(model->tractor()->track(mltIndex));
            if (!track || !track->is_valid())
                return;
            Mlt::Playlist playlist(*track.data());
            const int finalIndex = playlist.get_clip_index_at(insertPosition);
            if (finalIndex < 0)
                return;
            auto info = model->getClipInfo(trackIndex, finalIndex);
            if (!info)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(finalIndex);
            result["index"] = finalIndex;
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

char *sap_overwrite_clip_xml(void *mainWindowHandle, int trackIndex, int clipIndex, const char *xmlStr)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !xmlStr || !*xmlStr)
        return nullptr;
    const QString xml = QString::fromUtf8(xmlStr);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, clipIndex, xml, &result, &ok]() {
            auto *dock = mw->timelineDock();
            auto *model = dock->model();
            if (!model || trackIndex < 0 || trackIndex >= model->trackList().size())
                return;
            if (dock->isTrackLocked(trackIndex))
                return;

            const int destClipCount = dock->clipCount(trackIndex);
            if (clipIndex < 0 || clipIndex > destClipCount)
                return;

            int overwritePosition = 0;
            if (clipIndex >= destClipCount) {
                if (destClipCount > 0) {
                    auto lastInfo = model->getClipInfo(trackIndex, destClipCount - 1);
                    if (lastInfo)
                        overwritePosition = lastInfo->start + lastInfo->frame_count;
                }
            } else {
                auto destInfo = model->getClipInfo(trackIndex, clipIndex);
                if (!destInfo)
                    return;
                overwritePosition = destInfo->start;
            }

            // Same real, undoable Timeline::OverwriteCommand
            // sap_overwrite_clip pushes -- see sap_append_clip_xml for
            // why xml is taken ready-made here instead of a path.
            mw->undoStack()->push(new Timeline::OverwriteCommand(*model,
                                                                   trackIndex,
                                                                   overwritePosition,
                                                                   xml,
                                                                   false));

            const int mltIndex = model->trackList().at(trackIndex).mlt_index;
            QScopedPointer<Mlt::Producer> track(model->tractor()->track(mltIndex));
            if (!track || !track->is_valid())
                return;
            Mlt::Playlist playlist(*track.data());
            const int finalIndex = playlist.get_clip_index_at(overwritePosition);
            if (finalIndex < 0)
                return;
            auto info = model->getClipInfo(trackIndex, finalIndex);
            if (!info)
                return;
            result["clipId"] = QStringLiteral("t%1c%2").arg(trackIndex).arg(finalIndex);
            result["index"] = finalIndex;
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
            syncTimelineProducer(mw);
            // Renders off the same live Mlt::Producer/tractor that drives
            // the app's own preview (Controller::producer()), via the same
            // Controller::image() primitive Snapflow's own timeline/property
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

char *sap_playlist_append_image_sequence(void *mainWindowHandle,
                                         const char *path,
                                         int ttl,
                                         const char *begin)
{
    // Mirrors ImageProducerWidget::on_sequenceCheckBox_clicked: open first
    // frame, convert resource to printf pattern, set ttl/begin/shotcut_sequence,
    // count consecutive files, reopen producer, append to playlist bin.
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock() || !path || !*path)
        return nullptr;
    const QString firstPath = QString::fromUtf8(path);
    const QString beginOverride = begin ? QString::fromUtf8(begin) : QString();
    const int ttlFrames = ttl > 0 ? ttl : 1;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, firstPath, beginOverride, ttlFrames, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            if (!model)
                return;

            Mlt::Producer probe(MLT.profile(), firstPath.toUtf8().constData());
            if (!probe.is_valid() || probe.get_int("error"))
                return;
            if (!MLT.isImageProducer(&probe))
                return;

            QFileInfo info(firstPath);
            QString name = info.fileName();
            QString beginStr = beginOverride;
            int i = name.length();
            int digitCount = 0;
            for (; i && !name[i - 1].isDigit(); i--) {
            }
            for (; i && name[i - 1].isDigit(); i--, digitCount++) {
                if (beginOverride.isEmpty())
                    beginStr.prepend(name[i - 1]);
            }
            if (digitCount <= 0 || beginStr.isEmpty())
                return;

            QString patternName = name;
            patternName.replace(i, digitCount, QStringLiteral("0%1d").arg(digitCount).prepend('%'));
            QString serviceName = QString::fromUtf8(probe.get("mlt_service"));
            QString resource;
            if (!serviceName.isEmpty())
                resource = serviceName + QLatin1Char(':') + info.path() + QLatin1Char('/')
                           + patternName;
            else
                resource = info.path() + QLatin1Char('/') + patternName;

            // Count consecutive frames (allow gaps up to 100 like producer_qimage).
            QString countTemplate = name;
            countTemplate.replace(i, digitCount, QStringLiteral("%1"));
            const QString countPath = info.path() + QLatin1Char('/') + countTemplate;
            int imageCount = 0;
            int frameNum = beginStr.toInt();
            for (int gap = 0; gap < 100;) {
                if (QFile::exists(countPath.arg(frameNum, digitCount, 10, QChar('0')))) {
                    imageCount++;
                    gap = 0;
                } else {
                    gap++;
                }
                frameNum++;
            }
            if (imageCount <= 0)
                return;

            Mlt::Producer producer(MLT.profile(), resource.toUtf8().constData());
            if (!producer.is_valid() || producer.get_int("error"))
                return;
            // ImageProducerWidget uses the local define "shotcut_resource"
            // (not kOriginalResourceProperty "shotcut:resource").
            producer.set("shotcut_resource", firstPath.toUtf8().constData());
            producer.set(kSnapflowSequenceProperty, 1);
            producer.set("autolength", 1);
            producer.set("ttl", ttlFrames);
            producer.set("begin", beginStr.toLatin1().constData());
            const int lengthFrames = imageCount * ttlFrames;
            producer.set("length",
                         producer.frames_to_time(lengthFrames, mlt_time_clock));
            producer.set_in_and_out(0, lengthFrames - 1);

            model->append(producer);
            auto *playlist = model->playlist();
            if (!playlist)
                return;
            const int index = playlist->count() - 1;
            if (index < 0)
                return;
            QScopedPointer<Mlt::ClipInfo> clipInfo(playlist->clip_info(index));
            if (!clipInfo)
                return;
            result = playlistEntryToJson(index, clipInfo.data());
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(QJsonDocument(result).toJson(QJsonDocument::Compact));
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

char *sap_generator_create_title(void *mainWindowHandle,
                                 const char *mode,
                                 const char *text,
                                 const char *fgColour,
                                 const char *bgColour)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock() || !text || !*text)
        return nullptr;
    const QString modeStr = QString::fromUtf8(mode && *mode ? mode : "simple");
    const QString textStr = QString::fromUtf8(text);
    const QString fg = QString::fromUtf8(fgColour && *fgColour ? fgColour : "#ffffffff");
    const QString bg = QString::fromUtf8(bgColour && *bgColour ? bgColour : "#00000000");
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, modeStr, textStr, fg, bg, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            if (!model)
                return;
            Mlt::Producer producer(MLT.profile(), "color:");
            if (!producer.is_valid())
                return;
            producer.set("resource", bg.toUtf8().constData());
            producer.set("mlt_image_format", "rgba");
            const QString caption = QStringLiteral("Title: %1").arg(textStr);
            producer.set(kSnapflowCaptionProperty, caption.toUtf8().constData());
            MLT.setDurationFromDefault(&producer);
            const bool rich = (modeStr == QLatin1String("rich"));
            Mlt::Filter filter(MLT.profile(), rich ? "qtext" : "dynamictext");
            if (!filter.is_valid())
                return;
            // Both dynamictext and qtext default to halign=left/valign=top
            // (see filter_dynamictext.yml / filter_qtext.yml), which pins
            // the text to the top-left corner of the frame. A title card
            // generator should center its text by default; callers that
            // want a different placement can add their own filter.set
            // calls on the returned clip via the filter/keyframe RPCs.
            filter.set("halign", "centre");
            filter.set("valign", "middle");
            if (rich) {
                filter.set("html", textStr.toUtf8().constData());
            } else {
                filter.set("argument", textStr.toUtf8().constData());
                filter.set("fgcolour", fg.toUtf8().constData());
                filter.set("bgcolour", "#00000000");
            }
            if (producer.attach(filter) != 0)
                return;
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
            QJsonObject source;
            source["kind"] = "title";
            source["mode"] = modeStr;
            source["text"] = textStr;
            result["index"] = index;
            result["name"] = QStringLiteral("Title: %1").arg(textStr);
            result["source"] = source;
            result["durationFrames"] = info->frame_count;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_generator_create_color(void *mainWindowHandle, const char *hexColor)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->playlistDock() || !hexColor || !*hexColor)
        return nullptr;
    const QString hex = QString::fromUtf8(hexColor);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->playlistDock(),
        [mw, hex, &result, &ok]() {
            auto *model = mw->playlistDock()->model();
            if (!model)
                return;
            Mlt::Producer producer(MLT.profile(), "color:");
            if (!producer.is_valid())
                return;
            producer.set("resource", hex.toUtf8().constData());
            producer.set("mlt_image_format", "rgba");
            const QString caption = QStringLiteral("Color: %1").arg(hex);
            producer.set(kSnapflowCaptionProperty, caption.toUtf8().constData());
            MLT.setDurationFromDefault(&producer);
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
            QJsonObject source;
            source["kind"] = "color";
            source["hexColor"] = hex;
            result["index"] = index;
            result["name"] = caption;
            result["source"] = source;
            result["durationFrames"] = info->frame_count;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

namespace {

// Converts an inclusive [startFrame,endFrame] frame range to millisecond
// timestamps via the real project fps (SubtitlesModel's own primitive --
// Subtitles::SubtitleItem -- stores ms, not frames).
int64_t frameToMs(long long frame, double fps)
{
    if (fps <= 0.0)
        fps = 25.0;
    return static_cast<int64_t>(std::llround(static_cast<double>(frame) * 1000.0 / fps));
}

} // namespace

char *sap_subtitles_add_track(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid())
        return nullptr;
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, &result, &ok]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            const int trackIndex = model->trackCount();
            SubtitlesModel::SubtitleTrack track;
            track.name = QStringLiteral("Subtitle %1").arg(trackIndex + 1);
            track.lang = QString();
            model->addTrack(track);
            if (model->trackCount() != trackIndex + 1)
                return;
            result["trackIndex"] = trackIndex;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

int sap_subtitles_append_item(void *mainWindowHandle,
                              int trackIndex,
                              long long startFrame,
                              long long endFrame,
                              const char *text)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid())
        return -1;
    const QString textStr = QString::fromUtf8(text ? text : "");
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, startFrame, endFrame, textStr, &result]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            if (trackIndex < 0 || trackIndex >= model->trackCount())
                return;
            const double fps = MLT.profile().fps();
            const int before = model->itemCount(trackIndex);
            Subtitles::SubtitleItem item;
            item.start = frameToMs(startFrame, fps);
            item.end = frameToMs(endFrame, fps);
            item.text = textStr.toUtf8().toStdString();
            model->appendItem(trackIndex, item);
            if (model->itemCount(trackIndex) != before + 1)
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_subtitles_remove_items(void *mainWindowHandle, int trackIndex, const char *itemIndicesJson)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid()
        || !itemIndicesJson)
        return -1;
    QJsonParseError err;
    QJsonDocument indicesDoc = QJsonDocument::fromJson(QByteArray(itemIndicesJson), &err);
    if (err.error != QJsonParseError::NoError || !indicesDoc.isArray())
        return -1;
    std::vector<int> indices;
    for (const auto &v : indicesDoc.array()) {
        if (!v.isDouble())
            return -1;
        indices.push_back(v.toInt());
    }
    if (indices.empty())
        return -1;
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    const int first = indices.front();
    const int last = indices.back();
    if (static_cast<size_t>(last - first + 1) != indices.size())
        return -1; // not a contiguous run -- see sap_ffi.h caveat
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, first, last, &result]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            if (trackIndex < 0 || trackIndex >= model->trackCount())
                return;
            const int count = model->itemCount(trackIndex);
            if (first < 0 || last >= count)
                return;
            const int before = count;
            model->removeItems(trackIndex, first, last);
            if (model->itemCount(trackIndex) != before - (last - first + 1))
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_subtitles_import_srt(void *mainWindowHandle, const char *path, int newTrack)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid()
        || !path || !*path)
        return nullptr;
    const Subtitles::SubtitleVector srtItems = Subtitles::readFromSrtFile(path);
    if (srtItems.empty())
        return nullptr;
    const QString pathStr = QString::fromUtf8(path);
    QJsonObject result;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, srtItems, pathStr, newTrack, &result, &ok]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            QList<Subtitles::SubtitleItem> items;
            for (const auto &item : srtItems)
                items.append(item);
            int trackIndex;
            if (newTrack || model->trackCount() == 0) {
                trackIndex = model->trackCount();
                SubtitlesModel::SubtitleTrack track;
                track.name = QFileInfo(pathStr).completeBaseName();
                if (track.name.isEmpty())
                    track.name = QStringLiteral("Subtitle %1").arg(trackIndex + 1);
                track.lang = QString();
                model->importSubtitlesToNewTrack(track, items);
                if (model->trackCount() != trackIndex + 1)
                    return;
            } else {
                trackIndex = 0;
                model->importSubtitles(trackIndex, 0, items);
            }
            result["trackIndex"] = trackIndex;
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

char *sap_subtitles_export_srt(void *mainWindowHandle, int trackIndex, const char *path)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid()
        || !path || !*path)
        return nullptr;
    const QString pathStr = QString::fromUtf8(path);
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, pathStr, &ok]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            if (trackIndex < 0 || trackIndex >= model->trackCount())
                return;
            model->exportSubtitles(pathStr, trackIndex);
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(pathStr.toUtf8());
}

int sap_subtitles_burn_in(void *mainWindowHandle, int trackIndex)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->timelineDock()->subtitlesModel() || !mw->isMultitrackValid())
        return -1;
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, trackIndex, &result]() {
            auto *model = mw->timelineDock()->subtitlesModel();
            if (trackIndex < 0 || trackIndex >= model->trackCount())
                return;
            const SubtitlesModel::SubtitleTrack track = model->getTrack(trackIndex);
            Mlt::Producer *output = mw->multitrack();
            if (!output || !output->is_valid())
                return;
            // Mirror SubtitlesModel::doEditTrack's own scan (subtitlesmodel.cpp)
            // so re-burning the same track is idempotent instead of stacking
            // duplicate "subtitle" filters on the output each call.
            for (int i = 0; i < output->filter_count(); i++) {
                QScopedPointer<Mlt::Filter> existing(output->filter(i));
                if (existing && existing->is_valid()
                    && existing->get("mlt_service") == QStringLiteral("subtitle")
                    && existing->get("feed") == track.name) {
                    result = 0;
                    return;
                }
            }
            // Same filter construction as SubtitlesDock::burnInOnTimeline()
            // (subtitlesdock.cpp), attached directly to the tractor instead
            // of routed through MainWindow::onCreateOrEditFilterOnOutput
            // (which needs a live FiltersDock/selection to run headlessly).
            Mlt::Filter filter(MLT.profile(), "subtitle");
            filter.set(kSnapflowFilterProperty, "subtitles");
            filter.set("fgcolour", "#ffffffff");
            filter.set("bgcolour", "#00000000");
            filter.set("olcolour", "#aa000000");
            filter.set("outline", 3);
            filter.set("weight", QFont::Bold);
            filter.set("style", "normal");
            filter.set("snapflow:usePointSize", 1);
            filter.set("size", MLT.profile().height() / 20);
            filter.set("geometry", "20%/75%:60%x20%");
            filter.set("valign", "bottom");
            filter.set("halign", "center");
            filter.set("feed", track.name.toUtf8().constData());
            filter.set("typewriter", 0);
            if (output->attach(filter) != 0)
                return;
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_subtitles_set_style(void *mainWindowHandle, const char *styleJson)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->timelineDock() || !mw->isMultitrackValid() || !styleJson)
        return -1;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray(styleJson), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return -1;
    const QJsonObject style = document.object();
    int result = -1;
    QMetaObject::invokeMethod(
        mw->timelineDock(),
        [mw, style, &result]() {
            Mlt::Producer *output = mw->multitrack();
            if (!output || !output->is_valid())
                return;
            const QSet<QString> allowed{
                QStringLiteral("fgcolour"), QStringLiteral("bgcolour"),
                QStringLiteral("olcolour"), QStringLiteral("outline"),
                QStringLiteral("weight"), QStringLiteral("style"),
                QStringLiteral("size"), QStringLiteral("geometry"),
                QStringLiteral("valign"), QStringLiteral("halign")};
            for (int i = 0; i < output->filter_count(); ++i) {
                QScopedPointer<Mlt::Filter> filter(output->filter(i));
                if (!filter || !filter->is_valid()
                    || QString::fromUtf8(filter->get("mlt_service")) != QStringLiteral("subtitle"))
                    continue;
                for (auto it = style.constBegin(); it != style.constEnd(); ++it) {
                    if (!allowed.contains(it.key()))
                        continue;
                    const QByteArray key = it.key().toUtf8();
                    const QJsonValue value = it.value();
                    if (value.isString())
                        filter->set(key.constData(), value.toString().toUtf8().constData());
                    else if (value.isBool())
                        filter->set(key.constData(), value.toBool() ? 1 : 0);
                    else if (value.isDouble())
                        filter->set(key.constData(), value.toDouble());
                }
                result = 0;
            }
        },
        Qt::BlockingQueuedConnection);
    return result;
}

int sap_notes_set_text(void *mainWindowHandle, const char *text)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->notesDock())
        return -1;
    const QString textStr = QString::fromUtf8(text ? text : "");
    int result = -1;
    QMetaObject::invokeMethod(
        mw->notesDock(),
        [mw, textStr, &result]() {
            mw->notesDock()->setText(textStr);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_notes_get_text(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->notesDock())
        return nullptr;
    QString text;
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->notesDock(),
        [mw, &text, &ok]() {
            text = mw->notesDock()->getText();
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(text.toUtf8());
}

int sap_recent_add(void *mainWindowHandle, const char *path)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->recentDock() || !path || !*path)
        return -1;
    const QString pathStr = QString::fromUtf8(path);
    int result = -1;
    QMetaObject::invokeMethod(
        mw->recentDock(),
        [mw, pathStr, &result]() {
            mw->recentDock()->add(pathStr);
            result = 0;
        },
        Qt::BlockingQueuedConnection);
    return result;
}

char *sap_recent_remove(void *mainWindowHandle, const char *path)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw || !mw->recentDock() || !path || !*path)
        return nullptr;
    const QString pathStr = QString::fromUtf8(path);
    bool ok = false;
    QMetaObject::invokeMethod(
        mw->recentDock(),
        [mw, pathStr, &ok]() {
            const QString normalized = QDir::fromNativeSeparators(pathStr);
            if (!Settings.recent().contains(normalized))
                return;
            mw->recentDock()->remove(pathStr);
            ok = true;
        },
        Qt::BlockingQueuedConnection);
    if (!ok)
        return nullptr;
    return newCString(pathStr.toUtf8());
}

char *sap_recent_list(void *mainWindowHandle)
{
    auto *mw = mainWindowFromHandle(mainWindowHandle);
    if (!mw)
        return nullptr;
    QJsonArray result;
    for (const QString &s : Settings.recent())
        result.append(s);
    QJsonDocument doc(result);
    return newCString(doc.toJson(QJsonDocument::Compact));
}

void sap_free_string(char *s)
{
    std::free(s);
}

void sap_emit_event(const char *jsonPayload)
{
    // Forwards into the real Rust-side bridge (sap_ffi_notify_bridge,
    // sap-rust/src/ffi_backend.rs), which fans this out to every SAP
    // client currently bound to this process's document. Kept as its own
    // function (rather than connecting sap_ffi_notify_bridge directly to
    // MultitrackModel::modified) so the JSON payload shape stays owned by
    // this file, and so the stderr log below -- useful for headless
    // harness debugging -- has a single call site.
    if (jsonPayload) {
        std::fprintf(stderr, "[sap_ffi] event: %s\n", jsonPayload);
        sap_ffi_notify_bridge(jsonPayload);
    }
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
