/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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

#include <MltLink.h>

FilterController::FilterController(QObject* parent) : QObject(parent),
 m_mltService(0),
 m_metadataModel(this),
 m_attachedModel(this),
 m_currentFilterIndex(QmlFilter::NoCurrentFilter)
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
    QScopedPointer<Mlt::Properties> mltLinks(MLT.repository()->links());
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
                QScopedPointer<Mlt::Properties> mltMetadata(MLT.repository()->metadata(mlt_service_filter_type, meta->mlt_service().toLatin1().constData()));
                QString version;
                if (mltMetadata && mltMetadata->is_valid() && mltMetadata->get("version")) {
                    version = QString::fromLatin1(mltMetadata->get("version"));
                    if (version.startsWith("lavfi"))
                        version.remove(0, 5);
                }

                    // Check if mlt_service is available.
                if (mltFilters->get_data(meta->mlt_service().toLatin1().constData()) &&
                        (version.isEmpty() || meta->isMltVersion(version))) {
                    LOG_DEBUG() << "added filter" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta);

                    // Check if a keyframes minimum version is required.
                    if (!version.isEmpty() && meta->keyframes()) {
                        meta->setProperty("version", version);
                        meta->keyframes()->checkVersion(version);
                    }
                }
                else if (meta->type() == QmlMetadata::Link && mltLinks->get_data(meta->mlt_service().toLatin1().constData())) {
                    LOG_DEBUG() << "added link" << meta->name();
                    meta->loadSettings();
                    meta->setPath(subdir);
                    meta->setParent(0);
                    addMetadata(meta);
                }

                if (meta->isDeprecated())
                    meta->setName(meta->name() + " " + tr("(DEPRECATED)"));
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
        m_metadataModel.setIsClipProducer(service_type != mlt_service_playlist_type &&
            (service_type != mlt_service_tractor_type || !producer->get_int(kShotcutXmlProperty)));
        m_metadataModel.setIsChainProducer(service_type == mlt_service_chain_type);
    }
}

void FilterController::setCurrentFilter(int attachedIndex, bool isNew)
{
    if (attachedIndex == m_currentFilterIndex) {
        return;
    }
    m_currentFilterIndex = attachedIndex;

    // VUIs may instruct MLT filters to not render if they are doing the rendering
    // theirself, for example, Text: Rich. Component.onDestruction is not firing.
    if (m_mltService) {
        if (m_mltService->get_int("_hide")) {
            m_mltService->clear("_hide");
            MLT.refreshConsumer();
        }
    }

    QmlMetadata* meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter* filter = 0;
    if (meta) {
        emit currentFilterChanged(nullptr, nullptr, QmlFilter::NoCurrentFilter);
        m_mltService = m_attachedModel.getService(m_currentFilterIndex);
        if (!m_mltService) return;
        filter = new QmlFilter(*m_mltService, meta);
        filter->setIsNew(isNew);
        connect(filter, SIGNAL(changed()), SLOT(onQmlFilterChanged()));
        connect(filter, SIGNAL(changed(QString)), SLOT(onQmlFilterChanged(const QString&)));
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

void FilterController::onServiceInChanged(int delta, Mlt::Service* service)
{
    if (delta && m_currentFilter && (!service || m_currentFilter->service().get_service() == service->get_service())) {
        emit m_currentFilter->inChanged(delta);
    }
}

void FilterController::onServiceOutChanged(int delta, Mlt::Service* service)
{
    if (delta && m_currentFilter && (!service || m_currentFilter->service().get_service() == service->get_service())) {
        emit m_currentFilter->outChanged(delta);
    }
}

void FilterController::handleAttachedModelChange()
{
    if (m_currentFilter) {
        emit m_currentFilter->changed("disable");
    }
}

void FilterController::handleAttachedModelAboutToReset()
{
    setCurrentFilter(QmlFilter::NoCurrentFilter);
}

void FilterController::handleAttachedRowsRemoved(const QModelIndex&, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // Force update
    setCurrentFilter(qBound(0, first, m_attachedModel.rowCount() - 1));
}

void FilterController::handleAttachedRowsInserted(const QModelIndex&, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // Force update
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
    emit filterChanged(m_mltService);
}

void FilterController::onQmlFilterChanged(const QString &name)
{
    if (name == "disable") {
        QModelIndex index = m_attachedModel.index(m_currentFilterIndex);
        emit m_attachedModel.dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
    }
}

void FilterController::removeCurrent()
{
    if (m_currentFilterIndex > QmlFilter::NoCurrentFilter)
        m_attachedModel.remove(m_currentFilterIndex);
}

void FilterController::onProducerChanged()
{
    emit m_attachedModel.trackTitleChanged();
}

void FilterController::addMetadata(QmlMetadata* meta)
{
    m_metadataModel.add(meta);
}
