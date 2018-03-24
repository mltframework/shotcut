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
            return const_cast<Mlt::Animation&>(m_animations[parent.row()]).key_count();
        else
            return 0;
    } else if (m_metadata) {
        // parameters
        return m_metadata->keyframes()->parameterCount();
    }
    return 0;
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
        // keyframes
        if (index.row() < m_metadata->keyframes()->parameterCount() && index.internalId() < quintptr(m_animations.count()))
        switch (role) {
        case Qt::DisplayRole:
            if (m_filter) {
                // The property value of this keyframe
                QString name = m_metadata->keyframes()->parameter(index.row())->property();
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
        // parameters
        switch (role) {
        case Qt::DisplayRole:
        case ParameterNameRole:
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
//    LOG_DEBUG() << __FUNCTION__ << row << column << parent;
    QModelIndex result;
    if (parent.isValid()) {
        // keyframes
        if (parent.row() < m_animations.count())
            result = createIndex(row, column, parent.row());
    } else if (m_metadata && row < m_metadata->keyframes()->parameterCount()) {
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
    roles[ParameterNameRole] = "parameter";
    roles[PropertyNameRole] = "property";
    roles[IsCurvesRole] = "curves";
    roles[FrameNumberRole] = "frame";
    roles[KeyframeTypeRole] = "keyframeType";
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
    for (int i = 0; i < meta->keyframes()->parameterCount(); i++) {
        QString propertyName = meta->keyframes()->parameter(i)->property();
        m_animations << Mlt::Animation(filter->filter().get_animation(propertyName.toUtf8().constData()));
    }
    m_metadata = meta;
    m_filter = filter;
    if (m_animations.count() > 0) {
        beginInsertRows(QModelIndex(), 0, m_animations.count() - 1);
        endInsertRows();
    }
    // emit loaded();
}
