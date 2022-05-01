/*
 * Copyright (c) 2014-2020 Meltytech, LLC
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

#ifndef FILTERCONTROLLER_H
#define FILTERCONTROLLER_H

#include <QObject>
#include <QScopedPointer>
#include <QFuture>
#include "models/metadatamodel.h"
#include "models/attachedfiltersmodel.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlfilter.h"

class QTimerEvent;


class FilterController : public QObject
{
    Q_OBJECT

public:
    explicit FilterController(QObject *parent = 0);
    MetadataModel *metadataModel();
    AttachedFiltersModel *attachedModel();

    QmlMetadata *metadataForService(Mlt::Service *service);
    QmlFilter *currentFilter() const
    {
        return m_currentFilter.data();
    }

protected:
    void timerEvent(QTimerEvent *);

signals:
    void currentFilterChanged(QmlFilter *filter, QmlMetadata *meta, int index);
    void statusChanged(QString);
    void filterChanged(Mlt::Service *);

public slots:
    void setProducer(Mlt::Producer *producer = 0);
    void setCurrentFilter(int attachedIndex, bool isNew = false);
    void onFadeInChanged();
    void onFadeOutChanged();
    void onServiceInChanged(int delta, Mlt::Service *service = 0);
    void onServiceOutChanged(int delta, Mlt::Service *service = 0);
    void removeCurrent();
    void onProducerChanged();

private slots:
    void handleAttachedModelChange();
    void handleAttachedModelAboutToReset();
    void addMetadata(QmlMetadata *);
    void handleAttachedRowsRemoved(const QModelIndex &parent, int first, int last);
    void handleAttachedRowsInserted(const QModelIndex &parent, int first, int last);
    void handleAttachDuplicateFailed(int index);
    void onQmlFilterChanged();
    void onQmlFilterChanged(const QString &name);

private:
    void loadFilterMetadata();

    QFuture<void> m_future;
    QScopedPointer<QmlFilter> m_currentFilter;
    Mlt::Service *m_mltService;
    MetadataModel m_metadataModel;
    AttachedFiltersModel m_attachedModel;
    int m_currentFilterIndex;
};

#endif // FILTERCONTROLLER_H
