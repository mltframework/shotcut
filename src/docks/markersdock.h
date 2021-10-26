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

#ifndef MARKERSDOCK_H
#define MARKERSDOCK_H

#include <QDockWidget>
#include <QItemSelectionModel>

class MarkerTreeView;
class MarkersModel;
class QSortFilterProxyModel;

class MarkersDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MarkersDock(QWidget *parent = 0);
    ~MarkersDock();
    void setModel(MarkersModel* model);

signals:
    void seekRequested(int pos);

private slots:
    void onSelectionChanged(QModelIndex& index);

private:
    MarkersModel* m_model;
    QSortFilterProxyModel* m_proxyModel;
    MarkerTreeView* m_treeView;
};

#endif // MARKERSDOCK_H
