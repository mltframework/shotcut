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
#include "filtercontroller.h"
#include <QQmlEngine>
#include <QDir>
#include <QDebug>
#include <QQmlComponent>
#include <QtConcurrent/QtConcurrentRun>
#include "mltcontroller.h"
#include "settings.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlfilter.h"

FilterController::FilterController(QObject* parent) : QObject(parent),
 m_metadataModel(this),
 m_attachedModel(this),
 m_currentFilterIndex(-1)
{
    // Process filters in a separate thread and add them to the model asynchronously.
    connect(this, SIGNAL(newMetadataFound(QmlMetadata*)), this, SLOT(addMetadata(QmlMetadata*)), Qt::QueuedConnection);
    m_future = QtConcurrent::run(this, &FilterController::loadFilterMetadata);

    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)), this, SLOT(handleAttachedRowsRemoved(const QModelIndex&,int,int)));
}

void FilterController::loadFilterMetadata() {
    QQmlEngine engine;
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    foreach (QString dirName, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml"));
        foreach (QString fileName, subdir.entryList()) {
            qDebug() << "reading filter metadata" << dirName << fileName;
            QQmlComponent component(&engine, subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata*>(component.create());
            if (meta) {
                // Check if mlt_service is available.
                if (MLT.repository()->filters()->get_data(meta->mlt_service().toLatin1().constData())) {
                    qDebug() << "added filter" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    meta->moveToThread(this->thread());
                    emit newMetadataFound(meta);
                }
            } else if (!meta) {
                qWarning() << component.errorString();
            }
        }
    };
}

QmlMetadata *FilterController::metadataForService(Mlt::Service *service)
{
    m_future.waitForFinished();
    QmlMetadata* meta = NULL;
    int rowCount = m_metadataModel.rowCount();
    QString uniqueId = service->get("shotcut:filter");

    // Fallback to mlt_service for legacy filters
    if (uniqueId.isEmpty()) {
        uniqueId = service->get("mlt_service");
    }

    for (int i = 0; i < rowCount; i++) {
        QmlMetadata* tmpMeta = m_metadataModel.get(i);
        if (tmpMeta->uniqueId() == uniqueId) {
            meta = tmpMeta;
            break;
        }
    }

    return meta;
}

MetadataModel* FilterController::metadataModel()
{
    return &m_metadataModel;
}

AttachedFiltersModel* FilterController::attachedModel()
{
    return &m_attachedModel;
}

void FilterController::setProducer(Mlt::Producer *producer)
{
    m_currentFilter.reset(NULL);
    m_attachedModel.reset(producer);
}

void FilterController::attachFilter(int metadataIndex)
{
    QmlMetadata* meta = m_metadataModel.get(metadataIndex);
    QmlFilter* filter = NULL;
    if (meta) {
        m_currentFilterIndex = m_attachedModel.add(meta);
        Mlt::Filter* mltFilter = m_attachedModel.getFilter(m_currentFilterIndex);
        filter = new QmlFilter(mltFilter, meta);
        filter->setIsNew(true);
    } else {
        m_currentFilterIndex = -1;
    }
    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
}

void FilterController::setCurrentFilter(int attachedIndex)
{
    if (attachedIndex == m_currentFilterIndex) {
        return;
    }

    Mlt::Filter* mltFilter = m_attachedModel.getFilter(attachedIndex);
    QmlMetadata* meta = NULL;
    QmlFilter* filter = NULL;

    if (mltFilter && mltFilter->is_valid()) {
        m_currentFilterIndex = attachedIndex;
        meta = metadataForService(mltFilter);
        if (meta) {
            filter = new QmlFilter(mltFilter, meta);
        } else {
            delete mltFilter;
        }
    } else {
        m_currentFilterIndex = -1;
    }

    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
}

void FilterController::handleAttachedModelChange()
{
    MLT.refreshConsumer();
}

void FilterController::handleAttachedRowsRemoved(const QModelIndex&, int first, int)
{
    int newFilterIndex = first;
    if (newFilterIndex >= m_attachedModel.rowCount()) {
        newFilterIndex = m_attachedModel.rowCount() - 1;
    }
    m_currentFilterIndex = -1; // Force update
    setCurrentFilter(newFilterIndex);
}

void FilterController::addMetadata(QmlMetadata* meta)
{
    m_metadataModel.add(meta);
}

