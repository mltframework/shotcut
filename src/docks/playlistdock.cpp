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

#include "playlistdock.h"
#include "ui_playlistdock.h"
#include "actions.h"
#include "dialogs/durationdialog.h"
#include "dialogs/filedatedialog.h"
#include "dialogs/longuitask.h"
#include "dialogs/resourcedialog.h"
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
#include "widgets/lineeditclear.h"
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
#include <QActionGroup>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QInputDialog>

static const auto kInOutChangedTimeoutMs = 100;
static const auto kTilePaddingPx = 10;
static const auto kSpecialBinDataAll = QLatin1String("all");
static const auto kDetailedMode = QLatin1String("detailed");
static const auto kIconsMode = QLatin1String("icons");
static const auto kTiledMode = QLatin1String("tiled");

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
        const QStringList nameParts = index.data(Qt::DisplayRole).toString().split('\n');
        const bool roomEnoughForAllDetails = nameParts.size() == 1
                                             && lineHeight * 3 < thumb.height() + kTilePaddingPx;
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
        QRect centeredTextRect = option.rect;
        centeredTextRect.setHeight(lineHeight * (roomEnoughForAllDetails ? 3 : 2) + kTilePaddingPx);
        centeredTextRect.moveCenter(option.rect.center());

        QRect textRect = centeredTextRect;
        textRect.setLeft(thumb.width() + (("hidden" == setting) ? PlaylistModel::THUMBNAIL_HEIGHT : 10));

        QPoint textPoint = textRect.topLeft();
        textPoint.setY(textPoint.y() + lineHeight);
        painter->setFont(boldFont);
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
            painter->drawText(textPoint, tr("Date: %1").arg(index.data(PlaylistModel::FIELD_DATE).toString()));
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        const bool doubleHeight = Settings.playlistThumbnails() == "tall"
                                  || Settings.playlistThumbnails() == "large";
        return QSize(m_view->viewport()->width(),
                     PlaylistModel::THUMBNAIL_HEIGHT * (doubleHeight ? 2 : 1) + kTilePaddingPx);
    }

private slots:
    void emitSizeHintChanged()
    {
        emit sizeHintChanged(QModelIndex());
    }

private:
    QAbstractItemView *m_view;

};

class PlaylistProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PlaylistProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    void setMediaTypes(QList<PlaylistModel::MediaType> types)
    {
        m_mediaTypes = types;
        invalidateFilter();
    }

    void setBin(const QString &name = QString())
    {
        m_bin = name;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const
    {
        if (m_mediaTypes.size() > 0 && m_mediaTypes.size() < 4) {
            auto index = sourceModel()->index(row, PlaylistModel::COLUMN_MEDIA_TYPE, parent);
            if (!m_mediaTypes.contains(index.data(Qt::StatusTipRole)))
                return false;
        }

        if (!m_bin.isEmpty()) {
            auto index = sourceModel()->index(row, PlaylistModel::COLUMN_BIN, parent);
            if (index.data(Qt::DisplayRole).toString() != m_bin)
                return false;
        }

        auto filter = filterRegularExpression();
        auto index = sourceModel()->index(row, PlaylistModel::COLUMN_RESOURCE, parent);
        if (index.data(Qt::DisplayRole).toString().contains(filter))
            return true;
        index = sourceModel()->index(row, PlaylistModel::COLUMN_RESOURCE, parent);
        if (index.data(Qt::ToolTipRole).toString().contains(filter))
            return true;
        index = sourceModel()->index(row, PlaylistModel::COLUMN_COMMENT, parent);
        if (index.data(Qt::DisplayRole).toString().contains(filter))
            return true;
        return false;
    }

private:
    QList<PlaylistModel::MediaType> m_mediaTypes {PlaylistModel::Video, PlaylistModel::Audio, PlaylistModel::Image};
    QString m_bin;
};


PlaylistDock::PlaylistDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistDock),
    m_blockResizeColumnsToContents(false)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());

    int width = qRound(150 * devicePixelRatio());
    ui->splitter->setSizes({width, this->width() - width});
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->treeWidget->setVisible(false);
    ui->treeWidget->viewport()->setAcceptDrops(true);
    ui->treeWidget->setDragDropMode(QAbstractItemView::DropOnly);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QWidget::customContextMenuRequested, this, [ = ](const QPoint & pos) {
        QMenu menu(this);
        menu.addAction(Actions["playlistNewBin"]);
        menu.exec(mapToGlobal(pos));
    });
    connect(ui->treeWidget, &BinTree::copied, this, [ = ](const QString & filePath) {
        addFiles(-1, {filePath});
    });
    connect(ui->treeWidget, &BinTree::moved, this, [ = ](const QList<int> &rows,
    const QPointF & point) {
        auto item = ui->treeWidget->itemAt(point.x(), point.y());
        auto bin = item->text(0);
        if (item->data(0, Qt::UserRole).toString() == kSpecialBinDataAll)
            bin = QString();
        MAIN.undoStack()->push(new Playlist::MoveToBinCommand(m_model, ui->treeWidget, bin, rows));
        m_view->selectionModel()->clearSelection();
    });
    auto item = new QTreeWidgetItem(ui->treeWidget, {tr("ALL")});
    item->setData(0, Qt::UserRole, kSpecialBinDataAll);
    item->setSelected(true);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    QFont font;
    font.setItalic(true);
    item->setFont(0, font);
    auto icon = QIcon::fromTheme("quickopen",
                                 QIcon(":/icons/oxygen/32x32/actions/quickopen.png"));
    item->setIcon(0, icon);

    m_proxyModel = new PlaylistProxyModel(this);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSourceModel(&m_model);
    m_proxyModel->setFilterKeyColumn(-1);

    m_selectionModel = new QItemSelectionModel(m_proxyModel, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
            &PlaylistDock::selectionChanged);

    setupActions();

    m_mainMenu = new QMenu(tr("Playlist"), this);
    m_mainMenu->addAction(Actions["playlistOpenAction"]);
    m_mainMenu->addAction(Actions["playlistOpenPreviousAction"]);
    m_mainMenu->addAction(Actions["playlistOpenNextAction"]);
    m_mainMenu->addAction(Actions["playlistGoToAction"]);
    m_mainMenu->addAction(Actions["playlistRemoveCutAction"]);
    m_mainMenu->addAction(Actions["playlistCopyAction"]);
    m_mainMenu->addAction(Actions["playlistInsertCutAction"]);
    m_mainMenu->addAction(Actions["playlistUpdateAction"]);
    m_mainMenu->addAction(Actions["playlistMoveUpAction"]);
    m_mainMenu->addAction(Actions["playlistMoveDownAction"]);
    m_mainMenu->addAction(Actions["playlistUpdateThumbnailsAction"]);
    m_mainMenu->addAction(Actions["playlistSetFileDateAction"]);
    m_mainMenu->addAction(Actions["playlistAddFilesAction"]);
    m_mainMenu->addAction(Actions["playlistAppendCutAction"]);
    m_mainMenu->addSeparator();
    QMenu *selectMenu = m_mainMenu->addMenu(tr("Select"));
    selectMenu->addAction(Actions["playlistSelectAllAction"]);
    selectMenu->addAction(Actions["playlistSelectNoneAction"]);
    selectMenu->addAction(Actions["playlistSelectClip1Action"]);
    selectMenu->addAction(Actions["playlistSelectClip2Action"]);
    selectMenu->addAction(Actions["playlistSelectClip3Action"]);
    selectMenu->addAction(Actions["playlistSelectClip4Action"]);
    selectMenu->addAction(Actions["playlistSelectClip5Action"]);
    selectMenu->addAction(Actions["playlistSelectClip6Action"]);
    selectMenu->addAction(Actions["playlistSelectClip7Action"]);
    selectMenu->addAction(Actions["playlistSelectClip8Action"]);
    selectMenu->addAction(Actions["playlistSelectClip9Action"]);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(Actions["playlistRemoveAllAction"]);
    m_mainMenu->addAction(Actions["playlistAddToTimelineAction"]);
    m_mainMenu->addAction(Actions["playlistAddToSlideshowAction"]);
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(Actions["playlistNewBin"]);
    QMenu *sortByMenu = m_mainMenu->addMenu(tr("Sort"));
    sortByMenu->addAction(Actions["playlistSortByNameAction"]);
    sortByMenu->addAction(Actions["playlistSortByDateAction"]);
    QMenu *columnsMenu = m_mainMenu->addMenu(tr("Columns"));
    columnsMenu->addAction(Actions["playlistColumnsToggleThumbnailsAction"]);
    columnsMenu->addAction(Actions["playlistColumnsToggleClipAction"]);
    columnsMenu->addAction(Actions["playlistColumnsToggleInAction"]);
    columnsMenu->addAction(Actions["playlistColumnsToggleDurationAction"]);
    columnsMenu->addAction(Actions["playlistColumnsToggleStartAction"]);
    columnsMenu->addAction(Actions["playlistColumnsToggleDateAction"]);
    Actions.loadFromMenu(m_mainMenu);

    DockToolBar *toolbar = new DockToolBar(tr("Playlist Controls"));
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(QIcon::fromTheme("show-menu",
                                         QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    menuButton->setToolTip(tr("Playlist Menu"));
    menuButton->setAutoRaise(true);
    menuButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    toolbar->addWidget(menuButton);
    toolbar->addSeparator();
    toolbar->addAction(Actions["playlistAppendCutAction"]);
    toolbar->addAction(Actions["playlistRemoveCutAction"]);
    toolbar->addAction(Actions["playlistAddFilesAction"]);
    toolbar->addAction(Actions["playlistUpdateAction"]);
    toolbar->addSeparator();
    toolbar->addAction(Actions["playlistViewDetailsAction"]);
    toolbar->addAction(Actions["playlistViewTilesAction"]);
    toolbar->addAction(Actions["playlistViewIconsAction"]);
    toolbar->addSeparator();
    ui->verticalLayout->addWidget(toolbar);
    ui->verticalLayout->addSpacing(2);

    toolbar = new DockToolBar(tr("Playlist Filters"));
    toolbar->addAction(Actions["playlistBinView"]);
    ui->filtersLayout->addWidget(toolbar);

    auto toolbar2 = new QToolBar(tr("Playlist Filters"));
    toolbar2->setStyleSheet(
        QStringLiteral("QToolButton:checked { color:palette(highlighted-text); background-color:palette(highlight);}"));
    ui->filtersLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    toolbar2->addActions({Actions["playlistFiltersVideo"], Actions["playlistFiltersAudio"], Actions["playlistFiltersImage"], Actions["playlistFiltersOther"]});
    ui->filtersLayout->addWidget(toolbar2);
    auto lineEdit = new LineEditClear(this);
    lineEdit->setToolTip(tr("Only show files whose name, path, or comment contains some text"));
    lineEdit->setPlaceholderText(tr("search"));
    connect(lineEdit, &QLineEdit::textChanged, this, [ = ](const QString & search) {
        m_proxyModel->setFilterFixedString(search);
    });
    ui->filtersLayout->addWidget(lineEdit, 1);

    ui->stackedWidget->setCurrentIndex(0);

    m_iconsView = new PlaylistIconView(this);
    ui->listView->parentWidget()->layout()->addWidget(m_iconsView);
    m_iconsView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setModel(m_proxyModel);
    ui->listView->setModel(m_proxyModel);
    m_iconsView->setModel(m_proxyModel);
    ui->tableView->setSelectionModel(m_selectionModel);
    ui->listView->setSelectionModel(m_selectionModel);
    m_iconsView->setSelectionModel(m_selectionModel);

    QList<QAbstractItemView *> views;
    views << ui->tableView;
    views << ui->listView;
    views << m_iconsView;
    for (auto view : views) {
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
        Actions["playlistThumbnailsLeftAndRightAction"]->trigger();
    } else if (thumbs == "tall") {
        Actions["playlistThumbnailsTopAndBottomAction"]->trigger();
    } else if (thumbs == "small") {
        Actions["playlistThumbnailsInOnlySmallAction"]->trigger();
    } else if (thumbs == "large") {
        Actions["playlistThumbnailsInOnlyLargeAction"]->trigger();
    } else {
        Actions["playlistThumbnailsHiddenAction"]->trigger();
    }

    if (Settings.viewMode() == kDetailedMode) {
        Actions["playlistViewDetailsAction"]->trigger();
    } else if (Settings.viewMode() == kTiledMode) {
        Actions["playlistViewTilesAction"]->trigger();
    } else { /* if (Settings.viewMode() == kIconsMode) */
        Actions["playlistViewIconsAction"]->trigger();
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
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_A));
    action->setToolTip(tr("Add the Source to the playlist"));
    icon = QIcon::fromTheme("list-add",
                            QIcon(":/icons/oxygen/32x32/actions/list-add.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAppendCutActionTriggered);
    Actions.add("playlistAppendCutAction", action);

    action = new QAction(tr("Remove"), this);
    QList<QKeySequence> removeShortcuts;
    removeShortcuts << QKeySequence(Qt::SHIFT | Qt::Key_X);
    removeShortcuts << QKeySequence(Qt::SHIFT | Qt::Key_Z);
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
    Actions.add("playlistRemoveCutAction", action);

    action = new QAction(tr("Add Files"), this);
    action->setToolTip(tr("Add files to playlist"));
    icon = QIcon::fromTheme("list-add-files",
                            QIcon(":/icons/oxygen/32x32/actions/list-add-files.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddFilesActionTriggered);
    Actions.add("playlistAddFilesAction", action);

    action = new QAction(tr("Update"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_B));
    icon = QIcon::fromTheme("dialog-ok",
                            QIcon(":/icons/oxygen/32x32/actions/dialog-ok.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, &PlaylistDock::onUpdateActionTriggered);
    connect(this, &PlaylistDock::enableUpdate, action, &QAction::setEnabled);
    Actions.add("playlistUpdateAction", action);

    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    action = new QAction(tr("Tiles"), this);
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
    Actions.add("playlistViewTilesAction", action);

    action = new QAction(tr("Icons"), this);
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
    Actions.add("playlistViewIconsAction", action);

    action = new QAction(tr("Details"), this);
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
    Actions.add("playlistViewDetailsAction", action);

    action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the clip in the Source player"));
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onOpenActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistOpenAction", action);

    action = new QAction(tr("GoTo"), this);
    action->setToolTip(tr("Go to the start of this clip in the Project player"));
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
    connect(action, &QAction::triggered, this, &PlaylistDock::onGotoActionTriggered);
    Actions.add("playlistGoToAction", action);

    action = new QAction(tr("Copy"), this);
    action->setToolTip(tr("Open a copy of the clip in the Source player"));
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_C));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onCopyActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistCopyAction", action);

    action = new QAction(tr("Insert"), this);
    action->setToolTip(tr("Insert"));
    action->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_V));
    connect(action, &QAction::triggered, this, &PlaylistDock::onInsertCutActionTriggered);
    Actions.add("playlistInsertCutAction", action);

    action = new QAction(tr("Update Thumbnails"), this);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onUpdateThumbnailsActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 0);
    });
    Actions.add("playlistUpdateThumbnailsAction", action);

    action = new QAction(tr("Set Creation Time..."), this);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &PlaylistDock::onSetFileDateActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistSetFileDateAction", action);

    action = new QAction(tr("Remove All"), this);
    action->setToolTip(tr("Remove all items from the playlist"));
    connect(action, &QAction::triggered, this, &PlaylistDock::onRemoveAllActionTriggered);
    action->setEnabled(m_model.rowCount() > 0);
    Actions.add("playlistRemoveAllAction", action);

    action = new QAction(tr("Select All"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    connect(action, &QAction::triggered, this, &PlaylistDock::onSelectAllActionTriggered);
    action->setEnabled(m_proxyModel->rowCount() > 0);
    Actions.add("playlistSelectAllAction", action);

    action = new QAction(tr("Select None"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(action, &QAction::triggered, m_selectionModel, &QItemSelectionModel::clearSelection);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    Actions.add("playlistSelectNoneAction", action);

    action = new QAction(tr("Move Up"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));
    connect(action, &QAction::triggered, m_selectionModel, [ = ]() {
        raise();
        moveClipUp();
        decrementIndex();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistMoveUpAction", action);

    action = new QAction(tr("Move Down"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));
    connect(action, &QAction::triggered, m_selectionModel, [ = ]() {
        raise();
        moveClipDown();
        incrementIndex();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistMoveDownAction", action);

    action = new QAction(tr("Add Selected to Timeline"), this);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddToTimelineActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    Actions.add("playlistAddToTimelineAction", action);

    action = new QAction(tr("Add Selected to Slideshow"), this);
    connect(action, &QAction::triggered, this, &PlaylistDock::onAddToSlideshowActionTriggered);
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    Actions.add("playlistAddToSlideshowAction", action);

    action = new QAction(tr("Sort By Name"), this);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        resetPlaylistIndex();
        MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_RESOURCE,
                                                         Qt::AscendingOrder));
    });
    Actions.add("playlistSortByNameAction", action);

    action = new QAction(tr("Sort By Date"), this);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        resetPlaylistIndex();
        MAIN.undoStack()->push(new Playlist::SortCommand(m_model, PlaylistModel::COLUMN_DATE,
                                                         Qt::AscendingOrder));
    });
    Actions.add("playlistSortByDateAction", action);

    QActionGroup *thumbnailGroup = new QActionGroup(this);
    thumbnailGroup->setExclusive(true);

    action = new QAction(tr("Hidden"), this);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&](bool checked) {
        if (checked) {
            Settings.setPlaylistThumbnails("hidden");
            ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, true);
            ui->tableView->verticalHeader()->setDefaultSectionSize(m_defaultRowHeight);
        }
    });
    thumbnailGroup->addAction(action);
    Actions.add("playlistThumbnailsHiddenAction", action);

    action = new QAction(tr("In and Out - Left/Right"), this);
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
    Actions.add("playlistThumbnailsLeftAndRightAction", action);

    action = new QAction(tr("In and Out - Top/Bottom"), this);
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
    Actions.add("playlistThumbnailsTopAndBottomAction", action);

    action = new QAction(tr("In Only - Small"), this);
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
    Actions.add("playlistThumbnailsInOnlySmallAction", action);

    action = new QAction(tr("In Only - Large"), this);
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
    Actions.add("playlistThumbnailsInOnlyLargeAction", action);

    action = new QAction(tr("Play After Open"), this);
    action->setCheckable(true);
    action->setChecked(Settings.playlistAutoplay());
    connect(action, &QAction::triggered, this, [&](bool checked) {
        Settings.setPlaylistAutoplay(checked);
    });
    Actions.add("playlistPlayAfterOpenAction", action);

    action = new QAction(tr("Open Previous"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        decrementIndex();
        onOpenActionTriggered();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistOpenPreviousAction", action);

    action = new QAction(tr("Open Next"), this);
    action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        incrementIndex();
        onOpenActionTriggered();
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid() && m_model.playlist());
    });
    Actions.add("playlistOpenNextAction", action);

    action = new QAction(tr("Select Clip 1"), this);
    action->setShortcut(QKeySequence(Qt::Key_1));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(0);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 0);
    });
    Actions.add("playlistSelectClip1Action", action);
    \

    action = new QAction(tr("Select Clip 2"), this);
    action->setShortcut(QKeySequence(Qt::Key_2));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(1);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 1);
    });
    Actions.add("playlistSelectClip2Action", action);

    action = new QAction(tr("Select Clip 3"), this);
    action->setShortcut(QKeySequence(Qt::Key_3));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(2);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 2);
    });
    Actions.add("playlistSelectClip3Action", action);

    action = new QAction(tr("Select Clip 4"), this);
    action->setShortcut(QKeySequence(Qt::Key_4));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(3);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 3);
    });
    Actions.add("playlistSelectClip4Action", action);

    action = new QAction(tr("Select Clip 5"), this);
    action->setShortcut(QKeySequence(Qt::Key_5));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(4);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 4);
    });
    Actions.add("playlistSelectClip5Action", action);

    action = new QAction(tr("Select Clip 6"), this);
    action->setShortcut(QKeySequence(Qt::Key_6));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(5);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 5);
    });
    Actions.add("playlistSelectClip6Action", action);

    action = new QAction(tr("Select Clip 7"), this);
    action->setShortcut(QKeySequence(Qt::Key_7));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(6);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 6);
    });
    Actions.add("playlistSelectClip7Action", action);

    action = new QAction(tr("Select Clip 8"), this);
    action->setShortcut(QKeySequence(Qt::Key_8));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(7);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 7);
    });
    Actions.add("playlistSelectClip8Action", action);

    action = new QAction(tr("Select Clip 9"), this);
    action->setShortcut(QKeySequence(Qt::Key_9));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        setIndex(8);
    });
    connect(this, &PlaylistDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_proxyModel->rowCount() > 8);
    });
    Actions.add("playlistSelectClip9Action", action);

    action = new QAction(tr("Thumbnails"), this);
    action->setChecked(Settings.playlistShowColumn("thumbnails"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("thumbnails", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL, !checked);
    });
    Actions.add("playlistColumnsToggleThumbnailsAction", action);

    action = new QAction(tr("Clip"), this);
    action->setChecked(Settings.playlistShowColumn("clip"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("clip", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_RESOURCE, !checked);
    });
    Actions.add("playlistColumnsToggleClipAction", action);

    action = new QAction(tr("In"), this);
    action->setChecked(Settings.playlistShowColumn("in"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("in", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_IN, !checked);
    });
    Actions.add("playlistColumnsToggleInAction", action);

    action = new QAction(tr("Duration"), this);
    action->setChecked(Settings.playlistShowColumn("duration"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("duration", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_DURATION, !checked);
    });
    Actions.add("playlistColumnsToggleDurationAction", action);

    action = new QAction(tr("Start"), this);
    action->setChecked(Settings.playlistShowColumn("start"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("start", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_START, !checked);
    });
    Actions.add("playlistColumnsToggleStartAction", action);

    action = new QAction(tr("Date"), this);
    action->setChecked(Settings.playlistShowColumn("date"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        Settings.setPlaylistShowColumn("date", checked);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_DATE, !checked);
    });
    Actions.add("playlistColumnsToggleDateAction", action);

    action = new QAction(tr("Video"), this);
    action->setToolTip(tr("Show or hide video files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &PlaylistDock::onMediaTypeClicked);
    Actions.add("playlistFiltersVideo", action, this->windowTitle());

    action = new QAction(tr("Audio"), this);
    action->setToolTip(tr("Show or hide audio files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &PlaylistDock::onMediaTypeClicked);
    Actions.add("playlistFiltersAudio", action, this->windowTitle());

    action = new QAction(tr("Image"), this);
    action->setToolTip(tr("Show or hide image files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &PlaylistDock::onMediaTypeClicked);
    Actions.add("playlistFiltersImage", action, this->windowTitle());

    action = new QAction(tr("Other"), this);
    action->setToolTip(tr("Show or hide other kinds of files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &PlaylistDock::onMediaTypeClicked);
    Actions.add("playlistFiltersOther", action, this->windowTitle());

    action = new QAction(tr("New Bin"), this);
    action->setToolTip(tr("New Bin"));
    icon = QIcon::fromTheme("folder-new",
                            QIcon(":/icons/oxygen/32x32/actions/folder-new.png"));
    action->setIcon(icon);
    action->setDisabled(true);
    Actions.add("playlistNewBin", action, windowTitle());
    connect(action, &QAction::triggered, this, [ = ]() {
        QInputDialog dialog(this);
        dialog.setInputMode(QInputDialog::TextInput);
        dialog.setWindowTitle(action->text());
        dialog.setLabelText(tr("Name"));
        dialog.setWindowModality(QmlApplication::dialogModality());
        auto result = dialog.exec();
        auto name = dialog.textValue();
        if (result == QDialog::Accepted && !name.isEmpty()) {
            if (ui->treeWidget->findItems(name, Qt::MatchExactly).isEmpty())
                MAIN.undoStack()->push(new Playlist::NewBinCommand(m_model, ui->treeWidget, name));
        }
    });

    action = new QAction(tr("Bins"), this);
    action->setToolTip(tr("Show or hide the list of bins"));
    icon = QIcon::fromTheme("view-choose",
                            QIcon(":/icons/oxygen/32x32/actions/view-choose.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        ui->treeWidget->setVisible(checked);
        Actions["playlistNewBin"]->setEnabled(checked);
    });
    Actions.add("playlistBinView", action, windowTitle());
}

int PlaylistDock::position()
{
    int result = -1;
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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

void PlaylistDock::getSelectionRange(int *start, int *end)
{
    Mlt::Playlist *playlist = m_model.playlist();
    if (!playlist || !m_view->selectionModel()
            || !m_view->selectionModel()->selectedIndexes().size() ) {
        *start = -1;
        *end = -1;
        return;
    }
    // Find the earliest start and the latest end in the selection
    *start = std::numeric_limits<int>::max();
    *end = -1;
    for (const auto &index : m_view->selectionModel()->selectedIndexes()) {
        int row = m_proxyModel->mapToSource(index).row();
        int clipStart = playlist->clip_start(row);
        int clipEnd = clipStart + playlist->clip_length(row);
        *start = qMin(*start, clipStart);
        *end = qMax(*end, clipEnd);
    }
}

void PlaylistDock::incrementIndex()
{
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.incrementIndex(index);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::decrementIndex()
{
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
    if (!index.isValid())
        index = m_model.createIndex(0, 0);
    else
        index = m_model.decrementIndex(index);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::setIndex(int row)
{
    const auto index = m_proxyModel->index(row, 0);
    if (index.isValid())
        m_view->setCurrentIndex(index);
}

void PlaylistDock::moveClipUp()
{
    int row = m_proxyModel->mapToSource(m_view->currentIndex()).row();
    if (row > 0) {
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row - 1));
        resetPlaylistIndex();
    }
}

void PlaylistDock::moveClipDown()
{
    int row = m_proxyModel->mapToSource(m_view->currentIndex()).row();
    if (row + 1 < m_model.rowCount()) {
        MAIN.undoStack()->push(new Playlist::MoveCommand(m_model, row, row + 1));
        resetPlaylistIndex();
    }
}

void PlaylistDock::addFiles(int row, const QList<QUrl> &urls)
{
    auto resetIndex = true;
    ResourceDialog dialog(this);
    LongUiTask longTask(tr("Add Files"));
    int insertNextAt = row;
    bool first = true;
    QStringList fileNames = Util::sortedFileList(Util::expandDirectories(urls));
    qsizetype i = 0, count = fileNames.size();
    for (auto &path : fileNames) {
        if (MAIN.isSourceClipMyProject(path)) continue;
        longTask.reportProgress(Util::baseName(path), i++, count);
        if (MLT.checkFile(path)) {
            emit showStatusMessage(tr("Failed to open ").append(path));
            continue;
        }
        Mlt::Producer p;
        if (path.endsWith(".mlt") || path.endsWith(".xml")) {
            if (Settings.playerGPU() && MLT.profile().is_explicit()) {
                Mlt::Profile testProfile;
                Mlt::Producer producer(testProfile, path.toUtf8().constData());
                if (testProfile.width() != MLT.profile().width()
                        || testProfile.height() != MLT.profile().height()
                        || Util::isFpsDifferent(MLT.profile().fps(), testProfile.fps())) {
                    emit showStatusMessage(tr("Failed to open ").append(path));
                    continue;
                }
            }
            p = Mlt::Producer(MLT.profile(), path.toUtf8().constData());
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
                    Mlt::Properties properties;
                    properties.set(kShotcutSkipConvertProperty, 1);
                    MAIN.open(path, &properties, false);
                    if (MLT.producer() && MLT.producer()->is_valid()) {
                        producer = MLT.producer();
                        first = true;
                    }
                }
            }
            producer = MLT.setupNewProducer(producer);
            producer->set(kShotcutSkipConvertProperty, 1);
            if (ui->treeWidget->topLevelItemCount() > 1) {
                auto bin = ui->treeWidget->selectedItems().first()->text(0);
                producer->set(kShotcutBinsProperty, bin.toUtf8().constData());
            }
            if (!MLT.isLiveProducer(producer) || producer->get_int(kShotcutVirtualClip)) {
                ProxyManager::generateIfNotExists(*producer);
                if (row == -1)
                    MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML(producer)));
                else
                    MAIN.undoStack()->push(new Playlist::InsertCommand(m_model, MLT.XML(producer), insertNextAt++));
            } else {
                LongUiTask::cancel();
                DurationDialog durationDialog(this);
                durationDialog.setDuration(MLT.profile().fps() * 5);
                if (durationDialog.exec() == QDialog::Accepted) {
                    producer->set_in_and_out(0, durationDialog.duration() - 1);
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
            dialog.add(producer);
            delete producer;
        }
    }
    if (Settings.showConvertClipDialog() && dialog.producerCount() > 1 && dialog.hasTroubleClips()) {
        dialog.selectTroubleClips();
        dialog.setWindowTitle(tr("Dropped Files"));
        longTask.cancel();
        dialog.exec();
    } else if (Settings.showConvertClipDialog() && dialog.producerCount() == 1) {
        Mlt::Producer producer = dialog.producer(0);
        QString convertAdvice = Util::getConversionAdvice(&producer);
        if (!convertAdvice.isEmpty()) {
            longTask.cancel();
            Util::offerSingleFileConversion(convertAdvice, &producer, this);
        }
    }
    if (resetIndex)
        resetPlaylistIndex();
}

void PlaylistDock::loadBins()
{
    // clear the tree view except top item
    while (ui->treeWidget->topLevelItemCount() > 1)
        delete ui->treeWidget->takeTopLevelItem(1);

    auto props = m_model.playlist()->get_props(kShotcutBinsProperty);
    if (props && props->is_valid()) {
        for (int i = 0; i < props->count(); ++i) {
            auto name = QString::fromUtf8(props->get_name(i));
            if (!name.startsWith('_') && !name.startsWith('.')) {
                name = QString::fromUtf8(props->get(i));
                auto item = new QTreeWidgetItem(ui->treeWidget, {name});
                auto icon = QIcon::fromTheme("folder",
                                             QIcon(":/icons/oxygen/32x32/places/folder.png"));
                item->setIcon(0, icon);
            }
        }
        if (ui->treeWidget->topLevelItemCount() > 1 && !ui->treeWidget->isVisible()) {
            Actions["playlistBinView"]->trigger();
            ui->treeWidget->topLevelItem(0)->setSelected(true);
            sortBins();
        }
    }
}

void PlaylistDock::sortBins()
{
    auto all = ui->treeWidget->takeTopLevelItem(0);
    ui->treeWidget->sortItems(0, Qt::AscendingOrder);
    ui->treeWidget->insertTopLevelItem(0, all);
}

void PlaylistDock::onInsertCutActionTriggered()
{
    if (MLT.isClip() || MLT.savedProducer()) {
        show();
        raise();
        QMimeData mimeData;
        mimeData.setData(Mlt::XmlMimeType, MLT.XML(
                             MLT.isClip() ? nullptr : MLT.savedProducer()).toUtf8());
        auto index = m_proxyModel->mapToSource(m_view->currentIndex());
        onDropped(&mimeData, index.row());
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
            emit enableUpdate(true);
        } else {
            DurationDialog dialog(this);
            dialog.setDuration(MLT.profile().fps() * 5);
            if (dialog.exec() == QDialog::Accepted) {
                producer.set_in_and_out(0, dialog.duration() - 1);
                if (producer.get("mlt_service") && !strcmp(producer.get("mlt_service"), "avformat"))
                    producer.set("mlt_service", "avformat-novalidate");
                MAIN.undoStack()->push(new Playlist::AppendCommand(m_model, MLT.XML()));
                setPlaylistIndex(&producer, m_model.playlist()->count() - 1);
                emit enableUpdate(true);
            }
        }
    }
}

void PlaylistDock::onUpdateActionTriggered()
{
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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
            emit enableUpdate(true);
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
                emit enableUpdate(true);
            }
        }
    } else {
        emit showStatusMessage(tr("You cannot insert a playlist into a playlist!"));
        emit enableUpdate(false);
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
    for (const auto &index : m_view->selectionModel()->selectedIndexes()) {
        int row = m_proxyModel->mapToSource(index).row();
        if (!rowsRemoved.contains(row)) {
            int adjustment = 0;
            for (const int i : rowsRemoved) {
                if (row > i)
                    --adjustment;
            }
            rowsRemoved << row;
            row += adjustment;
            if (m_model.playlist()->clip_length(row) > 0)
                MAIN.undoStack()->push(new Playlist::RemoveCommand(m_model, row));
        }
    }
    if (n > 1)
        MAIN.undoStack()->endMacro();
    if (rowsRemoved.contains(MLT.producer()->get_int(kPlaylistIndexProperty))) {
        // Remove the playlist index property on the producer.
        resetPlaylistIndex();
        emit enableUpdate(false);
    }
}

void PlaylistDock::onSetFileDateActionTriggered()
{
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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
    if (MLT.producer() && MLT.producer()->is_valid() && m_proxyModel) {
        auto row = MLT.producer()->get_int(kPlaylistIndexProperty) - 1;
        if (row < 0 && m_model.rowCount() > 0) {
            resetPlaylistIndex();
            emit m_model.dataChanged(m_model.createIndex(0, PlaylistModel::COLUMN_THUMBNAIL),
                                     m_model.createIndex(m_model.playlist()->count() - 1, PlaylistModel::COLUMN_THUMBNAIL),
                                     QVector<int>() << PlaylistModel::COLUMN_THUMBNAIL);
            loadBins();
        }
    }
    emit producerOpened();
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
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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
        menu.addAction(Actions["playlistOpenAction"]);
        menu.addAction(Actions["playlistGoToAction"]);
        menu.addAction(Actions["playlistRemoveCutAction"]);
        menu.addAction(Actions["playlistCopyAction"]);
        menu.addAction(Actions["playlistInsertCutAction"]);
        menu.addAction(Actions["playlistUpdateAction"]);
        menu.addAction(Actions["playlistUpdateThumbnailsAction"]);
        menu.addAction(Actions["playlistUpdateThumbnailsAction"]);
        menu.addAction(Actions["playlistSetFileDateAction"]);
        menu.exec(mapToGlobal(pos));
    }
}


void PlaylistDock::viewDoubleClicked(const QModelIndex &index)
{
    if (!m_model.playlist()) return;
    auto index2 = m_proxyModel->mapToSource(index);
    Mlt::ClipInfo *i = m_model.playlist()->clip_info(index2.row());
    if (i) {
        if (qApp->keyboardModifiers() == Qt::ShiftModifier) {
            emit itemActivated(i->start);
        } else {
            Mlt::Producer *p = new Mlt::Producer(i->producer);
            p->set_in_and_out(i->frame_in, i->frame_out);
            setPlaylistIndex(p, index2.row());
            emit clipOpened(p, Settings.playlistAutoplay());
        }
        delete i;
        m_iconsView->resetMultiSelect();
    }
}

void PlaylistDock::onGotoActionTriggered()
{
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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
    emit enableUpdate(false);
    updateViewMode();
    ui->stackedWidget->setCurrentIndex(1);
}

void PlaylistDock::onPlaylistLoaded()
{
    onPlaylistCreated();
    bool nonEmptyModel = m_model.rowCount() > 0;
    Actions["playlistRemoveAllAction"]->setEnabled(nonEmptyModel);
    Actions["playlistSelectAllAction"]->setEnabled(nonEmptyModel);
}

void PlaylistDock::onPlaylistModified()
{
    if (!m_blockResizeColumnsToContents) {
        ui->tableView->resizeColumnsToContents();
        m_blockResizeColumnsToContents = true;
    }
    bool nonEmptyModel = m_model.rowCount() > 0;
    Actions["playlistRemoveAllAction"]->setEnabled(nonEmptyModel);
    Actions["playlistSelectAllAction"]->setEnabled(nonEmptyModel);
}

void PlaylistDock::onPlaylistCleared()
{
    auto all = ui->treeWidget->takeTopLevelItem(0);
    ui->treeWidget->clear();
    ui->treeWidget->addTopLevelItem(all);

    emit enableUpdate(false);
    m_blockResizeColumnsToContents = false;
    bool nonEmptyModel = m_model.rowCount() > 0;
    Actions["playlistRemoveAllAction"]->setEnabled(nonEmptyModel);
    Actions["playlistSelectAllAction"]->setEnabled(nonEmptyModel);
}

void PlaylistDock::onDropped(const QMimeData *data, int row)
{
    if (data && data->hasUrls()) {
        // Use QTimer to workaround stupid drag from Windows Explorer bug
        const auto &urls = data->urls();
        QTimer::singleShot(0, this, [ = ]() {
            addFiles(row, urls);
        });
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
                emit enableUpdate(true);
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
    for (const auto &index : indexes) {
        if (index.column()) continue;
        auto row = m_proxyModel->mapToSource(index).row();
        QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
        if (info && info->producer) {
            playlist.append(*info->producer, info->frame_in, info->frame_out);
        }
    }
    emit addAllTimeline(&playlist);
}

void PlaylistDock::onAddToSlideshowActionTriggered()
{
    MLT.pause();
    const QModelIndexList &indexes = m_view->selectionModel()->selectedIndexes();
    Mlt::Playlist playlist(MLT.profile());
    for (const auto &index : indexes) {
        if (index.column()) continue;
        auto row = m_proxyModel->mapToSource(index).row();
        QScopedPointer<Mlt::ClipInfo> info(m_model.playlist()->clip_info(row));
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
        Mlt::Playlist *slideshow = longTask.runAsync<Mlt::Playlist *>(tr("Generating"),
                                                                      &SlideshowGeneratorDialog::getSlideshow, &dialog);
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
    emit enableUpdate(false);
}

void PlaylistDock::updateViewMode()
{
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
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_INDEX, true);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_COMMENT, true);
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_THUMBNAIL,
                                       !Settings.playlistShowColumn("thumbnails"));
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_RESOURCE,
                                       !Settings.playlistShowColumn("clip"));
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_IN, !Settings.playlistShowColumn("in"));
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_DURATION,
                                       !Settings.playlistShowColumn("duration"));
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_START, !Settings.playlistShowColumn("start"));
        ui->tableView->setColumnHidden(PlaylistModel::COLUMN_DATE, !Settings.playlistShowColumn("date"));
        ui->tableView->resizeColumnsToContents();
        ui->tableView->show();
        ui->tableView->resizeColumnsToContents();
    } else if (mode == kTiledMode) {
        m_model.setViewMode(PlaylistModel::Tiled);
        m_view = ui->listView;
        ui->listView->setDragEnabled(true);
        ui->listView->setItemDelegate(new TiledItemDelegate(ui->listView));
        ui->listView->show();
    } else { /* if (mode == kIconsMode) */
        m_view = m_iconsView;
        m_model.setViewMode(PlaylistModel::Icons);
        m_iconsView->show();
    }
}

void PlaylistDock::resetPlaylistIndex()
{
    if (!m_model.playlist() || !m_model.playlist()->is_valid())
        return;
    if (MLT.producer())
        MLT.producer()->set(kPlaylistIndexProperty, nullptr, 0);
    // Clear the old values
    for (int j = 0; j < m_model.playlist()->count(); ++j) {
        Mlt::Producer clip(m_model.playlist()->get_clip(j));
        clip.parent().Mlt::Properties::clear(kPlaylistIndexProperty);
    }
    emit enableUpdate(false);
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
    auto index = m_proxyModel->mapToSource(m_view->currentIndex());
    onMoveClip(index.row(), model()->rowCount());
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
        emit enableUpdate(false);
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
        emit enableUpdate(false);
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
    QModelIndex index = m_proxyModel->mapToSource(m_view->currentIndex());
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
    for (int i = 0; i < m_proxyModel->rowCount(); i++) {
        m_view->selectionModel()->select(m_proxyModel->index(i, 0),
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
    emit enableUpdate(true);

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
        for (const auto &s : filenames) {
            urls << s;
        }
        mimeData.setUrls(urls);
        auto index = m_proxyModel->mapToSource(m_view->currentIndex());
        onDropped(&mimeData, index.row() + 1);
    }
}

void PlaylistDock::onMediaTypeClicked()
{
    QList<PlaylistModel::MediaType> types;
    if (Actions["playlistFiltersVideo"]->isChecked())
        types << PlaylistModel::Video;
    if (Actions["playlistFiltersAudio"]->isChecked())
        types << PlaylistModel::Audio;
    if (Actions["playlistFiltersImage"]->isChecked())
        types << PlaylistModel::Image;
    if (Actions["playlistFiltersOther"]->isChecked())
        types << PlaylistModel::Other;
    m_proxyModel->setMediaTypes(types);
}

void PlaylistDock::on_treeWidget_itemPressed(QTreeWidgetItem *item, int column)
{
    auto data = item->data(0, Qt::UserRole).toString();
    if (data == kSpecialBinDataAll) {
        m_proxyModel->setBin();
    } else {
        m_proxyModel->setBin(item->text(0));
    }
}

void PlaylistDock::on_treeWidget_itemSelectionChanged()
{
    // select ALL if a bin is deselected
    if (ui->treeWidget->selectedItems().size() < 1) {
        ui->treeWidget->topLevelItem(0)->setSelected(true);
        m_proxyModel->setBin();
    }
}

void BinTree::dropEvent(QDropEvent *event)
{
    auto position = dropIndicatorPosition();
    if (position == QAbstractItemView::BelowItem || position == QAbstractItemView::AboveItem)
        return;

    const auto mimeType = QLatin1String("application/x-qabstractitemmodeldatalist");

    if (event->mimeData()->hasFormat(mimeType)) {
        if (Qt::CopyAction == event->dropAction()) {
            auto encoded = event->mimeData()->data(mimeType);
            QDataStream stream(&encoded, QIODevice::ReadOnly);
            QMap<int, QVariant> roleDataMap;
            while (!stream.atEnd()) {
                int row, col;
                stream >> row >> col >> roleDataMap;
            }
            if (roleDataMap.contains(Qt::ToolTipRole)) {
                emit copied(roleDataMap[Qt::ToolTipRole].toString());
                event->acceptProposedAction();
            }
        } else if (Qt::MoveAction == event->dropAction()) {
            auto encoded = event->mimeData()->data(mimeType);
            QDataStream stream(&encoded, QIODevice::ReadOnly);
            QMap<int, QVariant> roleDataMap;
            QList<int> rows;
            while (!stream.atEnd()) {
                int row, col;
                stream >> row >> col >> roleDataMap;
                rows << row;
            }
            if (rows.size() > 0) {
                emit moved(rows, event->position());
                event->acceptProposedAction();
            }
        }
    }
    return QTreeWidget::dropEvent(event);
}
