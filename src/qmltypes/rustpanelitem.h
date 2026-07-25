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
    void poll();
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
    // Recomputes m_pollTimer's interval from window()->screen()->
    // refreshRate(), clamped to [kMinPollFps, kMaxPollFps]. Called on
    // construction (screen not known yet -> falls back to kMinPollFps),
    // whenever this item's window changes, and whenever that window's
    // screen or the screen's reported refresh rate changes (covers
    // dragging the window to a different-refresh-rate monitor).
    void updatePollIntervalForRefreshRate();

    static constexpr qreal kMinPollFps = 60.0;
    static constexpr qreal kMaxPollFps = 90.0;

    PanelHandle *m_handle = nullptr;
    QTimer m_pollTimer;
    // Applied once ensureHandle() creates the panel, in case setTheme()
    // was called (e.g. at dock construction, to match the app's current
    // theme) before the handle existed yet.
    QString m_pendingTheme;
    uint64_t m_appearanceGeneration = 0;
    // Applied once ensureHandle() creates the panel, same reasoning as
    // m_pendingTheme above. Null (not just empty) means setProjectPath()
    // was never called yet -- distinct from "no project open".
    QString m_pendingProjectPath;
    bool m_hasPendingProjectPath = false;
    // Last Qt::CursorShape applied via setCursor() -- compared each poll()
    // tick against panel_rust_cursor_shape() so an unchanged cursor kind
    // (the common case) doesn't call setCursor() every 80ms.
    int m_lastCursorShape = -1;
};
