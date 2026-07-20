// C ABI declarations for panel-rust (Phase 1 render-bridge spike).
// Mirrors sap-rust's sap_ffi.h pattern (plain extern "C", opaque handle).
#pragma once

#include <cstdint>

extern "C" {
struct PanelHandle;

PanelHandle *panel_rust_create(unsigned int width, unsigned int height);
void panel_rust_destroy(PanelHandle *handle);
bool panel_rust_input_click(PanelHandle *handle, unsigned int x, unsigned int y);
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
