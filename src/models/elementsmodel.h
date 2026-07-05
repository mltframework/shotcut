/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef ELEMENTSMODEL_H
#define ELEMENTSMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QImage>

class ElementsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ThumbnailRole = Qt::UserRole + 1,
        FilePathRole,
        HasAnimationRole,
    };

    explicit ElementsModel(const QDir &dir, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /// Called from the background thumbnail task when the image is ready.
    void updateThumbnail(const QString &filePath, QImage &image, const QModelIndex &persistentIndex);

    /// Reload the model from a new directory.
    void setDir(const QDir &dir);

private:
    QDir m_dir;
    QList<QFileInfo> m_files;
    QHash<int, QImage> m_overrideThumbnails; ///< in-memory overrides for animation frames
};

#endif // ELEMENTSMODEL_H
