/*
 * SAP (Snapshot App Protocol) FFI shim -- Option A from
 * memory/head/gen/rust-fork/02-rust-embedding.md: a thin extern "C" layer
 * over Snapflow's own real, already-traced Q_INVOKABLE/slot methods, plus one
 * entry point (sap_start_server, implemented on the Rust side) that starts
 * the SAP JSON-RPC server inside this process.
 *
 * This header/its .cpp are the ONLY new files inside snapflow/src/ for this
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
/* Internal, C++-linkage only -- NOT part of the Rust-facing ABI below, so
 * deliberately kept outside the extern "C" block. Called from
 * MainWindow::on_actionClose_triggered() (mainwindow.cpp) to bracket the
 * Close/New Project MLT teardown window.
 *
 * Why this exists: every sap_* entry point below crosses to the Qt main
 * thread via QMetaObject::invokeMethod(..., Qt::BlockingQueuedConnection)
 * from an ACP-driven tokio worker thread (sap-rust's runtime), and that
 * dispatch is funneled through mainWindowFromHandle() in sap_ffi.cpp.
 * Close/New Project's own teardown (on_actionClose_triggered,
 * closeProducer()) is stock upstream Shotcut code that reenters the Qt
 * event loop via QCoreApplication::processEvents() partway through tearing
 * down MLT's Producer/Consumer/model state -- a window that did not exist
 * before this fork added a background thread capable of queuing blocking
 * calls into that same state. Any sap_* call still in flight (already past
 * its mainWindowFromHandle() check) when that reentrant processEvents()
 * pumps the queue can end up touching a Mlt::Producer/Consumer mid-teardown.
 * Setting this flag for the duration of the close/new sequence makes
 * mainWindowFromHandle() reject (return nullptr for) any call that has not
 * yet been dispatched, closing the vast majority of that window. This does
 * NOT close the narrower race where a call is dispatched (past the
 * mainWindowFromHandle() check) in the brief interval before this flag is
 * set; fully closing that would require a check inside every sap_*
 * lambda's BlockingQueuedConnection body, not just at entry. */
void sap_ffi_begin_project_teardown();
void sap_ffi_end_project_teardown();
#endif

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

/* Enables/disables video-track compositing over lower tracks via the real
 * MultitrackModel::setTrackComposite() (qtblend/movit/cairoblend
 * transition's `disable` bit -- the Track Properties "Composite" toggle).
 * Returns 0 on success, -1 on error (invalid handle/index). */
int sap_set_track_composite(void *mainWindowHandle, int trackIndex, int composite);

/* Sets the project-wide timeline row height in pixels via the real
 * MultitrackModel::setTrackHeight() (a single `snapflow:trackHeight`
 * property on the tractor, not per-track -- same primitive the Timeline
 * panel's zoom/height slider uses). Value is clamped to [10, 150] by the
 * real setter itself. Returns 0 on success, -1 on error (invalid handle). */
int sap_set_track_height(void *mainWindowHandle, int height);

/* Sets the project video mode / canvas. When profileName is non-NULL and
 * non-empty, calls MainWindow::setProfile(name) (MLT built-in profile).
 * Otherwise applies width/height (required >0) and optional frame rate
 * (defaults 25/1) onto MLT.profile() and refreshes the preview profile.
 * Returns 0 on success, -1 on error. */
int sap_set_profile(void *mainWindowHandle,
                    const char *profileName,
                    int width,
                    int height,
                    int frameRateNum,
                    int frameRateDen);

/* Returns heap-allocated JSON:
 * {"width":W,"height":H,"frameRateNum":N,"frameRateDen":D} for the live
 * MLT profile, or NULL on error. Caller frees via sap_free_string. */
char *sap_get_profile(void *mainWindowHandle);

/* Attaches a new MLT filter of mltService to the clip's real per-instance
 * "cut" producer (per Mlt::ClipInfo::cut -- the same instance-specific
 * producer Snapflow's own filter panel binds to for a selected clip, so
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
                            const char *valueJson,
                            long long position);
/* NOTE: `position` < 0 means "no keyframe position" (set the plain static
 * property value, same as before this parameter was added). `position` >=
 * 0 sets a real MLT keyframe at that frame via `Mlt::Properties::
 * anim_set()` (linear interpolation) instead -- same primitive
 * `sap_filter_add_keyframe` uses with an explicit interpolation. */

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

/* Keyframe operations on one already-attached filter's property, via the
 * real `Mlt::Properties::anim_set()`/`Mlt::Animation` C-ABI-wrapped MLT
 * primitives (the same underlying animated-property machinery real
 * Snapflow's own keyframable filter panel/QmlFilter use -- see
 * qmlfilter.cpp/encodedock.cpp's own `get_anim()`/`anim_set()` call
 * sites). Interpolation is one of "linear" (default)/"smooth"/
 * "discrete"|"hold", matching `01-jsonrpc-spec.md`'s `filter.addKeyframe`
 * spec. `valueJson` holds one JSON-encoded scalar; string values that
 * don't parse as a number are stored via MLT's string `anim_set()`
 * overload, which has no interpolation-type parameter -- MLT itself only
 * interpolates numeric animated properties, so a string keyframe's
 * `interpolation` argument is accepted but has no effect (same real MLT
 * limitation, not a shim compromise). Same non-undoable caveat as
 * sap_filter_add (no lightweight QUndoCommand exists for raw MLT filter
 * property mutation). Returns 0 on success, -1 on error (invalid
 * handle/track/clip/filterIndex, or unparseable valueJson).
 *
 * sap_filter_list_keyframes returns a heap-allocated JSON array
 * `[{"position":N,"value":<number>,"interpolation":"linear"|"smooth"|
 * "discrete"},...]` in keyframe order (empty array, not NULL, when the
 * property has never been keyframed), or NULL on error (invalid
 * handle/track/clip/filterIndex). Caller must free via sap_free_string.
 * NOTE: values are always read back as doubles (`anim_get_double()`) --
 * string-valued keyframed properties round-trip through this call as
 * their numeric parse if any, else 0; rect/color animated properties
 * (e.g. Size Position Rotate's "rect") are not supported by this call at
 * all (out of scope for this shim; flagged as follow-up work). */
int sap_filter_add_keyframe(void *mainWindowHandle,
                            int trackIndex,
                            int clipIndex,
                            int filterIndex,
                            const char *property,
                            long long position,
                            const char *valueJson,
                            const char *interpolation);
char *sap_filter_list_keyframes(void *mainWindowHandle,
                                int trackIndex,
                                int clipIndex,
                                int filterIndex,
                                const char *property);
/* Removes the keyframe at exactly `position` via the real
 * `Mlt::Animation::remove()`. Returns 0 on success, -1 on error (invalid
 * handle/track/clip/filterIndex, no keyframes on that property, or no
 * keyframe exactly at `position`). */
int sap_filter_remove_keyframe(void *mainWindowHandle,
                               int trackIndex,
                               int clipIndex,
                               int filterIndex,
                               const char *property,
                               long long position);

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
 * see MultitrackModel::trimClipInValid()). ripple != 0 shifts every
 * downstream clip on the SAME track to close/
 * open the gap instead of leaving (or requiring) a blank -- the real
 * Timeline Ripple Trim mode, same `ripple` bool
 * Timeline::TrimClipInCommand/MultitrackModel::trimClipIn() already
 * accept; ripple == 0 keeps the original non-ripple behavior below. */
int sap_trim_clip_in(
    void *mainWindowHandle, int trackIndex, int clipIndex, long long newInFrame, int ripple);
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
 * sap_trim_clip_in, including the ripple parameter. Returns 0 on success,
 * -1 on error. */
int sap_trim_clip_out(
    void *mainWindowHandle, int trackIndex, int clipIndex, long long newOutFrame, int ripple);

/* Splits the clip at (trackIndex, clipIndex) at absolute frame position
 * (relative to the start of the timeline) via the real
 * Timeline::SplitCommand (undoable) -- the same primitive the "Split At
 * Playhead" action uses. Returns a heap-allocated JSON object string
 * `{"leftClipId":"t0c0","rightClipId":"t0c1","leftIndex":0,"rightIndex":1}`,
 * or NULL on error (invalid handle/track/clip, or position not inside the
 * clip). Caller must free via sap_free_string. */
char *sap_split_clip(void *mainWindowHandle, int trackIndex, int clipIndex, long long position);

/* Adds a real crossfade transition between two adjacent clips on
 * trackIndex, via the real Timeline::AddTransitionCommand (undoable) --
 * the same primitive that runs when a user drags one clip's edge to
 * overlap its neighbor in the real Timeline panel. This wrapper computes
 * the equivalent drag by moving secondClipIndex left by durationFrames
 * (non-ripple, matching sap_trim_clip_in/Out's single-clip semantics), so
 * the two clips end up overlapping by exactly durationFrames, joined by a
 * real MLT mix (luma/movit.luma_mix dissolve + mix:-2 crossfade, the exact
 * real transition services MultitrackModel::addTransition() attaches --
 * see multitrackmodel.cpp). Returns a heap-allocated JSON object
 * `{"trackIndex":N,"transitionIndex":N,"betweenClips":[first,second],
 * "durationFrames":N}` (transitionIndex is the new mix clip's slot in the
 * track's playlist -- everything from there onward shifts by one clip
 * index, same re-indexing caveat as sap_trim_clip_in's forward case), or
 * NULL on error (invalid handle/track/clip, non-adjacent clip indices,
 * locked track, or durationFrames <= 0 or >= either clip's own length).
 * Caller must free via sap_free_string. */
char *sap_transitions_add_crossfade(void *mainWindowHandle,
                                    int trackIndex,
                                    int firstClipIndex,
                                    int secondClipIndex,
                                    long long durationFrames);

/* Returns the clip's length in frames (Mlt::ClipInfo::frame_count, i.e.
 * frame_out - frame_in + 1) for the clip at (trackIndex, clipIndex), or -1
 * on error (invalid handle/track/clip). */
long long sap_clip_length_frames(void *mainWindowHandle, int trackIndex, int clipIndex);

/* Playlist ("Source"/bin panel, PlaylistDock, distinct from the per-track
 * timeline clips above) operations, via the real PlaylistModel slots
 * (append/insert/remove/move) -- these are NOT part of the undo stack in
 * real Snapflow (bin management isn't undoable there either), so this is a
 * faithful match, not a compromise. Each returns a heap-allocated JSON
 * object/array of the form `{"index":N,"name":"...","path":"...",
 * "durationFrames":N}` (name prefers the real `snapflow:caption` property,
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
 * real Snapflow -- see markerToProperties() in markersmodel.cpp). All
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

/* Binds this session's "current file" (MainWindow::fileName(), what
 * sap_save_project saves to) to filename, without opening/loading
 * anything from disk -- see MainWindow::setSapProjectFile's doc comment.
 * Intended to be called once, right after launch, with
 * <projectRoot>/<mltFileName> (per 09-project-folder-layout.md), so
 * project.save persists to the real project folder instead of
 * MainWindow::untitledFileName()'s scratch default. Returns 0 on success,
 * -1 if mainWindowHandle/filename is invalid. */
int sap_set_project_file(void *mainWindowHandle, const char *filename);

/* Loads an existing MLT XML project from disk via MainWindow::openForSap(),
 * replacing whatever playlist/multitrack is currently live, and binds
 * fileName() to path. Only safe on a freshly launched, unmodified session
 * -- openForSap skips continueModified()/checkAutoSave() modal dialogs.
 * Returns 0 on success, -1 on failure. */
int sap_open_project(void *mainWindowHandle, const char *path);

/* Resets the live session to an untitled/empty project without quitting
 * the process. Returns 0 on success, -1 on invalid handle. Not wired to
 * MCP project.close in the current plan (session Unbind only); kept as a
 * primitive for a later idle-process reset path. */
int sap_close_project(void *mainWindowHandle);

/* Returns 1 if MLT.projectFolder() is non-empty (folder-type project),
 * 0 if empty (file-type / no project folder), -1 on invalid handle.
 * Mirrors the live kSnapflowProjectFolder flag after open/save. */
int sap_is_project_folder(void *mainWindowHandle);

/* Sets MLT.setProjectFolder(folder). Pass empty/NULL to clear (file-type).
 * Used before the first project.save of a brand-new folder-type project so
 * saveXML writes kSnapflowProjectFolder=1. Returns 0 on success, -1 on
 * invalid handle. */
int sap_set_project_folder(void *mainWindowHandle, const char *folder);
/* Writes the current project to outputXmlPath as a self-contained MLT XML
 * file (absolute clip source paths, not project-relative) via the same
 * real MainWindow::saveXML() primitive "Save As" uses -- it already
 * branches on whatever's actually populated (timeline multitrack /
 * playlist-only / single producer / empty), so this is a faithful export
 * of "the current project" in every state, not just the common
 * timeline-has-clips case. Used by sap-rust's file.export to hand a real,
 * standalone-renderable XML to `melt` on a background thread. Returns 0 on
 * success, -1 on failure (invalid handle, or MLT.saveXML() itself fails). */
int sap_export_project_xml(void *mainWindowHandle, const char *outputXmlPath);

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
int sap_playback_fast_forward(void *mainWindowHandle);
char *sap_set_clip_speed(void *mainWindowHandle, int trackIndex, int clipIndex, double speed);

/* Transport controls -- same Player slots the editor Play/Pause/Stop
 * buttons drive. play uses speed (typically 1.0). pause uses position
 * (-1 keeps current). Returns 0 on success, -1 on error. */
int sap_playback_play(void *mainWindowHandle, double speed);
int sap_playback_pause(void *mainWindowHandle, long long position);
int sap_playback_stop(void *mainWindowHandle);

/* Returns heap-allocated JSON
 * {"playing":bool,"position":N,"duration":N} from Player + Controller
 * isPaused(), or NULL on error. Caller frees via sap_free_string. */
char *sap_playback_get_state(void *mainWindowHandle);

/* Appends an image-sequence producer to the playlist bin (native
 * qimage/pixbuf sequence path: printf resource, ttl, begin,
 * shotcut_sequence -- same as ImageProducerWidget sequence mode).
 * path is the first frame file; ttl is frames-per-image (default 1 when
 * <=0); begin may be NULL to auto-detect from the filename digits.
 * Returns playlist-entry JSON like sap_playlist_append, or NULL on error.
 * Caller frees via sap_free_string. */
char *sap_playlist_append_image_sequence(void *mainWindowHandle,
                                         const char *path,
                                         int ttl,
                                         const char *begin);

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

/* Inserts sourcePath as a real MLT producer BEFORE the clip currently at
 * clip-slot `clipIndex` on trackIndex (clipIndex == that track's current
 * clip count means "insert at the end", equivalent to append), RIPPLING
 * all downstream clips on that track forward to make room -- this is the
 * real distinct primitive from sap_move_clip/sap_append_clip: per
 * rust-fork/01-jsonrpc-spec.md's `edit.insertClip`, it wraps the same
 * Timeline::InsertCommand (timelinecommands.h:95) that
 * TimelineDock::insert() pushes internally, called directly here (not via
 * TimelineDock::insert() itself, which reads the system clipboard/"current
 * source" instead of taking a path -- same reasoning as sap_append_clip
 * vs. TimelineDock::append()). clipIndex is a clip-slot index rather than
 * an absolute frame, matching sap_move_clip's toClipIndex convention (this
 * FFI/Rust layer models tracks as an ordered clip list, not raw frame
 * offsets); the absolute insert frame is derived internally from the
 * target slot's start position exactly like sap_move_clip derives its own
 * targetPosition.
 *
 * Returns a heap-allocated, NUL-terminated JSON object string describing
 * the inserted clip, e.g. `{"clipId":"t0c1","index":1,"inFrame":0,
 * "outFrame":119}`, re-read from the real MultitrackModel::getClipInfo()
 * after the insert (not an echo of the request). NULL on error (invalid
 * handle/trackIndex, out-of-range clipIndex, locked track, or sourcePath
 * fails to open as a valid MLT producer). Caller must free the returned
 * pointer via sap_free_string. */
char *sap_insert_clip(void *mainWindowHandle, int trackIndex, int clipIndex, const char *sourcePath);

/* Places sourcePath as a real MLT producer starting at clip-slot `clipIndex`
 * on trackIndex, REPLACING (not rippling) whatever clip currently occupies
 * that slot -- the real distinct primitive from sap_insert_clip: per
 * rust-fork/01-jsonrpc-spec.md's `edit.overwriteClip`, it wraps the real
 * Timeline::OverwriteCommand (timelinecommands.h:123), a "drop and replace"
 * that leaves downstream clips at the same clip-slot indices, called
 * directly here (not via TimelineDock::overwrite(), which reads the system
 * clipboard/"current source" instead of taking a path -- same reasoning as
 * sap_append_clip vs. TimelineDock::append()). `clipIndex` == that track's
 * current clip count means "no clip to replace", equivalent to
 * sap_append_clip. clipIndex is a clip-slot index rather than an absolute
 * frame, matching sap_insert_clip/sap_move_clip's convention; the absolute
 * overwrite frame is derived internally from the target slot's start
 * position exactly like sap_insert_clip derives its own insert position.
 *
 * Returns a heap-allocated, NUL-terminated JSON object string describing
 * the placed clip, e.g. `{"clipId":"t0c1","index":1,"inFrame":0,
 * "outFrame":119}`, re-read from the real MultitrackModel::getClipInfo()
 * after the overwrite (not an echo of the request). NULL on error (invalid
 * handle/trackIndex, out-of-range clipIndex, locked track, or sourcePath
 * fails to open as a valid MLT producer). Caller must free the returned
 * pointer via sap_free_string. */
char *sap_overwrite_clip(void *mainWindowHandle, int trackIndex, int clipIndex, const char *sourcePath);

/* Returns a heap-allocated, NUL-terminated MLT XML string serializing the
 * *live* Mlt::Producer sitting at playlist bin index `index` (with any
 * attached filters intact -- e.g. a sap_generator_create_title clip's
 * dynamictext/qtext filter), for `{source:{playlistIndex}}` per
 * rust-fork/01-jsonrpc-spec.md's edit.appendClip/insertClip/overwriteClip
 * source union. Distinct from sap_playlist_get's "path" field, which is
 * only the raw resource string (e.g. "color:#00000000" for a title) --
 * reopening that resource fresh would silently drop the attached filter
 * chain, unlike this XML which is the exact producer as it sits in the
 * bin. Feed the result to sap_append_clip_xml/sap_insert_clip_xml/
 * sap_overwrite_clip_xml. NULL on error (invalid handle, out-of-range
 * index, or an unexpectedly invalid producer). Caller must free the
 * returned pointer via sap_free_string. */
char *sap_playlist_get_xml(void *mainWindowHandle, int index);

/* XML-sourced siblings of sap_append_clip/sap_insert_clip/
 * sap_overwrite_clip: identical undoable Timeline::AppendCommand/
 * InsertCommand/OverwriteCommand plumbing and return shape, but take a
 * ready-made MLT producer XML string directly instead of opening a
 * filesystem path -- for `{source:{xml}}` (caller already has raw
 * producer XML, e.g. clipboard/duplicate case) and `{source:
 * {playlistIndex}}` (resolve via sap_playlist_get_xml first) per
 * rust-fork/01-jsonrpc-spec.md's shared source union. NULL on error
 * (invalid handle/trackIndex, out-of-range clipIndex, locked track, or
 * empty xml). Caller must free string results via sap_free_string. */
char *sap_append_clip_xml(void *mainWindowHandle, int trackIndex, const char *xml);
char *sap_insert_clip_xml(void *mainWindowHandle, int trackIndex, int clipIndex, const char *xml);
char *sap_overwrite_clip_xml(void *mainWindowHandle, int trackIndex, int clipIndex, const char *xml);

/* Renders the pixels of the given absolute timeline frame from the
 * currently open project's live producer (Controller::producer(), the same
 * Mlt::Producer instance driving the app's own preview/player), using the
 * real Controller::image() primitive (mltcontroller.cpp) that Snapflow's own
 * timeline/property thumbnails already call, then encodes the result via
 * Qt's QImage::save() -- JPEG unless format is "png"/"PNG". *outLen
 * receives the byte length of the returned buffer; returns NULL (and
 * *outLen = 0) on error (invalid handle/frame/producer, or no image codec
 * plugin available for the requested format). Caller must free the
 * returned pointer via sap_free_bytes.
 *
 * Caveat (documented limitation, not a bug): Controller::image() seeks the
 * shared MLT.producer() instance to render, exactly like Snapflow's existing
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

/* Creates a real MLT title-card producer (a `color:` producer with a
 * `dynamictext` (mode="simple", default) or `qtext` (mode="rich") filter
 * attached -- the same construction TextProducerWidget::newProducer()/
 * createFilter() use, minus the QWidget UI) and appends it to the real
 * Playlist bin via `PlaylistModel::append()`. text is required (rich mode
 * sets the filter's `html` property verbatim; simple mode sets `argument`
 * plus `fgcolour`). fgColour/bgColour are `#AARRGGBB` strings (default
 * opaque white text, `#00000000` fully-transparent background). Returns a
 * heap-allocated JSON object `{"index":N,"name":"Title: ...",
 * "source":{"kind":"title","mode":"...","text":"..."},"durationFrames":N}`
 * matching sap-rust's `PlaylistEntry` wire shape, or NULL on error
 * (invalid handle, no playlist dock, missing/empty text). Caller must
 * free via sap_free_string. */
char *sap_generator_create_title(void *mainWindowHandle,
                                 const char *mode,
                                 const char *text,
                                 const char *fgColour,
                                 const char *bgColour);

/* Creates a real MLT plain `color:` producer (the same construction
 * ColorProducerWidget::newProducer() uses, minus the QWidget UI, and
 * minus any attached text filter -- see sap_generator_create_title for
 * that variant) and appends it to the real Playlist bin via
 * `PlaylistModel::append()`. hexColor is an `#AARRGGBB` string (required,
 * e.g. `#00000000` for a fully-transparent spacer clip). Returns a
 * heap-allocated JSON object `{"index":N,"name":"...",
 * "source":{"kind":"color","hexColor":"..."},"durationFrames":N}`
 * matching sap-rust's `PlaylistEntry` wire shape, or NULL on error
 * (invalid handle, no playlist dock, missing/empty hexColor). Caller
 * must free via sap_free_string. */
char *sap_generator_create_color(void *mainWindowHandle, const char *hexColor);

/* Subtitle operations on the real per-project `SubtitlesModel`
 * (`TimelineDock::subtitlesModel()`, loaded from the current tractor --
 * requires at least one clip already on the timeline;
 * `MainWindow::isMultitrackValid()` mirrors real Snapflow's own
 * `SubtitlesDock::addSubtitleTrack()` guard, since `SubtitlesModel`'s own
 * mutating methods silently no-op when no producer has been loaded via
 * `load()`). All mutating calls go through `SubtitlesModel`'s own real
 * QUndoCommand-pushing methods (`InsertTrackCommand`/
 * `OverwriteSubtitlesCommand`/`RemoveSubtitlesCommand` -- genuinely
 * undoable via Ctrl+Z, unlike filter.add/filter.setProperty). start/end
 * frame parameters are converted to/from milliseconds via the real
 * project fps (`MLT.profile().fps()`), since `SubtitlesModel`'s own
 * primitive uses millisecond timestamps, not frames. */

/* Adds a new subtitle track (auto-named "Subtitle N", 1-based, empty
 * language) via `SubtitlesModel::addTrack()`. Returns a heap-allocated
 * JSON object `{"trackIndex":N}`, or NULL on error (invalid handle, or no
 * multitrack producer loaded yet -- add a clip to the timeline first).
 * Caller must free via sap_free_string. */
char *sap_subtitles_add_track(void *mainWindowHandle);

/* Appends one subtitle cue [startFrame,endFrame] (inclusive, converted to
 * milliseconds) with text to trackIndex via `SubtitlesModel::appendItem()`
 * (a real, undoable `OverwriteSubtitlesCommand`). Returns 0 on success, -1
 * on error (invalid handle/trackIndex, or no multitrack producer loaded). */
int sap_subtitles_append_item(void *mainWindowHandle,
                              int trackIndex,
                              long long startFrame,
                              long long endFrame,
                              const char *text);

/* Removes the cues at itemIndicesJson (a JSON array of 0-based,
 * append-order indices) from trackIndex via `SubtitlesModel::removeItems()`
 * (a real, undoable `RemoveSubtitlesCommand`). NOTE: the real primitive
 * only supports removing one contiguous `[firstItemIndex,lastItemIndex]`
 * run at a time (there is no arbitrary-multi-select removal command) --
 * itemIndicesJson's sorted/deduplicated indices MUST already form such a
 * contiguous run (a single index always qualifies). Returns 0 on success,
 * -1 on error (invalid handle/trackIndex, any index out of range, empty
 * array, or a non-contiguous index set). */
int sap_subtitles_remove_items(void *mainWindowHandle,
                               int trackIndex,
                               const char *itemIndicesJson);

/* Imports path (an SRT file, read via the real
 * `Subtitles::readFromSrtFile`) into subtitle track 0 (creating it via
 * `addTrack()` first if no tracks exist yet) when newTrack is 0, or into a
 * brand-new track (via `SubtitlesModel::importSubtitlesToNewTrack()`, a
 * single undo macro combining the real `InsertTrackCommand` +
 * `OverwriteSubtitlesCommand`) when newTrack is non-zero. Returns a
 * heap-allocated JSON object `{"trackIndex":N}`, or NULL on error (invalid
 * handle, unreadable/empty-of-cues path, or no multitrack producer
 * loaded). Caller must free via sap_free_string. */
char *sap_subtitles_import_srt(void *mainWindowHandle, const char *path, int newTrack);

/* Exports trackIndex's cues to path via `SubtitlesModel::exportSubtitles()`
 * (wraps the real `Subtitles::writeToSrtFile`). Returns a heap-allocated
 * copy of path itself (relative-path resolution against the project root,
 * mirroring `sap_export_project_xml`'s own convention, is left to the
 * caller), or NULL on error (invalid handle/trackIndex, or the write
 * failed). Caller must free via sap_free_string. */
char *sap_subtitles_export_srt(void *mainWindowHandle, int trackIndex, const char *path);

/* Attaches (or, if a matching one is already attached, leaves in place) a
 * real MLT "subtitle" burn-in filter on the timeline output (the tractor
 * itself, i.e. an "on Output" filter), feeding it from trackIndex's cues,
 * mirroring `SubtitlesDock::burnInOnTimeline()`'s filter setup exactly
 * (same mlt_service, colours, outline, geometry, size) but driven
 * headlessly instead of through `MainWindow::onCreateOrEditFilterOnOutput`
 * (which requires a live FiltersDock selection). Idempotent per track:
 * re-running for the same trackIndex does not add a duplicate filter.
 * Returns 0 on success, -1 on error (invalid handle/trackIndex, or no
 * multitrack producer loaded). */
int sap_subtitles_burn_in(void *mainWindowHandle, int trackIndex);

/* Sets/gets the real project Notes free-text field via
 * `NotesDock::setText()`/`getText()` (saved/restored with the project XML
 * as a `snapflow:projectNotes` property, not a QUndoCommand -- matches
 * real Snapflow's own Notes panel, which has no undo/redo either).
 * sap_notes_set_text returns 0 on success, -1 on error (invalid handle).
 * sap_notes_get_text returns a heap-allocated copy of the current text
 * (empty string, not NULL, when there is none), or NULL on error (invalid
 * handle). Caller must free via sap_free_string. */
int sap_notes_set_text(void *mainWindowHandle, const char *text);
char *sap_notes_get_text(void *mainWindowHandle);

/* Real "recent files" MRU list operations via `RecentDock::add()`/
 * `remove()`, backed by `Settings.recent()`/`Settings.setRecent()` (a
 * `QSettings`-persisted, application-wide list -- NOT scoped to any one
 * project, same real Snapflow MRU semantics 01-jsonrpc-spec.md's
 * `recent.*` namespace note calls out as "low-value but real"; every
 * project shares the same underlying list). */

/* Adds path via the real `RecentDock::add()` (also updates
 * `Settings.setProjects()` when path ends in .mlt, matching real
 * Snapflow). Returns 0 on success, -1 on error (invalid handle). */
int sap_recent_add(void *mainWindowHandle, const char *path);

/* Removes path via the real `RecentDock::remove()`. Returns a
 * heap-allocated copy of path on success, or NULL on error (invalid
 * handle, or path was not present in the list). Caller must free via
 * sap_free_string. */
char *sap_recent_remove(void *mainWindowHandle, const char *path);

/* Returns a heap-allocated JSON array of strings -- the real recent-files
 * list (`Settings.recent()`, newest first) -- or NULL on error (invalid
 * handle). Caller must free via sap_free_string. */
char *sap_recent_list(void *mainWindowHandle);

/* Frees a string returned by sap_list_tracks. */
void sap_free_string(char *s);

/* Notification bridge hook (Qt -> SAP). Forwards jsonPayload (a small JSON
 * object with at least a "type" field naming the notification method, e.g.
 * `{"type":"edit.changed"}`) to sap_ffi_notify_bridge (implemented in Rust,
 * declared below), which fans it out to every SAP client currently bound
 * to this process's single open document via the same per-project
 * broadcast channel real RPC-driven edits use -- see server.rs's `serve`
 * doc comment for why "every project", not one. Also logs to stderr
 * (cheap, useful for headless-harness debugging). Safe to call before
 * sap_start_server has finished starting the server: sap_ffi_notify_bridge
 * silently drops the event in that case, matching this hook's own
 * best-effort semantics -- it must never block or throw on the Qt main
 * thread it runs on. */
void sap_emit_event(const char *jsonPayload);

/* Connects MultitrackModel::modified (the nearest real, already-emitted
 * aggregate signal, per 02-rust-embedding.md's "signal bridge" section) to
 * sap_emit_event. Must be called on the Qt main thread (it performs a plain
 * QObject::connect, not a cross-thread call) -- snapflow/src/main.cpp calls
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

/* Implemented in Rust (sap-rust/src/ffi_backend.rs) -- the actual bridge
 * body sap_emit_event above forwards into. jsonPayload must be a valid
 * NUL-terminated C string; NULL is a silent no-op. Non-blocking, never
 * panics/throws across the FFI boundary -- safe to call from a Qt signal
 * handler on the main thread. */
void sap_ffi_notify_bridge(const char *jsonPayload);

#ifdef __cplusplus
}
#endif

#endif /* SAP_FFI_H */
