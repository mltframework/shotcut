#include "rustpanelitem.h"
#include "panel_ffi.h"

#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

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
    // 80ms is well under human-perceptible chat latency and nowhere near
    // a hot loop.
    m_pollTimer.setInterval(80);
    connect(&m_pollTimer, &QTimer::timeout, this, &RustPanelItem::poll);
    m_pollTimer.start();
}

RustPanelItem::~RustPanelItem()
{
    if (m_handle)
        panel_rust_destroy(m_handle);
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
        QByteArray bytes = m_pendingTheme.toUtf8();
        panel_rust_set_theme(m_handle,
                              reinterpret_cast<const unsigned char *>(bytes.constData()),
                              static_cast<size_t>(bytes.size()));
    }
}

void RustPanelItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    ensureHandle();
    update();
}

void RustPanelItem::mousePressEvent(QMouseEvent *event)
{
    ensureHandle();
    forceActiveFocus(Qt::MouseFocusReason);
    if (m_handle) {
        panel_rust_input_click(m_handle,
                                static_cast<unsigned int>(event->position().x()),
                                static_cast<unsigned int>(event->position().y()));
        update();
    }
    event->accept();
}

void RustPanelItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void RustPanelItem::keyPressEvent(QKeyEvent *event)
{
    ensureHandle();
    if (m_handle) {
        const QByteArray text = event->text().toUtf8();
        if (panel_rust_input_key(m_handle,
                                  event->key(),
                                  reinterpret_cast<const unsigned char *>(text.constData()),
                                  static_cast<size_t>(text.size()),
                                  /*pressed=*/true))
            update();
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
                              /*pressed=*/false);
    }
    event->accept();
}

void RustPanelItem::setTheme(const QString &theme)
{
    m_pendingTheme = theme;
    if (!m_handle)
        return; // applied by ensureHandle() once the panel actually exists
    QByteArray bytes = theme.toUtf8();
    if (panel_rust_set_theme(m_handle,
                              reinterpret_cast<const unsigned char *>(bytes.constData()),
                              static_cast<size_t>(bytes.size())))
        update();
}

void RustPanelItem::poll()
{
    if (!m_handle)
        return;
    if (panel_rust_poll(m_handle))
        update();
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
