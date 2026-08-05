#include "chatrustdock.h"
#include "qmltypes/rustpanelitem.h"
#include "mainwindow.h"

#include <QKeySequence>
#include <QQuickWidget>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWidget>

ChatRustDock::ChatRustDock(QWidget *parent)
    : QDockWidget(parent)
{
    setObjectName("chatRustDock");
    setWindowTitle(tr("Chat (Rust)"));

    // QQuickWidget, not QQuickView + createWindowContainer: the latter
    // embeds a *separate native child window* in the widget tree, which
    // is a documented Qt flicker source during QMainWindow::restoreState()
    // dock reflows (the native child's own backing store gets
    // shown/hidden/resized out of sync with its parent widget). Reported
    // symptom ("black/white screen for a sub-second time" switching
    // Logging/Editing/FX/Color layouts) matches exactly: Qt Quick's
    // default clear color is white, this app's dark theme surface is
    // pure black (theme.rs's DARK.surface) -- the flash was that white
    // default briefly showing before real (black-background) content
    // painted. QQuickWidget renders into an offscreen buffer composited
    // by the normal widget painting system instead of a separate native
    // window, and exposes setClearColor() directly so the fallback color
    // matches the theme instead of Qt's white default.
    auto *view = new QQuickWidget();
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setClearColor(MainWindow::resolvedTheme() == "light" ? QColor(0xff, 0xff, 0xff)
                                                                : QColor(0x00, 0x00, 0x00));

    auto *panel = new RustPanelItem();
    // Keep the embedded item's size hint below the fresh-launch target.
    // QMainWindow's dock splitter can then grow it to the user's chosen
    // width without the QQuickWindow forcing a 400px minimum.
    panel->setWidth(240);
    panel->setHeight(260);
    view->setContent(QUrl(), nullptr, panel);

    // Phase 4 (chat-panel-ui-theme-parity.md): lowered from 280x300 so a
    // ~20%-of-window default width target isn't clamped back up on a
    // smaller monitor -- no maximum size is set (never was), so the dock
    // can still grow past its initial width via the native splitter drag
    // MainWindow::MainWindow's `resizeDocks(...)` call sets up.
    view->setMinimumSize(240, 260);

    setWidget(view);
    m_panel = panel;

    // changeTheme() already ran (statically, in main.cpp before
    // MainWindow exists) by the time this dock is constructed -- apply
    // its resolved result once now instead of waiting on a live signal
    // that doesn't exist in this app (theme switches require a restart,
    // see MainWindow::restartAfterChangeTheme()).
    applyTheme(MainWindow::resolvedTheme());

    // Window-wide so they fire regardless of which widget in the main
    // window currently has focus -- see the header comment. Checked against
    // Shotcut's own action shortcut table when this plan was written; none
    // used Ctrl+Alt+Up/Down or bare Ctrl+K.
    m_previousThreadShortcut = new QShortcut(QKeySequence("Ctrl+Alt+Up"), this);
    m_previousThreadShortcut->setContext(Qt::WindowShortcut);
    connect(m_previousThreadShortcut, &QShortcut::activated, this,
            [this]() { invokePanelCommand(RustPanelItem::PreviousThread); });

    m_nextThreadShortcut = new QShortcut(QKeySequence("Ctrl+Alt+Down"), this);
    m_nextThreadShortcut->setContext(Qt::WindowShortcut);
    connect(m_nextThreadShortcut, &QShortcut::activated, this,
            [this]() { invokePanelCommand(RustPanelItem::NextThread); });

    m_openSearchShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
    m_openSearchShortcut->setContext(Qt::WindowShortcut);
    connect(m_openSearchShortcut, &QShortcut::activated, this,
            [this]() { invokePanelCommand(RustPanelItem::OpenThreadSearch); });
}

void ChatRustDock::applyTheme(const QString &theme)
{
    if (m_panel)
        m_panel->setTheme(theme);
}

void ChatRustDock::projectOpened(const QString &path)
{
    if (m_panel)
        m_panel->setProjectPath(path);
}

void ChatRustDock::projectCreatedUntitled()
{
    if (m_panel)
        m_panel->projectCreatedUntitled();
}

void ChatRustDock::projectClosed()
{
    if (m_panel)
        m_panel->projectClosed();
}

void ChatRustDock::setProjectPath(const QString &path)
{
    if (m_panel)
        m_panel->setProjectPath(path);
}

void ChatRustDock::renameProjectPath(const QString &oldPath, const QString &newPath)
{
    if (m_panel)
        m_panel->renameProjectPath(oldPath, newPath);
}

void ChatRustDock::invokePanelCommand(int command)
{
    if (!isVisible() || !m_panel)
        return;
    m_panel->invokeCommand(static_cast<RustPanelItem::Command>(command));
}
