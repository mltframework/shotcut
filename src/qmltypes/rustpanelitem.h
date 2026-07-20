// Phase 1 render-bridge spike: composites panel-rust's (Slint, software
// renderer) output into a Qt Quick scene via QQuickPaintedItem -- the
// CPU-copy path, not the zero-copy GPU texture path (per the sequencing
// decision in memory/head/gen/plans/rust-qt-cross-render-option-b.md: prove
// this bridge works before attempting GPU interop).
#pragma once

#include <QQuickPaintedItem>
#include <QTimer>

struct PanelHandle;

class RustPanelItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit RustPanelItem(QQuickItem *parent = nullptr);
    ~RustPanelItem() override;

    void paint(QPainter *painter) override;

public slots:
    // Forwarded from ChatRustDock, itself driven by
    // MainWindow::changeTheme() -- see chatrustdock.cpp.
    void setTheme(const QString &theme);
    // Forwarded from ChatRustDock, itself driven by
    // MainWindow::producerOpened() -- see chatrustdock.cpp. Empty string
    // means no project currently open.
    void setProjectPath(const QString &path);

protected:
    // Qt's shortcut dispatch sends every focused item a ShortcutOverride
    // event before delivering the real key press; a standard QWidget text
    // input (QLineEdit et al.) accepts it so single-key global shortcuts
    // elsewhere in the app (timeline's Backspace/Delete/Z/X = Lift/Ripple
    // Delete) don't steal the keystroke instead of it reaching the field.
    // RustPanelItem never participated in that protocol, so those global
    // shortcuts silently won over every text field in the chat panel
    // (thread rename, agent profile fields, etc.) whenever this item held
    // focus -- accepting it here while focused restores normal typing/
    // editing precedence, matching how those built-in widgets behave.
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
