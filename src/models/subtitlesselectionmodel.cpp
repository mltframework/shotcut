/*
 * Copyright (c) 2024-2025 Meltytech, LLC
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

#include "subtitlesselectionmodel.h"
#include "Logger.h"
#include "models/subtitlesmodel.h"

SubtitlesSelectionModel::SubtitlesSelectionModel(QAbstractItemModel *model)
    : QItemSelectionModel(model)
    , m_selectedTrackIndex(-1)
{
    connect(this,
            &QItemSelectionModel::selectionChanged,
            this,
            [&](const QItemSelection &selected, const QItemSelection &deselected) {
                QVariantList result;
                foreach (auto modelIndex, selectedRows()) {
                    result << modelIndex.row();
                }
                m_selectedItems = result;
                if (selectedRows().size() == 1) {
                    m_lastSingleSelection = selectedRows()[0].row();
                }
                emit selectedItemsChanged();
            });
}

QModelIndex SubtitlesSelectionModel::selectedTrackModelIndex()
{
    SubtitlesModel *smodel = dynamic_cast<SubtitlesModel *>(model());
    return smodel->trackModelIndex(m_selectedTrackIndex);
}

int SubtitlesSelectionModel::selectedTrack()
{
    return m_selectedTrackIndex;
}

void SubtitlesSelectionModel::setSelectedTrack(int trackIndex)
{
    if (m_selectedTrackIndex != trackIndex) {
        m_selectedTrackIndex = trackIndex;
        clearSelection();
        m_lastSingleSelection = -1;
        emit selectedTrackModelIndexChanged(selectedTrackModelIndex());
    }
}

QVariantList SubtitlesSelectionModel::selectedItems()
{
    return m_selectedItems;
}

bool SubtitlesSelectionModel::isItemSelected(int itemIndex)
{
    if (m_selectedItems.contains(QVariant(itemIndex))) {
        return true;
    }
    return false;
}

void SubtitlesSelectionModel::selectItem(int itemIndex)
{
    QModelIndexList selected = selectedIndexes();
    if (selected.size() == 1 && selected[0].row() == itemIndex) {
        // This item is already selected
        return;
    }
    SubtitlesModel *smodel = dynamic_cast<SubtitlesModel *>(model());
    QModelIndex itemModelIndex = smodel->itemModelIndex(m_selectedTrackIndex, itemIndex);
    if (itemModelIndex.isValid()) {
        select(itemModelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        setCurrentIndex(itemModelIndex, QItemSelectionModel::NoUpdate);
    }
}

void SubtitlesSelectionModel::selectRange(int itemIndex)
{
    SubtitlesModel *smodel = dynamic_cast<SubtitlesModel *>(model());
    QModelIndex itemModelIndex = smodel->itemModelIndex(m_selectedTrackIndex, itemIndex);
    if (!itemModelIndex.isValid()) {
        return;
    }
    LOG_DEBUG() << m_lastSingleSelection << itemIndex;
    if (m_lastSingleSelection == -1) {
        select(itemModelIndex,
               QItemSelectionModel::ClearAndSelect | QItemSelectionModel::SelectCurrent
                   | QItemSelectionModel::Rows);
    } else {
        QModelIndex firstItemModelIndex = smodel->itemModelIndex(m_selectedTrackIndex,
                                                                 m_lastSingleSelection);
        QItemSelection newSelection(firstItemModelIndex, itemModelIndex);
        select(newSelection,
               QItemSelectionModel::ClearAndSelect | QItemSelectionModel::SelectCurrent
                   | QItemSelectionModel::Rows);
    }
    setCurrentIndex(itemModelIndex, QItemSelectionModel::NoUpdate);
}
