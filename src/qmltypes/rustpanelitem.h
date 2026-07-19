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
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

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
};
