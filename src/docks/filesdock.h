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

#ifndef FILESDOCK_H
#define FILESDOCK_H

#include <QDockWidget>
#include <QUndoCommand>
#include <QTimer>
#include <QFileSystemModel>
#include <QHash>
#include <QMutex>

namespace Ui {
class FilesDock;
}

class QAbstractItemView;
class QItemSelectionModel;
class QMenu;
class PlaylistIconView;
class FilesModel;
class FilesProxyModel;
class QSortFilterProxyModel;

class FilesDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit FilesDock(QWidget *parent = 0);
    ~FilesDock();

    struct CacheItem {
        int mediaType {-1}; // -1 = unknown
    };

    int getCacheMediaType(const QString &key);
    void setCacheMediaType(const QString &key, int mediaType);

signals:
    void clipOpened(QString);
    void selectionChanged();

public slots:
    void onOpenActionTriggered();
    void changeDirectory(const QString &path);

private slots:
    void viewCustomContextMenuRequested(const QPoint &pos);
    void onMediaTypeClicked();
    void onOpenOtherAdd();
    void onOpenOtherRemove();

    void on_locationsCombo_activated(int index);

    void on_addLocationButton_clicked();

    void on_removeLocationButton_clicked();

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void setupActions();
    void emitDataChanged(const QVector<int> &roles);
    void updateViewMode();
    void onUpdateThumbnailsActionTriggered();
    void onSelectAllActionTriggered();
    void incrementIndex(int step);
    void addOpenWithMenu(QMenu *menu);
    QString firstSelectedFilePath();
    QString firstSelectedMediaType();

    Ui::FilesDock *ui;
    QAbstractItemView *m_view;
    PlaylistIconView *m_iconsView;
    std::unique_ptr<QFileSystemModel> m_dirsModel;
    FilesModel *m_filesModel;
    QItemSelectionModel *m_selectionModel;
    QMenu *m_mainMenu;
    FilesProxyModel *m_filesProxyModel;
    QHash<QString, CacheItem> m_cache;
    QMutex m_cacheMutex;
};

#endif // FILESDOCK_H
