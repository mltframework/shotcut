/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "meltedunitsmodel.h"
#include <QtCore/QTimer>
#include <QDebug>

MeltedUnitsModel::MeltedUnitsModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_mvcp(0)
    , m_timer(0)
{
}

MeltedUnitsModel::~MeltedUnitsModel()
{
    onDisconnected();
}

int MeltedUnitsModel::rowCount(const QModelIndex& parent) const
{
    return m_units.size();
}

int MeltedUnitsModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant MeltedUnitsModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (!m_mvcp || role != Qt::DisplayRole || index.column() > 1)
        return result;
    switch (index.column()) {
        case 0:
            result = QString("U%1: %2").arg(index.row()).arg(m_units[index.row()]->objectName());
            break;
        case 1:
            result = m_units[index.row()]->property("status");
            break;
        default:
            break;
    }
    return result;
}

QVariant MeltedUnitsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        return (section == 0)? tr("Unit") : tr("Status");
    else
        return QVariant();
}

void MeltedUnitsModel::onConnected(MvcpThread *a_mvcp)
{
    m_mvcp = a_mvcp;
    connect(m_mvcp, SIGNAL(ulsResult(QStringList)), this, SLOT(onUlsResult(QStringList)));
    connect(m_mvcp, SIGNAL(ustaResult(QObject*)), this, SLOT(onUstaResult(QObject*)));
    m_mvcp->uls();
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    m_timer->start(1000);
}

void MeltedUnitsModel::onDisconnected()
{
    delete m_timer;
    m_timer = 0;
    emit beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    m_mvcp = 0;
    foreach (QObject* o, m_units)
        delete o;
    m_units.clear();
    emit endRemoveRows();
}

void MeltedUnitsModel::onTimeout()
{
    if (m_mvcp) m_mvcp->uls();
}

void MeltedUnitsModel::onUlsResult(QStringList units)
{
    for (quint8 i = 0; i < units.size(); i++) {
        if (i >= m_units.size()) {
            emit beginInsertRows(QModelIndex(), i, i);
            m_units.append(new QObject);
            emit endInsertRows();
        }
        m_units[i]->setObjectName(units[i]);
        emit dataChanged(createIndex(i, 0), createIndex(i, 0));
        m_mvcp->usta(i);
    }
}

void MeltedUnitsModel::onUstaResult(QObject* unit)
{
    quint8 i = unit->property("unit").toUInt();
    if (i < m_units.size()) {
        m_units[i]->setProperty("status", unit->property("status"));
        emit dataChanged(createIndex(i, 1), createIndex(i, 1));
    }
    delete unit;
}
