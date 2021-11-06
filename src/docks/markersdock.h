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

class EditMarkerWidget;
class MarkerTreeView;
class MarkersModel;
class QPushButton;
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
    void addRequested();

private slots:
    void onSelectionChanged(QModelIndex& index);
    void onAddRequested();
    void onRemoveRequested();
    void onClearSelectionRequested();
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void onValuesChanged();

private:
    void enableButtons(bool enable);

    MarkersModel* m_model;
    QSortFilterProxyModel* m_proxyModel;
    MarkerTreeView* m_treeView;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_clearButton;
    EditMarkerWidget* m_editMarkerWidget;
    bool m_blockSelectionEvent;
};

#endif // MARKERSDOCK_H
