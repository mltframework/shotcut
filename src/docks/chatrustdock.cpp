#include "chatrustdock.h"
#include "qmltypes/rustpanelitem.h"
#include "mainwindow.h"

#include <QQuickView>
#include <QVBoxLayout>
#include <QWidget>

ChatRustDock::ChatRustDock(QWidget *parent)
    : QDockWidget(parent)
{
    setObjectName("chatRustDock");
    setWindowTitle(tr("Chat (Rust)"));

    auto *view = new QQuickView();
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    auto *panel = new RustPanelItem();
    panel->setWidth(400);
    panel->setHeight(500);
    view->setContent(QUrl(), nullptr, panel);

    QWidget *container = QWidget::createWindowContainer(view, this);
    // Phase 4 (chat-panel-ui-theme-parity.md): lowered from 280x300 so a
    // ~20%-of-window default width target isn't clamped back up on a
    // smaller monitor -- no maximum size is set (never was), so the dock
    // can still grow past its initial width via the native splitter drag
    // MainWindow::MainWindow's `resizeDocks(...)` call sets up.
    container->setMinimumSize(240, 260);

    setWidget(container);
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
