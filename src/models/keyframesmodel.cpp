/*
 * Copyright (c) 2018-2019 Meltytech, LLC
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
                    case NameRole: {
                        QString type = tr("Hold");
                        switch (const_cast<Mlt::Animation&>(animation).key_get_type(index.row())) {
                        case mlt_keyframe_linear:
                            type = tr("Linear");
                            break;
                        case mlt_keyframe_smooth:
                            type = tr("Smooth");
                            break;
                        default:
                            break;
                        }
                        return QString("%1 - %2").arg(m_filter->timeFromFrames(position)).arg(type);
                    }
                    case FrameNumberRole:
                        return position;
                    case KeyframeTypeRole:
                        return const_cast<Mlt::Animation&>(animation).key_get_type(index.row());
                    case NumericValueRole:
                        return m_filter->getDouble(name, position);
                    case MinimumFrameRole: {
                        int result = (index.row() > 0 && position > 0) ? (animation.previous_key(position - 1) + 1) : 0;
//                        LOG_DEBUG() << "keyframeIndex" << index.row() << "minimumFrame" << result;
                        return result;
                    }
                    case MaximumFrameRole: {
                        int minimum = animation.previous_key(qMax(0, position - 1)) + 1;
                        int result = animation.next_key(position + 1) - 1;
                        result = (result < minimum) ? m_filter->duration() : result;
//                        LOG_DEBUG() << "keyframeIndex" << index.row() << "maximumFrame" << result;
                        return result - 1;
                    }
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
            return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->name();
        case PropertyNameRole:
            return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->property();
        case IsCurveRole:
            return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->isCurve();
        case MinimumValueRole:
            return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->minimum();
        case MaximumValueRole:
            return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->maximum();
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
    roles[NameRole]         = "name";
    roles[PropertyNameRole] = "property";
    roles[IsCurveRole]      = "isCurve";
    roles[MinimumValueRole] = "minimum";
    roles[MaximumValueRole] = "maximum";
    roles[FrameNumberRole]  = "frame";
    roles[KeyframeTypeRole] = "interpolation";
    roles[NumericValueRole] = "value";
    roles[MinimumFrameRole] = "minimumFrame";
    roles[MaximumFrameRole] = "maximumFrame";
    return roles;
}

void KeyframesModel::load(QmlFilter* filter, QmlMetadata* meta)
{
    beginResetModel();
    m_propertyNames.clear();
    m_keyframeCounts.clear();
    m_metadataIndex.clear();
    m_filter = filter;
    m_metadata = meta;
    if (m_filter && m_metadata)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (!m_metadata->keyframes()->parameter(i)->isSimple() || (m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)) {
            if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
                m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
                m_keyframeCounts << keyframeCount(m_propertyNames.count() - 1);
                m_metadataIndex << i;
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
            int frame_num = animation.key_get_frame(keyframeIndex);
            error = animation.remove(frame_num);
            if (!error) {
                animation.interpolate();
                beginRemoveRows(index(parameterIndex), keyframeIndex, keyframeIndex);
                m_keyframeCounts[parameterIndex] -= 1;
                endRemoveRows();
                foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties()) {
                    Mlt::Animation animation = m_filter->getAnimation(name);
                    if (animation.is_valid() && !animation.remove(frame_num))
                        animation.interpolate();
                }

                QModelIndex modelIndex;
                if (keyframeIndex > 0) {
                    modelIndex = index(keyframeIndex - 1, 0, index(parameterIndex));
                    emit dataChanged(modelIndex, modelIndex, QVector<int>() << MaximumFrameRole);
                }
                if (keyframeIndex < keyframeCount(parameterIndex)) {
                    modelIndex = index(keyframeIndex, 0, index(parameterIndex));
                    emit dataChanged(modelIndex, modelIndex, QVector<int>() << MinimumFrameRole);
                }
                emit m_filter->changed();
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

int KeyframesModel::keyframeIndex(int parameterIndex, int currentPosition)
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

bool KeyframesModel::setInterpolation(int parameterIndex, int keyframeIndex, InterpolationType type)
{
    bool error = true;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            if (!animation.key_set_type(keyframeIndex, mlt_keyframe_type(type))) {
//                LOG_DEBUG() << "keyframe index" << keyframeIndex << "keyframe type" << type;
                foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties()) {
                    Mlt::Animation animation = m_filter->getAnimation(name);
                    animation.key_set_type(keyframeIndex, mlt_keyframe_type(type));
                }
                QModelIndex modelIndex = index(keyframeIndex, 0, index(parameterIndex));
                emit dataChanged(modelIndex, modelIndex, QVector<int>() << KeyframeTypeRole << NameRole);
                error = false;
                emit m_filter->changed();
            }
        }
    }
    if (error)
        LOG_ERROR() << "failed to set keyframe" << "at parameter index" << parameterIndex << "keyframeIndex" << keyframeIndex << "to type" << type;
    return error;
}

bool KeyframesModel::setPosition(int parameterIndex, int keyframeIndex, int position)
{
    bool error = true;
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid()) {
            if (!animation.key_set_frame(keyframeIndex, position)) {
//                LOG_DEBUG() << "keyframe index" << keyframeIndex << "position" << position;
                foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties()) {
                    Mlt::Animation animation = m_filter->getAnimation(name);
                    if (animation.is_valid())
                        animation.key_set_frame(keyframeIndex, position);
                }
                QModelIndex modelIndex = index(keyframeIndex, 0, index(parameterIndex));
                emit dataChanged(modelIndex, modelIndex, QVector<int>() << FrameNumberRole << NameRole);
                updateNeighborsMinMax(parameterIndex, keyframeIndex);
                error = false;
                emit m_filter->changed();
            }
        }
    }
    if (error)
        LOG_ERROR() << "failed to set keyframe" << "at parameter index" << parameterIndex << "keyframeIndex" << keyframeIndex << "to position" << position;
    return error;
}

void KeyframesModel::addKeyframe(int parameterIndex, double value, int position, KeyframesModel::InterpolationType type)
{
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        m_filter->set(name, value, position,  mlt_keyframe_type(type));
        foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties())
            m_filter->set(name, value, position,  mlt_keyframe_type(type));
    }
}

void KeyframesModel::addKeyframe(int parameterIndex, int position)
{
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        auto parameter = m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex]);
        if (parameter->isRectangle()) {
            auto value = m_filter->getRect(name, position);
            Mlt::Animation anim = m_filter->getAnimation(name);
            if (anim.is_valid() && !anim.is_key(position)) {
                mlt_keyframe_type keyframeType = m_filter->getKeyframeType(anim, position, mlt_keyframe_type(-1));
                m_filter->blockSignals(true);
                m_filter->set(name, value, 1.0, position, keyframeType);
                m_filter->blockSignals(false);
            }
        } else if (parameter->isCurve()) {
            // Get the value from the existing position.
            double value = m_filter->getDouble(name, position);
            Mlt::Animation anim = m_filter->getAnimation(name);
            if (anim.is_valid() && !anim.is_key(position)) {
                mlt_keyframe_type keyframeType = m_filter->getKeyframeType(anim, position, mlt_keyframe_type(-1));
                // Simply adding a keyframe does not change the current value of
                // the filter parameter. So, no need to trigger a bunch of signals
                // and refresh the consumer. Besides, refreshing the consumer is
                // some how causing the player to seek ahead by one frame inadvertently
                // such that changing the parameter value causes the addition of a
                // keyframe just after this one. MLT.refreshConsumer() with
                // frame-dropping enabled may have dropped video of the most recent
                // frame from the producer, but Shotcut does not know about it
                // because it did not receive a "consumer-frame-show" event for it.
                m_filter->blockSignals(true);
                m_filter->set(name, value, position, keyframeType);
                for (auto& key : parameter->gangedProperties())
                    m_filter->set(key, value, position, keyframeType);
                m_filter->blockSignals(false);
            }
        } else {
            emit keyframeAdded(name, position);
        }
        onFilterChanged(name);
    }
}

void KeyframesModel::setKeyframe(int parameterIndex, double value, int position, KeyframesModel::InterpolationType type)
{
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        m_filter->filter().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), mlt_keyframe_type(type));
        foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties())
            m_filter->filter().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), mlt_keyframe_type(type));
        emit m_filter->changed();
        QModelIndex modelIndex = index(keyframeIndex(parameterIndex, position), 0, index(parameterIndex));
        emit dataChanged(modelIndex, modelIndex, QVector<int>() << NumericValueRole << NameRole);
        updateNeighborsMinMax(parameterIndex, modelIndex.row());
    }
}

bool KeyframesModel::isKeyframe(int parameterIndex, int position)
{
    if (m_filter && parameterIndex < m_propertyNames.count()) {
        QString name = m_propertyNames[parameterIndex];
        Mlt::Animation anim = m_filter->getAnimation(name);
        return anim.is_valid() && anim.is_key(position);
    }
    return false;
}

void KeyframesModel::reload()
{
    beginResetModel();
    m_propertyNames.clear();
    m_keyframeCounts.clear();
    m_metadataIndex.clear();
    if (m_filter)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (!m_metadata->keyframes()->parameter(i)->isSimple() || (m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)) {
            if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
                m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
                m_keyframeCounts << keyframeCount(m_propertyNames.count() - 1);
                m_metadataIndex << i;
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

void KeyframesModel::onFilterInChanged(int delta)
{
    for (int parameterIndex = 0; parameterIndex < m_propertyNames.count(); parameterIndex++) {
        int count = m_keyframeCounts[parameterIndex];
        if (count > 0) {
            Mlt::Animation animation = m_filter->getAnimation(m_propertyNames[parameterIndex]);
            if (animation.is_valid()) {
                for (int keyframeIndex = 0; keyframeIndex < count;) {
                    int newFrame = animation.key_get_frame(keyframeIndex) - delta;
                    if (animation.is_key(newFrame)) {
                        beginRemoveRows(index(parameterIndex), keyframeIndex, keyframeIndex);
                        animation.remove(animation.key_get_frame(keyframeIndex));
                        animation.interpolate();
                        m_keyframeCounts[parameterIndex] -= 1;
                        endRemoveRows();
                        --count;
                    } else {
                        animation.key_set_frame(keyframeIndex, newFrame);
                        ++keyframeIndex;
                    }
                }
                QModelIndex parentIndex = index(parameterIndex);
                emit dataChanged(index(0, 0, parentIndex), index(count - 1, 0, parentIndex), QVector<int>() << FrameNumberRole);
            }
        }
    }
}

void KeyframesModel::onFilterOutChanged(int delta)
{
    Q_UNUSED(delta)
    for (int parameterIndex = 0; parameterIndex < m_propertyNames.count(); parameterIndex++) {
        int count = m_keyframeCounts[parameterIndex];
        if (count > 0) {
            Mlt::Animation animation = m_filter->getAnimation(m_propertyNames[parameterIndex]);
            if (animation.is_valid()) {
                for (int keyframeIndex = 0; keyframeIndex < count;) {
                    int frame = animation.key_get_frame(keyframeIndex);
                    if (frame < 0) {
                        beginRemoveRows(index(parameterIndex), keyframeIndex, keyframeIndex);
                        animation.remove(animation.key_get_frame(keyframeIndex));
                        animation.interpolate();
                        m_keyframeCounts[parameterIndex] -= 1;
                        endRemoveRows();
                        --count;
                    } else {
                        ++keyframeIndex;
                    }
                }
            }
        }
    }
}

int KeyframesModel::keyframeCount(int index) const
{
    if (index < m_propertyNames.count())
        return qMax(const_cast<QmlFilter*>(m_filter)->keyframeCount(m_propertyNames[index]), 0);
    else
        return 0;
}

void KeyframesModel::updateNeighborsMinMax(int parameterIndex, int keyframeIndex)
{
    QModelIndex modelIndex;
    if (keyframeIndex > 0) {
        modelIndex = index(keyframeIndex - 1, 0, index(parameterIndex));
        emit dataChanged(modelIndex, modelIndex, QVector<int>() << MaximumFrameRole);
    }
    if (keyframeIndex < keyframeCount(parameterIndex) - 1) {
        modelIndex = index(keyframeIndex + 1, 0, index(parameterIndex));
        emit dataChanged(modelIndex, modelIndex, QVector<int>() << MinimumFrameRole);
    }
}
