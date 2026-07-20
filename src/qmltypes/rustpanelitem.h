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

protected:
    // Claims QEvent::ShortcutOverride for the Ctrl+Alt+Up/Down/Ctrl+K
    // chords whenever this item has Qt focus, so Qt's shortcut dispatch
    // routes them into keyPressEvent() below instead of also letting
    // ChatRustDock's window-wide QShortcut fire for the same keystroke
    // (Qt Quick items aren't part of the widget shortcut-vs-focused-editor
    // arbitration QLineEdit gets automatically -- see the analogous
    // has-text-focus override this pattern is modeled on).
    bool event(QEvent *e) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    // Never wired up before -- panel_rust_input_scroll existed on the Rust
    // side (and in panel_ffi.h once added alongside this) but nothing ever
    // called it, so wheel/trackpad scroll inside the chat panel (thread
    // list, message history, ...) silently did nothing.
    void wheelEvent(QWheelEvent *event) override;

private:
    void ensureHandle();
    void poll();

    PanelHandle *m_handle = nullptr;
    QTimer m_pollTimer;
    // Applied once ensureHandle() creates the panel, in case setTheme()
    // was called (e.g. at dock construction, to match the app's current
    // theme) before the handle existed yet.
    QString m_pendingTheme;
    uint64_t m_appearanceGeneration = 0;
};
