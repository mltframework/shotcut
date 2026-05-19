/*
 * Copyright (c) 2014-2026 Meltytech, LLC
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

#include "Logger.h"
#include "addonmetadataparser.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlutilities.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"

#include <MltLink.h>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTimerEvent>

static bool isExperimentalEnabled()
{
    return qApp && qApp->property("experimental").toBool();
}

static QString addOnServiceFromObjectName(const QString &objectName)
{
    static const QString kPrefix = QStringLiteral("addOn.");
    if (!objectName.startsWith(kPrefix))
        return QString();

    return objectName.mid(kPrefix.size());
}

static QString addOnKeywords(const AddOnFilterDescriptor &descriptor)
{
    QStringList keywords;
    keywords << descriptor.service << QStringLiteral("#addon");

    const bool supportsRgba = descriptor.imageFormats.contains(QStringLiteral("rgba"))
                              || descriptor.imageFormats.contains(QStringLiteral("rgba64"));
    const bool supportsYuv = descriptor.imageFormats.contains(QStringLiteral("yuv422"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv420p"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv422p16"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv420p10"))
                             || descriptor.imageFormats.contains(QStringLiteral("yuv444p10"));
    const bool supportsTenBit = descriptor.imageFormats.contains(QStringLiteral("rgba64"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv422p16"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv420p10"))
                                || descriptor.imageFormats.contains(QStringLiteral("yuv444p10"));

    if (supportsRgba)
        keywords << QStringLiteral("#rgba");
    if (supportsYuv)
        keywords << QStringLiteral("#yuv");
    if (supportsTenBit)
        keywords << QStringLiteral("#10bit");

    return keywords.join(QLatin1Char(' '));
}

FilterController::FilterController(QObject *parent)
    : QObject(parent)
    , m_metadataModel(this)
    , m_attachedModel(this)
    , m_currentFilterIndex(QmlFilter::NoCurrentFilter)
{
    startTimer(0);
    QObject::connect(&m_addOnServiceModel,
                     &AddOnServiceModel::enabledServicesChanged,
                     this,
                     &FilterController::handleAddOnServicesChanged);
    connect(&m_attachedModel, SIGNAL(changed()), this, SLOT(handleAttachedModelChange()));
    connect(&m_attachedModel,
            SIGNAL(modelAboutToBeReset()),
            this,
            SLOT(handleAttachedModelAboutToReset()));
    connect(&m_attachedModel,
            SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsRemoved(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this,
            SLOT(handleAttachedRowsInserted(const QModelIndex &, int, int)));
    connect(&m_attachedModel,
            SIGNAL(duplicateAddFailed(int)),
            this,
            SLOT(handleAttachDuplicateFailed(int)));
}

FilterController::~FilterController()
{
    delete m_addOnTempDir;
}

void FilterController::handleAddOnServicesChanged()
{
    auto *source = static_cast<InternalMetadataModel *>(m_metadataModel.sourceModel());
    if (!source)
        return;

    // Remove previously generated add-on metadata entries.
    for (int i = source->list().size() - 1; i >= 0; --i) {
        QmlMetadata *meta = source->get(i);
        if (meta && meta->objectName().startsWith(QStringLiteral("addOn."))) {
            source->remove(i);
        }
    }

    if (!isExperimentalEnabled())
        return;

    // Rebuild from current enabled service list.
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    loadAddOnFilterMetadata(mltFilters.data());
}

void FilterController::loadFilterMetadata()
{
    QScopedPointer<Mlt::Properties> mltFilters(MLT.repository()->filters());
    QScopedPointer<Mlt::Properties> mltLinks(MLT.repository()->links());
    QScopedPointer<Mlt::Properties> mltProducers(MLT.repository()->producers());
    QDir dir = QmlUtilities::qmlDir();
    dir.cd("filters");
    foreach (QString dirName,
             dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Executable)) {
        QDir subdir = dir;
        subdir.cd(dirName);
        subdir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        subdir.setNameFilters(QStringList("meta*.qml"));
        foreach (QString fileName, subdir.entryList()) {
            LOG_DEBUG() << "reading filter metadata" << dirName << fileName;
            QQmlComponent component(QmlUtilities::sharedEngine(), subdir.absoluteFilePath(fileName));
            QmlMetadata *meta = qobject_cast<QmlMetadata *>(component.create());
            if (meta) {
                QScopedPointer<Mlt::Properties> mltMetadata(
                    MLT.repository()->metadata(mlt_service_filter_type,
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
                    ("maskGlaxnimate" != meta->objectName() || mltProducers->get_data("glaxnimate"))
                    && (version.isEmpty() || meta->isMltVersion(version))) {
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

    if (isExperimentalEnabled())
        loadAddOnFilterMetadata(mltFilters.data());
}

void FilterController::loadAddOnFilterMetadata(Mlt::Properties *mltFilters)
{
    if (!mltFilters || !mltFilters->is_valid())
        return;

    m_addOnDescriptors.clear();

    const QStringList services = m_addOnServiceModel.enabledServices();
    for (const auto &service : services) {
        if (!mltFilters->get_data(service.toLatin1().constData())) {
            LOG_WARNING() << "Add-on service unavailable" << service;
            continue;
        }

        QScopedPointer<Mlt::Properties> mltMetadata(
            MLT.repository()->metadata(mlt_service_filter_type, service.toLatin1().constData()));
        if (!mltMetadata || !mltMetadata->is_valid()) {
            LOG_WARNING() << "Failed to query metadata for add-on service" << service;
            continue;
        }

        const AddOnFilterDescriptor descriptor = AddOnMetadataParser::parse(service,
                                                                            mltMetadata.data());
        LOG_DEBUG() << "add-on metadata" << service << "propertyCount"
                    << descriptor.parameters.size();
        m_addOnDescriptors[service] = descriptor;

        auto meta = new QmlMetadata;
        meta->setType(QmlMetadata::Filter);
        meta->setObjectName(QStringLiteral("addOn.%1").arg(service));
        meta->set_mlt_service(service);
        meta->setName(descriptor.title);
        meta->setProperty("keywords", addOnKeywords(descriptor));
        meta->loadSettings();
        meta->setIsAudio(descriptor.isAudio);

        QStringList keyframeableProperties;
        for (const auto &parameter : descriptor.parameters) {
            if (!parameter.supportsKeyframes)
                continue;

            auto *keyParam = new QmlKeyframesParameter(meta->keyframes());
            const QString paramType = parameter.type.trimmed().toLower();
            const QString displayName = parameter.title.isEmpty() ? parameter.name
                                                                  : parameter.title;
            keyParam->setProperty("name", displayName);
            keyParam->setProperty("property", parameter.name);

            const bool isNumericType = paramType == QStringLiteral("integer")
                                       || paramType == QStringLiteral("float");
            const bool isColorType = paramType == QStringLiteral("color");

            keyParam->setProperty("isCurve", isNumericType);
            keyParam->setProperty("isColor", isColorType);
            if (!parameter.unit.isEmpty())
                keyParam->setProperty("units", parameter.unit);

            if (isNumericType) {
                bool okMin = false;
                bool okMax = false;
                const double minValue = parameter.minimum.toDouble(&okMin);
                const double maxValue = parameter.maximum.toDouble(&okMax);
                keyParam->setProperty("minimum", okMin ? minValue : 0.0);
                keyParam->setProperty("maximum", okMax ? maxValue : 100.0);
            }

            auto parameterList = meta->keyframes()->parameters();
            if (parameterList.append)
                parameterList.append(&parameterList, keyParam);

            keyframeableProperties << parameter.name;
        }

        if (!keyframeableProperties.isEmpty()) {
            meta->keyframes()->setProperty("allowAnimateIn", true);
            meta->keyframes()->setProperty("allowAnimateOut", true);
            meta->keyframes()->setProperty("simpleProperties", keyframeableProperties);
        }

        addMetadata(meta);
    }
}

bool FilterController::ensureAddOnTempDir()
{
    if (!m_addOnTempDir) {
        m_addOnTempDir = new QTemporaryDir(QDir::tempPath() + "/shotcut-addon-XXXXXX");
    }
    if (!m_addOnTempDir || !m_addOnTempDir->isValid()) {
        LOG_WARNING() << "Add-on temporary directory is invalid";
        return false;
    }
    return true;
}

bool FilterController::ensureAddOnFilterQml(QmlMetadata *meta)
{
    if (!meta)
        return false;

    const QString service = addOnServiceFromObjectName(meta->objectName());
    if (service.isEmpty())
        return true;

    if (!ensureAddOnTempDir())
        return false;

    const QDir tempDir(m_addOnTempDir->path());
    const QString cachedFileName = service + QStringLiteral("_ui.qml");
    if (QFileInfo::exists(tempDir.filePath(cachedFileName))) {
        meta->setPath(tempDir);
        meta->setQmlFileName(cachedFileName);
        return true;
    }

    AddOnFilterDescriptor descriptor;
    const auto it = m_addOnDescriptors.constFind(service);
    if (it != m_addOnDescriptors.constEnd()) {
        descriptor = it.value();
    } else {
        LOG_WARNING() << "No cached descriptor for add-on service" << service
                      << "- reloading metadata";

        QScopedPointer<Mlt::Properties> mltMetadata(
            MLT.repository()->metadata(mlt_service_filter_type, service.toLatin1().constData()));
        if (!mltMetadata || !mltMetadata->is_valid()) {
            LOG_WARNING() << "Failed to query metadata for add-on service" << service;
            return false;
        }

        descriptor = AddOnMetadataParser::parse(service, mltMetadata.data());
        m_addOnDescriptors.insert(service, descriptor);
    }

    QString generationError;
    if (!m_addOnQmlGenerator.generate(descriptor, tempDir, cachedFileName, &generationError)) {
        LOG_WARNING() << "Failed to generate add-on UI QML for" << service << generationError;
        return false;
    }

    meta->setPath(tempDir);
    meta->setQmlFileName(cachedFileName);
    return true;
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
        QMetaObject::invokeMethod(this,
                                  "setCurrentFilter",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, m_currentFilterIndex));
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
                                         producer->type() == mlt_service_tractor_type,
                                         producer->get("mlt_service") != QString("xml-clip"));
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
    QmlFilter *filter = nullptr;
    if (meta) {
        if (meta->objectName().startsWith(QStringLiteral("addOn.")) && !ensureAddOnFilterQml(meta)) {
            emit statusChanged(tr("Failed to prepare add-on filter user interface."));
            emit currentFilterChanged(nullptr, nullptr, QmlFilter::NoCurrentFilter);
            m_currentFilter.reset();
            return;
        }
        emit currentFilterChanged(nullptr, nullptr, QmlFilter::NoCurrentFilter);
        std::unique_ptr<Mlt::Service> service(m_attachedModel.getService(m_currentFilterIndex));
        if (!service || !service->is_valid())
            return;
        m_mltService = Mlt::Service(service->get_service());
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

void FilterController::onGainChanged()
{
    if (m_currentFilter) {
        QString name = m_currentFilter->objectNameOrService();
        if (name == QStringLiteral("audioGain")) {
            emit m_currentFilter->changed();
        }
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
    if (delta && m_currentFilter
        && (!service || m_currentFilter->service().get_service() == service->get_service())) {
        emit m_currentFilter->inChanged(delta);
    }
}

void FilterController::onServiceOutChanged(int delta, Mlt::Service *service)
{
    if (delta && m_currentFilter
        && (!service || m_currentFilter->service().get_service() == service->get_service())) {
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

void FilterController::pauseUndoTracking()
{
    if (m_currentFilter && !m_attachedModel.isSourceClip()) {
        m_currentFilter->stopUndoTracking();
    }
}

void FilterController::resumeUndoTracking()
{
    if (m_currentFilter && !m_attachedModel.isSourceClip()) {
        m_currentFilter->startUndoTracking();
    }
}

void FilterController::addMetadata(QmlMetadata *meta)
{
    m_metadataModel.add(meta);
}

void FilterController::handleAttachedRowsAboutToBeRemoved(const QModelIndex &parent,
                                                          int first,
                                                          int last)
{
    auto filter = m_attachedModel.getService(first);
    m_motionTrackerModel.remove(m_motionTrackerModel.keyForFilter(filter));
}

void FilterController::addOrEditFilter(Mlt::Filter *filter, const QStringList &key_properties)
{
    int rows = m_attachedModel.rowCount();
    int serviceIndex = -1;
    for (int i = 0; i < rows; i++) {
        QScopedPointer<Mlt::Service> service(m_attachedModel.getService(i));
        bool servicesMatch = true;
        if (metadataForService(service.data())->uniqueId()
            != metadataForService(filter)->uniqueId()) {
            continue;
        }
        for (auto &k : key_properties) {
            const auto keyByteArray = k.toUtf8();
            const char *key = keyByteArray.constData();
            if (!service->property_exists(key) || !service->property_exists(key)) {
                servicesMatch = false;
                break;
            } else if (QString(service->get(key)) != QString(filter->get(key))) {
                servicesMatch = false;
                break;
            }
        }
        if (servicesMatch) {
            serviceIndex = i;
            break;
        }
    }
    if (serviceIndex < 0) {
        serviceIndex = m_attachedModel.addService(filter);
    }
    setCurrentFilter(serviceIndex);
}

void FilterController::setTrackTransitionService(const QString &service)
{
    if (service == QStringLiteral("qtblend")) {
        m_metadataModel.setHidden("qtBlendMode", false);
        m_metadataModel.setHidden("blendMode", true);
        m_metadataModel.setHidden("movitBlendMode", true);
    } else if (service == QStringLiteral("frei0r.cairoblend")) {
        m_metadataModel.setHidden("qtBlendMode", true);
        m_metadataModel.setHidden("blendMode", false);
        m_metadataModel.setHidden("movitBlendMode", true);
    } else if (service == QStringLiteral("movit.overlay")) {
        m_metadataModel.setHidden("qtBlendMode", true);
        m_metadataModel.setHidden("blendMode", true);
        m_metadataModel.setHidden("movitBlendMode", false);
    } else {
        m_metadataModel.setHidden("qtBlendMode", true);
        m_metadataModel.setHidden("blendMode", true);
        m_metadataModel.setHidden("movitBlendMode", true);
    }
}
