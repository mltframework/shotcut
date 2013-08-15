/*
 * Copyright (c) 2012-2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef MELTEDUNITSMODEL_H
#define MELTEDUNITSMODEL_H

#include <QAbstractTableModel>
#include <QTcpSocket>
#include "mvcpthread.h"
#include <mvcp_tokeniser.h>

class QTimer;

class MeltedUnitsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MeltedUnitsModel(QObject *parent = 0);
    ~MeltedUnitsModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
    void disconnected();
    void clipIndexChanged(quint8 unit, int index);
    void generationChanged(quint8 unit);
    void positionUpdated(quint8 unit, int position, double fps, int in, int out, int length, bool isPlaying);

public slots:
    void onConnected(MvcpThread*);
    void onConnected(const QString& address, quint16 port = 5250, quint8 unit = 0);
    void onDisconnected();

private slots:
    void readResponse();

private:
    MvcpThread* m_mvcp;
    QTcpSocket m_socket;
    QObjectList m_units;
    mvcp_tokeniser m_tokeniser;
    QByteArray m_data;
    bool m_statusSent;

    QString decodeStatus(unit_status status);

private slots:
    void onUlsResult(QStringList);
};

#endif // MELTEDUNITSMODEL_H
