/*
 * Copyright (c) 2024 Meltytech, LLC
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

#ifndef SUBTITLESSELECTIONMODEL_H
#define SUBTITLESSELECTIONMODEL_H

#include <QItemSelectionModel>

class SubtitlesSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
    Q_PROPERTY(QModelIndex selectedTrackModelIndex READ selectedTrackModelIndex NOTIFY
                   selectedTrackModelIndexChanged)
    Q_PROPERTY(QVariantList selectedItems READ selectedItems NOTIFY selectedItemsChanged)
public:
    explicit SubtitlesSelectionModel(QAbstractItemModel *model);

    QModelIndex selectedTrackModelIndex();
    Q_INVOKABLE int selectedTrack();
    void setSelectedTrack(int trackIndex);
    QVariantList selectedItems();
    Q_INVOKABLE bool isItemSelected(int itemIndex);
    Q_INVOKABLE void selectItem(int itemIndex);
    Q_INVOKABLE void selectRange(int itemIndex);

signals:
    void selectedTrackModelIndexChanged(QModelIndex trackModelIndex);
    void selectedItemsChanged();

private:
    int m_selectedTrackIndex;
    QVariantList m_selectedItems;
    int m_lastSingleSelection;
};

#endif // SUBTITLESSELECTIONMODEL_H
