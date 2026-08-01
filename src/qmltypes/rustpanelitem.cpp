#include "rustpanelitem.h"
#include "panel_ffi.h"

#include <QCoreApplication>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QPainter>
#include <QCursor>
#include <QQuickWindow>
#include <QScreen>
#include <QDebug>

void RustPanelItem::requestRepaint()
{
    update();
    // Flushes whatever's already posted (including the UpdateRequest/
    // paint-scheduling event `update()` just posted) without entering a
    // nested event loop the way QCoreApplication::processEvents() would
    // (this is called from mouse/key handlers and a QTimer callback --
    // processEvents() there risks reentrant timer/input dispatch). This
    // only drains the already-queued backlog synchronously; it does not
    // wait for or process any new incoming events.
    QCoreApplication::sendPostedEvents();
}

bool RustPanelItem::invokeCommand(Command command)
{
    ensureHandle();
    if (!m_handle)
        return false;
    return panel_rust_invoke_command(m_handle, static_cast<int>(command));
}

namespace {
bool isThreadCommandChord(const QKeyEvent *event)
{
    const Qt::KeyboardModifiers mods = event->modifiers();
    if (mods.testFlag(Qt::ControlModifier) && mods.testFlag(Qt::AltModifier)
        && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down))
        return true;
    if (mods.testFlag(Qt::ControlModifier) && !mods.testFlag(Qt::AltModifier)
        && !mods.testFlag(Qt::ShiftModifier) && event->key() == Qt::Key_K)
        return true;
    return false;
}
} // namespace

bool RustPanelItem::event(QEvent *event)
{
    // Two independent needs share this one override -- see the .h
    // declaration's doc comment. While this item has Qt focus, claim
    // every ShortcutOverride so global single-key app shortcuts
    // (timeline's Backspace/Delete/Z/X) never win over typing/editing in
    // a focused Slint TextInput; that subsumes the thread-command chords
    // too when focused. When this item has NO Qt focus, still claim just
    // the Ctrl+Alt+Up/Down/Ctrl+K chords so they keep working via
    // ChatRustDock's window-wide QShortcut fallback.
    if (event->type() == QEvent::ShortcutOverride) {
        if (hasFocus()) {
            event->accept();
            return true;
        }
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (isThreadCommandChord(keyEvent)) {
            event->accept();
            return true;
        }
        // Shotcut binds many single-key (no-modifier) shortcuts on its main
        // window (e.g. bare "A" for Append, bare "/" for its own binding,
        // per QAction::setShortcut with the default Qt::WindowShortcut
        // context) -- those fire whenever *any* widget in the window has
        // focus, since Qt Quick items (unlike QLineEdit/QTextEdit) aren't
        // part of Qt's built-in "focused editor wins over a bare-letter
        // shortcut" heuristic. Concretely reported: typing "/" into the
        // chat composer instead opened Shotcut's own Keyboard Shortcuts
        // editor and never reached the composer (or, by the same
        // mechanism, thread/skill search, settings search, dropdown
        // filters, or the mention popup) at all. QEvent::ShortcutOverride
        // is Qt's own purpose-built escape hatch for exactly this: sent to
        // the focused item *before* the shortcut system decides to fire a
        // match, and accepting it tells Qt "let me handle this key
        // normally instead" -- the same mechanism QLineEdit uses
        // internally. Only claimed while some editable Slint surface
        // actually owns focus, so it does nothing when this dock is merely
        // visible but not the thing being typed into.
        if (m_handle && panel_rust_has_text_focus(m_handle)) {
            event->accept();
            return true;
        }
    }
    return QQuickPaintedItem::event(event);
}

RustPanelItem::RustPanelItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    // tasks/v2/enhance.yaml#task-4: without this, Qt never delivers
    // hoverMoveEvent/hoverLeaveEvent at all, so has-hover-driven state
    // inside the Slint scene (hover-tinted backgrounds, mouse-cursor
    // bindings) could never update outside of an actual click.
    setAcceptHoverEvents(true);
    setFlag(QQuickItem::ItemHasContents, true);
    // Needed so the chat compose box (a Slint TextInput) can actually
    // receive keyPressEvent()s once clicked -- see mousePressEvent().
    setFlag(QQuickItem::ItemIsFocusScope, true);

    // Phase 4: nothing else drives this single-threaded render loop to
    // notice background agent activity (see agent_bridge.rs's module
    // docs) -- poll on a plain timer and repaint if anything changed.
    // Interval targets the real display refresh rate (60-90fps, see
    // updatePollIntervalForRefreshRate()) instead of a fixed 80ms
    // (12.5fps) -- animations (hover fades, the sidebar's `animate
    // width`, the loading spinner, Flickable momentum) only ever advance
    // on this tick (see panel_rust_poll's doc comment in lib.rs), so a
    // fixed low-rate poll was capping every animation's smoothness well
    // below what the display can actually show, independent of how fast
    // the panel itself could render a frame.
    updatePollIntervalForRefreshRate();
    connect(this, &QQuickItem::windowChanged, this,
            &RustPanelItem::updatePollIntervalForRefreshRate);
    // Candidate fix (source-level investigation only, unconfirmed on a
    // real machine) for a reported real-desktop bug where most input into
    // this panel silently never arrived until an unrelated OS-level
    // window-activate cycle "fixed" it: Slint's own internal window-active
    // notion (see panel-rust's panel_rust_set_window_active) was never
    // being told about the host window's activation state at all. This
    // item's window() can become non-null before or after
    // componentComplete() runs, so both call handleWindowChanged().
    connect(this, &QQuickItem::windowChanged, this, &RustPanelItem::handleWindowChanged);
    connect(&m_pollTimer, &QTimer::timeout, this, &RustPanelItem::poll);
    m_pollTimer.start();
}

void RustPanelItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    // Proactively claim focus once this item finishes construction/QML
    // property binding, instead of relying purely on mousePressEvent's
    // reactive forceActiveFocus() -- see the .h declaration's doc comment.
    forceActiveFocus(Qt::OtherFocusReason);
    // In case this item was already attached to a window before
    // componentComplete() ran (windowChanged's connection above would have
    // fired before this override could run this same tick).
    handleWindowChanged(window());
}

void RustPanelItem::handleWindowChanged(QQuickWindow *win)
{
    if (!win)
        return;
    connect(win, &QQuickWindow::activeChanged, this, &RustPanelItem::pushWindowActiveState,
            Qt::UniqueConnection);
    pushWindowActiveState();
}

void RustPanelItem::pushWindowActiveState()
{
    ensureHandle();
    if (!m_handle)
        return;
    if (QQuickWindow *win = window())
        panel_rust_set_window_active(m_handle, win->isActive());
}

RustPanelItem::~RustPanelItem()
{
    if (m_handle)
        panel_rust_destroy(m_handle);
}

void RustPanelItem::updatePollIntervalForRefreshRate()
{
    qreal targetFps = kMinPollFps;
    if (QQuickWindow *win = window()) {
        if (QScreen *screen = win->screen()) {
            const qreal refreshRate = screen->refreshRate();
            // Some platforms/backends (notably under Xvfb, as this repo's
            // own host-e2e/VNC-dev harnesses run) report 0 or a negative
            // placeholder rather than a real rate -- fall back to
            // kMinPollFps rather than let that collapse the poll interval
            // to something absurd via qBound's lower-clamp-first behavior.
            if (refreshRate > 0)
                targetFps = qBound(kMinPollFps, refreshRate, kMaxPollFps);
            connect(win, &QWindow::screenChanged, this,
                    &RustPanelItem::updatePollIntervalForRefreshRate, Qt::UniqueConnection);
            connect(screen, &QScreen::refreshRateChanged, this,
                    &RustPanelItem::updatePollIntervalForRefreshRate, Qt::UniqueConnection);
        }
    }
    m_activePollIntervalMs = qMax(1, qRound(1000.0 / targetFps));
    // Only push the new interval onto the live timer immediately if it's
    // currently running at the active cadence -- if it's idle-backed-off
    // (see applyPollCadence()), the next poll() tick that finds real
    // activity will pick up the freshly-computed m_activePollIntervalMs
    // then. Forcing it here unconditionally would undo the idle backoff
    // every time this slot fires (e.g. a spurious screenChanged while
    // fully idle).
    if (!m_pollTimerIdle && m_pollTimer.interval() != m_activePollIntervalMs)
        m_pollTimer.setInterval(m_activePollIntervalMs);

    // This function is already wired to fire on windowChanged/screenChanged
    // (see constructor/below) -- exactly the cases where devicePixelRatio
    // can change (dragged to a different-DPI monitor) without width()/
    // height() themselves changing, so geometryChange() alone would never
    // notice. ensureHandle() no-ops unless dpr or size actually changed.
    ensureHandle();
    update();
}

void RustPanelItem::applyPollCadence(bool active)
{
    if (active) {
        if (m_pollTimerIdle || m_pollTimer.interval() != m_activePollIntervalMs) {
            m_pollTimer.setInterval(m_activePollIntervalMs);
            m_pollTimerIdle = false;
        }
        return;
    }
    if (m_pollTimerIdle)
        return;
    const int idleIntervalMs = qMax(1, qRound(1000.0 / kIdlePollFps));
    m_pollTimer.setInterval(idleIntervalMs);
    m_pollTimerIdle = true;
}

void RustPanelItem::ensureHandle()
{
    // QQuickPaintedItem::width()/height() are logical (CSS) pixels; on a
    // HiDPI screen those are smaller than the physical pixel grid Qt will
    // actually composite into. Without accounting for devicePixelRatio here,
    // panel_rust_create() was handed the *logical* size, so the Slint
    // software renderer produced a buffer at 1x resolution that Qt then
    // upscaled to fill the physical area in paint() -- visibly blurry on
    // any scaled display. Physical-pixel sizing plus the density push below
    // (Slint's own scale_factor, see dispatch.rs's ScaleFactorChanged) fixes
    // both the buffer resolution and the on-screen layout math together.
    const qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
    const unsigned int w = static_cast<unsigned int>(qMax(1.0, width() * dpr));
    const unsigned int h = static_cast<unsigned int>(qMax(1.0, height() * dpr));
    const bool dprChanged = !qFuzzyCompare(dpr, m_lastDevicePixelRatio);
    if (m_handle) {
        const bool sizeChanged = panel_rust_width(m_handle) != w || panel_rust_height(m_handle) != h;
        if (sizeChanged) {
            // panel_rust_create resizes the existing singleton in place. Do
            // not destroy/recreate it for every Qt geometry change, because
            // Slint's process-global software platform is installed only
            // once.
            panel_rust_create(w, h);
        }
        if (dprChanged) {
            panel_rust_apply_host_appearance(m_handle, ++m_appearanceGeneration, m_isDark,
                                              nullptr, 0, nullptr, 0, 1.0f,
                                              static_cast<float>(dpr));
            m_lastDevicePixelRatio = dpr;
        }
        return;
    }
    const bool initialIdentityApplied = m_hasPendingProjectPath && !m_pendingProjectClosed;
    if (initialIdentityApplied) {
        const QByteArray path = m_pendingProjectPath.toUtf8();
        m_handle = panel_rust_create_with_identity(
            w,
            h,
            reinterpret_cast<const unsigned char *>(path.constData()),
            static_cast<size_t>(path.size()),
            m_pendingUntitledProject);
    } else {
        m_handle = panel_rust_create(w, h);
    }
    if (!m_handle) {
        qWarning() << "RustPanelItem: panel_rust_create failed";
        return;
    }
    if (!m_pendingTheme.isEmpty())
        m_isDark = m_pendingTheme != "light";
    panel_rust_apply_host_appearance(m_handle, ++m_appearanceGeneration, m_isDark, nullptr, 0,
                                      nullptr, 0, 1.0f, static_cast<float>(dpr));
    m_lastDevicePixelRatio = dpr;
    if (m_hasPendingProjectPath && !initialIdentityApplied) {
        if (m_pendingUntitledProject)
            panel_rust_project_created_untitled(m_handle);
        else if (m_pendingProjectClosed)
            panel_rust_project_closed(m_handle);
        else {
            const QByteArray path = m_pendingProjectPath.toUtf8();
            panel_rust_set_project_path(
                m_handle,
                reinterpret_cast<const unsigned char *>(path.constData()),
                static_cast<size_t>(path.size()));
        }
    }
}

void RustPanelItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    ensureHandle();
    requestRepaint();
}

void RustPanelItem::mousePressEvent(QMouseEvent *event)
{
    ensureHandle();
    forceActiveFocus(Qt::MouseFocusReason);
    if (m_handle) {
        panel_rust_input_click(m_handle,
                                static_cast<unsigned int>(event->position().x()),
                                static_cast<unsigned int>(event->position().y()));
        requestRepaint();
    }
    event->accept();
}

void RustPanelItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void RustPanelItem::hoverMoveEvent(QHoverEvent *event)
{
    ensureHandle();
    if (m_handle) {
        const QPointF pos = event->position();
        if (panel_rust_input_hover(m_handle,
                                    static_cast<unsigned int>(qMax(0.0, pos.x())),
                                    static_cast<unsigned int>(qMax(0.0, pos.y()))))
            update();
    }
    event->accept();
}

void RustPanelItem::hoverLeaveEvent(QHoverEvent *event)
{
    if (m_handle) {
        if (panel_rust_input_hover_exit(m_handle))
            update();
    }
    event->accept();
}

void RustPanelItem::wheelEvent(QWheelEvent *event)
{
    ensureHandle();
    if (m_handle) {
        // Trackpads report precise pixel deltas directly; a real mouse
        // wheel only reports angleDelta (eighths of a degree per Qt's
        // docs, 120 units == one physical notch on a typical wheel). Scale
        // the latter to a comparable logical-pixel amount -- 15 degrees
        // (one notch) -> ~60px, matching common desktop "3 lines" scroll
        // conventions -- since panel_rust_input_scroll expects logical
        // pixels either way (see its own doc comment).
        QPointF delta = event->pixelDelta();
        if (delta.isNull()) {
            constexpr qreal kPixelsPerDegree = 4.0;
            delta = QPointF(event->angleDelta()) / 8.0 * kPixelsPerDegree;
        }
        panel_rust_input_scroll(m_handle,
                                 static_cast<float>(event->position().x()),
                                 static_cast<float>(event->position().y()),
                                 static_cast<float>(delta.x()),
                                 static_cast<float>(delta.y()));
        requestRepaint();
    }
    event->accept();
}

void RustPanelItem::keyPressEvent(QKeyEvent *event)
{
    // Thread-switch/search chords must fire even when this item has Qt
    // focus but no Slint-side text editor does (e.g. the sidebar itself is
    // focused, nothing is) -- panel_rust_input_key's own focus guard would
    // silently drop them otherwise (see its doc comment in lib.rs).
    // ChatRustDock's global QShortcuts cover the remaining case (this item
    // has no Qt focus at all).
    if (isThreadCommandChord(event)) {
        const Qt::KeyboardModifiers mods = event->modifiers();
        if (mods.testFlag(Qt::AltModifier))
            invokeCommand(event->key() == Qt::Key_Up ? PreviousThread : NextThread);
        else
            invokeCommand(OpenThreadSearch);
        requestRepaint();
        event->accept();
        return;
    }
    ensureHandle();
    if (m_handle) {
        const QByteArray text = event->text().toUtf8();
        if (panel_rust_input_key(m_handle,
                                  event->key(),
                                  reinterpret_cast<const unsigned char *>(text.constData()),
                                  static_cast<size_t>(text.size()),
                                  /*pressed=*/true,
                                  static_cast<int>(event->modifiers())))
            requestRepaint();
    }
    event->accept();
}

void RustPanelItem::keyReleaseEvent(QKeyEvent *event)
{
    if (m_handle) {
        const QByteArray text = event->text().toUtf8();
        panel_rust_input_key(m_handle,
                              event->key(),
                              reinterpret_cast<const unsigned char *>(text.constData()),
                              static_cast<size_t>(text.size()),
                              /*pressed=*/false,
                              static_cast<int>(event->modifiers()));
    }
    event->accept();
}

void RustPanelItem::setTheme(const QString &theme)
{
    m_pendingTheme = theme;
    m_isDark = theme != "light";
    if (!m_handle)
        return; // applied by ensureHandle() once the panel actually exists
    if (panel_rust_apply_appearance(m_handle, ++m_appearanceGeneration, m_isDark))
        requestRepaint();
}

void RustPanelItem::setProjectPath(const QString &path)
{
    // Idempotent: setCurrentFile used to call this twice per open (signal +
    // direct). Even after that was fixed, producerOpened can re-emit the
    // same path; recreating the Rust singleton again would re-attach ACP
    // and race the shared gateway.
    if (m_handle && m_hasPendingProjectPath && !m_pendingUntitledProject
        && !m_pendingProjectClosed && m_pendingProjectPath == path && !path.isEmpty()) {
        return;
    }
    m_pendingProjectPath = path;
    m_hasPendingProjectPath = true;
    m_pendingUntitledProject = false;
    m_pendingProjectClosed = false;
    if (!m_handle)
        return; // applied by ensureHandle() once the panel actually exists
    recreateForPendingProject();
}

void RustPanelItem::projectCreatedUntitled()
{
    if (m_handle && m_hasPendingProjectPath && m_pendingUntitledProject
        && !m_pendingProjectClosed) {
        return;
    }
    m_pendingProjectPath.clear();
    m_hasPendingProjectPath = true;
    m_pendingUntitledProject = true;
    m_pendingProjectClosed = false;
    if (m_handle)
        recreateForPendingProject();
}

void RustPanelItem::projectClosed()
{
    m_pendingProjectPath.clear();
    m_hasPendingProjectPath = true;
    m_pendingUntitledProject = false;
    m_pendingProjectClosed = true;
    if (m_handle) {
        // Closing the document must release all project-owned thread actors
        // immediately. The next paint recreates a display-only panel with
        // no identity, so no ACP session can capture the host cwd between
        // close and the next project-open lifecycle event.
        panel_rust_destroy(m_handle);
        m_handle = nullptr;
        m_lastDevicePixelRatio = -1.0;
    }
}

void RustPanelItem::recreateForPendingProject()
{
    // A project switch changes the bridge's thread set and its cold-start
    // records, so forwarding only a path would leave the old project's
    // model alive. Recreate the process-local Rust singleton; ensureHandle()
    // immediately seeds it with the pending identity and hydrates that
    // project's physical store.
    panel_rust_destroy(m_handle);
    m_handle = nullptr;
    m_lastDevicePixelRatio = -1.0;
    ensureHandle();
}

void RustPanelItem::renameProjectPath(const QString &oldPath, const QString &newPath)
{
    // Keep the pending path coherent even with no panel yet: whatever the
    // panel is eventually told the project is, it must be the NEW path.
    // The rename itself only means anything to a LIVE panel, since its
    // whole job is rebinding threads that already exist -- with no handle
    // there is nothing to rebind, and threads created later are simply
    // created under the new path.
    m_pendingProjectPath = newPath;
    m_hasPendingProjectPath = true;
    if (!m_handle)
        return;
    const QByteArray oldBytes = oldPath.toUtf8();
    const QByteArray newBytes = newPath.toUtf8();
    panel_rust_rename_project_path(m_handle,
                                   reinterpret_cast<const unsigned char *>(oldBytes.constData()),
                                   static_cast<size_t>(oldBytes.size()),
                                   reinterpret_cast<const unsigned char *>(newBytes.constData()),
                                   static_cast<size_t>(newBytes.size()));
}

void RustPanelItem::poll()
{
    if (!m_handle)
        return;
    const bool needsPaint = panel_rust_poll(m_handle);
    if (needsPaint)
        requestRepaint();
    // idle_poll_backoff: back this timer off to kIdlePollFps whenever the
    // panel had nothing to repaint this tick, and restore/keep the full
    // display-refresh cadence whenever it did -- see applyPollCadence()'s
    // doc comment in the header.
    applyPollCadence(needsPaint);

    // ui/tokens/cursor_host.slint's CursorHost global tracks the desired
    // cursor kind declaratively (hover/focus change-handlers), already
    // mapped to a Qt::CursorShape int on the Rust side -- see
    // panel_rust_cursor_shape's doc comment in panel_ffi.h for why this
    // indirection exists instead of Slint's own internal cursor tracking.
    const int shape = panel_rust_cursor_shape(m_handle);
    if (shape != m_lastCursorShape) {
        m_lastCursorShape = shape;
        setCursor(QCursor(static_cast<Qt::CursorShape>(shape)));
    }
}

void RustPanelItem::paint(QPainter *painter)
{
    ensureHandle();
    if (!m_handle)
        return;

    panel_rust_render(m_handle);

    const unsigned int w = panel_rust_width(m_handle);
    const unsigned int h = panel_rust_height(m_handle);
    const unsigned char *buf = panel_rust_buffer_ptr(m_handle);
    const size_t len = panel_rust_buffer_len(m_handle);
    if (!buf || w == 0 || h == 0 || len < static_cast<size_t>(w) * h * 4)
        return;

    // PremultipliedRgbaColor is {r, g, b, a} bytes, premultiplied -- exact
    // match for QImage::Format_RGBA8888_Premultiplied, no conversion needed.
    QImage image(buf, static_cast<int>(w), static_cast<int>(h),
                 static_cast<int>(w) * 4, QImage::Format_RGBA8888_Premultiplied);
    // w/h are physical pixels (ensureHandle() sizes the buffer at
    // width()*devicePixelRatio); telling QPainter that via
    // setDevicePixelRatio makes drawImage(0, 0, ...) place it at this
    // item's logical (0,0)-(width(),height()) rect at full physical
    // resolution instead of stretching a 1x buffer across more physical
    // pixels than it has data for.
    image.setDevicePixelRatio(m_lastDevicePixelRatio > 0.0 ? m_lastDevicePixelRatio : 1.0);
    painter->drawImage(0, 0, image);
}
