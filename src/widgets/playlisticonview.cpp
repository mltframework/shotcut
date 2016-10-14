#include "playlisticonview.h"

#include "models/playlistmodel.h"
#include "settings.h"

#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QScrollBar>

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
    int row = index.row() / m_itemsPerRow;
    int col = index.row() % m_itemsPerRow;
    return QRect(col * m_gridSize.width(), row * m_gridSize.height(),
            m_gridSize.width(), m_gridSize.height());
}

void PlaylistIconView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsInserted(parent, start, end);
    updateSizes();
    viewport()->update();
}

void PlaylistIconView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    updateSizes();
    viewport()->update();
}

void PlaylistIconView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
    updateSizes();
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
    QModelIndex i = indexAt(rect.bottomRight());
    selectionModel()->select(i, command);
    viewport()->update();
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

void PlaylistIconView::paintEvent(QPaintEvent*)
{
    QPainter painter(viewport());
    QPalette pal(palette());
    painter.fillRect(rect(), pal.base());

    if (!model())
        return;

    QAbstractItemModel * m = model();
    QRect dragIndicator;

    for (int row = 0; row <= m->rowCount() / m_itemsPerRow; row++) {
        for (int col = 0; col < m_itemsPerRow; col++) {
            const int rowIdx = row * m_itemsPerRow + col;

            QModelIndex idx = m->index(rowIdx, 0);
            if (!idx.isValid())
                break;

            QRect itemRect(col * m_gridSize.width(), row * m_gridSize.height() - verticalScrollBar()->value(),
                    m_gridSize.width(), m_gridSize.height());

            if (itemRect.bottom() < 0 || itemRect.top() > this->height())
                continue;

            const bool selected = selectedIndexes().contains(idx);
            const QImage thumb = idx.data(Qt::DecorationRole).value<QImage>();

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
            painter.setPen(pal.color(QPalette::WindowText));
            painter.drawText(textRect, Qt::AlignCenter,
                    painter.fontMetrics().elidedText(idx.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, textRect.width()));

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
    QAbstractItemView::mouseReleaseEvent(event);
}

void PlaylistIconView::dragMoveEvent(QDragMoveEvent *e)
{
    m_draggingOverPos = e->pos();
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

    QModelIndex index = indexAt(event->pos());
    QRect rectAtDropPoint = visualRect(index);

    QAbstractItemView::DropIndicatorPosition dropPos =
        position(event->pos(), rectAtDropPoint, index);
    if (dropPos == QAbstractItemView::BelowItem)
        index = index.sibling(index.row() + 1, index.column());

    if (true) {
    //if (d->dropOn(event, &row, &col, &index)) {
        const Qt::DropAction action = event->dropAction();
        if (model()->dropMimeData(event->mimeData(), action, index.row(), index.column(), index)) {
            event->acceptProposedAction();
        }
    }
    stopAutoScroll();
    setState(NoState);
    viewport()->update();
}

void PlaylistIconView::resizeEvent(QResizeEvent *event)
{
    updateSizes();
    QAbstractItemView::resizeEvent(event);
}

void PlaylistIconView::setModel(QAbstractItemModel* model)
{
    QAbstractItemView::setModel(model);
    updateSizes();
    viewport()->update();
}

QAbstractItemView::DropIndicatorPosition PlaylistIconView::position(const QPoint &pos, const QRect &rect, const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (pos.x() < rect.center().x())
        return QAbstractItemView::AboveItem;
    else
        return QAbstractItemView::BelowItem;
}

void PlaylistIconView::updateSizes()
{
    if (!model() || !model()->rowCount())
    {
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

    verticalScrollBar()->setRange(0, m_gridSize.height() * model()->rowCount() / m_itemsPerRow - height() + m_gridSize.height());
    viewport()->update();
}
