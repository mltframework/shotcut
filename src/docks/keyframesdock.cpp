/*
 * Copyright (c) 2016-2017 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

KeyframesDock::KeyframesDock(MetadataModel* metadataModel, AttachedFiltersModel* attachedModel, QWidget *parent)
    : QDockWidget(tr("Keyframes"), parent)
    , m_qview(QmlUtilities::sharedEngine(), this)
    , m_currentFilter(0)
{
    LOG_DEBUG() << "begin";
    setObjectName("KeyframesDock");
    QIcon icon = QIcon::fromTheme("chronometer", QIcon(":/icons/oxygen/32x32/actions/chronometer.png"));
    setWindowIcon(icon);
    toggleViewAction()->setIcon(windowIcon());
    setMinimumWidth(300);
    m_qview.setFocusPolicy(Qt::StrongFocus);
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("metadatamodel", metadataModel);
    m_qview.rootContext()->setContextProperty("attachedfiltersmodel", attachedModel);
    m_qview.rootContext()->setContextProperty("producer", &m_producer);
    connect(&m_producer, SIGNAL(seeked(int)), SIGNAL(seeked(int)));
    connect(this, SIGNAL(producerInChanged()), &m_producer, SIGNAL(inChanged()));
    connect(this, SIGNAL(producerOutChanged()), &m_producer, SIGNAL(outChanged()));
    setCurrentFilter(0, 0);
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(resetQview()));

    LOG_DEBUG() << "end";
}

void KeyframesDock::setCurrentFilter(QmlFilter* filter, QmlMetadata* meta)
{
    disconnect(this, SIGNAL(changed()));
    if (filter && filter->producer().is_valid()) {
        m_producer.setProducer(filter->producer());
        m_qview.rootContext()->setContextProperty("filter", filter);
    } else {
        Mlt::Producer emptyProducer(mlt_producer(0));
        m_producer.setProducer(emptyProducer);
        filter = &m_emptyQmlFilter;
        meta = &m_emptyQmlMetadata;
    }
    m_currentFilter = filter;
    m_qview.rootContext()->setContextProperty("filter", filter);
    m_qview.rootContext()->setContextProperty("metadata", meta);
    connect(filter, SIGNAL(changed()), SIGNAL(changed()));
}

void KeyframesDock::setFadeInDuration(int duration)
{
    QObject* filterUi = m_qview.rootObject()->findChild<QObject*>("fadeIn");
    if (filterUi) {
        filterUi->setProperty("duration", duration);
    }
}

void KeyframesDock::setFadeOutDuration(int duration)
{
    QObject* filterUi = m_qview.rootObject()->findChild<QObject*>("fadeOut");
    if (filterUi) {
        filterUi->setProperty("duration", duration);
    }
}

void KeyframesDock::onFilterInChanged(Mlt::Filter* filter)
{
    if (m_currentFilter->filter().get_filter() == filter->get_filter())
        emit m_currentFilter->inChanged();
}

void KeyframesDock::onFilterOutChanged(Mlt::Filter* filter)
{
    if (m_currentFilter->filter().get_filter() == filter->get_filter())
        emit m_currentFilter->outChanged();
}

bool KeyframesDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void KeyframesDock::onSeeked(int position)
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

void KeyframesDock::resetQview()
{
    LOG_DEBUG() << "begin";

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
