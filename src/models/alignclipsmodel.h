/*
 * Copyright (c) 2022 Meltytech, LLC
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

#ifndef ALIGNCLIPSMODEL_H
#define ALIGNCLIPSMODEL_H

#include <QAbstractItemModel>

#include <limits>

class AlignClipsModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    enum Columns {
        COLUMN_ERROR = 0,
        COLUMN_NAME,
        COLUMN_OFFSET,
//        COLUMN_DRIFT, // Future use
        COLUMN_COUNT,
    };
    static const int INVALID_OFFSET = std::numeric_limits<int>::max();

    explicit AlignClipsModel(QObject* parent = 0);
    virtual ~AlignClipsModel();
    void clear();
    void addClip(const QString& name, int offset, int drift, const QString& error);
    void updateProgress(int row, int percent);
    int getProgress(int row) const;
    void updateOffsetAndDrift(int row, int offset, double drift, const QString& error);
    int getOffset(int row);
    double getDrift(int row);

protected:
    // Implement QAbstractItemModel
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;

private:
    typedef struct {
        QString name;
        int offset;
        double drift;
        QString error;
        int progress;
    } ClipAlignment;
    QList<ClipAlignment> m_clips;
};

#endif // ALIGNCLIPSMODEL_H
