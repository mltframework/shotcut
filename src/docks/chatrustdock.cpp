#include "chatrustdock.h"
#include "qmltypes/rustpanelitem.h"
#include "mainwindow.h"

#include <QQuickWidget>
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
}

void ChatRustDock::applyTheme(const QString &theme)
{
    if (m_panel)
        m_panel->setTheme(theme);
}

void ChatRustDock::updateProjectPath(bool withReopen)
{
    Q_UNUSED(withReopen)
    if (m_panel)
        m_panel->setProjectPath(MainWindow::singleton().fileName());
}
