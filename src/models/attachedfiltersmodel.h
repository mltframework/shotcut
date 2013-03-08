/*
 * Copyright (c) 2013 Meltytech, LLC
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

#ifndef ATTACHEDFILTERSMODEL_H
#define ATTACHEDFILTERSMODEL_H

#include <QAbstractListModel>
#include <mlt++/MltFilter.h>

class AttachedFiltersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AttachedFiltersModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
 
signals:
    void changed();

public slots:
    void add(const QString& name);
    void remove(int row);
    void reset();

private:
    int m_rows;

    void calculateRows();
    Mlt::Filter* nthFilter(int n) const;
};

#endif // ATTACHEDFILTERSMODEL_H
