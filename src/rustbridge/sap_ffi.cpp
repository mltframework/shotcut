/*
 * SAP (Snapshot App Protocol) FFI shim implementation. See sap_ffi.h for the
 * overall design note. Every function here that touches Qt/MLT state
 * crosses to the Qt main thread via
 * QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection) -- this is
 * the load-bearing threading rule from
 * memory/head/gen/rust-fork/02-rust-embedding.md, not a style choice.
 */

#include "sap_ffi.h"

#include "docks/timelinedock.h"
#include "mainwindow.h"
#include "models/multitrackmodel.h"
#include "player.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QThread>
#include <QString>
#include <QUndoStack>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

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
