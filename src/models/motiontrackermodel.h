/*
 * Copyright (c) 2023 Meltytech, LLC
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

#ifndef MOTIONTRACKERMODEL_H
#define MOTIONTRACKERMODEL_H

#include <MltProducer.h>
#include <QAbstractListModel>
#include <QString>
#include <QMap>
#include <QList>
#include <QRectF>

class QmlFilter;
namespace Mlt {
class Service;
}

class MotionTrackerModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct TrackingItem {
        int frame;
        QRectF rect;
    };

    explicit MotionTrackerModel(QObject *parent = nullptr);

    void load(Mlt::Producer *producer = nullptr, bool reset = true);
    QString add(const QString &name, const QString &data);
    void updateData(const QString &key, const QString &data);
    void remove(const QString &key);
    Q_INVOKABLE void setName(QmlFilter *filter, const QString &name);
    Q_INVOKABLE QString nextName() const;
    QString keyForRow(int row) const;
    QString keyForFilter(Mlt::Service *service);
    Q_INVOKABLE void reset(QmlFilter *filter, const QString &property, int row);
    QList<TrackingItem> trackingData(const QString &key) const;
    Q_INVOKABLE QList<QRectF> trackingData(int row) const;
    Q_INVOKABLE int keyframeIntervalFrames(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    //    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public slots:
    void removeFromService(Mlt::Service *service);

private:
    enum Roles {
        IdentifierRole = Qt::UserRole,
        TrackingDataRole = Qt::UserRole + 1
    };

    struct Item {
        QString name;
        QString trackingData;
        int intervalFrames;
    };

    QMap<QString, Item> m_data; // key is a UUID
};

#endif // MOTIONTRACKERMODEL_H
