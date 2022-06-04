/*
 * Copyright (c) 2021 Meltytech, LLC
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

#ifndef MARKERSMODEL_H
#define MARKERSMODEL_H

#include <MltProducer.h>

#include <QAbstractItemModel>
#include <QColor>
#include <QString>

namespace Markers {

class Marker
{
public:
    QString text;
    int start {-1};
    int end {-1};
    QColor color;
};

}

class MarkersModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList recentColors READ recentColors NOTIFY recentColorsChanged)

public:

    enum Roles {
        TextRole = Qt::UserRole + 1,
        StartRole,
        EndRole,
        ColorRole,
    };

    explicit MarkersModel(QObject *parent = 0);
    virtual ~MarkersModel();

    void load(Mlt::Producer *producer);
    Markers::Marker getMarker(int markerIndex);
    int uniqueKey() const;
    int markerIndexForPosition(int position);
    int markerIndexForRange(int start, int end);
    Q_INVOKABLE int nextMarkerPosition(int position);
    Q_INVOKABLE int prevMarkerPosition(int position);
    QModelIndex modelIndexForRow(int row);
    QMap<int, QString> ranges();
    QStringList recentColors();
    QList<Markers::Marker> getMarkers() const;
    QList<QColor> allColors() const;

    // These should only be called by the marker commands
    void doRemove(int markerIndex);
    void doInsert(int markerIndex, const Markers::Marker &marker);
    void doAppend(const Markers::Marker &marker);
    void doUpdate(int markerIndex,  const Markers::Marker &marker);
    void doClear();
    void doReplace(QList<Markers::Marker> &markers);
    void doShift(int shiftPosition, int shiftAmount);

signals:
    void rangesChanged();
    void modified();
    void recentColorsChanged();

public slots:
    void remove(int markerIndex);
    void append(const Markers::Marker &marker);
    void update(int markerIndex, const Markers::Marker &marker);
    void move(int markerIndex, int start, int end);
    void setColor(int markerIndex, const QColor &color);
    void clear();

protected:
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QHash<int, QByteArray> roleNames() const;

private:
    int markerCount() const;
    int keyIndex(int key) const;
    Mlt::Properties *getMarkerProperties(int markerIndex);
    void updateRecentColors(const QColor &color);

    Mlt::Producer *m_producer;
    QList<int> m_keys;
    QMap<QRgb, QString> m_recentColors;
};

#endif // MARKERSMODEL_H
