#include "rustpanelitem.h"
#include "panel_ffi.h"

#include <QCoreApplication>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
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

bool RustPanelItem::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(e);
        if (isThreadCommandChord(keyEvent)) {
            e->accept();
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
            e->accept();
            return true;
        }
    }
    return QQuickPaintedItem::event(e);
}

RustPanelItem::RustPanelItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
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
    connect(&m_pollTimer, &QTimer::timeout, this, &RustPanelItem::poll);
    m_pollTimer.start();
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
    const int intervalMs = qMax(1, qRound(1000.0 / targetFps));
    if (m_pollTimer.interval() != intervalMs)
        m_pollTimer.setInterval(intervalMs);
}

void RustPanelItem::ensureHandle()
{
    const unsigned int w = static_cast<unsigned int>(qMax(1.0, width()));
    const unsigned int h = static_cast<unsigned int>(qMax(1.0, height()));
    if (m_handle) {
        if (panel_rust_width(m_handle) == w && panel_rust_height(m_handle) == h)
            return;
        // panel_rust_create resizes the existing singleton in place. Do not
        // destroy/recreate it for every Qt geometry change, because Slint's
        // process-global software platform is installed only once.
        panel_rust_create(w, h);
        return;
    }
    m_handle = panel_rust_create(w, h);
    if (!m_handle)
        qWarning() << "RustPanelItem: panel_rust_create failed";
    if (m_handle && !m_pendingTheme.isEmpty()) {
        const bool dark = m_pendingTheme != "light";
        panel_rust_apply_appearance(m_handle, ++m_appearanceGeneration, dark);
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
    if (!m_handle)
        return; // applied by ensureHandle() once the panel actually exists
    const bool dark = theme != "light";
    if (panel_rust_apply_appearance(m_handle, ++m_appearanceGeneration, dark))
        requestRepaint();
}

void RustPanelItem::poll()
{
    if (!m_handle)
        return;
    if (panel_rust_poll(m_handle))
        requestRepaint();
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
    painter->drawImage(0, 0, image);
}
