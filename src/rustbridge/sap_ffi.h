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

/* Moves the track at fromTrackIndex to toTrackIndex via the real
 * TimelineDock::moveTrack() (Timeline::MoveTrackCommand, undoable) -- the
 * same primitive the Track Height panel's up/down buttons use. Returns 0
 * on success, -1 on error (invalid handle/index, or mismatched track
 * types -- video tracks can't swap with audio tracks). */
int sap_reorder_track(void *mainWindowHandle, int fromTrackIndex, int toTrackIndex);

/* Removes the clip at (trackIndex, clipIndex) via the real
 * TimelineDock::remove() (Timeline::RemoveCommand, undoable), replacing it
 * with a blank -- the same primitive the "Ripple Delete"/Delete timeline
 * action uses. Returns 0 on success, -1 on error (invalid handle/index,
 * locked track). */
int sap_remove_clip(void *mainWindowHandle, int trackIndex, int clipIndex);

/* Moves the clip at (fromTrackIndex, fromClipIndex) to the destination
 * clip-slot toClipIndex on toTrackIndex (toClipIndex == that track's
 * current clip count means "append at end"), via the real
 * TimelineDock::moveClip()/Timeline::MoveClipCommand path (the same one a
 * timeline drag-and-drop drives) -- this selects the source clip first
 * since moveClip()'s own onClipMoved() handler computes its position
 * delta from the current selection, exactly like a real drag does.
 * Returns a heap-allocated, NUL-terminated JSON object string describing
 * the clip at its final position, e.g. `{"clipId":"t1c2","index":2,
 * "inFrame":0,"outFrame":119}`, re-read from the real destination
 * playlist after the move (not an echo of the request). NULL on error
 * (invalid handle/index, locked track, or the move was rejected e.g. by
 * an overlapping non-ripple destination). Caller must free the returned
 * pointer via sap_free_string. */
char *sap_move_clip(void *mainWindowHandle,
                    int fromTrackIndex,
                    int fromClipIndex,
                    int toTrackIndex,
                    int toClipIndex,
                    int ripple);

/* Reads a track's real per-track video blend mode off its qtblend/
 * movit.overlay/frei0r.cairoblend transition (MultitrackModel's own
 * getVideoBlendTransition() lookup is private, so this replicates
 * TrackPropertiesWidget::getTransition()'s same transition-chain walk --
 * see trackpropertieswidget.cpp). Returns a heap-allocated, NUL-terminated
 * string holding the transition's mode property value (numeric string for
 * qtblend/movit.overlay's `compositing` property, a named string like
 * "normal"/"multiply" for cairoblend's property "1"), or NULL if the track
 * has no blend transition (e.g. the bottom video track) or on error.
 * Caller must free the returned pointer via sap_free_string. */
char *sap_get_track_blend_mode(void *mainWindowHandle, int trackIndex);

/* Sets a track's real per-track video blend mode via the real
 * Timeline::ChangeBlendModeCommand (undoable) -- the same primitive
 * TrackPropertiesWidget's blend mode combo box pushes. `mode` is
 * interpreted according to whichever transition type is actually present
 * on the track (see sap_get_track_blend_mode). Returns 0 on success, -1 on
 * error (invalid handle/index, or no blend transition on that track). */
int sap_set_track_blend_mode(void *mainWindowHandle, int trackIndex, const char *mode);

/* Sets the project-wide timeline row height in pixels via the real
 * MultitrackModel::setTrackHeight() (a single `shotcut:trackHeight`
 * property on the tractor, not per-track -- same primitive the Timeline
 * panel's zoom/height slider uses). Value is clamped to [10, 150] by the
 * real setter itself. Returns 0 on success, -1 on error (invalid handle). */
int sap_set_track_height(void *mainWindowHandle, int height);

/* Attaches a new MLT filter of mltService to the clip's real per-instance
 * "cut" producer (per Mlt::ClipInfo::cut -- the same instance-specific
 * producer Shotcut's own filter panel binds to for a selected clip, so
 * filters on one clip never leak onto other clips sharing the same source
 * file). propertiesJson (may be NULL/empty) is a flat JSON object of
 * string/number/bool values applied via Mlt::Properties::set() right after
 * attach. NOTE: unlike reorderTrack/removeClip/moveClip, this does NOT go
 * through a QUndoCommand (there is no lightweight one that doesn't also
 * require the full QmlMetadata-driven filter-panel machinery) -- it is a
 * direct, synchronous MLT mutation, not undoable via Ctrl+Z. Returns a
 * heap-allocated JSON object string `{"filterIndex":N,"mltService":"..."}`
 * (filterIndex is the filter's position in that clip's raw MLT filter
 * chain, i.e. producer->filter_count() at attach time), or NULL on error
 * (invalid handle/track/clip/mltService). Caller must free via
 * sap_free_string. */
char *sap_filter_add(void *mainWindowHandle,
                     int trackIndex,
                     int clipIndex,
                     const char *mltService,
                     const char *propertiesJson);

/* Sets one property on the filterIndex-th filter attached to the clip at
 * (trackIndex, clipIndex) (see sap_filter_add for indexing), via the real
 * Mlt::Properties::set(). valueJson holds one JSON-encoded scalar
 * (string/number/bool). Same non-undoable caveat as sap_filter_add.
 * Returns 0 on success, -1 on error (invalid handle/track/clip/filterIndex,
 * or unparseable valueJson). */
int sap_filter_set_property(void *mainWindowHandle,
                            int trackIndex,
                            int clipIndex,
                            int filterIndex,
                            const char *property,
                            const char *valueJson);

/* Returns a heap-allocated JSON array string describing every filter
 * attached to the clip at (trackIndex, clipIndex), in raw MLT filter-chain
 * order (matching sap_filter_add's filterIndex), of the form
 * `[{"filterIndex":0,"mltService":"..."},...]`, or NULL on error (invalid
 * handle/track/clip). Caller must free via sap_free_string. */
char *sap_filter_list(void *mainWindowHandle, int trackIndex, int clipIndex);

/* Removes the filterIndex-th filter attached to the clip at (trackIndex,
 * clipIndex) via the real Mlt::Service::detach(). Same non-undoable
 * caveat as sap_filter_add. Returns 0 on success, -1 on error (invalid
 * handle/track/clip/filterIndex). */
int sap_filter_remove(void *mainWindowHandle, int trackIndex, int clipIndex, int filterIndex);

/* Moves the filter at fromIndex to toIndex in the clip's raw MLT filter
 * chain via the real Mlt::Service::move_filter(). Same non-undoable
 * caveat as sap_filter_add. Returns 0 on success, -1 on error (invalid
 * handle/track/clip/index). */
int sap_filter_reorder(void *mainWindowHandle, int trackIndex, int clipIndex, int fromIndex, int toIndex);

/* Returns a heap-allocated JSON array string listing every clip on
 * trackIndex, in playlist order, of the form
 * `[{"clipId":"t0c0","index":0,"path":"...","inFrame":0,"outFrame":299},...]`
 * (built live from MultitrackModel::getClipInfo() for each slot -- "path"
 * is the clip's real MLT resource path), or NULL on error (invalid
 * handle/trackIndex). Caller must free via sap_free_string. */
char *sap_list_clips(void *mainWindowHandle, int trackIndex);

/* Adjusts the clip's in-point to absolute frame newInFrame via the real
 * Timeline::TrimClipInCommand (undoable, constructed and pushed directly
 * -- same primitive the timeline's edge-drag trim gesture uses, called
 * non-ripple/single-track like a plain drag on one clip's edge, no
 * transition auto-add/remove). Returns 0 on success, -1 on error (invalid
 * handle/track/clip/locked track, or newInFrame out of the valid range --
 * see MultitrackModel::trimClipInValid()). */
int sap_trim_clip_in(void *mainWindowHandle, int trackIndex, int clipIndex, long long newInFrame);
/* NOTE (real, load-bearing MultitrackModel::trimClipIn() behavior, not a
 * quirk of this shim): non-ripple trim-in with a positive delta (in-point
 * moving forward, i.e. shrinking the clip from its start) inserts a NEW
 * blank placeholder immediately to the clip's left when there is no
 * existing blank there to absorb the gap -- this shifts clipIndex for the
 * trimmed clip (and everything after it) up by one. Callers must re-query
 * edit.listClips after a forward trimClipIn to learn the clip's new index
 * (there is no way to return it from this call -- the shared Backend trait
 * signature is `-> BackendResult<()>`, same as MockBackend/MltBackend). */

/* Adjusts the clip's out-point to absolute frame newOutFrame via the real
 * Timeline::TrimClipOutCommand (undoable). Same caveats as
 * sap_trim_clip_in. Returns 0 on success, -1 on error. */
int sap_trim_clip_out(void *mainWindowHandle, int trackIndex, int clipIndex, long long newOutFrame);

/* Splits the clip at (trackIndex, clipIndex) at absolute frame position
 * (relative to the start of the timeline) via the real
 * Timeline::SplitCommand (undoable) -- the same primitive the "Split At
 * Playhead" action uses. Returns a heap-allocated JSON object string
 * `{"leftClipId":"t0c0","rightClipId":"t0c1","leftIndex":0,"rightIndex":1}`,
 * or NULL on error (invalid handle/track/clip, or position not inside the
 * clip). Caller must free via sap_free_string. */
char *sap_split_clip(void *mainWindowHandle, int trackIndex, int clipIndex, long long position);

/* Returns the clip's length in frames (Mlt::ClipInfo::frame_count, i.e.
 * frame_out - frame_in + 1) for the clip at (trackIndex, clipIndex), or -1
 * on error (invalid handle/track/clip). */
long long sap_clip_length_frames(void *mainWindowHandle, int trackIndex, int clipIndex);

/* Playlist ("Source"/bin panel, PlaylistDock, distinct from the per-track
 * timeline clips above) operations, via the real PlaylistModel slots
 * (append/insert/remove/move) -- these are NOT part of the undo stack in
 * real Shotcut (bin management isn't undoable there either), so this is a
 * faithful match, not a compromise. Each returns a heap-allocated JSON
 * object/array of the form `{"index":N,"name":"...","path":"...",
 * "durationFrames":N}` (name prefers the real `shotcut:caption` property,
 * falling back to the resource's file basename), or NULL/-1 on error.
 * Caller must free string results via sap_free_string. */
char *sap_playlist_append(void *mainWindowHandle, const char *sourcePath);
char *sap_playlist_insert(void *mainWindowHandle, int index, const char *sourcePath);
int sap_playlist_remove(void *mainWindowHandle, int index);
int sap_playlist_move(void *mainWindowHandle, int fromIndex, int toIndex);
char *sap_playlist_get(void *mainWindowHandle, int index);
char *sap_playlist_list(void *mainWindowHandle);

/* Timeline markers (real TimelineDock::markersModel(), MarkersModel), per
 * 01's `markers.*` namespace. Marker JSON shape:
 * `{"index":N,"frame":N,"endFrame":N|absent,"text":"...","color":"#RRGGBB"}`
 * -- "endFrame" is present only for range markers (end != start), mirroring
 * MockBackend/MltBackend's `Marker::end_frame`. `color` accepts anything
 * QColor's constructor parses ("#RRGGBB", "#AARRGGBB", named colors); the
 * output is always normalized to "#RRGGBB" (marker colors have no alpha in
 * real Shotcut -- see markerToProperties() in markersmodel.cpp). All
 * mutations go through the real Markers::AppendCommand/DeleteCommand/
 * UpdateCommand/ClearCommand (undoable via Ctrl+Z, MAIN.undoStack()) --
 * MarkersModel's own append/remove/update/move/setColor/clear slots
 * already construct+push these, so these wrappers call those slots
 * directly rather than duplicating command construction. Caller must free
 * string results via sap_free_string. */
char *sap_markers_append(void *mainWindowHandle, long long frame, const char *text, const char *color);
int sap_markers_remove(void *mainWindowHandle, int markerIndex);
/* Full-replace update (frame/endFrame/text/color all required) -- callers
 * on the Rust side resolve `markers.update`'s optional fields against the
 * marker's current state (via sap_markers_get) before calling this, same
 * division of labor as edit.setTrackProperties's independent per-field FFI
 * calls. endFrame == frame means a point marker (no range). */
char *sap_markers_update(void *mainWindowHandle,
                         int markerIndex,
                         long long frame,
                         long long endFrame,
                         const char *text,
                         const char *color);
char *sap_markers_move(void *mainWindowHandle, int markerIndex, long long start, long long end);
char *sap_markers_set_color(void *mainWindowHandle, int markerIndex, const char *color);
int sap_markers_clear(void *mainWindowHandle);
char *sap_markers_list(void *mainWindowHandle);
char *sap_markers_get(void *mainWindowHandle, int markerIndex);
/* Next/previous marker (start or end) strictly after/before fromFrame, or
 * -1 if none, via the real MarkersModel::nextMarkerPosition()/
 * prevMarkerPosition(). */
long long sap_markers_next(void *mainWindowHandle, long long fromFrame);
long long sap_markers_prev(void *mainWindowHandle, long long fromFrame);

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
