/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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

#include "shotcut_mlt_properties.h"
#include "filtercontroller.h"
#include <QQmlEngine>
#include <QDir>
#include <Logger.h>
#include <QQmlComponent>
#include <QTimerEvent>
#include "mltcontroller.h"
#include "settings.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlfilter.h"

FilterController::FilterController(QObject* parent) : QObject(parent),
 m_mltFilter(0),
 m_metadataModel(this),
 m_attachedModel(this),
 m_currentFilterIndex(-1)
{
    startTimer(0);
    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel, SIGNAL(modelAboutToBeReset()), this, SLOT(handleAttachedModelAboutToReset()));
    connect(&m_attachedModel, SIGNAL(rowsRemoved(const QModelIndex&,int,int)), this, SLOT(handleAttachedRowsRemoved(const QModelIndex&,int,int)));
    connect(&m_attachedModel, SIGNAL(rowsInserted(const QModelIndex&,int,int)), this, SLOT(handleAttachedRowsInserted(const QModelIndex&,int,int)));
    connect(&m_attachedModel, SIGNAL(duplicateAddFailed(int)), this, SLOT(handleAttachDuplicateFailed(int)));
}

void FilterController::loadFilterMetadata() {
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    foreach (QString dirName, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml"));
        foreach (QString fileName, subdir.entryList()) {
            LOG_DEBUG() << "reading filter metadata" << dirName << fileName;
            QQmlComponent component(QmlUtilities::sharedEngine(), subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata*>(component.create());
            if (meta) {
                // Check if mlt_service is available.
                if (mltFilters->get_data(meta->mlt_service().toLatin1().constData())) {
                    LOG_DEBUG() << "added filter" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta);

                    // Check if a keyframes minimum version is required.
                    QScopedPointer<Mlt::Properties> mltMetadata(MLT.repository()->metadata(filter_type, meta->mlt_service().toLatin1().constData()));
                    if (mltMetadata && mltMetadata->is_valid() && mltMetadata->get("version") && meta->keyframes()) {
                        QString version = QString::fromLatin1(mltMetadata->get("version"));
                        if (version.startsWith("lavfi"))
                            version.remove(0, 5);
                        meta->keyframes()->checkVersion(version);
                        // MLT frei0r module did get mlt_animation support until v6.10 (6.9 while in development).
                        if (meta->mlt_service().startsWith("frei0r.")) {
                            if (mlt_version_get_major() < 6 || mlt_version_get_minor() < 9)
                                meta->keyframes()->setDisabled();
                        }
                    }
                }
            } else if (!meta) {
                LOG_WARNING() << component.errorString();
            }
        }
    };
}

QmlMetadata *FilterController::metadataForService(Mlt::Service *service)
{
    QmlMetadata* meta = 0;
    int rowCount = m_metadataModel.rowCount();
    QString uniqueId = service->get(kShotcutFilterProperty);

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

void FilterController::timerEvent(QTimerEvent* event)
{
    loadFilterMetadata();
    killTimer(event->timerId());
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
    m_attachedModel.setProducer(producer);
    if (producer && producer->is_valid()) {
        mlt_service_type service_type = producer->type();
        m_metadataModel.setIsClipProducer(service_type != tractor_type
            && service_type != playlist_type);
    }
}

void FilterController::setCurrentFilter(int attachedIndex, bool isNew)
{
    if (attachedIndex == m_currentFilterIndex) {
        return;
    }
    m_currentFilterIndex = attachedIndex;

    QmlMetadata* meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter* filter = 0;
    if (meta) {
        m_mltFilter = m_attachedModel.getFilter(m_currentFilterIndex);
        filter = new QmlFilter(*m_mltFilter, meta);
        filter->setIsNew(isNew);
        connect(filter, SIGNAL(changed()), SLOT(onQmlFilterChanged()));
    }

    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
}

void FilterController::onFadeInChanged()
{
    if (m_currentFilter) {
        emit m_currentFilter->changed();
        emit m_currentFilter->animateInChanged();
    }
}

void FilterController::onFadeOutChanged()
{
    if (m_currentFilter) {
        emit m_currentFilter->changed();
        emit m_currentFilter->animateOutChanged();
    }
}

void FilterController::onFilterInChanged(int delta, Mlt::Filter* filter)
{
    if (delta && m_currentFilter && (!filter || m_currentFilter->filter().get_filter() == filter->get_filter())) {
        emit m_currentFilter->inChanged(delta);
    }
}

void FilterController::onFilterOutChanged(int delta, Mlt::Filter* filter)
{
    if (delta && m_currentFilter && (!filter || m_currentFilter->filter().get_filter() == filter->get_filter())) {
        emit m_currentFilter->outChanged(delta);
    }
}

void FilterController::handleAttachedModelChange()
{
    MLT.refreshConsumer();
}

void FilterController::handleAttachedModelAboutToReset()
{
    setCurrentFilter(-1);
}

void FilterController::handleAttachedRowsRemoved(const QModelIndex&, int first, int)
{
    m_currentFilterIndex = -2; // Force update
    setCurrentFilter(qBound(0, first, m_attachedModel.rowCount() - 1));
}

void FilterController::handleAttachedRowsInserted(const QModelIndex&, int first, int)
{
    m_currentFilterIndex = -2; // Force update
    setCurrentFilter(qBound(0, first, m_attachedModel.rowCount() - 1), true);
}

void FilterController::handleAttachDuplicateFailed(int index)
{
    const QmlMetadata* meta = m_attachedModel.getMetadata(index);
    emit statusChanged(tr("Only one %1 filter is allowed.").arg(meta->name()));
    setCurrentFilter(index);
}

void FilterController::onQmlFilterChanged()
{
    emit filterChanged(m_mltFilter);
}

void FilterController::addMetadata(QmlMetadata* meta)
{
    m_metadataModel.add(meta);
}
