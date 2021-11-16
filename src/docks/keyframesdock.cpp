/*
 * Copyright (c) 2016-2021 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "keyframesdock.h"

#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QDir>
#include <QUrl>
#include <QQmlContext>
#include <QIcon>
#include <QAction>

#include <Logger.h>

#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "models/attachedfiltersmodel.h"
#include "mltcontroller.h"
#include "settings.h"
#include "mainwindow.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlproducer.h"

KeyframesDock::KeyframesDock(QmlProducer* qmlProducer, QWidget *parent)
    : QDockWidget(tr("Keyframes"), parent)
    , m_qview(QmlUtilities::sharedEngine(), this)
    , m_qmlProducer(qmlProducer)
{
    LOG_DEBUG() << "begin";
    setObjectName("KeyframesDock");
    QIcon icon = QIcon::fromTheme("chronometer", QIcon(":/icons/oxygen/32x32/actions/chronometer.png"));
    setWindowIcon(icon);
    toggleViewAction()->setIcon(windowIcon());
    setMinimumSize(200, 50);
    m_qview.setFocusPolicy(Qt::StrongFocus);
    m_qview.quickWindow()->setPersistentSceneGraph(false);
#ifndef Q_OS_MAC
    m_qview.setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("keyframes", this);
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("parameters", &m_model);
    setCurrentFilter(0, 0);
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(load()));

    LOG_DEBUG() << "end";
}

int KeyframesDock::seekPrevious()
{
    if (m_qmlProducer) {
        int position = m_model.previousKeyframePosition(currentParameter(), m_qmlProducer->position() + m_qmlProducer->in());
        position -= m_qmlProducer->in();
        m_qmlProducer->setPosition(position);
        return m_model.keyframeIndex(currentParameter(), position + m_qmlProducer->in() - MAIN.filterController()->currentFilter()->in());
    }
    return 0;
}

int KeyframesDock::seekNext()
{
    if (m_qmlProducer) {
        int position = m_model.nextKeyframePosition(currentParameter(), m_qmlProducer->position() + m_qmlProducer->in());
        position -= m_qmlProducer->in();
        if (position > m_qmlProducer->position())
            m_qmlProducer->setPosition(position);
        else
            position = m_qmlProducer->position();
        return m_model.keyframeIndex(currentParameter(), position + m_qmlProducer->in() - MAIN.filterController()->currentFilter()->in());
    }
    return 0;
}

void KeyframesDock::setCurrentFilter(QmlFilter* filter, QmlMetadata* meta)
{
    if (!filter || !filter->producer().is_valid()) {
        filter = &m_emptyQmlFilter;
        meta = &m_emptyQmlMetadata;
    }
    m_model.load(filter, meta);
    disconnect(this, SIGNAL(changed()));
    connect(filter, SIGNAL(changed()), SIGNAL(changed()));
    connect(filter, SIGNAL(changed(QString)), &m_model, SLOT(onFilterChanged(QString)));
    connect(filter, SIGNAL(animateInChanged()), &m_model, SLOT(reload()));
    connect(filter, SIGNAL(animateOutChanged()), &m_model, SLOT(reload()));
    connect(filter, SIGNAL(inChanged(int)), &m_model, SLOT(onFilterInChanged(int)));
}

bool KeyframesDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        load(true);
    }
    return result;
}

void KeyframesDock::keyPressEvent(QKeyEvent* event)
{
    QDockWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void KeyframesDock::keyReleaseEvent(QKeyEvent* event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void KeyframesDock::onVisibilityChanged(bool visible)
{
    if (visible)
        load();
}

int KeyframesDock::currentParameter() const
{
    if (!m_qview.rootObject())
        return 0;
    return m_qview.rootObject()->property("currentTrack").toInt();
}

void KeyframesDock::load(bool force)
{
    LOG_DEBUG() << "begin";

    if (m_qview.source().isEmpty() || force) {
        QDir viewPath = QmlUtilities::qmlDir();
        viewPath.cd("views");
        viewPath.cd("keyframes");
        m_qview.engine()->addImportPath(viewPath.path());

        QDir modulePath = QmlUtilities::qmlDir();
        modulePath.cd("modules");
        m_qview.engine()->addImportPath(modulePath.path());

        m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_qview.quickWindow()->setColor(palette().window().color());
        QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("keyframes.qml"));
        m_qview.setSource(source);
    }
}

void KeyframesDock::reload()
{
    load(true);
}

void KeyframesDock::onProducerModified()
{
    // The clip name may have changed.
    if (m_qmlProducer)
        emit m_qmlProducer->producerChanged();
}
