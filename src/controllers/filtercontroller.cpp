/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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
#include "qmltypes/qmlapplication.h"

#include <MltLink.h>

FilterController::FilterController(QObject *parent) : QObject(parent),
    m_metadataModel(this),
    m_attachedModel(this),
    m_currentFilterIndex(QmlFilter::NoCurrentFilter)
{
    startTimer(0);
    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel, SIGNAL(modelAboutToBeReset()), this,
            SLOT(handleAttachedModelAboutToReset()));
    connect(&m_attachedModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)), this,
            SLOT(handleAttachedRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this,
            SLOT(handleAttachedRowsRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this,
            SLOT(handleAttachedRowsInserted(const QModelIndex &, int, int)));
    connect(&m_attachedModel, SIGNAL(duplicateAddFailed(int)), this,
            SLOT(handleAttachDuplicateFailed(int)));
}

void FilterController::loadFilterMetadata()
{
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QScopedPointer<Mlt::Properties> mltLinks(MLT.repository()->links());
    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
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
            QmlMetadata *meta = qobject_cast<QmlMetadata *>(component.create());
            if (meta) {
                QScopedPointer<Mlt::Properties> mltMetadata(MLT.repository()->metadata(mlt_service_filter_type,
                                                                                       meta->mlt_service().toLatin1().constData()));
                QString version;
                if (mltMetadata && mltMetadata->is_valid() && mltMetadata->get("version")) {
                    version = QString::fromLatin1(mltMetadata->get("version"));
                    if (version.startsWith("lavfi"))
                        version.remove(0, 5);
                }

                // Check if mlt_service is available.
                if (mltFilters->get_data(meta->mlt_service().toLatin1().constData()) &&
                        // Check if MLT glaxnimate producer is available if needed
                        ("maskGlaxnimate" != meta->objectName() || mltProducers->get_data("glaxnimate")) &&
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
                } else if (meta->type() == QmlMetadata::Link
                           && mltLinks->get_data(meta->mlt_service().toLatin1().constData())) {
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

QmlMetadata *FilterController::metadata(const QString &id)
{
    QmlMetadata *meta = 0;
    int rowCount = m_metadataModel.sourceRowCount();

    for (int i = 0; i < rowCount; i++) {
        QmlMetadata *tmpMeta = m_metadataModel.getFromSource(i);
        if (tmpMeta->uniqueId() == id) {
            meta = tmpMeta;
            break;
        }
    }

    return meta;
}

QmlMetadata *FilterController::metadataForService(Mlt::Service *service)
{
    QmlMetadata *meta = nullptr;
    QString uniqueId = service->get(kShotcutFilterProperty);

    // Fallback to mlt_service for legacy filters
    if (uniqueId.isEmpty()) {
        uniqueId = service->get("mlt_service");
    }

    return metadata(uniqueId);
}

bool FilterController::isOutputTrackSelected() const
{
    return m_attachedModel.producer() && m_attachedModel.producer()->is_valid()
           && mlt_service_tractor_type == m_attachedModel.producer()->type()
           && !m_attachedModel.producer()->get(kShotcutTransitionProperty)
           && m_attachedModel.rowCount() == 0;
}

void FilterController::loadFilterSets()
{
    auto dir = QmlApplication::dataDir();
    if (dir.cd("shotcut") && dir.cd("filter-sets")) {
        QStringList entries = dir.entryList(QDir::Files | QDir::Readable);
        for (const auto &s : entries) {
            auto meta = new QmlMetadata;
            meta->setType(QmlMetadata::FilterSet);
            if (s == QUrl::toPercentEncoding(QUrl::fromPercentEncoding(s.toUtf8())))
                meta->setName(QUrl::fromPercentEncoding(s.toUtf8()));
            else
                meta->setName(s);
            meta->set_mlt_service("stock");
            meta->loadSettings();
            addMetadata(meta);
        }
    }
    dir = Settings.appDataLocation();
    if (dir.cd("filter-sets")) {
        QStringList entries = dir.entryList(QDir::Files | QDir::Readable);
        for (const auto &s : entries) {
            auto meta = new QmlMetadata;
            meta->setType(QmlMetadata::FilterSet);
            if (s == QUrl::toPercentEncoding(QUrl::fromPercentEncoding(s.toUtf8())))
                meta->setName(QUrl::fromPercentEncoding(s.toUtf8()));
            else
                meta->setName(s);
            meta->loadSettings();
            addMetadata(meta);
        }
    }
}

void FilterController::onUndoOrRedo(Mlt::Service &service)
{
    MLT.refreshConsumer();
    if (m_currentFilter && m_mltService.is_valid()
            && service.get_service() == m_mltService.get_service()) {
        emit undoOrRedo();
        QMetaObject::invokeMethod(this, "setCurrentFilter", Qt::QueuedConnection, Q_ARG(int,
                                                                                        m_currentFilterIndex));
    }
}

void FilterController::timerEvent(QTimerEvent *event)
{
    loadFilterMetadata();
    loadFilterSets();
    killTimer(event->timerId());
}

MetadataModel *FilterController::metadataModel()
{
    return &m_metadataModel;
}

AttachedFiltersModel *FilterController::attachedModel()
{
    return &m_attachedModel;
}

void FilterController::setProducer(Mlt::Producer *producer)
{
    m_attachedModel.setProducer(producer);
    if (producer && producer->is_valid()) {
        m_metadataModel.updateFilterMask(!MLT.isTrackProducer(*producer),
                                         producer->type() == mlt_service_chain_type,
                                         producer->type() == mlt_service_playlist_type,
                                         producer->type() == mlt_service_tractor_type);
    } else {
        setCurrentFilter(QmlFilter::DeselectCurrentFilter);
    }
}

void FilterController::setCurrentFilter(int attachedIndex)
{
    if (attachedIndex == m_currentFilterIndex) {
        return;
    }
    m_currentFilterIndex = attachedIndex;

    // VUIs may instruct MLT filters to not render if they are doing the rendering
    // theirself, for example, Text: Rich. Component.onDestruction is not firing.
    if (m_mltService.is_valid()) {
        if (m_mltService.get_int("_hide")) {
            m_mltService.clear("_hide");
            MLT.refreshConsumer();
        }
    }

    QmlMetadata *meta = m_attachedModel.getMetadata(m_currentFilterIndex);
    QmlFilter *filter = 0;
    if (meta) {
        emit currentFilterChanged(nullptr, nullptr, QmlFilter::NoCurrentFilter);
        m_mltService = m_attachedModel.getService(m_currentFilterIndex);
        if (!m_mltService.is_valid()) return;
        filter = new QmlFilter(m_mltService, meta);
        filter->setIsNew(m_mltService.get_int(kNewFilterProperty));
        m_mltService.clear(kNewFilterProperty);
        connect(filter, SIGNAL(changed(QString)), SLOT(onQmlFilterChanged(const QString &)));
    }

    emit currentFilterChanged(filter, meta, m_currentFilterIndex);
    m_currentFilter.reset(filter);
    if (filter && !m_attachedModel.isSourceClip()) {
        filter->startUndoTracking();
    }
}

void FilterController::onFadeInChanged()
{
    if (m_currentFilter) {
        QString name = m_currentFilter->objectNameOrService();
        if (name.startsWith("fadeIn")) {
            emit m_currentFilter->changed();
            emit m_currentFilter->animateInChanged();
        }
    }
}

void FilterController::onFadeOutChanged()
{
    if (m_currentFilter) {
        QString name = m_currentFilter->objectNameOrService();
        if (name.startsWith("fadeOut")) {
            emit m_currentFilter->changed();
            emit m_currentFilter->animateOutChanged();
        }
    }
}

void FilterController::onServiceInChanged(int delta, Mlt::Service *service)
{
    if (delta && m_currentFilter && (!service
                                     || m_currentFilter->service().get_service() == service->get_service())) {
        emit m_currentFilter->inChanged(delta);
    }
}

void FilterController::onServiceOutChanged(int delta, Mlt::Service *service)
{
    if (delta && m_currentFilter && (!service
                                     || m_currentFilter->service().get_service() == service->get_service())) {
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

void FilterController::handleAttachedRowsRemoved(const QModelIndex &, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // Force update
    setCurrentFilter(qBound(0, first, qMax(m_attachedModel.rowCount() - 1, 0)));
}

void FilterController::handleAttachedRowsInserted(const QModelIndex &, int first, int)
{
    m_currentFilterIndex = QmlFilter::DeselectCurrentFilter; // Force update
    setCurrentFilter(qBound(0, first, qMax(m_attachedModel.rowCount() - 1, 0)));
}

void FilterController::handleAttachDuplicateFailed(int index)
{
    const QmlMetadata *meta = m_attachedModel.getMetadata(index);
    emit statusChanged(tr("Only one %1 filter is allowed.").arg(meta->name()));
    setCurrentFilter(index);
}

void FilterController::onQmlFilterChanged(const QString &name)
{
    if (name == "disable") {
        QModelIndex index = m_attachedModel.index(m_currentFilterIndex);
        emit m_attachedModel.dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
    }
    emit filterChanged(&m_mltService);
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

void FilterController::addMetadata(QmlMetadata *meta)
{
    m_metadataModel.add(meta);
}

void FilterController::handleAttachedRowsAboutToBeRemoved(const QModelIndex &parent, int first,
                                                          int last)
{
    auto filter = m_attachedModel.getService(first);
    m_motionTrackerModel.remove(m_motionTrackerModel.keyForFilter(filter));
}
