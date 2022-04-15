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

#include <QTimer>

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
                        double value = m_filter->getDouble(name, position);
                        QString units = m_metadata->keyframes()->parameter(m_metadataIndex[index.internalId()])->units();
                        return QString("%1 - %2\n%3%4").arg(m_filter->timeFromFrames(position)).arg(type).arg(value).arg(units);
                    }
                    case FrameNumberRole:
                        return position;
                    case KeyframeTypeRole:
                        return const_cast<Mlt::Animation&>(animation).key_get_type(index.row());
                    case NumericValueRole:
                        return m_filter->getDouble(name, position);
                    case MinimumFrameRole: {
                        int result = 0;
                        if (animation.previous_key(position - 1, result)) {
                            // first Keyframe
                            result = 0;
                        } else {
                            result += 1;
                        }

//                        LOG_DEBUG() << "keyframeIndex" << index.row() << "minimumFrame" << result;
                        return result;
                    }
                    case MaximumFrameRole: {
                        int result = 0;
                        if (animation.next_key(position + 1, result)) {
                            // Last Keyframe
                            result = m_filter->producer().get_out();
                        } else {
                            result -= 1;
                        }
//                        LOG_DEBUG() << "keyframeIndex" << index.row() << "maximumFrame" << result;
                        return result;
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
        {
            QmlKeyframesParameter* param = m_metadata->keyframes()->parameter(m_metadataIndex[index.row()]);
            if (param->rangeType() == QmlKeyframesParameter::MinMax) {
                return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->minimum();
            } else if (param->rangeType() == QmlKeyframesParameter::ClipLength) {
                return 0.0;
            }
            return 0.0;
        }
        case MaximumValueRole:
        {
            QmlKeyframesParameter* param = m_metadata->keyframes()->parameter(m_metadataIndex[index.row()]);
            if (param->rangeType() == QmlKeyframesParameter::MinMax) {
                return m_metadata->keyframes()->parameter(m_metadataIndex[index.row()])->maximum();
            } else if (param->rangeType() == QmlKeyframesParameter::ClipLength) {
                int length = m_filter->producer().get_length() - m_filter->in();
                return (double)length / MLT.profile().fps();
            }
            return 0.0;
        }
        case LowestValueRole:
        {
            QmlKeyframesParameter* param = m_metadata->keyframes()->parameter(m_metadataIndex[index.row()]);
            Mlt::Animation animation = m_filter->getAnimation(param->property());
            double min = std::numeric_limits<double>::max();
            if (animation.is_valid()) {
                for (int i = 0; i < animation.key_count(); i++) {
                    int frame = animation.key_get_frame(i);
                    if (frame >= 0) {
                        double value = m_filter->getDouble(param->property(), frame);
                        if (value < min) min = value;
                    }
                }
            }
            if (min == std::numeric_limits<double>::max()) min = 0;
            return min;
        }
        case HighestValueRole:
        {
            QmlKeyframesParameter* param = m_metadata->keyframes()->parameter(m_metadataIndex[index.row()]);
            Mlt::Animation animation = m_filter->getAnimation(param->property());
            double max = std::numeric_limits<double>::lowest();
            if (animation.is_valid()) {
                for (int i = 0; i < animation.key_count(); i++) {
                    int frame = animation.key_get_frame(i);
                    if (frame >= 0) {
                        double value = m_filter->getDouble(param->property(), frame);
                        if (value > max) max = value;
                    }
                }
            }
            if (max == std::numeric_limits<double>::lowest()) max = 0;
            return max;
        }
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
    roles[LowestValueRole] = "lowest";
    roles[HighestValueRole] = "highest";
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
    if (m_filter && m_metadata && m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
            m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
            m_keyframeCounts << keyframeCount(m_propertyNames.count() - 1);
            m_metadataIndex << i;
//        LOG_DEBUG() << m_propertyNames.last() << m_filter->get(m_propertyNames.last()) << keyframeCount(i);
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
        // Do not allow the user to delete the last keyframe.
        // Keyframes should be disabled in the filter panel instead
        if (animation.is_valid() && animation.key_count() > 1) {
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
                emit dataChanged(index(parameterIndex), index(parameterIndex), QVector<int>() << LowestValueRole << HighestValueRole);
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
            bool error = animation.previous_key(animation.is_key(currentPosition)? currentPosition - 1: currentPosition, result);
            if (!error)
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
            bool error = animation.next_key(animation.is_key(currentPosition)? currentPosition + 1: currentPosition, result);
            if (!error)
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
                emit m_filter->propertyChanged(name.toUtf8().constData());
            }
        }
    }
    if (error)
        LOG_ERROR() << "failed to set keyframe" << "at parameter index" << parameterIndex << "keyframeIndex" << keyframeIndex << "to type" << type;
    return error;
}

void KeyframesModel::setKeyframePosition(int parameterIndex, int keyframeIndex, int position)
{
    if (!m_filter) {
        LOG_ERROR() << "Invalid Filter" << parameterIndex;
        return;
    }

    if (parameterIndex >= m_propertyNames.count()) {
        LOG_ERROR() << "Invalid parameter index" << parameterIndex;
        return;
    }

    QString name = m_propertyNames[parameterIndex];
    Mlt::Animation animation = m_filter->getAnimation(name);
    if (!animation.is_valid()) {
        LOG_ERROR() << "Invalid animation" << parameterIndex;
        return;
    }

    if (keyframeIndex >= animation.key_count()) {
        LOG_ERROR() << "Invalid key index" << parameterIndex << keyframeIndex;
        return;
    }

    if (position < 0) {
        LOG_ERROR() << "Invalid key position" << parameterIndex << keyframeIndex << position;
        return;
    }

    int prevPosition = animation.key_get_frame(keyframeIndex);
    if (position == prevPosition) {
        LOG_ERROR() << "Position did not change" << parameterIndex << keyframeIndex << position;
        return;
    }

    if (animation.key_set_frame(keyframeIndex, position)) {
        LOG_ERROR() << "Failed to set position" << parameterIndex << keyframeIndex << position;
        return;
    }

    foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties()) {
        Mlt::Animation animation = m_filter->getAnimation(name);
        if (animation.is_valid())
            animation.key_set_frame(keyframeIndex, position);
    }
    QModelIndex modelIndex = index(keyframeIndex, 0, index(parameterIndex));
    emit dataChanged(modelIndex, modelIndex, QVector<int>() << FrameNumberRole << NameRole);
    updateNeighborsMinMax(parameterIndex, keyframeIndex);
    emit m_filter->changed();
    emit m_filter->propertyChanged(name.toUtf8().constData());
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
                m_filter->set(name, value, position, keyframeType);
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
                for (auto& key : parameter->gangedProperties()) {
                    value = m_filter->getDouble(key, position);
                    m_filter->set(key, value, position, keyframeType);
                }
                m_filter->blockSignals(false);
            }
        } else {
            emit keyframeAdded(name, position);
        }
        onFilterChanged(name);
    }
}

void KeyframesModel::setKeyframeValue(int parameterIndex, int keyframeIndex, double value)
{
    if (!m_filter) {
        LOG_ERROR() << "Invalid Filter" << parameterIndex;
        return;
    }

    if (parameterIndex >= m_propertyNames.count()) {
        LOG_ERROR() << "Invalid parameter index" << parameterIndex;
        return;
    }

    QString name = m_propertyNames[parameterIndex];
    Mlt::Animation animation = m_filter->getAnimation(name);
    if (!animation.is_valid()) {
        LOG_ERROR() << "Invalid animation" << parameterIndex;
        return;
    }

    if (keyframeIndex >= animation.key_count()) {
        LOG_ERROR() << "Invalid key index" << parameterIndex << keyframeIndex;
        return;
    }

    int position = animation.key_get_frame(keyframeIndex);
    if (position < 0) {
        LOG_ERROR() << "Invalid position" << parameterIndex << keyframeIndex;
        return;
    }

    mlt_keyframe_type type = animation.key_get_type(keyframeIndex);
    m_filter->service().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), type);
    foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties())
        m_filter->service().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), type);
    emit m_filter->changed();
    emit m_filter->propertyChanged(name.toUtf8().constData());
    QModelIndex modelIndex = index(keyframeIndex, 0, index(parameterIndex));
    emit dataChanged(modelIndex, modelIndex, QVector<int>() << NumericValueRole << NameRole);
    emit dataChanged(index(parameterIndex), index(parameterIndex), QVector<int>() << LowestValueRole << HighestValueRole);
}

void KeyframesModel::setKeyframeValuePosition(int parameterIndex, int keyframeIndex, double value, int position)
{
    if (!m_filter) {
        LOG_ERROR() << "Invalid Filter" << parameterIndex;
        return;
    }

    if (parameterIndex >= m_propertyNames.count()) {
        LOG_ERROR() << "Invalid parameter index" << parameterIndex;
        return;
    }

    QString name = m_propertyNames[parameterIndex];
    Mlt::Animation animation = m_filter->getAnimation(name);
    if (!animation.is_valid()) {
        LOG_ERROR() << "Invalid animation" << parameterIndex;
        return;
    }

    if (keyframeIndex >= animation.key_count()) {
        LOG_ERROR() << "Invalid key index" << parameterIndex << keyframeIndex;
        return;
    }

    if (position < 0) {
        LOG_ERROR() << "Invalid key position" << parameterIndex << keyframeIndex << position;
        return;
    }

    QVector<int> roles;
    int prevPosition = animation.key_get_frame(keyframeIndex);
    if (position != prevPosition) {
        if (animation.key_set_frame(keyframeIndex, position)) {
            LOG_ERROR() << "Failed to set position" << parameterIndex << keyframeIndex << position;
            return;
        }
        foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties()) {
            Mlt::Animation animation = m_filter->getAnimation(name);
            if (animation.is_valid())
                animation.key_set_frame(keyframeIndex, position);
        }
        roles << FrameNumberRole;
        updateNeighborsMinMax(parameterIndex, keyframeIndex);
    }

    mlt_keyframe_type type = animation.key_get_type(keyframeIndex);
    m_filter->service().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), type);
    foreach (name, m_metadata->keyframes()->parameter(m_metadataIndex[parameterIndex])->gangedProperties())
        m_filter->service().anim_set(name.toUtf8().constData(), value, position, m_filter->duration(), type);
    emit m_filter->changed();
    emit m_filter->propertyChanged(name.toUtf8().constData());
    roles << NumericValueRole << NameRole;
    QModelIndex modelIndex = index(keyframeIndex, 0, index(parameterIndex));
    emit dataChanged(modelIndex, modelIndex, roles);
    emit dataChanged(index(parameterIndex), index(parameterIndex), QVector<int>() << LowestValueRole << HighestValueRole);
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

bool KeyframesModel::advancedKeyframesInUse()
{
    if (m_filter && m_metadata && m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
            return true;
        }
    }
    return false;
}

void KeyframesModel::removeAdvancedKeyframes()
{
    if (m_filter && m_metadata && m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0) {
        for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
            QString name = m_metadata->keyframes()->parameter(i)->property();
            if (m_filter->keyframeCount(name) > 0) {
                m_filter->set(name, m_filter->get(name, 0));
                for (auto& key : m_metadata->keyframes()->parameter(i)->gangedProperties()) {
                    m_filter->set(key, m_filter->get(key, 0));
                }
            }
        }
        reload();
    }
}

bool KeyframesModel::simpleKeyframesInUse()
{
    return m_filter && m_metadata && (m_filter->animateIn() > 0 || m_filter->animateOut() > 0);
}

void KeyframesModel::removeSimpleKeyframes()
{
    if (simpleKeyframesInUse()) {
        for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
            QString name = m_metadata->keyframes()->parameter(i)->property();
            auto parameter = m_metadata->keyframes()->parameter(i);
            if (parameter->gangedProperties().count() > 0) {
                // Do not attempt to detect all matching keyframes for ganged properties.
                // Always convert to advanced.
                continue;
            }
            bool clearKeyframes = true;
            // Find out if all keyframe values are the same. If they are all the same,
            // then clear keyframes and set the parameter to a single value.
            if (parameter->isRectangle()) {
                auto firstValue = m_filter->getRect(name, 0);
                Mlt::Animation anim = m_filter->getAnimation(name);
                if (anim.is_valid()) {
                    for (int k = 1; k < anim.key_count(); k++) {
                        auto value = m_filter->getRect(name, anim.key_get_frame(k));
                        if (value != firstValue) {
                            clearKeyframes = false;
                            break;
                        }
                    }
                }
                if (clearKeyframes) {
                    m_filter->set(name, firstValue);
                }
            } else {
                double firstValue = m_filter->getDouble(name, 0);
                Mlt::Animation anim = m_filter->getAnimation(name);
                if (anim.is_valid()) {
                    for (int k = 1; k < anim.key_count(); k++) {
                        auto value = m_filter->getDouble(name, anim.key_get_frame(k));
                        if (value != firstValue) {
                            clearKeyframes = false;
                            break;
                        }
                    }
                }
                if (clearKeyframes) {
                    m_filter->set(name, firstValue);
                    m_filter->blockSignals(true);
                    for (auto& key : parameter->gangedProperties()) {
                        m_filter->set(key, m_filter->getDouble(key, 0));
                    }
                    m_filter->blockSignals(false);
                }
            }
        }
        m_filter->clearAnimateInOut();
    }
}

void KeyframesModel::reload()
{
    beginResetModel();
    m_propertyNames.clear();
    m_keyframeCounts.clear();
    m_metadataIndex.clear();
    if (m_filter && m_metadata && m_filter->animateIn() <= 0 && m_filter->animateOut() <= 0)
    for (int i = 0; i < m_metadata->keyframes()->parameterCount(); i++) {
        if (m_filter->keyframeCount(m_metadata->keyframes()->parameter(i)->property()) > 0) {
            m_propertyNames << m_metadata->keyframes()->parameter(i)->property();
            m_keyframeCounts << keyframeCount(m_propertyNames.count() - 1);
            m_metadataIndex << i;
        }
    }
    endResetModel();
}

void KeyframesModel::onFilterChanged(const QString& property)
{
    bool isKeyframeProperty = false;
    for (int p = 0; p < m_metadata->keyframes()->parameterCount() && isKeyframeProperty == false; p++) {
        if (m_metadata->keyframes()->parameter(p)->property() == property) {
            isKeyframeProperty = true;
            break;
        }
    }
    if (!isKeyframeProperty) {
        // Does not affect this model.
        return;
    }

    int i = m_propertyNames.indexOf(property);
    if (i < 0) {
        // First keyframe added. Reset model to add this parameter.
        reload();
        return;
    }

    int prevCount = m_keyframeCounts[i];
    m_keyframeCounts[i] = keyframeCount(i);
    if (m_keyframeCounts[i] == 0) {
        // All keyframes removed. Reset model to remove this parameter.
        reload();
    }
    else if (prevCount != m_keyframeCounts[i]) {
        // Keyframe count changed. Remove all old and insert all new.
        if (prevCount > 0) {
            beginRemoveRows(index(i), 0, prevCount - 1);
            endRemoveRows();
        }
        beginInsertRows(index(i), 0, m_keyframeCounts[i] - 1);
        endInsertRows();
        emit dataChanged(index(i), index(i), QVector<int>() << LowestValueRole << HighestValueRole);
    }
    else {
        // Keyframe count is unchanged. A value must have changed.
        emit dataChanged(index(i), index(i), QVector<int>() << LowestValueRole << HighestValueRole);
        emit dataChanged(index(0, 0, index(i)), index(m_keyframeCounts[i] - 1, 0, index(i)), QVector<int>() << NumericValueRole);
    }
}

void KeyframesModel::onFilterInChanged(int /*delta*/)
{
    QTimer::singleShot(0, this, SLOT(reload()));
}


void KeyframesModel::trimFilterIn(int in)
{
    Mlt::Service& service = m_filter->service();
    if (service.is_valid() && service.type() == mlt_service_filter_type) {
        Mlt::Filter filter = service;
        MLT.adjustFilter(&filter, filter.get_in(), filter.get_out(), in - filter.get_in(), 0);
    }
}

void KeyframesModel::trimFilterOut(int out)
{
    Mlt::Service& service = m_filter->service();
    if (service.is_valid() && service.type() == mlt_service_filter_type) {
        Mlt::Filter filter = service;
        MLT.adjustFilter(&filter, filter.get_in(), filter.get_out(), 0, filter.get_out() - out);
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
