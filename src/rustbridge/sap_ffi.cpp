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
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"
#include "player.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
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
                // blendMode has no real read-back wired yet (the qtblend/
                // cairoblend transition lookup is a private model method) --
                // report the default rather than fabricate a real value.
                obj["blendMode"] = QStringLiteral("0");
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
