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
#include "mltcontroller.h"

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
        if (parent.row() < m_keyframeCounts.count())
            return m_keyframeCounts[parent.row()];
        return 0;
    }
    // parameters
    return m_propertyNames.count();
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
//        LOG_DEBUG() << "keyframe" << index.internalId() << index.row() << role;
        // keyframes
        if (m_filter && index.internalId() < quintptr(m_propertyNames.count())) {
            QString name = m_propertyNames[index.internalId()];
            Mlt::Animation animation = m_filter->getAnimation(name);
            if (animation.is_valid()) {
                int position = const_cast<Mlt::Animation&>(animation).key_get_frame(index.row());
                if (position >= 0) {
                    switch (role) {
                    case Qt::DisplayRole:
                    case NameRole:
                        return m_filter->timeFromFrames(position);
                    case FrameNumberRole:
                        return position;
                    case KeyframeTypeRole:
                        return const_cast<Mlt::Animation&>(animation).key_get_type(index.row());
                    default:
                        break;
                    }
                }
            }
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
        result = createIndex(row, column, parent.row());
    } else if (row < m_propertyNames.count()) {
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
    beginResetModel();
    m_propertyNames.clear();
    m_filter = filter;
    m_metadata = meta;
    if (m_filter)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (!m_metadata->keyframes()->parameter(i)->isSimple() || (m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)) {
            if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
                m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
                m_keyframeCounts << keyframeCount(i);
//            LOG_DEBUG() << m_propertyNames.last() << m_filter->get(m_propertyNames.last()) << keyframeCount(i);
            }
        }
    }
    endResetModel();
    emit loaded();
}

bool KeyframesModel::remove(int parameterIndex, int keyframeIndex)
{
    bool error = true;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            error = animation.remove(animation.key_get_frame(keyframeIndex));
            if (!error) {
                beginRemoveRows(index(parameterIndex), keyframeIndex, keyframeIndex);
                endRemoveRows();
            }
        }
    }
    return error;
}

int KeyframesModel::previousKeyframePosition(int parameterIndex, int currentPosition)
{
    int result = -1;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            currentPosition -= m_filter->in();
            result = animation.previous_key(animation.is_key(currentPosition)? currentPosition - 1: currentPosition);
            result += m_filter->in();
        }
    }
    return result;
}

int KeyframesModel::nextKeyframePosition(int parameterIndex, int currentPosition)
{
    int result = -1;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            currentPosition -= m_filter->in();
            result = animation.next_key(animation.is_key(currentPosition)? currentPosition + 1: currentPosition);
            result += m_filter->in();
        }
    }
    return result;
}

KeyframesModel::keyframeIndex(int parameterIndex, int currentPosition)
{
    int result = -1;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            for (int i = 0; i < animation.key_count() && result == -1; i++) {
                int frame = animation.key_get_frame(i);
                if (frame == currentPosition)
                    result = i;
                else if (frame > currentPosition)
                    break;
            }
        }
    }
    return result;
}

int KeyframesModel::parameterIndex(const QString& propertyName) const
{
    return m_propertyNames.indexOf(propertyName);
}

void KeyframesModel::reload()
{
    beginResetModel();
    m_propertyNames.clear();
    m_keyframeCounts.clear();
    if (m_filter)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (!m_metadata->keyframes()->parameter(i)->isSimple() || (m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)) {
            if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
                m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
                m_keyframeCounts << keyframeCount(i);
            }
        }
    }
    endResetModel();
}

void KeyframesModel::onFilterChanged(const QString& property)
{
//    LOG_DEBUG() << property;
    int i = m_propertyNames.indexOf(property);
    if (i > -1) {
        int count = m_keyframeCounts[i];
        m_keyframeCounts[i] = keyframeCount(i);
        if (count > 0) {
            beginRemoveRows(index(i), 0, count - 1);
            endRemoveRows();
        }
        if (m_keyframeCounts[i] > 0) {
//            LOG_DEBUG() << property << m_filter->get(property) << m_keyframeCounts[i];
            beginInsertRows(index(i), 0, m_keyframeCounts[i] - 1);
            endInsertRows();
        } else {
            reload();
        }
    } else {
        reload();
    }
}

int KeyframesModel::keyframeCount(int index) const
{

    return qMax(const_cast<QmlFilter*>(m_filter)->keyframeCount(m_propertyNames[index]), 0);
}
