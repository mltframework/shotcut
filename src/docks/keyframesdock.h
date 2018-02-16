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

#ifndef KEYFRAMESDOCK_H
#define KEYFRAMESDOCK_H

#include "qmltypes/qmlproducer.h"
#include "qmltypes/qmlfilter.h"
#include "models/metadatamodel.h"

#include <QDockWidget>
#include <QQuickWidget>
#include <QScopedPointer>

class QmlFilter;
class QmlMetadata;
class MetadataModel;
class AttachedFiltersModel;

class KeyframesDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit KeyframesDock(MetadataModel* metadataModel, AttachedFiltersModel* attachedModel, QWidget *parent = 0);

signals:
    void changed(); /// Notifies when a filter parameter changes.
    void seeked(int);
    void producerInChanged();
    void producerOutChanged();

public slots:
    void setCurrentFilter(QmlFilter* filter, QmlMetadata* meta);
    void setFadeInDuration(int duration);
    void setFadeOutDuration(int duration);
    void onFilterInChanged(Mlt::Filter* filter);
    void onFilterOutChanged(Mlt::Filter* filter);

protected:
    bool event(QEvent *event);

private slots:
    void onSeeked(int position);
    void resetQview();

private:
    QQuickWidget m_qview;
    QmlProducer m_producer;
    QmlMetadata m_emptyQmlMetadata;
    QmlFilter m_emptyQmlFilter;
    QmlFilter* m_currentFilter;
};

#endif // KEYFRAMESDOCK_H
