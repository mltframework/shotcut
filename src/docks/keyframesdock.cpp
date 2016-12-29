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

#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlview.h"
#include "models/metadatamodel.h"
#include "models/attachedfiltersmodel.h"

KeyframesDock::KeyframesDock(MetadataModel* metadataModel, AttachedFiltersModel* attachedModel, QWidget *parent) :
    QDockWidget(tr("Keyframes"), parent),
    m_qview(QmlUtilities::sharedEngine(), this)
{
    LOG_DEBUG() << "begin";
    setObjectName("KeyframesDock");
    QIcon icon = QIcon::fromTheme("view-keyframes", QIcon(":/icons/oxygen/32x32/actions/view-filter.png"));
    setWindowIcon(icon);
    toggleViewAction()->setIcon(windowIcon());
    setMinimumWidth(300);
    m_qview.setFocusPolicy(Qt::StrongFocus);
    setWidget(&m_qview);

    QmlUtilities::setCommonProperties(m_qview.rootContext());
    m_qview.rootContext()->setContextProperty("view", new QmlView(&m_qview));
    m_qview.rootContext()->setContextProperty("metadatamodel", metadataModel);
    m_qview.rootContext()->setContextProperty("attachedfiltersmodel", attachedModel);
    setCurrentFilter(0, 0, -1);
    connect(m_qview.quickWindow(), SIGNAL(sceneGraphInitialized()), SLOT(resetQview()));

    LOG_DEBUG() << "end";
}

void KeyframesDock::clearCurrentFilter()
{
    m_qview.rootContext()->setContextProperty("metadata", 0);
    QMetaObject::invokeMethod(m_qview.rootObject(), "clearCurrentFilter");
    disconnect(this, SIGNAL(changed()));
}

void KeyframesDock::setCurrentFilter(QmlFilter* filter, QmlMetadata* meta, int index)
{
    m_qview.rootContext()->setContextProperty("filter", filter);
    m_qview.rootContext()->setContextProperty("metadata", meta);
    QMetaObject::invokeMethod(m_qview.rootObject(), "setCurrentFilter", Q_ARG(QVariant, QVariant(index)));
    if (filter)
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

bool KeyframesDock::event(QEvent *event)
{
    bool result = QDockWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        resetQview();
    }
    return result;
}

void KeyframesDock::resetQview()
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
    viewPath.cd("keyframes");
    m_qview.engine()->addImportPath(viewPath.path());

    QDir modulePath = QmlUtilities::qmlDir();
    modulePath.cd("modules");
    m_qview.engine()->addImportPath(modulePath.path());

    m_qview.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qview.quickWindow()->setColor(palette().window().color());
    QUrl source = QUrl::fromLocalFile(viewPath.absoluteFilePath("keyframes.qml"));
    m_qview.setSource(source);

    QObject::connect(m_qview.rootObject(), SIGNAL(currentFilterRequested(int)),
        SIGNAL(currentFilterRequested(int)));
    emit currentFilterRequested(-1);
}
