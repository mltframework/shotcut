/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
#include "controllers/filtercontroller.h"
#include "qmltypes/qmlmetadata.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "qmltypes/qmlapplication.h"
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
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

AttachedFiltersModel::AttachedFiltersModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dropRow(-1)
    , m_normFilterCount(0)
    , m_normLinkCount(0)
{
}

Mlt::Service *AttachedFiltersModel::getService(int row) const
{
    Mlt::Service *result = nullptr;
    int mltIndex = mltFilterIndex(row);
    if (mltIndex >= 0) {
        // Service is a filter
        result = m_producer->filter(mltIndex);
    } else {
        mltIndex = mltLinkIndex(row);
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
    case Qt::DisplayRole: {
        QVariant result;
        const QmlMetadata *meta = m_metaList[index.row()];
        if (meta) {
            result = meta->name();
        } else {
            // Fallback is raw mlt_service name
            Mlt::Service *service = getService(index.row());
            if (service && service->is_valid() && service->get("mlt_service")) {
                result = QString::fromUtf8(service->get("mlt_service"));
            }
            delete service;
        }
        return result;
    }
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
    if (role == Qt::CheckStateRole) {
        Mlt::Service *service = getService(index.row());
        if (service && service->is_valid()) {
            service->set("disable", !service->get_int("disable"));
            emit changed();
            QModelIndex modelIndex = createIndex(index.row(), 0);
            emit dataChanged(modelIndex, modelIndex, QVector<int>() << Qt::CheckStateRole);
        }
        delete service;
        return true;
    }
    return false;
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

    QModelIndex fromIndex = createIndex(sourceRow, 0);
    QModelIndex toIndex = createIndex(destinationRow, 0);

    if (fromIndex.isValid() && toIndex.isValid() &&
            (destinationRow < sourceRow || destinationRow > sourceRow + 1)) {
        if (beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationRow)) {
            if (destinationRow > sourceRow) {
                // Moving down: Convert to "post move" indexing
                destinationRow--;
            }
            int mltSrcFilterIndex = mltFilterIndex(sourceRow);
            int mltDstFilterIndex = mltFilterIndex(destinationRow);
            int mltSrcLinkIndex = mltLinkIndex(sourceRow);
            int mltDstLinkIndex = mltLinkIndex(destinationRow);
            if (mltSrcFilterIndex >= 0 && mltDstFilterIndex >= 0) {
                m_event->block();
                m_producer->move_filter(mltSrcFilterIndex, mltDstFilterIndex);
                m_event->unblock();
                m_metaList.move(sourceRow, destinationRow);
                endMoveRows();
                emit changed();
                return true;
            } else if (mltSrcLinkIndex >= 0 && mltDstLinkIndex >= 0) {
                m_event->block();

                Mlt::Chain chain(*(m_producer.data()));
                chain.move_link(mltSrcLinkIndex, mltDstLinkIndex);
                m_event->unblock();
                m_metaList.move(sourceRow, destinationRow);
                endMoveRows();
                emit changed();
                return true;
            } else {
                LOG_ERROR() << "Failed to move filter" << sourceRow << destinationRow;
                return false;
            }
        }
    }
    return false;
}

void AttachedFiltersModel::add(QmlMetadata *meta)
{
    if (!m_producer) return;

    if (!meta->allowMultiple()) {
        for (int i = 0; i < m_metaList.count(); i++) {
            const QmlMetadata *attachedMeta = m_metaList[i];
            if (attachedMeta && meta->uniqueId() == attachedMeta->uniqueId()) {
                emit duplicateAddFailed(i);
                return;
            }
        }
    }
    if (m_producer->is_valid() && mlt_service_tractor_type != m_producer->type()
            && !QmlApplication::confirmOutputFilter()) {
        return;
    }

    // Put the filter after the last filter that is less than or equal in sort order.
    int insertRow = 0;
    for (int i = m_metaList.count() - 1; i >= 0; i--) {
        if (sortOrder(m_metaList[i]) <= sortOrder(meta)) {
            insertRow = i + 1;
            break;
        }
    }

    int mltIndex = -1;
    if (meta->type() == QmlMetadata::Filter) {
        Mlt::Filter *filter = new Mlt::Filter(MLT.profile(), meta->mlt_service().toUtf8().constData());
        if (filter->is_valid()) {
            if (!meta->objectName().isEmpty())
                filter->set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
            filter->set_in_and_out(
                m_producer->get(kFilterInProperty) ? m_producer->get_int(kFilterInProperty) : m_producer->get_in(),
                m_producer->get(kFilterOutProperty) ? m_producer->get_int(kFilterOutProperty) :
                m_producer->get_out());

            // Calculate the MLT index for the new filter.
            if (m_metaList.count() == 0) {
                mltIndex = m_producer->filter_count();
            } else if (insertRow == 0) {
                mltIndex = m_normFilterCount;
            } else {
                mltIndex = mltFilterIndex(insertRow - 1) + 1;
            }
            beginInsertRows(QModelIndex(), insertRow, insertRow);
            if (MLT.isSeekable())
                MLT.pause();
            m_event->block();
            m_producer->attach(*filter);
            m_producer->move_filter(m_producer->filter_count() - 1, mltIndex);
            m_event->unblock();
            m_metaList.insert(insertRow, meta);

            endInsertRows();
            emit addedOrRemoved(m_producer.data());
            emit changed();
        } else LOG_WARNING() << "Failed to load filter" << meta->mlt_service();
        delete filter;
    } else if (meta->type() == QmlMetadata::Link) {
        if (m_producer->type() != mlt_service_chain_type) {
            LOG_WARNING() << "Not a chain";
        }
        if (meta->seekReverse() && m_producer->get_int("meta.media.has_b_frames") != 0) {
            emit requestConvert(tr("This file has B-frames, which is not supported by %1.").arg(meta->name()),
                                false, true);
            return;
        }
        Mlt::Link *link = new Mlt::Link(meta->mlt_service().toUtf8().constData());
        if (link && link->is_valid()) {
            LOG_WARNING() << "Add Link" << insertRow << meta->mlt_service().toUtf8().constData();

            if (!meta->objectName().isEmpty())
                link->set(kShotcutFilterProperty, meta->objectName().toUtf8().constData());
            link->set_in_and_out(
                m_producer->get(kFilterInProperty) ? m_producer->get_int(kFilterInProperty) : m_producer->get_in(),
                m_producer->get(kFilterOutProperty) ? m_producer->get_int(kFilterOutProperty) :
                m_producer->get_out());

            beginInsertRows(QModelIndex(), insertRow, insertRow);
            if (MLT.isSeekable())
                MLT.pause();
            m_event->block();
            Mlt::Chain chain(*(m_producer.data()));
            if (m_metaList.count() == 0) {
                mltIndex = chain.link_count();
            } else if (insertRow == 0) {
                mltIndex = m_normLinkCount;
            } else {
                mltIndex = mltLinkIndex(insertRow - 1) + 1;
            }
            chain.attach(*link);
            chain.move_link(chain.link_count() - 1, mltIndex);
            m_event->unblock();
            m_metaList.insert(insertRow, meta);
            endInsertRows();
            emit addedOrRemoved(m_producer.data());
            emit changed();
        } else LOG_WARNING() << "Failed to load link" << meta->mlt_service();
        delete link;
    }
}

void AttachedFiltersModel::remove(int row)
{
    int filterIndex = mltFilterIndex(row);
    int linkIndex = mltLinkIndex(row);
    if (filterIndex >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        Mlt::Filter *filter = m_producer->filter(filterIndex);
        m_event->block();
        m_producer->detach(*filter);
        m_event->unblock();
        m_metaList.removeAt(row);
        endRemoveRows();
        emit addedOrRemoved(m_producer.data());
        emit changed();
        delete filter;
    } else if (linkIndex >= 0 ) {
        beginRemoveRows(QModelIndex(), row, row);
        Mlt::Chain chain(*(m_producer.data()));
        Mlt::Link *link = chain.link(linkIndex);
        m_event->block();
        chain.detach(*link);
        m_event->unblock();
        m_metaList.removeAt(row);
        endRemoveRows();
        emit addedOrRemoved(m_producer.data());
        emit changed();
        delete link;
    } else {
        LOG_WARNING() << "Invalid index:" << row;
    }
}

bool AttachedFiltersModel::move(int fromRow, int toRow)
{
    QModelIndex parent = QModelIndex();

    if (fromRow < 0 || toRow < 0) {
        return false;
    }

    if (toRow > fromRow) {
        // Moving down: put it under the destination index
        toRow++;
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
    m_normFilterCount = 0;
    m_normLinkCount = 0;

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
                    if (link->get_int("_loader")) {
                        m_normLinkCount++;
                    } else {
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
                if (filter->get_int("_loader")) {
                    m_normFilterCount++;
                } else {
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

int AttachedFiltersModel::mltFilterIndex(int row) const
{
    if (row >= 0 && m_producer && m_producer->is_valid()) {
        int linkCount = 0;
        if (m_producer->type() == mlt_service_chain_type) {
            Mlt::Chain chain(*m_producer);
            linkCount = chain.link_count() - m_normLinkCount;
            if (row < linkCount) {
                // This row refers to an MLT link, not a filter
                return -1;
            }
        }
        int mltIndex = m_normFilterCount + row - linkCount;
        if (mltIndex >= 0 && mltIndex < m_producer->filter_count()) {
            return mltIndex;
        }
    }
    return -1;
}

int AttachedFiltersModel::mltLinkIndex(int row) const
{
    if (row >= 0 && m_producer && m_producer->is_valid()
            && m_producer->type() == mlt_service_chain_type) {
        Mlt::Chain chain(*m_producer);
        int linkCount = chain.link_count() - m_normLinkCount;
        if (row < linkCount) {
            return m_normLinkCount + row;
        }
    }
    return -1;
}

void AttachedFiltersModel::producerChanged(mlt_properties, AttachedFiltersModel *model)
{
    model->reset(model->m_producer.data());
}
