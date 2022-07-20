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
#include "widgets/docktoolbar.h"
#include "widgets/playlisticonview.h"
#include "widgets/playlisttable.h"
#include "widgets/playlistlistview.h"
#include "util.h"
#include "commands/playlistcommands.h"
#include "proxymanager.h"
#include "qmltypes/qmlapplication.h"
#include <Logger.h>

#include <QItemSelectionModel>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QPainter>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDir>
#include <QGuiApplication>
#include <QClipboard>

static const int kInOutChangedTimeoutMs = 100;

#define kDetailedMode "detailed"
#define kIconsMode "icons"
#define kTiledMode "tiled"

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

    m_selectionModel = new QItemSelectionModel(&m_model, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
            &PlaylistDock::selectionChanged);

    setupActions();

    m_mainMenu = new QMenu(tr("Playlist Main Menu"), this);
    m_mainMenu->addAction(m_actions["playlistOpenAction"]);
    m_mainMenu->addAction(m_actions["playlistOpenPreviousAction"]);
    m_mainMenu->addAction(m_actions["playlistOpenNextAction"]);
    m_mainMenu->addAction(m_actions["playlistGoToAction"]);
    m_mainMenu->addAction(m_actions["playlistRemoveCutAction"]);
    m_mainMenu->addAction(m_actions["playlistCopyAction"]);
    m_mainMenu->addAction(m_actions["playlistInsertCutAction"]);
    m_mainMenu->addAction(m_actions["playlistUpdateAction"]);
    m_mainMenu->addAction(m_actions["playlistMoveUpAction"]);
    m_mainMenu->addAction(m_actions["playlistMoveDownAction"]);
    m_mainMenu->addAction(m_actions["playlistUpdateThumbnailsAction"]);
    m_mainMenu->addAction(m_actions["playlistSetFileDateAction"]);
    m_mainMenu->addSeparator();
    QMenu *selectMenu = m_mainMenu->addMenu(tr("Select"));
    selectMenu->addAction(m_actions["playlistSelectAllAction"]);
    selectMenu->addAction(m_actions["playlistSelectNoneAction"]);
    selectMenu->addAction(m_actions["playlistSelectClip1Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip2Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip3Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip4Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip5Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip6Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip7Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip8Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip9Action"]);
    selectMenu->addAction(m_actions["playlistSelectClip10Action"]);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(m_actions["playlistRemoveAllAction"]);
    m_mainMenu->addAction(m_actions["playlistAddToTimelineAction"]);
    m_mainMenu->addAction(m_actions["playlistAddToSlideshowAction"]);
    m_mainMenu->addSeparator();
    QMenu *sortByMenu = m_mainMenu->addMenu(tr("Sort"));
    sortByMenu->addAction(m_actions["playlistSortByNameAction"]);
    sortByMenu->addAction(m_actions["playlistSortByDateAction"]);
    m_mainMenu->addSeparator();

    DockToolBar *toolbar = new DockToolBar(tr("Timeline Controls"));
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(QIcon::fromTheme("show-menu",
                                         QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    menuButton->setToolTip(tr("Playlist Menu"));
    menuButton->setAutoRaise(true);
    menuButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    toolbar->addWidget(menuButton);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["playlistAppendCutAction"]);
    toolbar->addAction(m_actions["playlistRemoveCutAction"]);
    toolbar->addAction(m_actions["playlistAddFilesAction"]);
    toolbar->addAction(m_actions["playlistUpdateAction"]);
    toolbar->addSeparator();
    toolbar->addAction(m_actions["playlistViewDetailsAction"]);
    toolbar->addAction(m_actions["playlistViewTilesAction"]);
    toolbar->addAction(m_actions["playlistViewIconsAction"]);
    toolbar->addSeparator();

    ui->verticalLayout->setMenuBar(toolbar);

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

    connect(ui->tableView, SIGNAL(movedToEnd()), SLOT(onMovedToEnd()));
    connect(ui->listView, SIGNAL(movedToEnd()), SLOT(onMovedToEnd()));
    connect(&m_model, SIGNAL(cleared()), this, SLOT(onPlaylistCleared()));
    connect(&m_model, SIGNAL(created()), this, SLOT(onPlaylistCreated()));
    connect(&m_model, SIGNAL(loaded()), this, SLOT(onPlaylistLoaded()));
    connect(&m_model, SIGNAL(modified()), this, SLOT(onPlaylistModified()));
    connect(&m_model, SIGNAL(dropped(const QMimeData *, int)), this, SLOT(onDropped(const QMimeData *,
                                                                                    int)));
    connect(&m_model, SIGNAL(moveClip(int, int)), SLOT(onMoveClip(int, int)));

    m_defaultRowHeight = ui->tableView->verticalHeader()->defaultSectionSize();
    QString thumbs = Settings.playlistThumbnails();
    if (thumbs == "wide") {
        m_actions["playlistThumbnailsLeftAndRightAction"]->trigger();
    } else if (thumbs == "tall") {
        m_actions["playlistThumbnailsTopAndBottomAction"]->trigger();
    } else if (thumbs == "small") {
        m_actions["playlistThumbnailsInOnlySmallAction"]->trigger();
    } else if (thumbs == "large") {
        m_actions["playlistThumbnailsInOnlyLargeAction"]->trigger();
    } else {
        m_actions["playlistThumbnailsHiddenAction"]->trigger();
    }

    if (Settings.viewMode() == kDetailedMode) {
        m_actions["playlistViewDetailsAction"]->trigger();
    } else if (Settings.viewMode() == kTiledMode) {
        m_actions["playlistViewTilesAction"]->trigger();
    } else { /* if (Settings.viewMode() == kIconsMode) */
        m_actions["playlistViewIconsAction"]->trigger();
    }

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

void PlaylistDock::setupActions()
{
    QIcon icon;
    QAction *action;

    action = new QAction(tr("Append"), this);
    action->setObjectName("playlistAppendCutAction");
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_A));
    action->setToolTip(tr("Add the Source to the playlist"));
    icon = QIcon::fromTheme("list-add",
                            QIcon(":/icons/oxygen/32x32/actions/list-add.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAppendCutActionTriggered);
    connect(this, &PlaylistDock::producerOpened, action, [ = ]() {
        action->setEnabled(!MLT.isMultitrack() && !MLT.isPlaylist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Remove"), this);
    action->setObjectName("playlistRemoveCutAction");

    QList<QKeySequence> removeShortcuts;
    removeShortcuts << QKeySequence(Qt::SHIFT + Qt::Key_X);
    removeShortcuts << QKeySequence(Qt::SHIFT + Qt::Key_Z);
    action->setShortcuts(removeShortcuts);
    action->setToolTip(tr("Remove cut"));
    icon = QIcon::fromTheme("list-remove",
                            QIcon(":/icons/oxygen/32x32/actions/list-remove.png"));
    action->setIcon(icon);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onRemoveActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->selectionModel()->selectedIndexes().size() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Add Files"), this);
    action->setObjectName("playlistAddFilesAction");
    action->setToolTip(tr("Add files to playlist"));
    icon = QIcon::fromTheme("list-add-files",
                            QIcon(":/icons/oxygen/32x32/actions/list-add-files.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddFilesActionTriggered);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Update"), this);
    action->setObjectName("playlistUpdateAction");
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_B));
    icon = QIcon::fromTheme("dialog-ok",
                            QIcon(":/icons/oxygen/32x32/actions/dialog-ok.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, &PlaylistDock::onUpdateActionTriggered);
    connect(this, &PlaylistDock::enableUpdate, action, &QAction::setEnabled);
    m_actions[action->objectName()] = action;

    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    action = new QAction(tr("Tiles"), this);
    action->setObjectName("playlistViewTilesAction");
    action->setToolTip(tr("View as tiles"));
    icon = QIcon::fromTheme("view-list-details",
                            QIcon(":/icons/oxygen/32x32/actions/view-list-details.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setViewMode(kTiledMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Icons"), this);
    action->setObjectName("playlistViewIconsAction");
    action->setToolTip(tr("View as icons"));
    icon = QIcon::fromTheme("view-list-icons",
                            QIcon(":/icons/oxygen/32x32/actions/view-list-icons.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setViewMode(kIconsMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Details"), this);
    action->setObjectName("playlistViewDetailsAction");
    action->setToolTip(tr("View as details"));
    icon = QIcon::fromTheme("view-list-text",
                            QIcon(":/icons/oxygen/32x32/actions/view-list-text.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setViewMode(kDetailedMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Open"), this);
    action->setObjectName("playlistOpenAction");
    action->setToolTip(tr("Open the clip in the Source player"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onOpenActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("GoTo"), this);
    action->setObjectName("playlistGoToAction");
    action->setToolTip(tr("Go to the start of this clip in the Project player"));
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Return));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onGotoActionTriggered);
    connect(this, &PlaylistDock::producerOpened, action, [ = ]() {
        action->setEnabled(!MLT.isMultitrack() && !MLT.isPlaylist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Copy"), this);
    action->setObjectName("playlistCopyAction");
    action->setToolTip(tr("Open a copy of the clip in the Source player"));
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_C));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onCopyActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Insert"), this);
    action->setObjectName("playlistInsertCutAction");
    action->setToolTip(tr("Insert"));
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_V));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onInsertCutActionTriggered);
    connect(this, &PlaylistDock::producerOpened, action, [ = ]() {
        action->setEnabled(MLT.isClip() || MLT.savedProducer());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Update Thumbnails"), this);
    action->setObjectName("playlistUpdateThumbnailsAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onUpdateThumbnailsActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Set Creation Time..."), this);
    action->setObjectName("playlistSetFileDateAction");
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onSetFileDateActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Remove All"), this);
    action->setObjectName("playlistRemoveAllAction");
    action->setToolTip(tr("Remove all items from the playlist"));
    connect(action, &QAction::triggered, this, &PlaylistDock::onRemoveAllActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select All"), this);
    action->setObjectName("playlistSelectAllAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    connect(action, &QAction::triggered, this, &PlaylistDock::onSelectAllActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select None"), this);
    action->setObjectName("playlistSelectNoneAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
    connect(action, &QAction::triggered, m_selectionModel, &QItemSelectionModel::clearSelection);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Move Up"), this);
    action->setObjectName("playlistMoveUpAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));
    connect(action, &QAction::triggered, m_selectionModel, [ = ]() {
        raise();
        moveClipUp();
        decrementIndex();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Move Down"), this);
    action->setObjectName("playlistMoveDownAction");
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
    connect(action, &QAction::triggered, m_selectionModel, [ = ]() {
        raise();
        moveClipDown();
        incrementIndex();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Add Selected to Timeline"), this);
    action->setObjectName("playlistAddToTimelineAction");
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddToTimelineActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Add Selected to Slideshow"), this);
    action->setObjectName("playlistAddToSlideshowAction");
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddToSlideshowActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Sort By Name"), this);
    action->setObjectName("playlistSortByNameAction");
    connect(action, &QAction::triggered, this, [&](bool checked) {
        resetPlaylistIndex();
        MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_RESOURCE,
                                                         Qt::AscendingOrder));
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Sort By Date"), this);
    action->setObjectName("playlistSortByDateAction");
    connect(action, &QAction::triggered, this, [&](bool checked) {
        resetPlaylistIndex();
        MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_DATE,
                                                         Qt::AscendingOrder));
    });
    m_actions[action->objectName()] = action;

    QActionGroup *thumbnailGroup = new QActionGroup(this);
    thumbnailGroup->setExclusive(true);

    action = new QAction(tr("Hidden"), this);
    action->setObjectName("playlistThumbnailsHiddenAction");
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            Settings.setPlaylistThumbnails("hidden");
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, true);
            ui->tableView->verticalHeader()->setDefaultSectionSize(m_defaultRowHeight);
        }
    });
    thumbnailGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("In and Out - Left/Right"), this);
    action->setObjectName("playlistThumbnailsLeftAndRightAction");
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            bool refreshThumbs = Settings.playlistThumbnails() != "tall";
            Settings.setPlaylistThumbnails("wide");
            if (refreshThumbs) m_model.refreshThumbnails();
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
            ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT);
            ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
        }
    });
    thumbnailGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("In and Out - Top/Bottom"), this);
    action->setObjectName("playlistThumbnailsTopAndBottomAction");
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            bool refreshThumbs = Settings.playlistThumbnails() != "wide";
            Settings.setPlaylistThumbnails("tall");
            if (refreshThumbs) m_model.refreshThumbnails();
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
            ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT * 2);
            ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
        }
    });
    thumbnailGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("In Only - Small"), this);
    action->setObjectName("playlistThumbnailsInOnlySmallAction");
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            bool refreshThumbs = Settings.playlistThumbnails() == "hidden";
            Settings.setPlaylistThumbnails("small");
            if (refreshThumbs) m_model.refreshThumbnails();
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
            ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT);
            ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
        }
    });
    thumbnailGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("In Only - Large"), this);
    action->setObjectName("playlistThumbnailsInOnlyLargeAction");
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            bool refreshThumbs = Settings.playlistThumbnails() == "hidden";
            Settings.setPlaylistThumbnails("large");
            if (refreshThumbs) m_model.refreshThumbnails();
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, false);
            ui->tableView->verticalHeader()->setDefaultSectionSize(PlaylistModel::THUMBNAIL_HEIGHT * 2);
            ui->tableView->resizeColumnToContents(PlaylistModel::COLUMN_THUMBNAIL);
        }
    });
    thumbnailGroup->addAction(action);
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Play After Open"), this);
    action->setObjectName("playlistPlayAfterOpenAction");
    action->setCheckable(true);
    action->setChecked(Settings.playlistAutoplay());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setPlaylistAutoplay(checked);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Open Previous"), this);
    action->setObjectName("playlistOpenPreviousAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Up));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        decrementIndex();
        onOpenActionTriggered();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Open Next"), this);
    action->setObjectName("playlistOpenNextAction");
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Down));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        incrementIndex();
        onOpenActionTriggered();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 1"), this);
    action->setObjectName("playlistSelectClip1Action");
    action->setShortcut(QKeySequence(Qt::Key_1));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(0);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 0);
    });
    m_actions[action->objectName()] = action;
    \

    action = new QAction(tr("Select Clip 2"), this);
    action->setObjectName("playlistSelectClip2Action");
    action->setShortcut(QKeySequence(Qt::Key_2));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(1);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 1);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 3"), this);
    action->setObjectName("playlistSelectClip3Action");
    action->setShortcut(QKeySequence(Qt::Key_3));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(2);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 2);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 4"), this);
    action->setObjectName("playlistSelectClip4Action");
    action->setShortcut(QKeySequence(Qt::Key_4));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(3);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 3);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 5"), this);
    action->setObjectName("playlistSelectClip5Action");
    action->setShortcut(QKeySequence(Qt::Key_5));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(4);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 4);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 6"), this);
    action->setObjectName("playlistSelectClip6Action");
    action->setShortcut(QKeySequence(Qt::Key_6));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(5);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 5);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 7"), this);
    action->setObjectName("playlistSelectClip7Action");
    action->setShortcut(QKeySequence(Qt::Key_7));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(6);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 6);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 8"), this);
    action->setObjectName("playlistSelectClip8Action");
    action->setShortcut(QKeySequence(Qt::Key_8));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(7);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 7);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 9"), this);
    action->setObjectName("playlistSelectClip9Action");
    action->setShortcut(QKeySequence(Qt::Key_9));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(8);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 8);
    });
    m_actions[action->objectName()] = action;

    action = new QAction(tr("Select Clip 10"), this);
    action->setObjectName("playlistSelectClip10Action");
    action->setShortcut(QKeySequence(Qt::Key_0));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(9);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_model.rowCount() > 9);
    });
    m_actions[action->objectName()] = action;
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

void PlaylistDock::onInsertCutActionTriggered()
{
    if (MLT.isClip() || MLT.savedProducer()) {
        show();
        raise();
        QMimeData mimeData;
        mimeData.setData(Mlt::XmlMimeType, MLT.XML(
                             MLT.isClip() ? nullptr : MLT.savedProducer()).toUtf8());
        onDropped(&mimeData, m_view->currentIndex().row());
    }
}

void PlaylistDock::onAppendCutActionTriggered()
{
    Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
    if (producer.is_valid() && !MAIN.isSourceClipMyProject()) {
        if (!MLT.isLiveProducer(&producer)) {
            ProxyManager::generateIfNotExists(producer);
            MAIN.undoStack()->push(
                new Playlist::AppendCommand(m_model, MLT.XML(&producer)));
            setPlaylistIndex(&producer, m_model.playlist()->count() - 1);
            enableUpdate(true);
        } else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                producer.set_in_and_out(0, dialog.duration() - 1);
                if (producer.get("mlt_service") && !strcmp(producer.get("mlt_service"), "avformat"))
                    producer.set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML()));
                setPlaylistIndex(&producer, m_model.playlist()->count() - 1);
                enableUpdate(true);
            }
        }
    }
}

void PlaylistDock::onUpdateActionTriggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist() || MAIN.isSourceClipMyProject()) return;
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(index.row()));
    Mlt::Producer producer(MLT.isClip() ? MLT.producer() : MLT.savedProducer());
    if (!info || !producer.is_valid()) return;
    if (producer.type() != mlt_service_playlist_type) {
        show();
        raise();
        if (!MLT.isLiveProducer(&producer)) {
            ProxyManager::generateIfNotExists(producer);
            MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(&producer), index.row()));
            setPlaylistIndex(&producer, index.row());
            enableUpdate(true);
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
                enableUpdate(true);
            }
        }
    } else {
        emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        enableUpdate(false);
    }
}

void PlaylistDock::onRemoveActionTriggered()
{
    if (!m_model.playlist() || !m_view->selectionModel()) return;
    show();
    raise();
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
        enableUpdate(false);
    }
}

void PlaylistDock::onSetFileDateActionTriggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    int count = m_model.playlist()->count();
    if (count == 0) return;
    int i = index.row() >= count ? count - 1 : index.row();
    QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(i));
    if (info && info->producer && info->producer->is_valid()) {
        show();
        raise();
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

void PlaylistDock::onProducerOpened()
{
    if (MLT.producer() && MLT.producer()->is_valid()) {
        auto row = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
        if (row < 0 && m_model.rowCount() > 0) {
            resetPlaylistIndex();
            emit m_model.dataChanged(m_model.createIndex(0, PlaylistModel::COLUMN_THUMBNAIL),
                                     m_model.createIndex(m_model.playlist()->count() - 1, PlaylistModel::COLUMN_THUMBNAIL),
                                     QVector<int>() << PlaylistModel::COLUMN_THUMBNAIL);
        }
    }
    producerOpened();
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

void PlaylistDock::onOpenActionTriggered()
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
        menu.addAction(m_actions["playlistOpenAction"]);
        menu.addAction(m_actions["playlistGoToAction"]);
        menu.addAction(m_actions["playlistRemoveCutAction"]);
        menu.addAction(m_actions["playlistCopyAction"]);
        menu.addAction(m_actions["playlistInsertCutAction"]);
        menu.addAction(m_actions["playlistUpdateAction"]);
        menu.addAction(m_actions["playlistUpdateThumbnailsAction"]);
        menu.addAction(m_actions["playlistUpdateThumbnailsAction"]);
        menu.addAction(m_actions["playlistSetFileDateAction"]);
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

void PlaylistDock::onGotoActionTriggered()
{
    QModelIndex index = m_view->currentIndex();
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        emit itemActivated(i->start);
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::onRemoveAllActionTriggered()
{
    resetPlaylistIndex();
    MAIN.undoStack()->push(new Playlist::ClearCommand(m_model));
}

void PlaylistDock::onPlaylistCreated()
{
    enableUpdate(false);
    updateViewMode();
    ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::onPlaylistLoaded()
{
    onPlaylistCreated();
}

void PlaylistDock::onPlaylistModified()
{
    if (m_model.rowCount() == 1)
        ui->tableView->resizeColumnsToContents();
}

void PlaylistDock::onPlaylistCleared()
{
    enableUpdate(false);
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
        for (auto &path : fileNames) {
            if (MAIN.isSourceClipMyProject(path)) continue;
            longTask.reportProgress(Util::baseName(path), i++, count);
            if (MLT.checkFile(path)) {
                emit showStatusMessage(tr("Failed to open ").append(path));
                continue;
            }
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
                if (!MLT.isLiveProducer(producer) || producer->get_int(kShotcutVirtualClip)) {
                    ProxyManager::generateIfNotExists(*producer);
                    if (row == -1)
                        MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(producer)));
                    else
                        MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(producer), insertNextAt++));
                } else {
                    LongUiTask::cancel();
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
            } else if (!MLT.isLiveProducer()) {
                Mlt::Producer p(MLT.profile(), "xml-string", data->data(Mlt::XmlMimeType).constData());
                ProxyManager::generateIfNotExists(p);
                if (row == -1) {
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(&p)));
                    setPlaylistIndex(MLT.producer(), m_model.playlist()->count() - 1);
                } else {
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(&p), row));
                    setPlaylistIndex(MLT.producer(), row);
                }
                enableUpdate(true);
            } else {
                LongUiTask::cancel();
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

void PlaylistDock::onAddToTimelineActionTriggered()
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

void PlaylistDock::onAddToSlideshowActionTriggered()
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

void PlaylistDock::onProducerChanged(Mlt::Producer *producer)
{
    if (!producer || !producer->is_valid())
        return;
    int index = producer->get_int(kPlaylistIndexProperty) - 1;
    if (index < 0 || !m_model.playlist() || !m_model.playlist()->is_valid()
            || index >= m_model.playlist()->count())
        return;
    MAIN.undoStack()->push(new Playlist::UpdateCommand(m_model, MLT.XML(producer), index));
    enableUpdate(false);
}

void PlaylistDock::updateViewMode()
{
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

    QString mode = Settings.viewMode();
    if (mode == kDetailedMode) {
        m_model.setViewMode(PlaylistModel::Detailed);
        m_view = ui->tableView;
        ui->tableView->setModel(&m_model);
        ui->tableView->resizeColumnsToContents();
        ui->tableView->show();

        ui->tableView->resizeColumnsToContents();
    } else if (mode == kTiledMode) {
        m_model.setViewMode(PlaylistModel::Tiled);
        m_view = ui->listView;
        ui->listView->setDragEnabled(true);
        ui->listView->setItemDelegate(new TiledItemDelegate(ui->listView));
        ui->listView->setModel(&m_model);
        ui->listView->show();

    } else { /* if (mode == kIconsMode) */
        m_model.setViewMode(PlaylistModel::Icons);
        m_view = m_iconsView;
        m_iconsView->setModel(&m_model);
        m_iconsView->show();

    }
    m_view->setSelectionModel(m_selectionModel);
    emit selectionChanged();
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
    enableUpdate(false);
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
        enableUpdate(false);
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
        enableUpdate(false);
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

void PlaylistDock::onCopyActionTriggered()
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid() || !m_model.playlist()) return;
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index.row());
    if (i) {
        show();
        raise();
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

void PlaylistDock::onSelectAllActionTriggered()
{
    show();
    raise();
    m_view->selectionModel()->clearSelection();
    for (int i = 0; i < m_model.rowCount(); i++) {
        m_view->selectionModel()->select(m_model.index(i, 0),
                                         QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void PlaylistDock::onUpdateThumbnailsActionTriggered()
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
    enableUpdate(true);

    // The clip name may have changed.
    emitDataChanged(QVector<int>() << PlaylistModel::FIELD_RESOURCE);
}

void PlaylistDock::onAddFilesActionTriggered()
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
