/*
 * Copyright (c) 2016-2020 Meltytech, LLC
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

#ifndef KEYFRAMESDOCK_H
#define KEYFRAMESDOCK_H

#include "qmltypes/qmlfilter.h"
#include "models/metadatamodel.h"
#include "sharedframe.h"
#include "models/keyframesmodel.h"

#include <QDockWidget>
#include <QQuickWidget>
#include <QScopedPointer>

class QmlFilter;
class QmlMetadata;
class MetadataModel;
class AttachedFiltersModel;
class QmlProducer;

class KeyframesDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit KeyframesDock(QmlProducer *qmlProducer, QWidget *parent = 0);

    KeyframesModel &model()
    {
        return m_model;
    }
    Q_INVOKABLE int seekPrevious();
    Q_INVOKABLE int seekNext();
    int currentParameter() const;

signals:
    void changed(); /// Notifies when a filter parameter changes.
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void resetZoom();
    void seekPreviousSimple();
    void seekNextSimple();

public slots:
    void setCurrentFilter(QmlFilter *filter, QmlMetadata *meta);
    void load(bool force = false);
    void onProducerModified();

protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private slots:
    void onVisibilityChanged(bool visible);

private:
    QQuickWidget m_qview;
    QmlMetadata m_emptyQmlMetadata;
    QmlFilter m_emptyQmlFilter;
    KeyframesModel m_model;
    QmlProducer *m_qmlProducer;
};

#endif // KEYFRAMESDOCK_H
