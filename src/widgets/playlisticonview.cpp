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

#include "playlisticonview.h"

#include "models/playlistmodel.h"
#include "settings.h"
#include <Logger.h>

#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QScrollBar>
#include <QSortFilterProxyModel>

PlaylistIconView::PlaylistIconView(QWidget *parent)
    : QAbstractItemView(parent)
    , m_gridSize(170, 100)
    , m_draggingOverPos(QPoint())
    , m_itemsPerRow(3)
{
    verticalScrollBar()->setSingleStep(100);
    verticalScrollBar()->setPageStep(400);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&Settings, SIGNAL(playlistThumbnailsChanged()), SLOT(updateSizes()));
}

QRect PlaylistIconView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();
    int row = index.row() / m_itemsPerRow;
    int col = index.row() % m_itemsPerRow;
    return QRect(col * m_gridSize.width(), row * m_gridSize.height(),
                 m_gridSize.width(), m_gridSize.height());
}

void PlaylistIconView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);
    updateSizes();
}

void PlaylistIconView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    updateSizes();
}

void PlaylistIconView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                   const QVector<int> &roles)
{
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
    updateSizes();
}

void PlaylistIconView::selectionChanged(const QItemSelection &selected,
                                        const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected, deselected);
    viewport()->update();
}

void PlaylistIconView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_UNUSED(index);
    Q_UNUSED(hint);
}

QModelIndex PlaylistIconView::indexAt(const QPoint &point) const
{
    if (!model())
        return QModelIndex();

    if (point.x() / m_gridSize.width() >= m_itemsPerRow)
        return QModelIndex();

    int row = (point.y() + verticalScrollBar()->value()) / m_gridSize.height();
    int col = (point.x() / m_gridSize.width()) % m_itemsPerRow;
    return model()->index(row * m_itemsPerRow + col, 0);
}

QModelIndex PlaylistIconView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(cursorAction);
    Q_UNUSED(modifiers);
    return QModelIndex();
}

int PlaylistIconView::horizontalOffset() const
{
    return 0;
}

int PlaylistIconView::verticalOffset() const
{
    return 0;
}

bool PlaylistIconView::isIndexHidden(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return false;
}

void PlaylistIconView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    QModelIndex topLeft;
    if (!selectionModel()->selectedIndexes().isEmpty()) {
        topLeft = selectionModel()->selectedIndexes().first();
    } else if (!m_isRangeSelect) {
        selectionModel()->select(indexAt(rect.topLeft()), command);
        return;
    }
    if (m_isToggleSelect) {
        command = QItemSelectionModel::Select;
        selectionModel()->select(indexAt(rect.bottomRight()), command);
        return;
    } else if (m_isRangeSelect && topLeft.isValid()) {
        QModelIndex bottomRight = indexAt(rect.bottomRight());
        selectionModel()->select(QItemSelection(topLeft, bottomRight), command);
        return;
    } else if (topLeft.isValid()) {
        selectionModel()->select(indexAt(rect.topLeft()), command);
        return;
    }
    m_pendingSelect = indexAt(rect.topLeft());
}

QRegion PlaylistIconView::visualRegionForSelection(const QItemSelection &selection) const
{
    Q_UNUSED(selection);
    return QRegion();
}

void PlaylistIconView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    viewport()->update();
    QAbstractItemView::currentChanged(current, previous);
}

void PlaylistIconView::paintEvent(QPaintEvent *)
{
    QPainter painter(viewport());
    QPalette pal(palette());
    const auto proxy = tr("P", "The first letter or symbol of \"proxy\"");
    const auto oldFont = painter.font();
    auto boldFont(oldFont);
    boldFont.setBold(true);
    painter.fillRect(rect(), pal.base());

    if (!model())
        return;

    auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    QRect dragIndicator;

    for (int row = 0; row <= proxyModel->rowCount() / m_itemsPerRow; row++) {
        for (int col = 0; col < m_itemsPerRow; col++) {
            const int rowIdx = row * m_itemsPerRow + col;

            QModelIndex idx = proxyModel->index(rowIdx, 0);
            if (!idx.isValid())
                break;

            QRect itemRect(col * m_gridSize.width(), row * m_gridSize.height() - verticalScrollBar()->value(),
                           m_gridSize.width(), m_gridSize.height());

            if (itemRect.bottom() < 0 || itemRect.top() > this->height())
                continue;

            const bool selected = selectedIndexes().contains(idx);
            const QImage thumb = proxyModel->mapToSource(idx).data(Qt::DecorationRole).value<QImage>();

            QRect imageBoundingRect = itemRect;
            imageBoundingRect.setHeight(0.7 * imageBoundingRect.height());
            imageBoundingRect.adjust(0, 10, 0, 0);

            QRect imageRect(QPoint(), thumb.size());
            imageRect.moveCenter(imageBoundingRect.center());

            QRect textRect = itemRect;
            textRect.setTop(imageBoundingRect.bottom());
            textRect.adjust(3, 0, -3, 0);

            QRect buttonRect = itemRect.adjusted(2, 2, -2, -2);

            if (selected) {
                painter.fillRect(buttonRect, pal.highlight());
            } else {
                painter.fillRect(buttonRect, pal.button());

                painter.setPen(pal.color(QPalette::Button).lighter());
                painter.drawLine(buttonRect.topLeft(), buttonRect.topRight());
                painter.drawLine(buttonRect.topLeft(), buttonRect.bottomLeft());

                painter.setPen(pal.color(QPalette::Button).darker());
                painter.drawLine(buttonRect.topRight(), buttonRect.bottomRight());
                painter.drawLine(buttonRect.bottomLeft(), buttonRect.bottomRight());
            }

            painter.drawImage(imageRect, thumb);
            QStringList nameParts = proxyModel->mapToSource(idx).data(Qt::DisplayRole).toString().split('\n');
            if (nameParts.size() > 1) {
                const auto indexPos = imageRect.topLeft() + QPoint(5, 15);
                painter.setFont(boldFont);
                painter.setPen(pal.color(QPalette::Dark).darker());
                painter.drawText(indexPos, proxy);
                painter.setPen(pal.color(QPalette::WindowText));
                painter.drawText(indexPos - QPoint(1, 1), proxy);
                painter.setFont(oldFont);
            }
            painter.setPen(pal.color(QPalette::WindowText));
            painter.drawText(textRect, Qt::AlignCenter,
                             painter.fontMetrics().elidedText(nameParts.first(), Qt::ElideMiddle, textRect.width()));

            if (!m_draggingOverPos.isNull() && itemRect.contains(m_draggingOverPos)) {
                QAbstractItemView::DropIndicatorPosition dropPos =
                    position(m_draggingOverPos, itemRect, idx);
                dragIndicator.setSize(QSize(4, itemRect.height()));
                if (dropPos == QAbstractItemView::AboveItem)
                    dragIndicator.moveTopLeft(itemRect.topLeft() - QPoint(dragIndicator.width() / 2, 0));
                else
                    dragIndicator.moveTopLeft(itemRect.topRight() - QPoint(dragIndicator.width() / 2 - 1, 0));
            }
        }
    }
    if (!dragIndicator.isNull()) {
        painter.fillRect(dragIndicator, pal.buttonText());
    }
}

void PlaylistIconView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_draggingOverPos.isNull() && m_pendingSelect.isValid()) {
            selectionModel()->select(m_pendingSelect, QItemSelectionModel::ClearAndSelect);
            viewport()->update();
        }
        m_pendingSelect = QModelIndex();
    }
    QAbstractItemView::mouseReleaseEvent(event);
}

void PlaylistIconView::dragMoveEvent(QDragMoveEvent *e)
{
    m_draggingOverPos = e->position().toPoint();
    QAbstractItemView::dragMoveEvent(e);
}

void PlaylistIconView::dragLeaveEvent(QDragLeaveEvent *e)
{
    m_draggingOverPos = QPoint();
    QAbstractItemView::dragLeaveEvent(e);
}

void PlaylistIconView::dropEvent(QDropEvent *event)
{
    m_draggingOverPos = QPoint();

    QModelIndex index = indexAt(event->position().toPoint());
    QRect rectAtDropPoint = visualRect(index);

    QAbstractItemView::DropIndicatorPosition dropPos =
        position(event->position().toPoint(), rectAtDropPoint, index);
    if (dropPos == QAbstractItemView::BelowItem)
        index = index.sibling(index.row() + 1, index.column());

    const Qt::DropAction action = event->dropAction();
    int row = (index.row() != -1) ? index.row() : model()->rowCount();
    if (model()->dropMimeData(event->mimeData(), action, row, index.column(), index))
        event->acceptProposedAction();

    stopAutoScroll();
    setState(NoState);
    viewport()->update();
}

void PlaylistIconView::resizeEvent(QResizeEvent *event)
{
    updateSizes();
    QAbstractItemView::resizeEvent(event);
}

void PlaylistIconView::setModel(QAbstractItemModel *model)
{
    QAbstractItemView::setModel(model);
    updateSizes();
}

void PlaylistIconView::keyPressEvent(QKeyEvent *event)
{
    QAbstractItemView::keyPressEvent(event);
    event->ignore();
    m_isToggleSelect = (event->modifiers() & Qt::ControlModifier);
    m_isRangeSelect = (event->modifiers() & Qt::ShiftModifier);
}

void PlaylistIconView::keyReleaseEvent(QKeyEvent *event)
{
    QAbstractItemView::keyPressEvent(event);
    event->ignore();
    resetMultiSelect();
}

QAbstractItemView::DropIndicatorPosition PlaylistIconView::position(const QPoint &pos,
                                                                    const QRect &rect, const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (pos.x() < rect.center().x())
        return QAbstractItemView::AboveItem;
    else
        return QAbstractItemView::BelowItem;
}

void PlaylistIconView::updateSizes()
{
    if (!model() || !model()->rowCount()) {
        verticalScrollBar()->setRange(0, 0);
        return;
    }

    QSize size;
    if (Settings.playlistThumbnails() == "tall")
        size = QSize(PlaylistModel::THUMBNAIL_WIDTH, PlaylistModel::THUMBNAIL_HEIGHT * 2);
    else if (Settings.playlistThumbnails() == "large")
        size = QSize(PlaylistModel::THUMBNAIL_WIDTH * 2, PlaylistModel::THUMBNAIL_HEIGHT * 2);
    else if (Settings.playlistThumbnails() == "wide")
        size = QSize(PlaylistModel::THUMBNAIL_WIDTH * 2, PlaylistModel::THUMBNAIL_HEIGHT);
    else
        size = QSize(PlaylistModel::THUMBNAIL_WIDTH, PlaylistModel::THUMBNAIL_HEIGHT);

    size.setWidth(size.width() + 10);

    m_itemsPerRow = qMax(1, viewport()->width() / size.width());
    m_gridSize = QSize(viewport()->width() / m_itemsPerRow, size.height() + 40);

    if (!verticalScrollBar())
        return;

    verticalScrollBar()->setRange(0,
                                  m_gridSize.height() * model()->rowCount() / m_itemsPerRow - height() + m_gridSize.height());
    viewport()->update();
}

void PlaylistIconView::resetMultiSelect()
{
    m_isToggleSelect = false;
    m_isRangeSelect = false;
}
