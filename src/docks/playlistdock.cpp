/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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
#include "mainwindow.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "widgets/playlisticonview.h"
#include "util.h"
#include "commands/playlistcommands.h"
#include <Logger.h>

#include <QMenu>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDir>

class TiledItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TiledItemDelegate(QAbstractItemView * view, QWidget *parent = 0)
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
        const bool roomEnoughForAllDetails = lineHeight * 4 < thumb.height();
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
        centeredTextRect.setHeight(lineHeight * (roomEnoughForAllDetails ? 4 : 2));
        centeredTextRect.moveCenter(option.rect.center());

        QRect textRect = centeredTextRect;
        textRect.setLeft(thumb.width() + 10);

        QPoint textPoint = textRect.topLeft();
        textPoint.setY(textPoint.y() + lineHeight);
        painter->setFont(boldFont);
        painter->drawText(textPoint,
                painter->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, textRect.width()));
        painter->setFont(oldFont);

        textPoint.setY(textPoint.y() + lineHeight);
        painter->drawText(textPoint, tr("Duration: %1").arg(index.data(PlaylistModel::FIELD_DURATION).toString()));
        if (roomEnoughForAllDetails) {
            textPoint.setY(textPoint.y() + lineHeight);
            painter->drawText(textPoint, tr("In: %1").arg(index.data(PlaylistModel::FIELD_IN).toString()));
            textPoint.setY(textPoint.y() + lineHeight);
            painter->drawText(textPoint, tr("Start: %1").arg(index.data(PlaylistModel::FIELD_START).toString()));
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
    QAbstractItemView * m_view;

};

PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());

    m_iconsView = new PlaylistIconView(this);
    ui->listView->parentWidget()->layout()->addWidget(m_iconsView);
    m_iconsView->setSelectionMode(QAbstractItemView::SingleSelection);

    QList<QAbstractItemView*> views;
    views << ui->tableView;
    views << ui->listView;
    views << m_iconsView;
    foreach (QAbstractItemView* view, views) {
        view->setDragDropMode(QAbstractItemView::DragDrop);
        view->setDropIndicatorShown(true);
        view->setDragDropOverwriteMode(false);
        view->setAcceptDrops(true);
        view->setDefaultDropAction(Qt::MoveAction);
        view->setAlternatingRowColors(true);
        connect(view, SIGNAL(customContextMenuRequested(QPoint)), SLOT(viewCustomContextMenuRequested(QPoint)));
        connect(view, SIGNAL(doubleClicked(QModelIndex)), SLOT(viewDoubleClicked(QModelIndex)));
    }

    connect(ui->actionDetailed, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));
    connect(ui->actionIcons, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));
    connect(ui->actionTiled, SIGNAL(triggered(bool)), SLOT(updateViewModeFromActions()));

    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(on_removeButton_clicked()));
    connect(&m_model, SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(&m_model, SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(&m_model, SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(&m_model, SIGNAL(modified()), this, SLOT(onPlaylistLoaded()));
    connect(&m_model, SIGNAL(dropped(const QMimeData*,int)), this, SLOT(onDropped(const QMimeData*,int)));
    connect(&m_model, SIGNAL(moveClip(int,int)), SLOT(onMoveClip(int,int)));
    connect(&m_model, SIGNAL(closed()), SLOT(onPlaylistClosed()));

    m_defaultRowHeight = ui->tableView->verticalHeader()->defaultSectionSize();
    QString thumbs = Settings.playlistThumbnails();
    if (thumbs == "wide") {
        ui->actionLeftAndRight->setChecked(true);
        on_actionLeftAndRight_triggered(true);
    }
    else if (thumbs == "tall") {
        ui->actionTopAndBottom->setChecked(true);
        on_actionTopAndBottom_triggered(true);
    }
    else if (thumbs == "small") {
        ui->actionInOnlySmall->setChecked(true);
        on_actionInOnlySmall_triggered(true);
    }
    else if (thumbs == "large") {
        ui->actionInOnlyLarge->setChecked(true);
        on_actionInOnlyLarge_triggered(true);
    }
    else {
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
        Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
        if (i) result = i->start;
        delete i;
    }
    return result;
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
    if (row > 0)
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row - 1));
}

void PlaylistDock::moveClipDown()
{
    int row = m_view->currentIndex().row();
    if (row + 1 < m_model.rowCount())
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row + 1));
}

void PlaylistDock::on_menuButton_clicked()
{
    QPoint pos = ui->menuButton->mapToParent(QPoint(0, 0));
    QMenu menu(this);
    QModelIndex index = m_view->currentIndex();
    if (index.isValid() && m_model.playlist()) {
        if (!MAIN.isMultitrackValid())
            menu.addAction(ui->actionGoto);
        if (MLT.isClip())
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionOpen);
        menu.addAction(ui->actionUpdate);
        menu.addAction(ui->actionRemove);
        menu.addSeparator();
    }
    menu.addAction(ui->actionRemoveAll);
    menu.addAction(ui->actionAddToTimeline);
    menu.addSeparator();

    QMenu* viewModeMenu = menu.addMenu(tr("View mode"));
    QActionGroup modeGroup(this);
    modeGroup.addAction(ui->actionDetailed);
    modeGroup.addAction(ui->actionTiled);
    modeGroup.addAction(ui->actionIcons);
    viewModeMenu->addActions(modeGroup.actions());

    menu.addSeparator();
    QMenu* subMenu = menu.addMenu(tr("Thumbnails"));
    QActionGroup group(this);
    group.addAction(ui->actionThumbnailsHidden);
    group.addAction(ui->actionInOnlyLarge);
    group.addAction(ui->actionInOnlySmall);
    group.addAction(ui->actionLeftAndRight);
    group.addAction(ui->actionTopAndBottom);
    subMenu->addActions(group.actions());
    menu.exec(mapToGlobal(pos));
}

void PlaylistDock::on_actionInsertCut_triggered()
{
    if (MLT.isClip() || MLT.savedProducer()) {
        QMimeData mimeData;
        mimeData.setData(Mlt::XmlMimeType, MLT.XML(
            MLT.isClip()? 0 : MLT.savedProducer()).toUtf8());
        onDropped(&mimeData, m_view->currentIndex().row());
    }
}

void PlaylistDock::on_actionAppendCut_triggered()
{
    if (MLT.producer() && MLT.producer()->is_valid() && !MAIN.isSourceClipMyProject()) {
        if (MLT.isSeekableClip()
            || (MLT.savedProducer() && MLT.isSeekable(MLT.savedProducer()))) {
            MAIN.undoStack()->push(
                new Playlist::AppendCommand(m_model,
                    MLT.XML(MLT.isClip()? 0 : MLT.savedProducer())));
            MLT.producer()->set(kPlaylistIndexProperty, m_model.playlist()->count());
            setUpdateButtonEnabled(true);
        } else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                    MLT.producer()->set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML()));
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
    if (!index.isValid() || !m_model.playlist()) return;
    Mlt::ClipInfo* info = m_model.playlist()->clip_info(index.row());
    if (!info || MAIN.isSourceClipMyProject()) return;
    if (MLT.producer()->type() != playlist_type) {
        if (MLT.isSeekable()) {
            MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(), index.row()));
            MLT.producer()->set(kPlaylistIndexProperty, index.row() + 1);
            setUpdateButtonEnabled(true);
        }
        else {
            // change the duration
            DurationDialog dialog(this);
            dialog.setDuration(info->frame_count);
            if (dialog.exec() == QDialog::Accepted) {
                MLT.producer()->set_in_and_out(0, dialog.duration() - 1);
                if (MLT.producer()->get("mlt_service") && !strcmp(MLT.producer()->get("mlt_service"), "avformat"))
                    MLT.producer()->set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(), index.row()));
            }
        }
    }
    else {
        emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        setUpdateButtonEnabled(false);
    }
    delete info;
}

void PlaylistDock::on_removeButton_clicked()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    MAIN.undoStack()->push(new Playlist::RemoveCommand(m_model, index.row()));
    int count = m_model.playlist()->count();
    if (count == 0) return;
    int i = index.row() >= count? count-1 : index.row();
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(i));
    if (info) {
        emit itemActivated(info->start);
        int j = MLT.producer()->get_int(kPlaylistIndexProperty);
        if (j > i + 1) {
            MLT.producer()->set(kPlaylistIndexProperty, j - 1);
        } else if (j == i + 1) {
            // Remove the playlist index property on the producer.
            MLT.producer()->set(kPlaylistIndexProperty, 0, 0);
            setUpdateButtonEnabled(false);
        }
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
}

void PlaylistDock::on_actionOpen_triggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        QString xml = MLT.XML(i->producer);
        Mlt::Producer* p = new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
        p->set_in_and_out(i->frame_in, i->frame_out);
        p->set(kPlaylistIndexProperty, index.row() + 1);
        emit clipOpened(p);
        delete i;
    }
}

void PlaylistDock::viewCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = m_view->currentIndex();
    if (index.isValid() && m_model.playlist()) {
        QMenu menu(this);
        if (!MAIN.isMultitrackValid())
            menu.addAction(ui->actionGoto);
        if (MLT.isClip())
            menu.addAction(ui->actionInsertCut);
        menu.addAction(ui->actionOpen);

        QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index.row()));
        if (info && MLT.producer()->get_int(kPlaylistIndexProperty) == index.row() + 1) {
            if (MLT.producer()->get_in() != info->frame_in || MLT.producer()->get_out() != info->frame_out)
                menu.addAction(ui->actionUpdate);
        }

        menu.addAction(ui->actionRemove);
        menu.exec(mapToGlobal(pos));
    }
}


void PlaylistDock::viewDoubleClicked(const QModelIndex &index)
{
    if (!m_model.playlist()) return;
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        if (qApp->keyboardModifiers() == Qt::ShiftModifier) {
            emit itemActivated(i->start);
        } else {
            QString xml = MLT.XML(i->producer);
            Mlt::Producer* p = new Mlt::Producer(MLT.profile(), "xml-string", xml.toUtf8().constData());
            p->set_in_and_out(i->frame_in, i->frame_out);
            p->set(kPlaylistIndexProperty, index.row() + 1);
            emit clipOpened(p);
        }
        delete i;
    } else {
        MAIN.openVideo();
    }
}

void PlaylistDock::on_actionGoto_triggered()
{
    QModelIndex index = m_view->currentIndex();
    Mlt::ClipInfo* i = m_model.playlist()->clip_info(index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
    }
}

void PlaylistDock::on_actionRemoveAll_triggered()
{
    MAIN.undoStack()->push(new Playlist::ClearCommand(m_model));
}

void PlaylistDock::onPlaylistCreated()
{
    ui->removeButton->setEnabled(true);
    ui->updateButton->setEnabled(false);
    ui->menuButton->setEnabled(true);
    ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::onPlaylistLoaded()
{
    onPlaylistCreated();
    ui->tableView->resizeColumnsToContents();
}

void PlaylistDock::onPlaylistCleared()
{
    ui->removeButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->menuButton->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
}

void PlaylistDock::onPlaylistClosed()
{
    ui->addButton->setDisabled(true);
}

void PlaylistDock::onDropped(const QMimeData *data, int row)
{
    if (data && data->hasUrls()) {
        int insertNextAt = row;
        bool first = true;
        QStringList fileNames = Util::sortedFileList(Util::expandDirectories(data->urls()));
        foreach (QString path, fileNames) {
            if (MAIN.isSourceClipMyProject(path)) continue;
            Mlt::Producer p(MLT.profile(), path.toUtf8().constData());
            if (p.is_valid()) {
                // Convert MLT XML to a virtual clip.
                if (!qstrcmp(p.get("mlt_service"), "xml")) {
                    p.set(kShotcutVirtualClip, 1);
                    p.set("resource", path.toUtf8().constData());
                    first = false;
                }
                Mlt::Producer* producer = &p;
                if (first) {
                    first = false;
                    if (!MLT.producer() || !MLT.producer()->is_valid()) {
                        MAIN.open(path);
                        if (MLT.producer() && MLT.producer()->is_valid())
                            producer = MLT.producer();
                    }
                }
                // Convert avformat to avformat-novalidate so that XML loads faster.
                if (!qstrcmp(producer->get("mlt_service"), "avformat")) {
                    producer->set("mlt_service", "avformat-novalidate");
                    producer->set("mute_on_pause", 0);
                }
                MLT.setImageDurationFromDefault(producer);
                if (row == -1)
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(producer)));
                else
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(producer), insertNextAt++));
            }
        }
    }
    else if (data && data->hasFormat(Mlt::XmlMimeType)) {
        if (MLT.producer() && MLT.producer()->is_valid()) {
            if (MLT.producer()->type() == playlist_type) {
                emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
            } else if (MAIN.isSourceClipMyProject()) {
                return;
            } else if (MLT.isSeekable()) {
                if (row == -1) {
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, data->data(Mlt::XmlMimeType)));
                    MLT.producer()->set(kPlaylistIndexProperty, m_model.playlist()->count());
                } else {
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, data->data(Mlt::XmlMimeType), row));
                    MLT.producer()->set(kPlaylistIndexProperty, row + 1);
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
}

void PlaylistDock::onMoveClip(int from, int to)
{
    MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, from, to));
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
    emit addAllTimeline(m_model.playlist());
}

void PlaylistDock::on_updateButton_clicked()
{
    int index = MLT.producer()->get_int(kPlaylistIndexProperty);
    if (index > 0 && index <= m_model.playlist()->count()) {
        MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(), index - 1));
        setUpdateButtonEnabled(false);
    }
}

void PlaylistDock::updateViewModeFromActions()
{
    PlaylistModel::ViewMode mode;

    if (ui->actionDetailed->isChecked())
    {
        Settings.setViewMode(kDetailedMode);
        mode = PlaylistModel::Detailed;
    }
    else if (ui->actionIcons->isChecked())
    {
        Settings.setViewMode(kIconsMode);
        mode = PlaylistModel::Icons;
    }
    else
    {
        Settings.setViewMode(kTiledMode);
        mode = PlaylistModel::Tiled;
    }

    setViewMode(mode);
}

void PlaylistDock::setViewMode(PlaylistModel::ViewMode mode)
{
    if (m_model.viewMode() == mode)
        return;

    ui->listView->setModel(0);
    ui->tableView->setModel(0);
    m_iconsView->setModel(0);
    ui->listView->hide();
    ui->tableView->hide();
    m_iconsView->hide();

    if (ui->listView->itemDelegate()) {
        QAbstractItemDelegate * delegate = ui->listView->itemDelegate();
        ui->listView->setItemDelegate(0);
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

void PlaylistDock::keyPressEvent(QKeyEvent* event)
{
    QDockWidget::keyPressEvent(event);
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void PlaylistDock::keyReleaseEvent(QKeyEvent* event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}
