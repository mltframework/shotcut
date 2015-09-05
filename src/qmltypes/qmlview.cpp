/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "qmlview.h"
#include <QQuickView>
#include <QDebug>
#include <QSGMaterial>
#include <QSGSimpleTextureNode>

#include <QTimer>

#include <private/qquickitem_p.h>
#include <private/qsgrenderer_p.h>
#include <private/qquickcanvasitem_p.h>

QmlView::QmlView(QQuickView* qview)
    : QObject(qview)
    , m_qview(qview)
{
}

QPoint QmlView::pos()
{
    return m_qview->mapToGlobal(QPoint(0,0));
}

class QTBUG47714WorkaroundRenderListener : public QObject
{
public:
    QTBUG47714WorkaroundRenderListener(QQuickItem * item)
        : item(item)
        , oldTexture(0)
    {
        startTimer(0);
    }

    void timerEvent(QTimerEvent * event)
    {
        if (item) {
            QQuickWindow * window = item->window();
            connect(window, &QQuickWindow::beforeSynchronizing,
                    this, &QTBUG47714WorkaroundRenderListener::beforeSync);
            connect(window, &QQuickWindow::afterSynchronizing,
                    this, &QTBUG47714WorkaroundRenderListener::afterSync);
        }
        killTimer(event->timerId());
    }

    QSGSimpleTextureNode * nodeFromItem()
    {
        if (item.isNull())
        {
            deleteLater();
            return 0;
        }
        QQuickItemPrivate * priv = QQuickItemPrivate::get(item);
        QSGTransformNode * tnode = priv->itemNode();
        QSGGeometryNode * geom = 0;
        if (tnode->firstChild()->type() == QSGNode::GeometryNodeType)
            geom = static_cast<QSGGeometryNode*>(tnode->firstChild());
        else if (tnode->firstChild()->type() == QSGNode::OpacityNodeType
                && tnode->firstChild()->firstChild()
                && tnode->firstChild()->firstChild()->type() == QSGNode::GeometryNodeType)
            geom = static_cast<QSGGeometryNode*>(tnode->firstChild()->firstChild());

        return dynamic_cast<QSGSimpleTextureNode*>(geom);
    }


    void beforeSync()
    {
        QSGSimpleTextureNode * texNode = nodeFromItem();
        if (texNode)
            texNode->setOwnsTexture(false);
    }

    void afterSync()
    {
        QSGSimpleTextureNode * texNode = nodeFromItem();
        if (texNode) {
            oldTexture = texNode->texture();
        } else {
            delete oldTexture;
            oldTexture = 0;
        }
    }

    QPointer<QQuickItem> item;
    QSGTexture * oldTexture;
};

void QmlView::applyQTBUG47714Workaround(QObject * item)
{
    new QTBUG47714WorkaroundRenderListener(static_cast<QQuickItem*>(item));
}
