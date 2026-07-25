// C ABI declarations for panel-rust (Phase 1 render-bridge spike).
// Mirrors sap-rust's sap_ffi.h pattern (plain extern "C", opaque handle).
#pragma once

#include <cstdint>

extern "C" {
struct PanelHandle;

PanelHandle *panel_rust_create(unsigned int width, unsigned int height);
void panel_rust_destroy(PanelHandle *handle);
// Whether the composer/a local terminal/a secondary text input (search
// boxes, dropdown filters, the mention popup) currently owns focus -- see
// RustPanelItem::event()'s QEvent::ShortcutOverride handling.
bool panel_rust_has_text_focus(PanelHandle *handle);
bool panel_rust_input_click(PanelHandle *handle, unsigned int x, unsigned int y);
// Hover-only pointer movement (no button held) / the pointer leaving the
// panel's bounds entirely -- tasks/v2/enhance.yaml#task-4. Needed for
// has-hover-driven state (hover-tinted backgrounds, mouse-cursor bindings)
// to update at all outside of a click; see RustPanelItem's
// hoverMoveEvent/hoverLeaveEvent overrides.
bool panel_rust_input_hover(PanelHandle *handle, unsigned int x, unsigned int y);
bool panel_rust_input_hover_exit(PanelHandle *handle);
// Current OS mouse-cursor kind for whichever interactive Slint component
// last reported a hover/focus change (ui/tokens/cursor_host.slint's
// CursorHost global), already mapped to a Qt::CursorShape int value on the
// Rust side (see panel-rust's qt_cursor_shape_for_kind) -- mirrors
// map_qt_key's "map Qt-specific values in Rust" convention. Poll alongside
// panel_rust_poll and feed straight into setCursor(static_cast<Qt::CursorShape>(...)).
int panel_rust_cursor_shape(PanelHandle *handle);
// Forwards a Qt wheel/touchpad gesture in logical pixels -- see
// panel_rust_input_scroll's own doc comment in panel-rust/src/lib.rs.
bool panel_rust_input_scroll(PanelHandle *handle, float x, float y, float delta_x, float delta_y);
// qt_key is QKeyEvent::key(); text/text_len is QKeyEvent::text() as UTF-8
// (may be empty for pure modifier presses); modifiers is the raw
// QKeyEvent::modifiers() bitmask (Qt::KeyboardModifiers). See panel-rust's
// map_qt_key for the Qt -> Slint key mapping this expects.
bool panel_rust_input_key(PanelHandle *handle, int qt_key, const unsigned char *text, size_t text_len, bool pressed, int modifiers);
// Focus-independent command dispatch for host-global shortcuts (switch
// thread, open thread search) that must work even when panel_rust_input_key
// above would drop the event because neither the compose box nor a local
// terminal owns Slint focus. See panel-rust's panel_rust_invoke_command doc
// comment for the command ids (0 = previous thread, 1 = next thread,
// 2 = open thread search).
bool panel_rust_invoke_command(PanelHandle *handle, int command);
// theme is "dark"/"light"/etc, per MainWindow::changeTheme()'s resolved
// theme name.
bool panel_rust_set_theme(PanelHandle *handle, const unsigned char *theme, size_t theme_len);
bool panel_rust_apply_appearance(PanelHandle *handle, uint64_t generation, bool dark);
// active_project_binding phase: the currently-open MLT project's path
// (MainWindow::fileName()), pushed whenever MainWindow::producerOpened
// fires. Empty buffer (zero length) means no project open -- always
// call this on producerOpened, even when closing, so panel-rust's
// stored path can't go stale.
bool panel_rust_set_project_path(PanelHandle *handle, const unsigned char *path, size_t path_len);
// Save-As specifically: the project KEPT ITS IDENTITY but changed path.
// Distinct from panel_rust_set_project_path because the panel cannot tell
// "Save-As A->B" from "closed A, opened B" by watching the path alone --
// both look like one path replacing another -- and rebinding threads on
// the latter would merge two different projects' chat histories. Only the
// host knows which happened, so only the host can say. An empty old path
// (an Untitled project saved for the first time) is deliberately NOT a
// rename: those threads were created unscoped and must stay that way.
bool panel_rust_rename_project_path(PanelHandle *handle,
                                    const unsigned char *old_path,
                                    size_t old_path_len,
                                    const unsigned char *new_path,
                                    size_t new_path_len);
// Drains queued agent-bridge events (phase 4, rui-acp-client) into the
// Slint model. Must be polled periodically (see RustPanelItem's QTimer) --
// nothing else notices background agent activity on this single-threaded
// render loop. Returns whether a repaint is needed.
bool panel_rust_poll(PanelHandle *handle);
bool panel_rust_render(PanelHandle *handle);
const unsigned char *panel_rust_buffer_ptr(PanelHandle *handle);
size_t panel_rust_buffer_len(PanelHandle *handle);
unsigned int panel_rust_width(PanelHandle *handle);
unsigned int panel_rust_height(PanelHandle *handle);
}
