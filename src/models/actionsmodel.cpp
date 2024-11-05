/*
 * Copyright (c) 2022-2024 Meltytech, LLC
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

#include "actionsmodel.h"

#include "actions.h"
#include <Logger.h>

#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QPalette>
#include <QFont>

ActionsModel::ActionsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    const auto keys = Actions.keys();
    for (const QString &key : keys) {
        m_actions.append(Actions[key]);
    }
}

QAction *ActionsModel::action(const QModelIndex &index) const
{
    if (index.row() < m_actions.size()) {
        return m_actions[index.row()];
    }
    return nullptr;
}

int ActionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_actions.size();
}

int ActionsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return COLUMN_COUNT;
}

QVariant ActionsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    switch (role) {
    case Qt::StatusTipRole:
    case Qt::DecorationRole:
    case Qt::TextAlignmentRole:
    case Qt::CheckStateRole:
    case Qt::SizeHintRole:
    case Qt::BackgroundRole:
        return result;
    }

    if (!index.isValid() || index.column() < 0 || index.column() >= COLUMN_COUNT || index.row() < 0
            || index.row() >= m_actions.size()) {
        LOG_ERROR() << "Invalid Index: " << index.row() << index.column() << role;
        return result;
    }

    QAction *action = m_actions[index.row()];
    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        switch (index.column()) {
        case COLUMN_ACTION:
            result = action->property(Actions.displayProperty).toString();
            break;
        case COLUMN_SEQUENCE1: {
            QList<QKeySequence> sequences = action->shortcuts();
            if (sequences.size() > 0)
                result = sequences[0].toString(QKeySequence::NativeText);
        }
        break;
        case COLUMN_SEQUENCE2: {
            result = action->property(Actions.hardKeyProperty);
            if (result.toString().isEmpty()) {
                QList<QKeySequence> sequences = action->shortcuts();
                if (sequences.size() > 1)
                    result = sequences[1].toString(QKeySequence::NativeText);
            }
        }
        break;
        default:
            LOG_ERROR() << "Invalid Column" << index.column() << role;
            break;
        }
        break;
    case Qt::ToolTipRole:
        result = action->toolTip();
        break;
    case Qt::FontRole:
        if (index.column() == COLUMN_SEQUENCE1) {
            QFont font;
            QList<QKeySequence> sequences = action->shortcuts();
            if (sequences.size() > 0 && sequences[0] != action->property(Actions.defaultKey1Property)) {
                font.setBold(true);
            }
            result = font;
        } else if (index.column() == COLUMN_SEQUENCE2) {
            QFont font;
            QList<QKeySequence> sequences = action->shortcuts();
            if (action->property(Actions.hardKeyProperty).isValid()) {
                font.setItalic(true);
            } else if (sequences.size() > 1 && sequences[1] != action->property(Actions.defaultKey2Property)) {
                font.setBold(true);
            }
            result =  font;
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == COLUMN_SEQUENCE2 && action->property(Actions.hardKeyProperty).isValid()) {
            result =  QGuiApplication::palette().color(QPalette::Disabled, QPalette::Text);
        }
        break;
    case ActionsModel::HardKeyRole:
        result = action->property(Actions.hardKeyProperty);
        break;
    case ActionsModel::DefaultKeyRole:
        switch (index.column()) {
        case COLUMN_SEQUENCE1:
            result = action->property(Actions.defaultKey1Property);
            break;
        case COLUMN_SEQUENCE2:
            result = action->property(Actions.defaultKey2Property);
            break;
        default:
            break;
        }
        break;
    default:
        auto names = roleNames();
        LOG_ERROR() << "Invalid Role" << index.row() << index.column() << names[role] << role;
        break;
    }
    return result;
}

bool ActionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() )
        return false;

    if (role != Qt::EditRole)
        return false;

    if (index.column() != COLUMN_SEQUENCE1 && index.column() != COLUMN_SEQUENCE2)
        return false;

    if (index.row() < 0 || index.row() >= m_actions.size()) {
        return false;
    }

    QKeySequence ks;
    if (value.canConvert<QKeySequence>())
        ks = value.value<QKeySequence>();
    else if (value.canConvert<QString>())
        ks = QKeySequence(value.toString());
    else
        return false;

    QAction *action = m_actions[index.row()];

    if (!ks.isEmpty()) {
        for (const auto a : std::as_const(m_actions)) {
            QList<QKeySequence> sequences = a->shortcuts();
            for (int i = 0; i < sequences.size(); i++) {
                if (sequences[i] == ks) {
                    if (a != action) {
                        QString error = tr("Shortcut %1 is used by %2").arg(ks.toString(), a->property(
                                                                                Actions.displayProperty).toString());
                        emit editError(error);
                    }
                    return false;
                }
            }
            QString hardKey = a->property(Actions.hardKeyProperty).toString();
            if (!hardKey.isEmpty() && hardKey == ks.toString()) {
                QString error = tr("Shortcut %1 is reserved for use by %2").arg(ks.toString(), a->property(
                                                                                    Actions.displayProperty).toString());
                emit editError(error);
                return false;
            }
        }
    }

    QList<QKeySequence> oldShortcuts = action->shortcuts();
    QList<QKeySequence> newShortcuts;
    if (index.column() == COLUMN_SEQUENCE1) {
        newShortcuts << ks;
        if (oldShortcuts.size() > 1 && oldShortcuts[1] != ks) {
            newShortcuts << oldShortcuts[1];
        }
    } else if  (index.column() == COLUMN_SEQUENCE2) {
        if (oldShortcuts.size() > 0 && oldShortcuts[0] != ks) {
            newShortcuts << oldShortcuts[0];
        }
        newShortcuts << ks;
    }

    Actions.overrideShortcuts(action->objectName(), newShortcuts);

    emit dataChanged(this->index(index.row(), COLUMN_SEQUENCE1), this->index(index.row(),
                                                                             COLUMN_SEQUENCE2));
    return true;
}

QVariant ActionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case COLUMN_ACTION:
            return tr("Action");
        case COLUMN_SEQUENCE1:
            return tr("Shortcut 1");
        case COLUMN_SEQUENCE2:
            return tr("Shortcut 2");
        default:
            auto names = roleNames();
            LOG_ERROR() << "Invalid section" << section << names[role] << role;
            break;
        }
    }
    return QVariant();
}

QModelIndex ActionsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (column < 0 || column >= COLUMN_COUNT || row < 0 || row >= m_actions.size())
        return QModelIndex();
    return createIndex(row, column, (int)0);
}

QModelIndex ActionsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

Qt::ItemFlags ActionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == COLUMN_SEQUENCE1 || index.column() == COLUMN_SEQUENCE2) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

QHash<int, QByteArray> ActionsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[HardKeyRole] = "hardKey";
    roles[DefaultKeyRole] = "defaultKey";
    return roles;
}
