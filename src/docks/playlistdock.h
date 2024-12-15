/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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

#ifndef PLAYLISTDOCK_H
#define PLAYLISTDOCK_H

#include <QDockWidget>
#include <QUndoCommand>
#include <QTimer>
#include <QTreeWidget>
#include "models/playlistmodel.h"

namespace Ui {
class PlaylistDock;
}

class QAbstractItemView;
class QItemSelectionModel;
class QMenu;
class PlaylistIconView;
class PlaylistProxyModel;

class BinTree : public QTreeWidget
{
    Q_OBJECT

public:
    explicit BinTree(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {}

signals:
    void copied(QString);
    void moved(QList<int>, QPointF);

protected:
    void dropEvent(QDropEvent *event);
};

class PlaylistDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistDock(QWidget *parent = 0);
    ~PlaylistDock();
    PlaylistModel *model()
    {
        return &m_model;
    }
    int position();
    void replaceClipsWithHash(const QString &hash, Mlt::Producer &producer);
    void getSelectionRange(int *start, int *end);

signals:
    void clipOpened(Mlt::Producer *producer, bool play = false);
    void itemActivated(int start);
    void showStatusMessage(QString);
    void addAllTimeline(Mlt::Playlist *, bool skipProxy = false, bool emptyTrack = false);
    void producerOpened();
    void selectionChanged();
    void enableUpdate(bool);

public slots:
    void onOpenActionTriggered();
    void onAppendCutActionTriggered();
    void onProducerOpened();
    void onInChanged();
    void onOutChanged();
    void onProducerChanged(Mlt::Producer *producer);
    void onProducerModified();
    void onPlayerDragStarted();
    void onPlaylistModified();
    void onPlaylistCreated();
    void onPlaylistLoaded();
    void onPlaylistCleared();

private slots:

    void viewCustomContextMenuRequested(const QPoint &pos);
    void viewDoubleClicked(const QModelIndex &index);
    void onDropped(const QMimeData *data, int row);
    void onMoveClip(int from, int to);
    void onMovedToEnd();
    void onInTimerFired();
    void onOutTimerFired();
    void onMediaTypeClicked();
    void on_treeWidget_itemSelectionChanged();

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void setupActions();
    void resetPlaylistIndex();
    void emitDataChanged(const QVector<int> &roles);
    void setPlaylistIndex(Mlt::Producer *producer, int row);
    void updateViewMode();
    void onAddFilesActionTriggered();
    void onUpdateThumbnailsActionTriggered();
    void onAddToTimelineActionTriggered();
    void onAddToSlideshowActionTriggered();
    void onSetFileDateActionTriggered();
    void onRemoveAllActionTriggered();
    void onGotoActionTriggered();
    void onCopyActionTriggered();
    void onSelectAllActionTriggered();
    void onInsertCutActionTriggered();
    void onUpdateActionTriggered();
    void onRemoveActionTriggered();
    void incrementIndex();
    void decrementIndex();
    void setIndex(int row);
    void moveClipUp();
    void moveClipDown();
    void addFiles(int row, const QList<QUrl> &urls);
    void loadBins();
    void sortBins();
    void assignToBin(Mlt::Properties &properties, QString bin = QString());

    Ui::PlaylistDock *ui;
    QAbstractItemView *m_view;
    PlaylistIconView *m_iconsView;
    PlaylistModel m_model;
    QItemSelectionModel *m_selectionModel;
    int m_defaultRowHeight;
    QTimer m_inChangedTimer;
    QTimer m_outChangedTimer;
    QMenu *m_mainMenu;
    bool m_blockResizeColumnsToContents;
    PlaylistProxyModel *m_proxyModel;
};

#endif // PLAYLISTDOCK_H
