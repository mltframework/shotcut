/*
 * Copyright (c) 2012-2021 Meltytech, LLC
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

#include "playlistdock.h"
#include "ui_playlistdock.h"
#include "dialogs/durationdialog.h"
#include "dialogs/filedatedialog.h"
#include "dialogs/longuitask.h"
#include "dialogs/slideshowgeneratordialog.h"
#include "mainwindow.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "widgets/playlisticonview.h"
#include "widgets/playlisttable.h"
#include "widgets/playlistlistview.h"
#include "util.h"
#include "commands/playlistcommands.h"
#include "proxymanager.h"
#include "qmltypes/qmlapplication.h"
#include <Logger.h>

#include <QMenu>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDir>
#include <QGuiApplication>
#include <QClipboard>

static const int kInOutChangedTimeoutMs = 100;

class TiledItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TiledItemDelegate(QAbstractItemView *view, QWidget *parent = nullptr)
        : QStyledItemDelegate(parent),
          m_view(view)
    {
        connect(&Settings, SIGNAL(playlistThumbnailsChanged()),
                SLOT(emitSizeHintChanged()));
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const QImage thumb = index.data(Qt::DecorationRole).value<QImage>();
        const QString setting = Settings.playlistThumbnails();
        const int lineHeight = painter->fontMetrics().height();
        const bool roomEnoughForAllDetails = lineHeight * 5 < thumb.height();
        const QFont oldFont = painter->font();
        QFont boldFont(oldFont);
        boldFont.setBold(true);

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight().color());
        } else {
            if (option.features & QStyleOptionViewItem::Alternate)
                painter->fillRect(option.rect, option.palette.alternateBase());
        }

        QRect thumbRect(QPoint(), thumb.size());
        thumbRect.moveCenter(option.rect.center());
        thumbRect.moveLeft(0);
        painter->drawImage(thumbRect, thumb);

        const QPoint indexPos = option.rect.topLeft() + QPoint(5, 20);
        const QString indexStr = "#" + index.data(PlaylistModel::FIELD_INDEX).toString();
        painter->setFont(boldFont);
        painter->setPen(option.palette.color(QPalette::Dark).darker());
        painter->drawText(indexPos, indexStr);
        painter->setPen(option.palette.color(QPalette::WindowText));
        painter->drawText(indexPos - QPoint(1, 1), indexStr);
        painter->setFont(oldFont);

        QRect centeredTextRect = option.rect;
        centeredTextRect.setHeight(lineHeight * (roomEnoughForAllDetails ? 5 : 3));
        centeredTextRect.moveCenter(option.rect.center());

        QRect textRect = centeredTextRect;
        textRect.setLeft(thumb.width() + 10);

        QPoint textPoint = textRect.topLeft();
        textPoint.setY(textPoint.y() + lineHeight);
        painter->setFont(boldFont);
        QStringList nameParts = index.data(Qt::DisplayRole).toString().split('\n');
        painter->drawText(textPoint,
                          painter->fontMetrics().elidedText(nameParts.first(), Qt::ElideMiddle, textRect.width()));
        painter->setFont(oldFont);
        if (nameParts.size() > 1) {
            textPoint.setY(textPoint.y() + lineHeight);
            painter->drawText(textPoint, nameParts.last());
        }

        textPoint.setY(textPoint.y() + lineHeight);
        painter->drawText(textPoint, tr("Duration: %1").arg(index.data(
                                                                PlaylistModel::FIELD_DURATION).toString()));
        if (roomEnoughForAllDetails) {
            textPoint.setY(textPoint.y() + lineHeight);
            painter->drawText(textPoint, tr("In: %1").arg(index.data(PlaylistModel::FIELD_IN).toString()));
            textPoint.setY(textPoint.y() + lineHeight);
            painter->drawText(textPoint, tr("Start: %1").arg(index.data(
                                                                 PlaylistModel::FIELD_START).toString()));
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        const bool doubleHeight = Settings.playlistThumbnails() == "tall"
                                  || Settings.playlistThumbnails() == "large";
        const int spacing = 10;
        return QSize(m_view->viewport()->width(),
                     PlaylistModel::THUMBNAIL_HEIGHT * (doubleHeight ? 2 : 1) + spacing);
    }

private slots:
    void emitSizeHintChanged()
    {
        emit sizeHintChanged(QModelIndex());
    }

private:
    QAbstractItemView *m_view;

};

PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());
    ui->actionPlayAfterOpen->setChecked(Settings.playlistAutoplay());

    ui->stackedWidget->setCurrentIndex(0);

    m_iconsView = new PlaylistIconView(this);
    ui->listView->parentWidget()->layout()->addWidget(m_iconsView);
    m_iconsView->setSelectionMode(QAbstractItemView::SingleSelection);

    QList<QAbstractItemView *> views;
    views << ui->tableView;
    views << ui->listView;
    views << m_iconsView;
    foreach (QAbstractItemView *view, views) {
        view->setDragDropMode(QAbstractItemView::DragDrop);
        view->setDropIndicatorShown(true);
        view->setDragDropOverwriteMode(false);
        view->setAcceptDrops(true);
        view->setDefaultDropAction(Qt::MoveAction);
        view->setAlternatingRowColors(true);
        connect(view, SIGNAL(customContextMenuRequested(QPoint)),
                SLOT(viewCustomContextMenuRequested(QPoint)));
        connect(view, SIGNAL(doubleClicked(QModelIndex)), SLOT(viewDoubleClicked(QModelIndex)));
    }

    connect(ui->actionDetailed, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));
    connect(ui->actionIcons, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));
    connect(ui->actionTiled, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));
    connect(ui->tableView, SIGNAL(movedToEnd()), SLOT(onMovedToEnd()));
    connect(ui->listView, SIGNAL(movedToEnd()), SLOT(onMovedToEnd()));
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));
    connect(&m_model, SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(&m_model, SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(&m_model, SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(&m_model, SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(&m_model, SIGNAL(dropped(const QMimeData *, int)), this, SLOT(onDropped(const QMimeData *,
                                                                                    int)));
    connect(&m_model, SIGNAL(moveClip(int, int)), SLOT(onMoveClip(int, int)));
    connect(&m_model, SIGNAL(closed()), SLOT(onPlaylistClosed()));

    m_defaultRowHeight = ui->tableView->verticalHeader()->defaultSectionSize();
    QString thumbs = Settings.playlistThumbnails();
    if (thumbs == "wide") {
        ui->actionLeftAndRight->setChecked(true);
        on_actionLeftAndRight_triggered(true);
    } else if (thumbs == "tall") {
        ui->actionTopAndBottom->setChecked(true);
        on_actionTopAndBottom_triggered(true);
    } else if (thumbs == "small") {
        ui->actionInOnlySmall->setChecked(true);
        on_actionInOnlySmall_triggered(true);
    } else if (thumbs == "large") {
        ui->actionInOnlyLarge->setChecked(true);
        on_actionInOnlyLarge_triggered(true);
    } else {
        ui->actionThumbnailsHidden->setChecked(true);
        on_actionThumbnailsHidden_triggered(true);
    }

    if (Settings.viewMode() == kTiledMode)
        ui->actionTiled->setChecked(true);
    else if (Settings.viewMode() == kIconsMode)
        ui->actionIcons->setChecked(true);
    else
        ui->actionDetailed->setChecked(true);

    updateViewModeFromActions();

    m_inChangedTimer.setInterval(kInOutChangedTimeoutMs);
    m_inChangedTimer.setSingleShot(true);
    m_outChangedTimer.setInterval(kInOutChangedTimeoutMs);
    m_outChangedTimer.setSingleShot(true);
    connect(&m_inChangedTimer, SIGNAL(timeout()), this, SLOT(onInTimerFired()));
    connect(&m_outChangedTimer, SIGNAL(timeout()), this, SLOT(onOutTimerFired()));

    LOG_DEBUG() << "end";
}

PlaylistDock::~PlaylistDock()
{
    delete ui;
}

int PlaylistDock::position()
{
    int result = -1;
    QModelIndex index = m_view->currentIndex();
    if (index.isValid() && m_model.playlist()) {
        Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
        if (i) result = i->start;
        delete i;
    }
    return result;
}

void PlaylistDock::replaceClipsWithHash(const QString &hash, Mlt::Producer &producer)
{
    QList<Mlt::Producer> producers;
    for (int i = 0; i < m_model.rowCount(); ++i) {
        QScopedPointer<Mlt::Producer> clip(m_model.playlist()->get_clip(i));
        if (Util::getHash(clip->parent()) == hash) {
            clip->set(kPlaylistIndexProperty, i + 1);
            producers << *clip;
        }
    }
    auto n = producers.size();
    if (n > 1) {
        MAIN.undoStack()->beginMacro(tr("Replace %n playlist items", nullptr, n));
    }
    for (auto &clip : producers) {
        Util::applyCustomProperties(producer, clip.parent(), clip.get_in(), clip.get_out());
        MAIN.undoStack()->push(
            new Playlist::ReplaceCommand(m_model, MLT.XML(&producer),
                                         clip.get_int(kPlaylistIndexProperty) - 1));
    }
    if (n > 1) {
        MAIN.undoStack()->endMacro();
    }
}

void PlaylistDock::incrementIndex()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.incrementIndex(index);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::decrementIndex()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.decrementIndex(index);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::setIndex(int row)
{
    QModelIndex index = m_model.createIndex(row, 0);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::moveClipUp()
{
    int row = m_view->currentIndex().row();
    if (row > 0) {
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row - 1));
        resetPlaylistIndex();
    }
}

void PlaylistDock::moveClipDown()
{
    int row = m_view->currentIndex().row();
    if (row + 1 < m_model.rowCount()) {
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row + 1));
        resetPlaylistIndex();
    }
}

void PlaylistDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    QModelIndex index = m_view->currentIndex();
    if (index.isValid() && m_model.playlist()) {
        menu.addAction(ui->actionOpen);
        if (!MAIN.isMultitrackValid())
            menu.addAction(ui->actionGoto);
        menu.addAction(ui->actionRemove);
        menu.addAction(ui->actionCopy);
        if (MLT.isClip())
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionUpdate);
        menu.addAction(ui->actionUpdateThumbnails);
        menu.addAction(ui->actionSetFileDate);
        menu.addSeparator();
    }
    menu.addAction(ui->actionRemoveAll);
    menu.addAction(ui->actionSelectAll);
    menu.addAction(ui->actionSelectNone);
    menu.addAction(ui->actionAddToTimeline);
    menu.addAction(ui->actionAddToSlideshow);
    menu.addSeparator();

    QMenu *sortByMenu = menu.addMenu(tr("Sort"));
    QActionGroup sortGroup(this);
    sortGroup.addAction(ui->actionSortByName);
    sortGroup.addAction(ui->actionSortByDate);
    sortByMenu->addActions(sortGroup.actions());

    menu.addSeparator();
    QMenu *viewModeMenu = menu.addMenu(tr("View mode"));
    QActionGroup modeGroup(this);
    modeGroup.addAction(ui->actionDetailed);
    modeGroup.addAction(ui->actionTiled);
    modeGroup.addAction(ui->actionIcons);
    viewModeMenu->addActions(modeGroup.actions());

    QMenu *subMenu = menu.addMenu(tr("Thumbnails"));
    QActionGroup group(this);
    group.addAction(ui->actionThumbnailsHidden);
    group.addAction(ui->actionInOnlyLarge);
    group.addAction(ui->actionInOnlySmall);
    group.addAction(ui->actionLeftAndRight);
    group.addAction(ui->actionTopAndBottom);
    subMenu->addActions(group.actions());

    menu.addAction(ui->actionPlayAfterOpen);

    menu.exec(mapToGlobal(pos));
}

void PlaylistDock::on_actionInsertCut_triggered()
{
    if (MLT.isClip() || MLT.savedProducer()) {
        QMimeData mimeData;
        mimeData.setData(Mlt::XmlMimeType, MLT.XML(
                             MLT.isClip() ? nullptr : MLT.savedProducer()).toUtf8());
        onDropped(&mimeData, m_view->currentIndex().row());
    }
}

void PlaylistDock::on_actionAppendCut_triggered()
{
    Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
    if (producer.is_valid() && !MAIN.isSourceClipMyProject()) {
        if (MLT.isSeekableClip()
                || (MLT.savedProducer() && MLT.isSeekable(MLT.savedProducer()))) {
            ProxyManager::generateIfNotExists(producer);
            MAIN.undoStack()->push(
                new Playlist::AppendCommand(m_model, MLT.XML(&producer)));
            setPlaylistIndex(&producer, m_model.playlist()->count() - 1);
            setUpdateButtonEnabled(true);
        } else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                producer.set_in_and_out(0, dialog.duration() - 1);
                if (producer.get("mlt_service") && !strcmp(producer.get("mlt_service"), "avformat"))
                    producer.set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML()));
                setPlaylistIndex(&producer, m_model.playlist()->count() - 1);
                setUpdateButtonEnabled(true);
            }
        }
    }
}

void PlaylistDock::on_actionInsertBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted) {
        QModelIndex index = m_view->currentIndex();
        if (index.isValid())
            m_model.insertBlank(dialog.duration(), index.row());
        else
            m_model.appendBlank(dialog.duration());
    }
}

void PlaylistDock::on_actionAppendBlank_triggered()
{
    DurationDialog dialog(this);
    dialog.setDuration(MLT.profile().fps() * 5);
    if (dialog.exec() == QDialog::Accepted)
        m_model.appendBlank(dialog.duration());
}

void PlaylistDock::on_actionUpdate_triggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist() || MAIN.isSourceClipMyProject()) return;
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index.row()));
    Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
    if (!info || !producer.is_valid()) return;
    if (producer.type() != mlt_service_playlist_type) {
        if (MLT.isSeekableClip()
                || (MLT.savedProducer() && MLT.isSeekable(MLT.savedProducer()))) {
            ProxyManager::generateIfNotExists(producer);
            MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(&producer), index.row()));
            setPlaylistIndex(&producer, index.row());
            setUpdateButtonEnabled(true);
        } else {
            // change the duration
            DurationDialog dialog(this);
            dialog.setDuration(info->frame_count);
            if (dialog.exec() == QDialog::Accepted) {
                producer.set_in_and_out(0, dialog.duration() - 1);
                if (producer.get("mlt_service") && !strcmp(producer.get("mlt_service"), "avformat"))
                    producer.set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(), index.row()));
                setPlaylistIndex(&producer, index.row());
                setUpdateButtonEnabled(true);
            }
        }
    } else {
        emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::on_removeButton_clicked()
{
    if (!m_model.playlist() || !m_view->selectionModel()) return;
    QList<int> rowsRemoved;
    int n = m_view->selectionModel()->selectedIndexes().size();
    if (n > 1)
        MAIN.undoStack()->beginMacro(tr("Remove %n playlist items", nullptr, n));
    foreach (auto index, m_view->selectionModel()->selectedIndexes()) {
        int row = index.row();
        if (!rowsRemoved.contains(row)) {
            int adjustment = 0;
            foreach (int i, rowsRemoved) {
                if (row > i)
                    --adjustment;
            }
            row += adjustment;
            rowsRemoved << index.row();
            if (m_model.playlist()->clip_length(row) > 0)
                MAIN.undoStack()->push(new Playlist::RemoveCommand(m_model, row));
        }
    }
    if (n > 1)
        MAIN.undoStack()->endMacro();
    if (rowsRemoved.contains(MLT.producer()->get_int(kPlaylistIndexProperty))) {
        // Remove the playlist index property on the producer.
        resetPlaylistIndex();
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::on_actionSetFileDate_triggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    int count = m_model.playlist()->count();
    if (count == 0) return;
    int i = index.row() >= count ? count - 1 : index.row();
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(i));
    if (info && info->producer && info->producer->is_valid()) {
        QString title = info->producer->get("mlt_service");
        QString resource = ProxyManager::resource(*info->producer);
        QFileInfo fileInfo(resource);
        if (fileInfo.exists()) {
            title = fileInfo.baseName();
        }
        FileDateDialog dialog(resource, info->producer, this);
        dialog.setWindowModality(QmlApplication::dialogModality());
        dialog.exec();
    }
}

void PlaylistDock::setUpdateButtonEnabled(bool modified)
{
    ui->updateButton->setEnabled(modified);
}

void PlaylistDock::onProducerOpened()
{
    if (!MLT.isMultitrack())
        ui->addButton->setEnabled(true);
    if (MLT.producer() && MLT.producer()->is_valid()) {
        auto row = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
        if (row < 0 && m_model.rowCount() > 0) {
            resetPlaylistIndex();
            emit m_model.dataChanged(m_model.createIndex(0, PlaylistModel::COLUMN_THUMBNAIL),
                                     m_model.createIndex(m_model.playlist()->count() - 1, PlaylistModel::COLUMN_THUMBNAIL),
                                     QVector<int>() << PlaylistModel::COLUMN_THUMBNAIL);
        }
    }
}

void PlaylistDock::onInChanged()
{
    // Order of in/out timers can be important, resolve the other first
    if (m_outChangedTimer.isActive()) {
        m_outChangedTimer.stop();
        onOutTimerFired();
    }
    m_inChangedTimer.start();
}

void PlaylistDock::onOutChanged()
{
    // Order of in/out timers can be important, resolve the other first
    if (m_inChangedTimer.isActive()) {
        m_inChangedTimer.stop();
        onInTimerFired();
    }
    m_outChangedTimer.start();
}

void PlaylistDock::on_actionOpen_triggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        Mlt::Producer *p = new Mlt::Producer(i->producer);
        p->set_in_and_out(i->frame_in, i->frame_out);
        setPlaylistIndex(p, index.row());
        emit clipOpened(p, Settings.playlistAutoplay());
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::viewCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = m_view->currentIndex();
    if (index.isValid() && m_model.playlist()) {
        QMenu menu(this);
        menu.addAction(ui->actionOpen);
        if (!MAIN.isMultitrackValid())
            menu.addAction(ui->actionGoto);
        menu.addAction(ui->actionRemove);
        menu.addAction(ui->actionCopy);
        if (MLT.isClip())
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionUpdate);
        menu.addAction(ui->actionUpdateThumbnails);
        menu.addAction(ui->actionSetFileDate);
        menu.exec(mapToGlobal(pos));
    }
}


void PlaylistDock::viewDoubleClicked(const QModelIndex &index)
{
    if (!m_model.playlist()) return;
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        if (qApp->keyboardModifiers() == Qt::ShiftModifier) {
            emit itemActivated(i->start);
        } else {
            Mlt::Producer *p = new Mlt::Producer(i->producer);
            p->set_in_and_out(i->frame_in, i->frame_out);
            setPlaylistIndex(p, index.row());
            emit clipOpened(p, Settings.playlistAutoplay());
        }
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::on_actionGoto_triggered()
{
    QModelIndex index = m_view->currentIndex();
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::on_actionRemoveAll_triggered()
{
    resetPlaylistIndex();
    MAIN.undoStack()->push(new Playlist::ClearCommand(m_model));
}

void PlaylistDock::on_actionSortByName_triggered()
{
    resetPlaylistIndex();
    MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_RESOURCE,
                                                     Qt::AscendingOrder));
}

void PlaylistDock::on_actionSortByDate_triggered()
{
    resetPlaylistIndex();
    MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_DATE,
                                                     Qt::AscendingOrder));
}

void PlaylistDock::onPlaylistCreated()
{
    ui->removeButton->setEnabled(true);
    ui->updateButton->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::onPlaylistLoaded()
{
    onPlaylistCreated();
    ui->tableView->resizeColumnsToContents();
}

void PlaylistDock::onPlaylistModified()
{
    if (m_model.rowCount() == 1)
        ui->tableView->resizeColumnsToContents();
    if (m_model.rowCount() > 0) {
        ui->removeButton->setEnabled(true);
    }
}

void PlaylistDock::onPlaylistCleared()
{
    ui->removeButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
}

void PlaylistDock::onPlaylistClosed()
{
    ui->addButton->setDisabled(true);
}

void PlaylistDock::onDropped(const QMimeData *data, int row)
{
    bool resetIndex = true;
    if (data && data->hasUrls()) {
        LongUiTask longTask(tr("Add Files"));
        int insertNextAt = row;
        bool first = true;
        QStringList fileNames = Util::sortedFileList(Util::expandDirectories(data->urls()));
        auto i = 0, count = fileNames.size();
        for (const auto &path : fileNames) {
            if (MAIN.isSourceClipMyProject(path)) continue;
            longTask.reportProgress(Util::baseName(path), i++, count);
            Mlt::Producer p;
            if (path.endsWith(".mlt") || path.endsWith(".xml")) {
                p = Mlt::Producer(MLT.profile(), "xml", path.toUtf8().constData());
                if (p.is_valid()) {
                    // Convert MLT XML to a virtual clip.
                    p.set(kShotcutVirtualClip, 1);
                    p.set("resource", path.toUtf8().constData());
                    first = false;
                }
            } else {
                p = Mlt::Producer(MLT.profile(), path.toUtf8().constData());
            }
            if (p.is_valid()) {
                Mlt::Producer *producer = &p;
                if (first) {
                    first = false;
                    if (!MLT.producer() || !MLT.producer()->is_valid()) {
                        MAIN.open(path, nullptr, false);
                        if (MLT.producer() && MLT.producer()->is_valid()) {
                            producer = MLT.producer();
                            first = true;
                        }
                    }
                }
                producer = MLT.setupNewProducer(producer);
                if (MLT.isSeekable(producer) || producer->get_int(kShotcutVirtualClip)) {
                    ProxyManager::generateIfNotExists(*producer);
                    if (row == -1)
                        MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(producer)));
                    else
                        MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(producer), insertNextAt++));
                } else {
                    DurationDialog dialog(this);
                    dialog.setDuration(MLT.profile().fps() * 5);
                    if (dialog.exec() == QDialog::Accepted) {
                        producer->set_in_and_out(0, dialog.duration() - 1);
                        if (row == -1)
                            MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(producer)));
                        else
                            MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(producer), insertNextAt++));
                    }
                }
                if (first) {
                    first = false;
                    setIndex(0);
                    resetIndex = false;
                }
                delete producer;
            }
        }
    } else if (data && data->hasFormat(Mlt::XmlMimeType)) {
        if (MLT.producer() && MLT.producer()->is_valid()) {
            if (MLT.producer()->type() == mlt_service_playlist_type) {
                emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
            } else if (MAIN.isSourceClipMyProject()) {
                return;
            } else if (MLT.isSeekable()) {
                Mlt::Producer p(MLT.profile(), "xml-string", data->data(Mlt::XmlMimeType).constData());
                ProxyManager::generateIfNotExists(p);
                if (row == -1) {
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(&p)));
                    setPlaylistIndex(MLT.producer(), m_model.playlist()->count() - 1);
                } else {
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(&p), row));
                    setPlaylistIndex(MLT.producer(), row);
                }
                setUpdateButtonEnabled(true);
            } else {
                DurationDialog dialog(this);
                dialog.setDuration(MLT.profile().fps() * 5);
                if (dialog.exec() == QDialog::Accepted) {
                    MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                    if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                        MLT.producer()->set("mlt_service", "avformat-novalidate");
                    if (row == -1)
                        MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML()));
                    else
                        MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(), row));
                }
            }
        }
    }
    if (resetIndex)
        resetPlaylistIndex();
}

void PlaylistDock::onMoveClip(int from, int to)
{
    MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, from, to));
    m_view->clearSelection();
    resetPlaylistIndex();
}

void PlaylistDock::onPlayerDragStarted()
{
    if (isVisible())
        ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::on_addButton_clicked()
{
    on_actionAppendCut_triggered();
}

void PlaylistDock::on_actionThumbnailsHidden_triggered(bool checked)
{
    if (checked) {
        Settings.setPlaylistThumbnails("hidden");
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, true);
        ui->tableView->verticalHeader()->setDefaultSectionSize(m_defaultRowHeight);
    }
}

void PlaylistDock::on_actionLeftAndRight_triggered(bool checked)
{
    if (checked) {
        bool refreshThumbs = Settings.playlistThumbnails() != "tall";
        Settings.setPlaylistThumbnails("wide");
        if (refreshThumbs) m_model.refreshThumbnails();
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT);
        ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
    }
}

void PlaylistDock::on_actionTopAndBottom_triggered(bool checked)
{
    if (checked) {
        bool refreshThumbs = Settings.playlistThumbnails() != "wide";
        Settings.setPlaylistThumbnails("tall");
        if (refreshThumbs) m_model.refreshThumbnails();
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT * 2);
        ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
    }
}

void PlaylistDock::on_actionInOnlySmall_triggered(bool checked)
{
    if (checked) {
        bool refreshThumbs = Settings.playlistThumbnails() == "hidden";
        Settings.setPlaylistThumbnails("small");
        if (refreshThumbs) m_model.refreshThumbnails();
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT);
        ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
    }
}

void PlaylistDock::on_actionInOnlyLarge_triggered(bool checked)
{
    if (checked) {
        bool refreshThumbs = Settings.playlistThumbnails() == "hidden";
        Settings.setPlaylistThumbnails("large");
        if (refreshThumbs) m_model.refreshThumbnails();
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT * 2);
        ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
    }
}

void PlaylistDock::on_actionAddToTimeline_triggered()
{
    const QModelIndexList &indexes = m_view->selectionModel()->selectedIndexes();
    Mlt::Playlist playlist(MLT.profile());
    foreach (auto index, indexes) {
        if (index.column()) continue;
        QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index.row()));
        if (info && info->producer) {
            playlist.append(*info->producer, info->frame_in, info->frame_out);
        }
    }
    emit addAllTimeline(&playlist);
}

void PlaylistDock::on_actionAddToSlideshow_triggered()
{
    const QModelIndexList &indexes = m_view->selectionModel()->selectedIndexes();
    Mlt::Playlist playlist(MLT.profile());
    foreach (auto index, indexes) {
        if (index.column()) continue;
        QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index.row()));
        if (info && info->producer) {
            playlist.append(*info->producer, info->frame_in, info->frame_out);
        }
    }
    if (playlist.count() <= 0 ) {
        return;
    }

    SlideshowGeneratorDialog dialog(this, playlist);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted ) {
        LongUiTask longTask(QObject::tr("Generate Slideshow"));
        Mlt::Playlist *slideshow = longTask.runAsync<Mlt::Playlist *>(tr("Generating"), &dialog,
                                                                      &SlideshowGeneratorDialog::getSlideshow);
        if (slideshow) {
            if ( slideshow->count() > 0 ) {
                emit addAllTimeline(slideshow, /* skipProxy */ true);
            }
            delete slideshow;
        }
    }
}

void PlaylistDock::on_updateButton_clicked()
{
    int index = MLT.producer()->get_int(kPlaylistIndexProperty);
    if (index > 0 && index <= m_model.playlist()->count()) {
        MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(), index - 1));
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::onProducerChanged(Mlt::Producer *producer)
{
    if (!producer || !producer->is_valid())
        return;
    int index = producer->get_int(kPlaylistIndexProperty) - 1;
    if (index < 0 || !m_model.playlist() || !m_model.playlist()->is_valid()
            || index >= m_model.playlist()->count())
        return;
    MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(producer), index));
    setUpdateButtonEnabled(false);
}

void PlaylistDock::updateViewModeFromActions()
{
    PlaylistModel::ViewMode mode;

    if (ui->actionDetailed->isChecked()) {
        Settings.setViewMode(kDetailedMode);
        mode = PlaylistModel::Detailed;
    } else if (ui->actionIcons->isChecked()) {
        Settings.setViewMode(kIconsMode);
        mode = PlaylistModel::Icons;
    } else {
        Settings.setViewMode(kTiledMode);
        mode = PlaylistModel::Tiled;
    }

    setViewMode(mode);
}

void PlaylistDock::setViewMode(PlaylistModel::ViewMode mode)
{
    if (m_model.viewMode() == mode)
        return;

    ui->listView->setModel(nullptr);
    ui->tableView->setModel(nullptr);
    m_iconsView->setModel(nullptr);
    ui->listView->hide();
    ui->tableView->hide();
    m_iconsView->hide();

    if (ui->listView->itemDelegate()) {
        QAbstractItemDelegate *delegate = ui->listView->itemDelegate();
        ui->listView->setItemDelegate(nullptr);
        delete delegate;
    }

    m_model.setViewMode(mode);

    if (mode == PlaylistModel::Detailed) {
        m_view = ui->tableView;
        ui->tableView->setModel(&m_model);
        ui->tableView->resizeColumnsToContents();
        ui->tableView->show();

    } else if (mode == PlaylistModel::Tiled) {
        m_view = ui->listView;
        ui->listView->setDragEnabled(true);
        ui->listView->setItemDelegate(new TiledItemDelegate(ui->listView));
        ui->listView->setModel(&m_model);
        ui->listView->show();
    } else if (mode == PlaylistModel::Icons) {
        m_view = m_iconsView;
        m_iconsView->setModel(&m_model);
        m_iconsView->show();
    }
    m_model.refreshThumbnails();
}

void PlaylistDock::resetPlaylistIndex()
{
    if (MLT.producer())
        MLT.producer()->set(kPlaylistIndexProperty, nullptr, 0);
    // Clear the old values
    for (int j = 0; j < m_model.playlist()->count(); ++j) {
        Mlt::Producer clip(m_model.playlist()->get_clip(j));
        clip.parent().Mlt::Properties::clear(kPlaylistIndexProperty);
    }
    setUpdateButtonEnabled(false);
}

void PlaylistDock::emitDataChanged(const QVector<int> &roles)
{
    auto row = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
    if (row < 0 || row >= m_model.rowCount()) return;
    auto index = m_model.createIndex(row, PlaylistModel::COLUMN_RESOURCE);
    emit m_model.dataChanged(index, index, roles);
}

void PlaylistDock::setPlaylistIndex(Mlt::Producer *producer, int row)
{
    // Clear the old values
    for (int j = 0; j < m_model.playlist()->count(); ++j) {
        Mlt::Producer clip(m_model.playlist()->get_clip(j));
        clip.parent().Mlt::Properties::clear(kPlaylistIndexProperty);
    }
    producer->set(kPlaylistIndexProperty, row + 1);
}

#include "playlistdock.moc"

void PlaylistDock::on_tilesButton_clicked()
{
    ui->actionTiled->setChecked(true);
    ui->actionIcons->setChecked(false);
    ui->actionDetailed->setChecked(false);
    updateViewModeFromActions();
}

void PlaylistDock::on_iconsButton_clicked()
{
    ui->actionIcons->setChecked(true);
    ui->actionTiled->setChecked(false);
    ui->actionDetailed->setChecked(false);
    updateViewModeFromActions();
}

void PlaylistDock::on_detailsButton_clicked()
{
    ui->actionDetailed->setChecked(true);
    ui->actionTiled->setChecked(false);
    ui->actionIcons->setChecked(false);
    updateViewModeFromActions();
}

void PlaylistDock::onMovedToEnd()
{
    onMoveClip(m_view->currentIndex().row(), model()->rowCount());
}

void PlaylistDock::onInTimerFired()
{
    int index = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
    if (index < 0 || !m_model.playlist() || !m_model.playlist()->is_valid())
        return;
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index));
    if (info && info->producer
            && info->producer->get_producer() == MLT.producer()->get_producer()
            && info->frame_in != MLT.producer()->get_in()) {
        MAIN.undoStack()->push(new Playlist::TrimClipInCommand(m_model, index, MLT.producer()->get_in()));
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::onOutTimerFired()
{
    int index = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
    if (index < 0 || !m_model.playlist() || !m_model.playlist()->is_valid())
        return;
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index));
    if (info && info->producer
            && info->producer->get_producer() == MLT.producer()->get_producer()
            && info->frame_out != MLT.producer()->get_out()) {
        MAIN.undoStack()->push(new Playlist::TrimClipOutCommand(m_model, index, MLT.producer()->get_out()));
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        event->accept();
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void PlaylistDock::keyReleaseEvent(QKeyEvent *event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void PlaylistDock::on_actionCopy_triggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        resetPlaylistIndex();
        QString xml = MLT.XML(i->producer);
        Mlt::Producer *p = new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
        p->set_in_and_out(i->frame_in, i->frame_out);
        QGuiApplication::clipboard()->setText(MLT.XML(p));
        emit clipOpened(p);
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::on_actionPlayAfterOpen_triggered(bool checked)
{
    Settings.setPlaylistAutoplay(checked);
}

void PlaylistDock::on_actionSelectAll_triggered()
{
    m_view->selectionModel()->clearSelection();
    for (int i = 0; i < m_model.rowCount(); i++) {
        m_view->selectionModel()->select(m_model.index(i, 0),
                                         QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void PlaylistDock::on_actionSelectNone_triggered()
{
    m_view->selectionModel()->clearSelection();
}

void PlaylistDock::on_actionUpdateThumbnails_triggered()
{
    if (!m_model.playlist()) return;
    m_view->selectionModel()->clearSelection();
    for (auto i = 0; i < m_model.rowCount(); i++) {
        m_model.updateThumbnails(i);
    }
}

void PlaylistDock::onProducerModified()
{
    if (!m_model.playlist()) return;
    setUpdateButtonEnabled(true);

    // The clip name may have changed.
    emitDataChanged(QVector<int>() << PlaylistModel::FIELD_RESOURCE);
}

void PlaylistDock::on_addFilesButton_clicked()
{
    QMimeData mimeData;
    QList<QUrl> urls;

    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    LOG_DEBUG() << Util::getFileDialogOptions();
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open File"), path,
                                                          tr("All Files (*);;MLT XML (*.mlt)"), nullptr, Util::getFileDialogOptions());

    if (filenames.length() > 0) {
        Settings.setOpenPath(QFileInfo(filenames.first()).path());
        foreach (const QString &s, filenames) {
            urls << s;
        }
        mimeData.setUrls(urls);
        onDropped(&mimeData, m_view->currentIndex().row() + 1);
    }
}
