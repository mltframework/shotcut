/*
 * Copyright (c) 2013-2021 Meltytech, LLC
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

#include "filtersdock.h"
#include <QQuickView>
#include <QQuickItem>
#include <QtWidgets/QScrollArea>
#include <QQmlEngine>
#include <QDir>
#include <QUrl>
#include <QQmlContext>
#include <QAction>
#include <QIcon>
#include <Logger.h>
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "models/metadatamodel.h"
#include "models/attachedfiltersmodel.h"
#include "mltcontroller.h"

FiltersDock::FiltersDock(MetadataModel* metadataModel, AttachedFiltersModel* attachedModel, QWidget *parent) :
    QDockWidget(tr("Filters"), parent),
    m_qview(QmlUtilities::sharedEngine(), this)
{
    LOG_DEBUG() << "begin";
    setObjectName("FiltersDock");
    QIcon filterIcon = QIcon::fromTheme("view-filter", QIcon(":/icons/oxygen/32x32/actions/view-filter.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());
    m_qview.setFocusPolicy(Qt::StrongFocus);
    m_qview.quickWindow()->setPersistentSceneGraph(false);
#ifdef Q_OS_MAC
    setFeatures(DockWidgetClosable | DockWidgetMovable);
#else
    m_qview.setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("metadatamodel", metadataModel);
    m_qview.rootContext()->setContextProperty("attachedfiltersmodel", attachedModel);
    m_qview.rootContext()->setContextProperty("producer", &m_producer);
    connect(&m_producer, SIGNAL(seeked(int)), SIGNAL(seeked(int)));
    connect(this, SIGNAL(producerInChanged(int)), &m_producer, SIGNAL(inChanged(int)));
    connect(this, SIGNAL(producerOutChanged(int)), &m_producer, SIGNAL(outChanged(int)));
    setCurrentFilter(0, 0, QmlFilter::NoCurrentFilter);
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(resetQview()));

    LOG_DEBUG() << "end";
}

void FiltersDock::setCurrentFilter(QmlFilter* filter, QmlMetadata* meta, int index)
{
    if (filter && filter->producer().is_valid()) {
        m_producer.setProducer(filter->producer());
    } else {
        Mlt::Producer emptyProducer(mlt_producer(0));
        m_producer.setProducer(emptyProducer);
    }
    m_qview.rootContext()->setContextProperty("filter", filter);
    m_qview.rootContext()->setContextProperty("metadata", meta);
    if (filter)
        connect(filter, SIGNAL(changed()), SIGNAL(changed()));
    else
        disconnect(this, SIGNAL(changed()));
    QMetaObject::invokeMethod(m_qview.rootObject(), "setCurrentFilter", Q_ARG(QVariant, QVariant(index)));
}

bool FiltersDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void FiltersDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_F) {
        event->ignore();
    } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        event->accept();
    }
}

void FiltersDock::onSeeked(int position)
{
    if (m_producer.producer().is_valid()) {
        if (MLT.isMultitrack()) {
            // Make the position relative to clip's position on a timeline track.
            position -= m_producer.producer().get_int(kPlaylistStartProperty);
        } else {
            // Make the position relative to the clip's in point.
            position -= m_producer.in();
        }
        m_producer.seek(qBound(0, position, m_producer.duration()));
    }
}

void FiltersDock::onShowFrame(const SharedFrame& frame)
{
    if (m_producer.producer().is_valid()) {
        int position = frame.get_position();
        if (MLT.isMultitrack()) {
            // Make the position relative to clip's position on a timeline track.
            position -= m_producer.producer().get_int(kPlaylistStartProperty);
        } else {
            // Make the position relative to the clip's in point.
            position -= m_producer.in();
        }
        if (position >= 0 && position <= m_producer.duration())
            m_producer.seek(position);
    }
}

void FiltersDock::openFilterMenu() const
{
    QMetaObject::invokeMethod(m_qview.rootObject(), "openFilterMenu");
}

void FiltersDock::resetQview()
{
    LOG_DEBUG() << "begin";
    if (m_qview.status() != QQuickWidget::Null) {
        QObject* root = m_qview.rootObject();
        QObject::disconnect(root, SIGNAL(currentFilterRequested(int)),
                            this, SIGNAL(currentFilterRequested(int)));

        m_qview.setSource(QUrl(""));
    }

    QDir viewPath = QmlUtilities::qmlDir();
    viewPath.cd("views");
    viewPath.cd("filter");
    m_qview.engine()->addImportPath(viewPath.path());

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview.engine()->addImportPath(modulePath.path());

    m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview.quickWindow()->setColor(palette().window().color());
    QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("filterview.qml"));
    m_qview.setSource(source);

    QObject::connect(m_qview.rootObject(), SIGNAL(currentFilterRequested(int)),
        SIGNAL(currentFilterRequested(int)));
    emit currentFilterRequested(QmlFilter::NoCurrentFilter);
}
