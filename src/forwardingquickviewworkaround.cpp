#include "forwardingquickviewworkaround.h"

#include <QDebug>
#include <QApplication>
#include <QQuickItem>

ForwardingQuickViewWorkaround::ForwardingQuickViewWorkaround(QQmlEngine * engine, QObject * receiver)
    : QQuickWidget(engine, 0)
    , m_receiver(receiver)
    , m_insideEventFilter(false)
{
    connect(qApp, SIGNAL(focusObjectChanged(QObject*)), SLOT(onFocusObjectChanged(QObject*)));
}

void ForwardingQuickViewWorkaround::onFocusObjectChanged(QObject *obj)
{
    if (!m_previousFocusObject.isNull())
        m_previousFocusObject->removeEventFilter(this);
    if (obj)
        obj->installEventFilter(this);
    m_previousFocusObject = obj;
}

void ForwardingQuickViewWorkaround::keyPressEvent(QKeyEvent* e)
{
    QQuickWidget::keyPressEvent(e);
    if (!e->isAccepted())
        qApp->sendEvent(m_receiver, e);
}

void ForwardingQuickViewWorkaround::keyReleaseEvent(QKeyEvent* e)
{
    QQuickWidget::keyReleaseEvent(e);
    if (!e->isAccepted())
        qApp->sendEvent(m_receiver, e);
}

bool ForwardingQuickViewWorkaround::eventFilter(QObject* target, QEvent* event)
{
    if (m_insideEventFilter)
        return false;

    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
        return false;

    if (qApp->focusWidget() != this)
        return false;

    QQuickItem * o = qobject_cast<QQuickItem*>(target);
    if (!o)
        return false;

    m_insideEventFilter = true;
    Q_ASSERT(o);
    quickWindow()->sendEvent(o, event);
    if (!event->isAccepted()) {
        event->accept();
        qApp->sendEvent(m_receiver, event);
    }
    m_insideEventFilter = false;
    return true;
}

