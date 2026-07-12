/*
 * SAP (Snapshot App Protocol) FFI shim -- Option A from
 * memory/head/gen/rust-fork/02-rust-embedding.md: a thin extern "C" layer
 * over Shotcut's own real, already-traced Q_INVOKABLE/slot methods, plus one
 * entry point (sap_start_server, implemented on the Rust side) that starts
 * the SAP JSON-RPC server inside this process.
 *
 * This header/its .cpp are the ONLY new files inside shotcut/src/ for this
 * feature -- everything else in the fork is unmodified except an additive
 * edit to CMakeLists.txt and a few lines in main.cpp that call into here.
 *
 * Threading rule (load-bearing, not optional -- see 02-rust-embedding.md):
 * every function below that touches Qt/MLT state crosses to the Qt main
 * thread via QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection)
 * internally, in sap_ffi.cpp. Callers may call these from any thread.
 */

#ifndef SAP_FFI_H
#define SAP_FFI_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Implemented in sap_ffi.cpp (C++), called from Rust (sap-rust/src/ffi.rs) ---- */

/* Returns the new track's 0-based index (per the wrapped
 * TimelineDock::addVideoTrack()/addAudioTrack()), or -1 on error. */
int sap_add_video_track(void *mainWindowHandle);
int sap_add_audio_track(void *mainWindowHandle);

/* Removes the track at trackIndex (via TimelineDock::removeTrack(), which
 * operates on the "current" track -- this sets currentTrack first). Returns
 * 0 on success, -1 on error (invalid handle/index). */
int sap_remove_track(void *mainWindowHandle, int trackIndex);

/* Set a track's mute/hidden(video)/locked state via the real
 * MultitrackModel::setTrackMute()/setTrackHidden()/setTrackLock() slots
 * (the same primitives the real Track Properties panel uses -- see
 * trackpropertieswidget.cpp), each independently callable. Returns 0 on
 * success, -1 on error (invalid handle/trackIndex). */
int sap_set_track_muted(void *mainWindowHandle, int trackIndex, int muted);
int sap_set_track_hidden(void *mainWindowHandle, int trackIndex, int hidden);
int sap_set_track_locked(void *mainWindowHandle, int trackIndex, int locked);

/* Returns a heap-allocated, NUL-terminated JSON array string describing the
 * project's audio/video tracks, e.g. `[{"index":0,"kind":"video"}, ...]`,
 * built from the real MultitrackModel::trackList(). NULL on error. Caller
 * must free the returned pointer via sap_free_string. */
char *sap_list_tracks(void *mainWindowHandle);

/* Saves the current project via MainWindow::saveXML() (which calls
 * Controller::saveXML(), mltcontroller.cpp:489), to its current filename or
 * the untitled default. Returns 0 on success, -1 on failure. */
int sap_save_project(void *mainWindowHandle);

/* Number of commands available to undo/redo on MAIN.undoStack(). -1 on
 * error. */
int sap_get_undo_depth(void *mainWindowHandle);
int sap_get_redo_depth(void *mainWindowHandle);

/* Applies the next undo/redo command on MAIN.undoStack(). Returns 0 on
 * success, -1 on error (invalid handle or undo stack). */
int sap_project_undo(void *mainWindowHandle);
int sap_project_redo(void *mainWindowHandle);

/* Seeks the main-window player to frame. Returns 0 after Player::seek()
 * runs on the Qt GUI thread, or -1 for an invalid handle/player/frame or
 * failed cross-thread invocation. */
int sap_playback_seek(void *mainWindowHandle, long long frame);

/* Opens sourcePath as a real Mlt::Producer (the same primitive
 * MainWindow::open()/Controller::open() use for a user-selected file) and
 * appends it to the track at trackIndex via the real, undoable
 * Timeline::AppendCommand -- the same QUndoCommand
 * TimelineDock::appendFromPlaylist() pushes internally
 * (timelinedock.cpp), so this is a genuine user-equivalent append (visible
 * to Ctrl+Z), not a bypass of undo/redo. Unlike TimelineDock::append(),
 * which only reads from the system clipboard/"current source", this takes
 * the source path directly.
 *
 * Returns a heap-allocated, NUL-terminated JSON object string describing
 * the appended clip, e.g. `{"clipId":"t0c0","index":0,"inFrame":0,
 * "outFrame":119}`, built from the real MultitrackModel::getClipInfo()
 * after the append. NULL on error (invalid handle/trackIndex, or
 * sourcePath fails to open as a valid MLT producer). Caller must free the
 * returned pointer via sap_free_string. */
char *sap_append_clip(void *mainWindowHandle, int trackIndex, const char *sourcePath);

/* Renders the pixels of the given absolute timeline frame from the
 * currently open project's live producer (Controller::producer(), the same
 * Mlt::Producer instance driving the app's own preview/player), using the
 * real Controller::image() primitive (mltcontroller.cpp) that Shotcut's own
 * timeline/property thumbnails already call, then encodes the result via
 * Qt's QImage::save() -- JPEG unless format is "png"/"PNG". *outLen
 * receives the byte length of the returned buffer; returns NULL (and
 * *outLen = 0) on error (invalid handle/frame/producer, or no image codec
 * plugin available for the requested format). Caller must free the
 * returned pointer via sap_free_bytes.
 *
 * Caveat (documented limitation, not a bug): Controller::image() seeks the
 * shared MLT.producer() instance to render, exactly like Shotcut's existing
 * thumbnail code -- it is not isolated from a concurrently *playing* live
 * preview. Calling this while the consumer is actively playing can visibly
 * perturb playback position. This is acceptable for a headless/offscreen
 * agent-driven session (no user-visible preview is running there); a
 * fully isolated grab (rendering off a cloned producer/consumer so it never
 * touches shared playback state) is flagged as follow-up work. */
unsigned char *sap_get_frame(void *mainWindowHandle,
                             long long frame,
                             const char *format,
                             int *outLen);

/* Frees a byte buffer returned by sap_get_frame. */
void sap_free_bytes(unsigned char *buf);

/* Frees a string returned by sap_list_tracks. */
void sap_free_string(char *s);

/* Notification bridge hook (Qt -> SAP). Currently a stub: logs to stderr.
 * Full wiring into the SAP broadcast/notification channel (so edit.changed
 * reaches connected JSON-RPC clients without the client having to poll) is
 * follow-up work -- see sap_ffi.cpp and the README's "Real FFI" section for
 * exactly what's stubbed. The extern "C" symbol is real and linkable so that
 * follow-up work is a body-only change, not a new call site. */
void sap_emit_event(const char *jsonPayload);

/* Connects MultitrackModel::modified (the nearest real, already-emitted
 * aggregate signal, per 02-rust-embedding.md's "signal bridge" section) to
 * sap_emit_event. Must be called on the Qt main thread (it performs a plain
 * QObject::connect, not a cross-thread call) -- shotcut/src/main.cpp calls
 * this once, before spawning the SAP server's background thread. */
void sap_install_notification_bridge(void *mainWindowHandle);

/* ---- Implemented in Rust (sap-rust/src/ffi_backend.rs), declared here so
 * the C++ side (main.cpp) can call it. ----
 *
 * Spawns a tokio runtime and runs the SAP JSON-RPC server (sap_rust::server
 * ::serve) against a real FfiBackend wrapping the functions above, listening
 * on socketPath and requiring sap.hello to present `token`. Blocks the
 * calling thread for the server's entire lifetime -- callers MUST invoke
 * this from a dedicated background std::thread, never the Qt main thread. */
void sap_start_server(void *mainWindowHandle, const char *socketPath, const char *token);

#ifdef __cplusplus
}
#endif

#endif /* SAP_FFI_H */
