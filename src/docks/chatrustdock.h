// Phase 2 real-integration spike: hosts the Rust-rendered (Slint) chat
// panel inside a real Shotcut dock, per
// memory/head/gen/plans/rust-qt-cross-render-option-b.md and
// memory/head/gen/plans/slint-dock-plugin-system.md. Additive only --
// mirrors NotesDock's role as a QDockWidget sibling; does not modify any
// existing dock.
#ifndef CHATRUSTDOCK_H
#define CHATRUSTDOCK_H

#include <QDockWidget>

class RustPanelItem;
class QShortcut;

class ChatRustDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ChatRustDock(QWidget *parent = 0);

public slots:
    // Forwards to the embedded RustPanelItem; connected to
    // MainWindow::changeTheme()'s resolved theme name (see
    // mainwindow.cpp) so the chat panel's Slint markup tracks the app's
    // Dark/Light/System theme instead of being hardcoded.
    void applyTheme(const QString &theme);
    // Forwards the active project's path to the embedded RustPanelItem;
    // connected to MainWindow::producerOpened (see mainwindow.cpp), which
    // fires on every project open/close/switch. `withReopen` is unused --
    // matches MainWindow::producerOpened(bool)'s signal signature so a
    // direct connect() works without a lambda.
    void updateProjectPath(bool withReopen = true);

private:
    // Global thread-switch/search shortcuts (Ctrl+Alt+Up/Down, Ctrl+K),
    // active window-wide so they work even when nothing inside this dock
    // has Qt focus -- RustPanelItem::keyPressEvent already handles the
    // same chords when this dock (or something in it) does have focus, so
    // these exist purely to cover the "dock not focused at all" case. Only
    // dispatch while this dock is actually visible, so a hidden/closed
    // dock doesn't steal these combos from the rest of Shotcut.
    void invokePanelCommand(int command);

    RustPanelItem *m_panel = nullptr;
    QShortcut *m_previousThreadShortcut = nullptr;
    QShortcut *m_nextThreadShortcut = nullptr;
    QShortcut *m_openSearchShortcut = nullptr;
};

#endif // CHATRUSTDOCK_H
