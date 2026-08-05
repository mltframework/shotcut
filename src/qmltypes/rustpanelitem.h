// Phase 1 render-bridge spike: composites panel-rust's (Slint, software
// renderer) output into a Qt Quick scene via QQuickPaintedItem -- the
// CPU-copy path, not the zero-copy GPU texture path (per the sequencing
// decision in memory/head/gen/plans/rust-qt-cross-render-option-b.md: prove
// this bridge works before attempting GPU interop).
#pragma once

#include <QQuickPaintedItem>
#include <QTimer>
#include <QWheelEvent>

struct PanelHandle;
class QQuickWindow;

class RustPanelItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit RustPanelItem(QQuickItem *parent = nullptr);
    ~RustPanelItem() override;

    void paint(QPainter *painter) override;

    // Command ids for invokeCommand() -- kept in sync with panel-rust's
    // panel_rust_invoke_command doc comment (lib.rs).
    enum Command {
        PreviousThread = 0,
        NextThread = 1,
        OpenThreadSearch = 2,
    };

    // Focus-independent dispatch straight to panel_rust_invoke_command,
    // bypassing the compose/terminal-focus gate that keyPressEvent's normal
    // panel_rust_input_key path enforces. Used both by keyPressEvent (for
    // the recognized chords below, while this item has Qt focus but no
    // Slint-side text editor does) and by ChatRustDock's global QShortcuts
    // (for when this item has no Qt focus at all). Returns false if the
    // Rust panel handle doesn't exist yet.
    bool invokeCommand(Command command);

public slots:
    // Forwarded from ChatRustDock, itself driven by
    // MainWindow::changeTheme() -- see chatrustdock.cpp.
    void setTheme(const QString &theme);
    // Forwarded from ChatRustDock, itself driven by
    // MainWindow::producerOpened() -- see chatrustdock.cpp. Empty string
    // means no project currently open.
    void setProjectPath(const QString &path);
    void projectCreatedUntitled();
    void projectClosed();
    void renameProjectPath(const QString &oldPath, const QString &newPath);

protected:
    // Qt's shortcut dispatch sends every focused item a ShortcutOverride
    // event before delivering the real key press. Two independent needs
    // share this one override: (1) a standard QWidget text input (QLineEdit
    // et al.) accepts it so single-key global shortcuts elsewhere in the
    // app (timeline's Backspace/Delete/Z/X = Lift/Ripple Delete) don't
    // steal the keystroke instead of it reaching a focused chat text field
    // (thread rename, agent profile fields, etc.); (2) the Ctrl+Alt+Up/
    // Down/Ctrl+K thread-command chords need routing into keyPressEvent()
    // below even when this item has no Qt focus at all (ChatRustDock's
    // window-wide QShortcut is the fallback then) -- see invokeCommand's
    // own doc comment. See the .cpp implementation for how both are
    // combined without either one dropping the other's case.
    bool event(QEvent *event) override;
    // QQuickItem's QQmlParserStatus hook: fires once this item finishes
    // construction/QML property binding, whether or not it has ever been
    // clicked. Proactively claims focus here instead of relying solely on
    // mousePressEvent's reactive forceActiveFocus() -- candidate fix for a
    // reported real-desktop bug (source-level only, unconfirmed without a
    // real machine) where most input into this panel silently never
    // arrived until an unrelated OS-level window-activate cycle (alt-tab
    // away and back) started working. See the .cpp implementation for why
    // this alone isn't assumed sufficient (it also wires up window-active
    // forwarding below).
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    // tasks/v2/enhance.yaml#task-4: hover-only mouse movement (no button
    // held) was never forwarded to Slint at all -- setAcceptHoverEvents(true)
    // in the constructor plus these two overrides close that gap, so
    // has-hover-driven state (hover-tinted backgrounds, mouse-cursor
    // bindings) can update the way it does under Slint's own built-in
    // windowing backends.
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    // Never wired up before -- panel_rust_input_scroll existed on the Rust
    // side (and in panel_ffi.h once added alongside this) but nothing ever
    // called it, so wheel/trackpad scroll inside the chat panel (thread
    // list, message history, ...) silently did nothing.
    void wheelEvent(QWheelEvent *event) override;

private:
    void ensureHandle();
    void recreateForPendingProject();
    void poll();
    // Connects to the new QQuickWindow's activeChanged signal (once this
    // item is actually attached to one -- windowChanged can fire before or
    // after componentComplete()) and immediately pushes its current active
    // state, so panel-rust's Slint-side WindowActiveChanged tracking is
    // never left stale from the moment a window first exists.
    void handleWindowChanged(QQuickWindow *win);
    // Forwards window()->isActive() to panel_rust_set_window_active().
    // Wired to QQuickWindow::activeChanged by handleWindowChanged() above;
    // also called once up front so the panel learns the window's initial
    // active state without waiting for the first activeChanged signal.
    void pushWindowActiveState();
    // QQuickItem::update() only *schedules* a repaint for whenever Qt's
    // event loop next gets around to it -- under this app's forced
    // QSG_RENDER_LOOP=basic (see main.cpp) plus QQuickWidget hosting,
    // that scheduling reliably only gets flushed by *other* event traffic
    // (confirmed live: mouse movement flushes it, but a keyboard-only
    // trigger with zero subsequent mouse activity leaves the repaint
    // queued indefinitely -- state changes correctly, nothing paints
    // until some other event nudges the loop). Wraps update() with an
    // explicit flush of already-posted events so a repaint request is
    // never silently left waiting on unrelated event traffic to arrive.
    void requestRepaint();
    // Recomputes m_activePollIntervalMs from window()->screen()->
    // refreshRate(), clamped to [kMinPollFps, kMaxPollFps]. Called on
    // construction (screen not known yet -> falls back to kMinPollFps),
    // whenever this item's window changes, and whenever that window's
    // screen or the screen's reported refresh rate changes (covers
    // dragging the window to a different-refresh-rate monitor). Does not
    // itself touch m_pollTimer's live interval -- see applyPollCadence().
    void updatePollIntervalForRefreshRate();
    // idle_poll_backoff: panel_rust_poll's return value already tells us
    // whether anything needed a repaint this tick (streamed content,
    // Slint `animate`/`animation-tick()` motion, a busy spinner/pulse --
    // see panel_rust_poll's doc comment in lib.rs for the full list).
    // Nothing about m_pollTimer itself ever used that signal: it ran
    // unconditionally at the full display-refresh cadence (60-90fps)
    // forever, from construction to destruction, even with the panel
    // fully idle (no threads, nothing streaming, no animation) -- pure
    // wasted wakeups (Slint animation-clock update, an FFI call, a full
    // frame-input snapshot/dispatch, N thread/MCP-row scans) many times a
    // second doing nothing. Mouse/key/wheel input handlers already call
    // requestRepaint() directly (see their call sites in the .cpp), so
    // they stay instant regardless of this timer's cadence -- only
    // *background* activity (a streamed token arriving, an animation
    // starting) needs this timer to notice it, and it can afford to
    // notice that a little later while idle. `active` mirrors the most
    // recent panel_rust_poll() result: true keeps/restores the full
    // refresh-rate cadence, false backs the timer off to kIdlePollFps so
    // it still wakes up often enough to notice new background activity
    // promptly (worst case one idle tick of latency, then it snaps back
    // to full cadence) without spinning at 60-90Hz for nothing.
    void applyPollCadence(bool active);

    static constexpr qreal kMinPollFps = 60.0;
    static constexpr qreal kMaxPollFps = 90.0;
    // Idle-backoff cadence used whenever the previous poll tick found
    // nothing to repaint (no stream activity, no active animation, no
    // busy spinner/pulse -- see applyPollCadence()'s doc comment). Fast
    // enough that a background agent event or a newly-started animation
    // is noticed within a single frame time most users won't perceive as
    // latency; slow enough to cut this timer's idle wakeup rate by
    // roughly an order of magnitude versus the 60-90fps active cadence.
    static constexpr qreal kIdlePollFps = 8.0;

    PanelHandle *m_handle = nullptr;
    QTimer m_pollTimer;
    // Interval (ms) for the active/full cadence, last computed by
    // updatePollIntervalForRefreshRate() from the real display refresh
    // rate. Stored separately from m_pollTimer's live interval because
    // the timer may currently be sitting at the slower idle cadence
    // instead -- see applyPollCadence().
    int m_activePollIntervalMs = qRound(1000.0 / kMinPollFps);
    // Whether m_pollTimer is currently at the idle-backoff cadence
    // (true) or the active/full cadence (false). Starts false so the
    // very first poll() tick after construction runs at full cadence
    // (matches prior behavior) until it establishes the panel is
    // actually idle.
    bool m_pollTimerIdle = false;
    // Applied once ensureHandle() creates the panel, in case setTheme()
    // was called (e.g. at dock construction, to match the app's current
    // theme) before the handle existed yet.
    QString m_pendingTheme;
    // Mirrors setTheme()'s dark/light decision so ensureHandle() can push a
    // density-only appearance update (see m_lastDevicePixelRatio below)
    // without needing a fresh theme string. Defaults to the dark theme this
    // app opens in before setTheme() is ever called from ChatRustDock.
    bool m_isDark = true;
    uint64_t m_appearanceGeneration = 0;
    // Applied once ensureHandle() creates the panel, same reasoning as
    // m_pendingTheme above. Null (not just empty) means setProjectPath()
    // was never called yet -- distinct from "no project open".
    QString m_pendingProjectPath;
    bool m_hasPendingProjectPath = false;
    bool m_pendingUntitledProject = false;
    bool m_pendingProjectClosed = false;
    // Last Qt::CursorShape applied via setCursor() -- compared each poll()
    // tick against panel_rust_cursor_shape() so an unchanged cursor kind
    // (the common case) doesn't call setCursor() every 80ms.
    int m_lastCursorShape = -1;
    // Last devicePixelRatio the Rust panel was told about, so ensureHandle()
    // only re-pushes appearance/resizes the buffer when it actually changed
    // (moved to a different-DPI screen), not on every geometryChange.
    // -1 forces the first ensureHandle() call to always apply it.
    qreal m_lastDevicePixelRatio = -1.0;
};
