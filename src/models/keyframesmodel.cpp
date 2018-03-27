/*
 * Copyright (c) 2018 Meltytech, LLC
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

#include "keyframesmodel.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlfilter.h"

#include <Logger.h>

static const quintptr NO_PARENT_ID = quintptr(-1);

KeyframesModel::KeyframesModel(QObject* parent)
    : QAbstractItemModel(parent)
{

}

KeyframesModel::~KeyframesModel()
{

}

int KeyframesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // keyframes
        if (parent.row() < m_animations.count())
            return qMax(keyframeCount(parent.row()), 0);
        else
            return 0;
    }
    // parameters
    return m_animations.count();
}

int KeyframesModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant KeyframesModel::data(const QModelIndex& index, int role) const
{
    if (!m_metadata || !index.isValid())
        return QVariant();
    if (index.parent().isValid()) {
//        LOG_DEBUG() << "keyframe" << index.row() << role;
        // keyframes
        if (index.internalId() < quintptr(m_animations.count()) && index.row() < keyframeCount(index.internalId()))
        switch (role) {
        case Qt::DisplayRole:
        case NameRole:
            if (m_filter && m_filter->filter().is_valid()) {
                // The property value of this keyframe
                QString name = m_metadata->keyframes()->parameter(index.internalId())->property();
                int position = const_cast<Mlt::Animation&>(m_animations[index.internalId()]).key_get_frame(index.row());
                return const_cast<QmlFilter*>(m_filter)->get(name.toUtf8().constData(), position);
            }
        case FrameNumberRole:
            return const_cast<Mlt::Animation&>(m_animations[index.internalId()]).key_get_frame(index.row());
        case KeyframeTypeRole:
            return const_cast<Mlt::Animation&>(m_animations[index.internalId()]).key_get_type(index.row());
        default:
            break;
        }
    } else if (index.row() < m_metadata->keyframes()->parameterCount()) {
//        LOG_DEBUG() << "parameter" << index.row() << role;
        // parameters
        switch (role) {
        case Qt::DisplayRole:
        case NameRole:
            return m_metadata->keyframes()->parameter(index.row())->name();
        case PropertyNameRole:
            return m_metadata->keyframes()->parameter(index.row())->property();
        case IsCurvesRole:
            //TODO use parmeter metadata
            return false;
        default:
            break;
        }
    }
    return QVariant();
}

QModelIndex KeyframesModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column > 0)
        return QModelIndex();
    QModelIndex result;
    if (parent.isValid()) {
        // keyframes
        if (parent.row() < m_animations.count() && row < keyframeCount(parent.row()))
            result = createIndex(row, column, parent.row());
    } else if (row < m_animations.count()) {
        result = createIndex(row, column, NO_PARENT_ID);
    }
    return result;
}

QModelIndex KeyframesModel::parent(const QModelIndex& index) const
{
    if (!index.isValid() || index.internalId() == NO_PARENT_ID)
        return QModelIndex();
    else
        return createIndex(index.internalId(), 0, NO_PARENT_ID);
}

QHash<int, QByteArray> KeyframesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[PropertyNameRole] = "property";
    roles[IsCurvesRole] = "curves";
    roles[FrameNumberRole] = "frame";
    roles[KeyframeTypeRole] = "interpolation";
    return roles;
}

void KeyframesModel::load(QmlFilter* filter, QmlMetadata* meta)
{
    if (m_metadata) {
        beginResetModel();
        m_metadata = 0;
        m_animations.clear();
        endResetModel();
    }
    m_filter = filter;
    m_metadata = meta;
    for (int i = 0; i < meta->keyframes()->parameterCount(); i++) {
        QString propertyName = meta->keyframes()->parameter(i)->property();
        // Cause a string property to be interpreted as animated value.
        filter->getDouble(propertyName, 0);
        m_animations << Mlt::Animation(filter->filter().get_animation(propertyName.toUtf8().constData()));
//        LOG_DEBUG() << propertyName << filter->get(propertyName) << keyframeCount(i);
    }
    if (m_animations.count() > 0) {
        beginInsertRows(QModelIndex(), 0, m_animations.count() - 1);
        endInsertRows();
    }
    emit loaded();
}

int KeyframesModel::keyframeCount(int index) const
{
    return const_cast<Mlt::Animation&>(m_animations[index]).key_count();
}
