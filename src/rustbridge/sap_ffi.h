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
