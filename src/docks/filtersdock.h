/*
 * Copyright (c) 2013-2019 Meltytech, LLC
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

#ifndef FILTERSDOCK_H
#define FILTERSDOCK_H

#include <QDockWidget>
#include <QObject>
#include <QQuickView>
#include <QQuickWidget>

#include "sharedframe.h"
#include "qmltypes/qmlproducer.h"

class QmlFilter;
class QmlMetadata;
class MetadataModel;
class AttachedFiltersModel;

class FiltersDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit FiltersDock(MetadataModel* metadataModel, AttachedFiltersModel* attachedModel, QWidget *parent = 0);

    QmlProducer* qmlProducer() { return &m_producer; }

signals:
    void currentFilterRequested(int attachedIndex);
    void changed(); /// Notifies when a filter parameter changes.
    void seeked(int);
    void producerInChanged(int delta);
    void producerOutChanged(int delta);

public slots:
    void setCurrentFilter(QmlFilter* filter, QmlMetadata* meta, int index);
    void onSeeked(int position);
    void onShowFrame(const SharedFrame& frame);
    void openFilterMenu() const;
    void resetQview();

protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent* event);

private:
    QQuickWidget m_qview;
    QmlProducer m_producer;
};

#endif // FILTERSDOCK_H
