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
class QLineEdit;
class QSortFilterProxyModel;
class QToolButton;

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

public slots:
    void onMarkerSelectionRequest(int markerIndex);

private slots:
    void onSelectionChanged(QModelIndex& index);
    void onRowClicked(const QModelIndex& index);
    void onAddRequested();
    void onRemoveRequested();
    void onClearSelectionRequested();
    void onRemoveAllRequested();
    void onSearchChanged();
    void onColorColumnToggled(bool checked);
    void onTextColumnToggled(bool checked);
    void onStartColumnToggled(bool checked);
    void onEndColumnToggled(bool checked);
    void onDurationColumnToggled(bool checked);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void onValuesChanged();
    void onModelReset();
    void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

private:
    void enableButtons(bool enable);

    MarkersModel* m_model;
    QSortFilterProxyModel* m_proxyModel;
    MarkerTreeView* m_treeView;
    QToolButton* m_addButton;
    QToolButton* m_removeButton;
    QToolButton* m_clearButton;
    QToolButton* m_moreButton;
    QLineEdit* m_searchField;
    QToolButton* m_clearSearchButton;
    EditMarkerWidget* m_editMarkerWidget;
};

#endif // MARKERSDOCK_H
