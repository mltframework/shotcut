/*
 * Copyright (c) 2016-2024 Meltytech, LLC
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

#ifndef SRC_WIDGETS_PLAYLISTICONVIEW_H
#define SRC_WIDGETS_PLAYLISTICONVIEW_H

#include <QAbstractItemView>

class PlaylistIconView : public QAbstractItemView
{
    Q_OBJECT
public:
    PlaylistIconView(QWidget *parent);
    void resetMultiSelect();
    void setIconRole(int role);

    QRect visualRect(const QModelIndex &index) const Q_DECL_OVERRIDE;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) Q_DECL_OVERRIDE;
    QModelIndex indexAt(const QPoint &point) const Q_DECL_OVERRIDE;
    QModelIndex moveCursor(CursorAction cursorAction,
                           Qt::KeyboardModifiers modifiers) Q_DECL_OVERRIDE;
    int horizontalOffset() const Q_DECL_OVERRIDE;
    int verticalOffset() const Q_DECL_OVERRIDE;
    bool isIndexHidden(const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setSelection(const QRect &rect,
                      QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE;
    QRegion visualRegionForSelection(const QItemSelection &selection) const Q_DECL_OVERRIDE;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) Q_DECL_OVERRIDE;

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *e) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *e) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void setModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void rowsInserted(const QModelIndex &parent, int start, int end) Q_DECL_OVERRIDE;
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) Q_DECL_OVERRIDE;
    void dataChanged(const QModelIndex &topLeft,
                     const QModelIndex &bottomRight,
                     const QVector<int> &roles = QVector<int>()) Q_DECL_OVERRIDE;

public slots:
    void updateSizes();

protected slots:
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) Q_DECL_OVERRIDE;

private:
    int rowWidth() const;
    QAbstractItemView::DropIndicatorPosition position(const QPoint &pos,
                                                      const QRect &rect,
                                                      const QModelIndex &index) const;

    QSize m_gridSize;
    QPoint m_draggingOverPos;
    int m_itemsPerRow;
    bool m_isToggleSelect{false};
    bool m_isRangeSelect{false};
    QModelIndex m_pendingSelect;
    int m_iconRole;
};

#endif
