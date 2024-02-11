/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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

#include "attachedfiltersmodel.h"
#include "mltcontroller.h"
#include "mainwindow.h"
#include "commands/filtercommands.h"
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlmetadata.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QGuiApplication>
#include <QClipboard>

#include <Logger.h>
#include <MltChain.h>
#include <MltLink.h>

static int sortOrder(const QmlMetadata *meta)
{
    // Sort order is: Link, GPU, Video, Audio
    if (meta) {
        if (meta->type() == QmlMetadata::Link) {
            return 0;
        } else if (meta->needsGPU()) {
            return 1;
        } else if (!meta->isAudio()) {
            return 2;
        } else if (meta->isAudio()) {
            return 3;
        }
    }
    // If there is no metadata, assume the filter is video.
    return 2;
}

static int normalFilterCount(Mlt::Producer *producer)
{
    int count = 0;
    if (producer && producer->is_valid()) {
        for (int i = 0; i < producer->filter_count(); i++) {
            Mlt::Filter *filter = producer->filter(i);
            if (filter->is_valid() && filter->get_int("_loader")) {
                count++;
            } else {
                i = producer->filter_count();
            }
            delete filter;
        }
    }
    return count;
}

static int normalLinkCount(Mlt::Producer *producer)
{
    int count = 0;
    if (producer && producer->is_valid() && producer->type() == mlt_service_chain_type) {
        Mlt::Chain chain(*producer);
        for (int i = 0; i < chain.link_count(); i++) {
            Mlt::Link *link = chain.link(i);
            if (link->is_valid() && link->get_int("_loader")) {
                count++;
            } else {
                i = chain.link_count();
            }
            delete link;
        }
    }
    return count;
}

static int mltFilterIndex(Mlt::Producer *producer, int row)
{
    if (row >= 0 && producer && producer->is_valid()) {
        int linkCount = 0;
        if (producer->type() == mlt_service_chain_type) {
            Mlt::Chain chain(*producer);
            linkCount = chain.link_count() - normalLinkCount(producer);
            if (row < linkCount) {
                // This row refers to an MLT link, not a filter
                return -1;
            }
        }
        int mltIndex = normalFilterCount(producer) + row - linkCount;
        if (mltIndex >= 0 && mltIndex < producer->filter_count()) {
            return mltIndex;
        }
    }
    return -1;
}

static int mltLinkIndex(Mlt::Producer *producer, int row)
{
    if (row >= 0 && producer && producer->is_valid() && producer->type() == mlt_service_chain_type) {
        Mlt::Chain chain(*producer);
        int mltIndex = normalLinkCount(producer) + row;
        if (mltIndex >= 0 && mltIndex < chain.link_count()) {
            return mltIndex;
        }
    }
    return -1;
}

AttachedFiltersModel::AttachedFiltersModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dropRow(-1)
{
}

Mlt::Service *AttachedFiltersModel::getService(int row) const
{
    Mlt::Service *result = nullptr;
    int mltIndex = mltFilterIndex(m_producer.get(), row);
    if (mltIndex >= 0) {
        // Service is a filter
        result = m_producer->filter(mltIndex);
    } else {
        mltIndex = mltLinkIndex(m_producer.get(), row);
        if (mltIndex >= 0) {
            // Service is a link
            Mlt::Chain chain(*m_producer);
            result = chain.link(mltIndex);
        }
    }
    return result;
}

QmlMetadata *AttachedFiltersModel::getMetadata(int row) const
{
    if (row < m_metaList.count() && row >= 0) {
        return m_metaList[row];
    }
    return 0;
}

void AttachedFiltersModel::setProducer(Mlt::Producer *producer)
{
    if (!producer || !m_producer || (producer->get_parent() != m_producer->get_parent())) {
        reset(producer);
    }
}

QString AttachedFiltersModel::producerTitle() const
{
    if (m_producer)
        return Util::producerTitle(*m_producer);
    else
        return QString();
}

bool AttachedFiltersModel::isProducerSelected() const
{
    return !m_producer.isNull() && m_producer->is_valid() && !m_producer->is_blank()
           && MLT.isSeekable(m_producer.get());
}

bool AttachedFiltersModel::supportsLinks() const
{
    if ( !m_producer.isNull() && m_producer->is_valid()
            && m_producer->type() == mlt_service_chain_type ) {
        return true;
    }
    return false;
}

QString AttachedFiltersModel::name(int row) const
{
    QString name;
    auto meta = getMetadata(row);
    if (meta) {
        name = meta->name();
    } else {
        QScopedPointer<Mlt::Service> service(getService(row));
        if (service && service->is_valid() && service->get("mlt_service")) {
            name = QString::fromUtf8(service->get("mlt_service"));
        }
    }
    return name;
}

int AttachedFiltersModel::rowCount(const QModelIndex &) const
{
    if (m_producer && m_producer->is_valid())
        return m_metaList.count();
    else
        return 0;
}

Qt::ItemFlags AttachedFiltersModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled;
    else
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;
}

QVariant AttachedFiltersModel::data(const QModelIndex &index, int role) const
{
    if ( !m_producer || !m_producer->is_valid()
            || index.row() >= m_metaList.size())
        return QVariant();
    switch (role ) {
    case Qt::DisplayRole:
        return name(index.row());
    case Qt::CheckStateRole: {
        Mlt::Service *service = getService(index.row());
        QVariant result = Qt::Unchecked;
        if (service && service->is_valid() && !service->get_int("disable"))
            result = Qt::Checked;
        delete service;
        return result;
    }
    break;
    case TypeDisplayRole: {
        QVariant result;
        const QmlMetadata *meta = m_metaList[index.row()];
        if (meta && meta->isAudio()) {
            result = tr("Audio");
        } else if (meta && meta->type() == QmlMetadata::Link) {
            result = tr("Time");
        } else if (meta && meta->needsGPU()) {
            result = tr("GPU");
        } else {
            result = tr("Video");
        }
        return result;
    }
    case PluginTypeRole: {
        const QmlMetadata *meta = m_metaList[index.row()];
        QVariant result = meta ? meta->type() : QmlMetadata::Filter;
        return result;
    }
    break;
    default:
        break;
    }
    return QVariant();
}

bool AttachedFiltersModel::setData(const QModelIndex &index, const QVariant &, int role)
{
    if (role != Qt::CheckStateRole || !m_producer || !m_producer->is_valid()) {
        return false;
    }
    int mltIndex = mltFilterIndex(m_producer.data(), index.row());
    Mlt::Filter *filter = m_producer->filter(mltIndex);
    if (filter && filter->is_valid()) {
        bool disabled = filter->get_int("disable");
        if (isSourceClip()) {
            doSetDisabled(*m_producer.data(), index.row(), !disabled);
        } else {
            MAIN.undoStack()->push(new Filter::DisableCommand(*this, name(index.row()), index.row(),
                                                              !disabled));
        }
    } else {
        LOG_ERROR() << "Invalid filter index" << index.row();
    }
    delete filter;
    return true;
}

void AttachedFiltersModel::doSetDisabled(Mlt::Producer &producer, int row, bool disabled)
{
    int mltIndex = mltFilterIndex(&producer, row);
    Mlt::Filter *filter = producer.filter(mltIndex);
    if (filter->is_valid()) {
        filter->set("disable", disabled);
        emit changed();
        if (isProducerLoaded(producer)) {
            Q_ASSERT(row >= 0);
            QModelIndex modelIndex = createIndex(row, 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>() << Qt::CheckStateRole);
        }
    } else {
        LOG_ERROR() << "Invalid filter index" << row;
    }
    delete filter;
}

Mlt::Service AttachedFiltersModel::doGetService(Mlt::Producer &producer, int row)
{
    Mlt::Service service;
    int mltIndex = mltFilterIndex(&producer, row);
    Mlt::Filter *filter = producer.filter(mltIndex);
    if (filter && filter->is_valid()) {
        service = Mlt::Service(filter->get_service());
    } else {
        LOG_ERROR() << "Invalid filter index" << row;
    }
    delete filter;
    return service;
}

QHash<int, QByteArray> AttachedFiltersModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles[Qt::CheckStateRole] = "checkState";
    roles[TypeDisplayRole] = "typeDisplay";
    roles[PluginTypeRole] = "pluginType";
    return roles;
}

Qt::DropActions AttachedFiltersModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool AttachedFiltersModel::insertRows(int row, int, const QModelIndex &)
{
    if (m_producer && m_producer->is_valid()) {
        if (m_dropRow == -1)
            m_dropRow = row;
        return true;
    } else {
        return false;
    }
}

bool AttachedFiltersModel::removeRows(int row, int, const QModelIndex &parent)
{
    if (m_producer && m_producer->is_valid() && m_dropRow >= 0 && row != m_dropRow) {
        bool result = moveRows(parent, row, 1, parent, m_dropRow);
        m_dropRow = -1;
        return result;
    } else {
        return false;
    }
}

bool AttachedFiltersModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                                    const QModelIndex &destinationParent, int destinationRow)
{
    if (!m_producer || !m_producer->is_valid() || sourceParent != destinationParent || count != 1) {
        return false;
    }

    if (destinationRow == sourceRow) {
        return false;
    }

    if (isSourceClip()) {
        doMoveService(*m_producer, sourceRow, destinationRow);
    } else {
        MAIN.undoStack()->push(new Filter::MoveCommand(*this, name(sourceRow), sourceRow, destinationRow));
    }

    return true;
}

void AttachedFiltersModel::doMoveService(Mlt::Producer &producer, int fromRow, int toRow)
{
    int mltSrcFilterIndex = mltFilterIndex(&producer, fromRow);
    int mltDstFilterIndex = mltFilterIndex(&producer, toRow);
    int mltSrcLinkIndex = mltLinkIndex(&producer, fromRow);
    int mltDstLinkIndex = mltLinkIndex(&producer, toRow);
    if (isProducerLoaded(producer)) {
        int modelToRow = toRow;
        if (modelToRow > fromRow) {
            // Satisfy the model move requirements
            modelToRow++;
        }

        QModelIndex fromIndex = createIndex(fromRow, 0);
        QModelIndex toIndex = createIndex(modelToRow, 0);
        if (!fromIndex.isValid() || !toIndex.isValid()) {
            LOG_ERROR() << "Invalid Index" << fromIndex << toIndex;
            return;
        }
        if (!beginMoveRows(fromIndex.parent(), fromRow, fromRow, toIndex.parent(), modelToRow)) {
            LOG_ERROR() << "Invalid Move" << fromRow << modelToRow;
            return;
        }
        if (mltSrcFilterIndex >= 0 && mltDstFilterIndex >= 0) {
            m_event->block();
            producer.move_filter(mltSrcFilterIndex, mltDstFilterIndex);
            m_event->unblock();
            m_metaList.move(fromRow, toRow);
            endMoveRows();
            emit changed();
        } else if (mltSrcLinkIndex >= 0 && mltDstLinkIndex >= 0) {
            m_event->block();
            Mlt::Chain chain(producer);
            chain.move_link(mltSrcLinkIndex, mltDstLinkIndex);
            m_event->unblock();
            m_metaList.move(fromRow, toRow);
            endMoveRows();
            emit changed();
        } else {
            endMoveRows();
            LOG_ERROR() << "Failed to move filter" << fromRow << toRow;
        }
    } else {
        if (mltSrcFilterIndex >= 0 && mltDstFilterIndex >= 0) {
            producer.move_filter(mltSrcFilterIndex, mltDstFilterIndex);
            emit changed();
        } else if (mltSrcLinkIndex >= 0 && mltDstLinkIndex >= 0) {
            Mlt::Chain chain(producer);
            chain.move_link(mltSrcLinkIndex, mltDstLinkIndex);
            emit changed();
        } else {
            LOG_ERROR() << "Failed to move filter" << fromRow << toRow;
        }
    }
}

int AttachedFiltersModel::add(QmlMetadata *meta)
{
    int insertRow = -1;
    if (!m_producer) return -1;

    if (!meta->allowMultiple()) {
        for (int i = 0; i < m_metaList.count(); i++) {
            const QmlMetadata *attachedMeta = m_metaList[i];
            if (attachedMeta && meta->uniqueId() == attachedMeta->uniqueId()) {
                emit duplicateAddFailed(i);
                return -1;
            }
        }
    }
    if (m_producer->is_valid() && mlt_service_tractor_type != m_producer->type()
            && !QmlApplication::confirmOutputFilter()) {
        return -1;
    }

    switch (meta->type()) {
    case QmlMetadata::Filter: {
        Mlt::Filter filter(MLT.profile(), meta->mlt_service().toUtf8().constData());
        if (filter.is_valid()) {
            insertRow = findInsertRow(meta);
            filter.set(kNewFilterProperty, 1);
            if (!meta->objectName().isEmpty())
                filter.set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
            filter.set_in_and_out(
                m_producer->get(kFilterInProperty) ? m_producer->get_int(kFilterInProperty) : m_producer->get_in(),
                m_producer->get(kFilterOutProperty) ? m_producer->get_int(kFilterOutProperty) :
                m_producer->get_out());
            if (isSourceClip()) {
                doAddService(*m_producer, filter, insertRow);
            } else {
                MAIN.undoStack()->push(new Filter::AddCommand(*this, meta->name(), filter, insertRow));
            }
        }
    }
    break;
    case QmlMetadata::Link: {
        if (m_producer->type() != mlt_service_chain_type) {
            LOG_ERROR() << "Not a chain";
            return -1;
        }
        if (meta->seekReverse() && m_producer->get_int("meta.media.has_b_frames") != 0) {
            emit requestConvert(tr("This file has B-frames, which is not supported by %1.").arg(meta->name()),
                                false, true);
            return -1;
        }
        Mlt::Link link(meta->mlt_service().toUtf8().constData());
        if (link.is_valid()) {
            insertRow = findInsertRow(meta);
            link.set(kNewFilterProperty, 1);
            if (!meta->objectName().isEmpty())
                link.set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
            link.set_in_and_out(
                m_producer->get(kFilterInProperty) ? m_producer->get_int(kFilterInProperty) : m_producer->get_in(),
                m_producer->get(kFilterOutProperty) ? m_producer->get_int(kFilterOutProperty) :
                m_producer->get_out());
            if (isSourceClip()) {
                doAddService(*m_producer, link, insertRow);
            } else {
                MAIN.undoStack()->push(new Filter::AddCommand(*this, meta->name(), link, insertRow));
            }
        }
    }
    break;
    case QmlMetadata::FilterSet: {
        Mlt::Producer filterSetProducer = getFilterSetProducer(meta);
        if (!filterSetProducer.is_valid() || filterSetProducer.filter_count() == 0) {
            LOG_ERROR() << "Invalid producer" << meta->name() << filterSetProducer.filter_count();
            return -1;
        }
        for (int i = 0; i < filterSetProducer.filter_count(); i++) {
            Mlt::Filter *filter = filterSetProducer.filter(i);
            if (filter->is_valid() && !filter->get_int("_loader")) {
                QmlMetadata *tmpMeta = MAIN.filterController()->metadataForService(filter);
                insertRow = findInsertRow(meta);
                if (!meta->objectName().isEmpty())
                    filter->set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
                filter->set_in_and_out(
                    m_producer->get(kFilterInProperty) ? m_producer->get_int(kFilterInProperty) : m_producer->get_in(),
                    m_producer->get(kFilterOutProperty) ? m_producer->get_int(kFilterOutProperty) :
                    m_producer->get_out());
                if (isSourceClip()) {
                    doAddService(*m_producer, *filter, insertRow);
                } else {
                    Filter::AddCommand::AddType type = Filter::AddCommand::AddSet;
                    if (i == filterSetProducer.filter_count() - 1) {
                        type = Filter::AddCommand::AddSetLast;
                    }
                    MAIN.undoStack()->push(new Filter::AddCommand(*this, meta->name(), *filter, insertRow, type));
                }
            }
            delete filter;
        }
    }
    break;
    default:
        LOG_ERROR() << "Unknown type" << meta->type();
        break;
    }

    return insertRow;
}

void AttachedFiltersModel::doAddService(Mlt::Producer &producer, Mlt::Service &service, int row)
{
    LOG_DEBUG() << row;
    if (!producer.is_valid()) {
        LOG_ERROR() << "Invalid producer";
        return;
    }

    switch (service.type()) {
    case mlt_service_filter_type: {
        int linkRows = 0;
        if (producer.type() == mlt_service_chain_type) {
            Mlt::Chain chain(producer);
            linkRows = chain.link_count() - normalLinkCount(&producer);
        }
        int normFilterCount = normalFilterCount(&producer);
        int mltIndex = normFilterCount + row - linkRows;
        if (mltIndex < 0) {
            LOG_ERROR() << "Invalid MLT index" << row;
            return;
        }
        Mlt::Filter filter(service);
        if (isProducerLoaded(producer)) {
            beginInsertRows(QModelIndex(), row, row);
            if (MLT.isSeekable())
                MLT.pause();
            m_event->block();
            producer.attach(filter);
            producer.move_filter(m_producer->filter_count() - 1, mltIndex);
            m_event->unblock();
            QmlMetadata *meta = MAIN.filterController()->metadataForService(&service);
            m_metaList.insert(row, meta);
            endInsertRows();
            emit addedOrRemoved(m_producer.data());

        } else {
            producer.attach(filter);
            producer.move_filter(producer.filter_count() - 1, mltIndex);
        }
        emit changed();
    }
    break;
    case mlt_service_link_type: {
        if (producer.type() != mlt_service_chain_type) {
            LOG_ERROR() << "Not a chain";
            return;
        }
        Mlt::Chain chain(producer);
        int normLinkCount = normalLinkCount(&producer);
        int mltIndex = normLinkCount + row;
        Mlt::Link link(service);
        if (isProducerLoaded(producer)) {
            beginInsertRows(QModelIndex(), row, row);
            if (MLT.isSeekable())
                MLT.pause();
            m_event->block();
            chain.attach(link);
            chain.move_link(chain.link_count() - 1, mltIndex);
            m_event->unblock();
            QmlMetadata *meta = MAIN.filterController()->metadataForService(&service);
            m_metaList.insert(row, meta);
            endInsertRows();
            emit addedOrRemoved(m_producer.data());
        } else {
            chain.attach(link);
            chain.move_link(chain.link_count() - 1, mltIndex);
        }
        emit changed();
    }
    break;
    default:
        LOG_ERROR() << "invalid service type" << service.type();
        break;
    }
}

void AttachedFiltersModel::remove(int row)
{
    LOG_DEBUG() << row;
    if (isSourceClip()) {
        doRemoveService(*m_producer, row);
    } else {
        int mltIndex = mltLinkIndex(m_producer.get(), row);
        if (mltIndex >= 0) {
            Mlt::Chain chain(*(m_producer.get()));
            Mlt::Link *link = chain.link(mltIndex);
            MAIN.undoStack()->push(new Filter::RemoveCommand(*this, name(row), *link, row));
            delete link;
        } else {
            mltIndex = mltFilterIndex(m_producer.get(), row);
            if (mltIndex >= 0) {
                Mlt::Filter *filter = m_producer->filter(mltIndex);
                MAIN.undoStack()->push(new Filter::RemoveCommand(*this, name(row), *filter, row));
                delete filter;
            }
        }
    }
}

void AttachedFiltersModel::doRemoveService(Mlt::Producer &producer, int row)
{
    int filterIndex = mltFilterIndex(&producer, row);
    int linkIndex = mltLinkIndex(&producer, row);
    LOG_DEBUG() << row << filterIndex << linkIndex;
    if (linkIndex >= 0 ) {
        Mlt::Chain chain(producer);
        Mlt::Link *link = chain.link(linkIndex);
        if (isProducerLoaded(producer)) {
            beginRemoveRows(QModelIndex(), row, row);
            m_event->block();
            chain.detach(*link);
            m_event->unblock();
            m_metaList.removeAt(row);
            endRemoveRows();
            emit addedOrRemoved(m_producer.get());
        } else {
            chain.detach(*link);
        }
        emit changed();
        delete link;
    } else if (filterIndex >= 0) {
        Mlt::Filter *filter = producer.filter(filterIndex);
        if (isProducerLoaded(producer)) {
            beginRemoveRows(QModelIndex(), row, row);
            m_event->block();
            producer.detach(*filter);
            m_event->unblock();
            m_metaList.removeAt(row);
            endRemoveRows();
            emit addedOrRemoved(m_producer.get());
        } else {
            producer.detach(*filter);
        }
        emit changed();
        delete filter;
    } else {
        LOG_WARNING() << "invalid service:" << producer.type();
    }
}

bool AttachedFiltersModel::move(int fromRow, int toRow)
{
    QModelIndex parent = QModelIndex();
    if (fromRow < 0 || toRow < 0) {
        return false;
    }
    return moveRows(parent, fromRow, 1, parent, toRow);
}

void AttachedFiltersModel::reset(Mlt::Producer *producer)
{
    beginResetModel();
    m_event.reset();
    if (producer && producer->is_valid())
        m_producer.reset(new Mlt::Producer(producer));
    else if (MLT.isClip() && qstrcmp("_hide", MLT.producer()->get("resource")))
        m_producer.reset(new Mlt::Producer(MLT.producer()));
    else
        m_producer.reset();
    m_metaList.clear();

    if (m_producer && m_producer->is_valid()) {
        Mlt::Event *event = m_producer->listen("service-changed", this,
                                               (mlt_listener)AttachedFiltersModel::producerChanged);
        m_event.reset(event);
        int count = 0;
        if (m_producer->type() == mlt_service_chain_type) {
            Mlt::Chain chain(*m_producer.data());
            count = chain.link_count();
            for (int i = 0; i < count; i++) {
                Mlt::Link *link = chain.link(i);
                if (link && link->is_valid()) {
                    if (!link->get_int("_loader")) {
                        QmlMetadata *newMeta = MAIN.filterController()->metadataForService(link);
                        m_metaList.append(newMeta);
                    }
                }
                delete link;
            }
        }
        count = m_producer->filter_count();
        for (int i = 0; i < count; i++) {
            Mlt::Filter *filter = m_producer->filter(i);
            if (filter && filter->is_valid()) {
                if (!filter->get_int("_loader")) {
                    QmlMetadata *newMeta = MAIN.filterController()->metadataForService(filter);
                    m_metaList.append(newMeta);
                }
            }
            delete filter;
        }
    }

    endResetModel();
    emit trackTitleChanged();
    emit isProducerSelectedChanged();
    emit supportsLinksChanged();
}

Mlt::Producer AttachedFiltersModel::getFilterSetProducer(QmlMetadata *meta)
{
    Mlt::Producer filterSetProducer;
    auto name = meta->name();
    auto dir = QmlApplication::dataDir();
    dir.cd("shotcut");
    dir.cd("filter-sets");
    if (!QFileInfo::exists(dir.filePath(name))) {
        dir = Settings.appDataLocation();
        if (!dir.cd("filter-sets"))
            return filterSetProducer;
    }

    auto fileName = QUrl::toPercentEncoding(name.toUtf8());
    QFile filtersetFile(dir.filePath(fileName));
    QString xml;
    if (filtersetFile.open(QIODevice::ReadOnly)) {
        xml = QString::fromUtf8(filtersetFile.readAll());
    } else {
        filtersetFile.setFileName(dir.filePath(name));
        if (filtersetFile.open(QIODevice::ReadOnly)) {
            xml = QString::fromUtf8(filtersetFile.readAll());
        }
    }
    if (MLT.isMltXml(xml)) {
        filterSetProducer = Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
    }
    return filterSetProducer;
}

int AttachedFiltersModel::findInsertRow(QmlMetadata *meta)
{
    // Put the filter after the last filter that is less than or equal in sort order.
    int insertRow = 0;
    for (int i = m_metaList.count() - 1; i >= 0; i--) {
        if (sortOrder(m_metaList[i]) <= sortOrder(meta)) {
            insertRow = i + 1;
            break;
        }
    }
    return insertRow;
}

void AttachedFiltersModel::producerChanged(mlt_properties, AttachedFiltersModel *model)
{
    model->reset(model->m_producer.data());
}

bool AttachedFiltersModel::isProducerLoaded(Mlt::Producer &producer) const
{
    return m_producer && m_producer->get_service() == producer.get_service();
}

bool AttachedFiltersModel::isSourceClip() const
{
    return MLT.isClip() && !m_producer->get_int(kPlaylistIndexProperty);
}
