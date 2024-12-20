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

#include "filesdock.h"
#include "ui_filesdock.h"
#include "actions.h"
#include "mainwindow.h"
#include "settings.h"
#include "widgets/docktoolbar.h"
#include "widgets/playlisticonview.h"
#include "widgets/playlisttable.h"
#include "widgets/playlistlistview.h"
#include "util.h"
#include "qmltypes/qmlapplication.h"
#include "widgets/lineeditclear.h"
#include "models/playlistmodel.h"
#include "database.h"
#include <Logger.h>

#include <QItemSelectionModel>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QPainter>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDir>
#include <QGuiApplication>
#include <QClipboard>
#include <QActionGroup>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QInputDialog>
#include <QStandardPaths>
#include <QRunnable>
#include <QThreadPool>
#include <QMutexLocker>

static const auto kInOutChangedTimeoutMs = 100;
static const auto kTilePaddingPx = 10;
static const auto kDetailedMode = QLatin1String("detailed");
static const auto kIconsMode = QLatin1String("icons");
static const auto kTiledMode = QLatin1String("tiled");


static void cacheThumbnail(FilesModel *model, const QString &filePath, const QString &cacheKey,
                           const QImage &image, const QModelIndex &index);

class FilesThumbnailTask : public QRunnable
{
    FilesModel *m_model;
    QString m_filePath;
    QModelIndex m_index;
    bool m_force;

public:
    FilesThumbnailTask(FilesModel *model, const QString &filePath, const QModelIndex &index,
                       bool force = false)
        : QRunnable()
        , m_model(model)
        , m_filePath(filePath)
        , m_index(index)
        , m_force(force)
    {
    }

private:
    QString cacheKey(Mlt::Producer producer, int frameNumber) const
    {
        QString time = producer.frames_to_time(frameNumber, mlt_time_clock);
        // Reduce the precision to centiseconds to increase chance for cache hit
        // without much loss of accuracy.
        time = time.left(time.size() - 1);
        QString key;
        QString resource = Util::getHash(producer);
        if (resource.isEmpty()) {
            key = QStringLiteral("%1 %2 %3").arg(producer.get("mlt_service"), producer.get("resource"), time);
            QCryptographicHash hash(QCryptographicHash::Sha1);
            hash.addData(key.toUtf8());
            key = hash.result().toHex();
        } else {
            key = QStringLiteral("%1 %2").arg(resource, time);
        }
        return key;
    }

    bool isValidService(Mlt::Producer &producer) const
    {
        if (producer.is_valid()) {
            auto service = QString::fromLatin1(producer.get("mlt_service"));
            return (service.startsWith("avformat") ||
                    service == "qimage" ||
                    service == "pixbuf" ||
                    service == "glaxnimate");
        }
        return false;
    }

public:
    void run()
    {
        LOG_DEBUG() << "Mlt::Producer" << m_filePath;
        QImage image;
        auto key = QStringLiteral("X");
        static Mlt::Profile profile {"atsc_720p_60"};
        Mlt::Producer producer(profile, "abnormal", m_filePath.toUtf8().constData());
        int frame = 0;
        if (producer.is_valid())
            key = cacheKey(producer, frame);
        image = DB.getThumbnail(key);
        if (image.isNull() && isValidService(producer)) {
            Mlt::Filter scaler(profile, "swscale");
            Mlt::Filter padder(profile, "resize");
            Mlt::Filter converter(profile, "avcolor_space");
            producer.attach(scaler);
            producer.attach(padder);
            producer.attach(converter);

            auto width = PlaylistModel::THUMBNAIL_WIDTH * 2;
            auto height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
            image = MLT.image(producer, frame, width, height);
        }
        cacheThumbnail(m_model, m_filePath, key, image, m_index);
    }
};

class FilesModel : public QFileSystemModel
{
public:
    enum Roles {
        DateRole = QFileSystemModel::FilePermissions + 1,
        MediaTypeRole,
        MediaTypeStringRole,
        ThumbnailRole,
    };

    explicit FilesModel(FilesDock *parent = nullptr)
        : QFileSystemModel(parent)
        , m_dock(parent)
    {
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (MediaTypeStringRole == role || (index.column() == 2 && Qt::DisplayRole == role)) {
            static QString names[] = {
                tr("Video"),
                tr("Image"),
                tr("Audio"),
                tr("Other"),
            };
            return names[mediaType(index)];
        }
        switch (role) {
        case Qt::ToolTipRole:
            return filePath(index);
        case DateRole: {
            QFileInfo info(filePath(index));
            return info.lastModified();
        }
        case MediaTypeRole:
            return mediaType(index);
        case ThumbnailRole: {
            int width = PlaylistModel::THUMBNAIL_WIDTH;
            QImage image;
            auto path = filePath(index);
            auto thumbnailKey = m_dock->getCacheThumbnailKey(path);

            if (!thumbnailKey.isEmpty())
                image = DB.getThumbnail(thumbnailKey);

            if (image.isNull()) {
                QThreadPool::globalInstance()->start(
                    new FilesThumbnailTask(const_cast<FilesModel *>(this), path, index));
                image = QImage(width, width, QImage::Format_ARGB32);
                image.fill(Qt::transparent);
            }
            return image;
        }
        default:
            break;
        }
        return QFileSystemModel::data(index, role);
    }

private:
    FilesDock *m_dock;

    int mediaType(const QModelIndex &index) const
    {
        auto path = filePath(index);
        auto mediaType = m_dock->getCacheMediaType(path);

        if (mediaType > -1)
            return mediaType;

        Mlt::Producer producer(MLT.profile(), path.toUtf8().constData());
        mediaType = PlaylistModel::Other;
        if (producer.is_valid()) {
            if (MLT.isImageProducer(&producer)) {
                mediaType = PlaylistModel::Image;
            } else {
                auto service = QString::fromLatin1(producer.get("mlt_service"));
                if (service.startsWith(QLatin1String("avformat"))) {
                    if (producer.get_int("video_index") > -1
                            && Util::getSuggestedFrameRate(&producer) != 90000)
                        mediaType = PlaylistModel::Video;
                    else if (producer.get_int("audio_index") > -1)
                        mediaType = PlaylistModel::Audio;
                }
            }
        }
        LOG_DEBUG() << "Mlt::Producer" << path << mediaType;
        m_dock->setCacheMediaType(path, mediaType);
        return mediaType;
    }

public:

    void cacheThumbnail(const QString &filePath, const QString &key, QImage image,
                        const QModelIndex &index)
    {
        bool save = !image.isNull();
        if (image.isNull()) {
            image = QImage(PlaylistModel::THUMBNAIL_WIDTH, PlaylistModel::THUMBNAIL_WIDTH,
                           QImage::Format_ARGB32);
            image.fill(Qt::transparent);
        }
        if (save)
            DB.putThumbnail(key, image);
        m_dock->setCacheThumbnailKey(filePath, key);
        emit dataChanged(index, index);
    }
};

static void cacheThumbnail(FilesModel *model, const QString &filePath, const QString &cacheKey,
                           const QImage &image, const QModelIndex &index)
{
    model->cacheThumbnail(filePath, cacheKey, image, index);
}


class FilesTileDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    FilesTileDelegate(QAbstractItemView *view, QWidget *parent = nullptr)
        : QStyledItemDelegate(parent),
          m_view(view)
    {
        connect(&Settings, SIGNAL(playlistThumbnailsChanged()),
                SLOT(emitSizeHintChanged()));
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const QImage thumb = index.data(FilesModel::ThumbnailRole).value<QImage>();
        const int lineHeight = painter->fontMetrics().height();
        const QStringList nameParts = index.data(Qt::DisplayRole).toString().split('\n');
        const QFont oldFont = painter->font();
        QFont boldFont(oldFont);
        boldFont.setBold(true);

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight().color());
        } else {
            if (option.features & QStyleOptionViewItem::Alternate)
                painter->fillRect(option.rect, option.palette.alternateBase());
        }

        auto thumbRect = thumb.rect();
        const float width = qRound(16.f / 9.f * option.rect.height());
        thumbRect = QRect(0, 0, width, qRound(width * thumbRect.height() / thumbRect.width()));
        thumbRect.moveCenter(option.rect.center());
        thumbRect.moveLeft(0);
        painter->drawImage(thumbRect, thumb);
        auto textRect = option.rect;
        textRect.setHeight(lineHeight * 3 + kTilePaddingPx);
        textRect.moveCenter(option.rect.center());
        textRect.setLeft(thumbRect.width() + kTilePaddingPx);

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
        painter->drawText(textPoint, tr("Date: %1").arg(
                              index.data(FilesModel::DateRole).toDateTime().toString("yyyy-MM-dd HH:mm:ss")));
        textPoint.setY(textPoint.y() + lineHeight);
        auto mediaType = index.data(FilesModel::MediaTypeStringRole).toString();
        painter->drawText(textPoint, tr("Type: %1").arg(mediaType));
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(m_view->viewport()->width(), PlaylistModel::THUMBNAIL_HEIGHT + kTilePaddingPx);
    }

private slots:
    void emitSizeHintChanged()
    {
        emit sizeHintChanged(QModelIndex());
    }

private:
    QAbstractItemView *m_view;

};

class FilesProxyModel : public QSortFilterProxyModel
{
public:
    explicit FilesProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setMediaTypes(QList<PlaylistModel::MediaType> types)
    {
        m_mediaTypes = types;
        invalidateFilter();
    }

    void setCurrentPath(const QString path)
    {
        m_currentPath = path;
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const
    {
        // QCoreApplication::processEvents();
        const auto index = sourceModel()->index(row, 0, parent);

        // For some reason sometimes folders appear
        if (!index.isValid())
            return false;

        // auto filePath = index.data(QFileSystemModel::FilePathRole).toString();
        // if (static_cast<QFileSystemModel *>(sourceModel())->isDir(index)
        //         && filePath != m_currentPath && filePath.startsWith(m_currentPath)) {
        //     LOG_DEBUG() << filePath << m_currentPath;
        //     return false;
        // }

        // Media types
        if (m_mediaTypes.size() > 0 && m_mediaTypes.size() < 4) {
            if (!m_mediaTypes.contains(index.data(FilesModel::MediaTypeRole)))
                return false;
        }

        // Text search
        return index.data(QFileSystemModel::FileNameRole).toString().contains(filterRegularExpression());
    }

private:
    QList<PlaylistModel::MediaType> m_mediaTypes {
        PlaylistModel::Video, PlaylistModel::Audio, PlaylistModel::Image, PlaylistModel::Other};
    QString m_currentPath;
};

FilesDock::FilesDock(QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::FilesDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    toggleViewAction()->setIcon(windowIcon());

    const auto ls = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    const auto home = ls.first();
    m_filesModel = new FilesModel(this);
    m_filesModel->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    m_filesModel->setFilter(QDir::Files);
    m_filesModel->setReadOnly(true);
    m_filesModel->setRootPath(home);
    m_filesProxyModel = new FilesProxyModel(this);
    m_filesProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filesProxyModel->setSourceModel(m_filesModel);
    m_filesProxyModel->setFilterRole(QFileSystemModel::FileNameRole);
    m_filesProxyModel->setRecursiveFilteringEnabled(true);
    m_filesProxyModel->setCurrentPath(home);
    m_selectionModel = new QItemSelectionModel(m_filesProxyModel, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
            &FilesDock::selectionChanged);

    m_dirsModel.setReadOnly(true);
    m_dirsModel.setRootPath(QString());
    m_dirsModel.setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    m_dirsModel.setOption(QFileSystemModel::DontWatchForChanges);
    m_dirsModel.setFilter(QDir::Drives | QDir::Dirs | QDir::NoDotAndDotDot);

    int width = qRound(150 * devicePixelRatio());
    ui->splitter->setSizes({width, this->width() - width});
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->treeView->setModel(&m_dirsModel);
    ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    for (int i = 1; i < m_dirsModel.columnCount(); ++i)
        ui->treeView->hideColumn(i);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    const auto homeIndex = m_dirsModel.index(home);
    ui->treeView->setExpanded(homeIndex, true);
    ui->treeView->scrollTo(homeIndex);
    ui->treeView->setCurrentIndex(homeIndex);
    connect(ui->treeView, &QWidget::customContextMenuRequested, this, [ = ](const QPoint & pos) {
        QMenu menu(this);
        menu.exec(mapToGlobal(pos));
    });
    connect(ui->treeView, &QAbstractItemView::clicked, this, [ = ](const QModelIndex & index) {
        auto filePath = m_dirsModel.filePath(index);
        LOG_DEBUG() << "clicked" << filePath;
        m_filesModel->setRootPath(filePath);
        m_view->setRootIndex(m_filesProxyModel->mapFromSource(m_filesModel->index(filePath)));
        m_view->scrollToTop();
        m_filesProxyModel->setCurrentPath(m_dirsModel.filePath(index));
    });

    setupActions();

    m_mainMenu = new QMenu(tr("Files"), this);
    m_mainMenu->addAction(Actions["filesOpenAction"]);
    m_mainMenu->addAction(Actions["filesOpenPreviousAction"]);
    m_mainMenu->addAction(Actions["filesOpenNextAction"]);
    m_mainMenu->addAction(Actions["filesUpdateThumbnailsAction"]);
    m_mainMenu->addSeparator();
    QMenu *selectMenu = m_mainMenu->addMenu(tr("Select"));
    selectMenu->addAction(Actions["filesSelectAllAction"]);
    selectMenu->addAction(Actions["filesSelectNoneAction"]);
    Actions.loadFromMenu(m_mainMenu);

    DockToolBar *toolbar = new DockToolBar(tr("Files Controls"));
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(QIcon::fromTheme("show-menu",
                                         QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
    menuButton->setToolTip(tr("Files Menu"));
    menuButton->setAutoRaise(true);
    menuButton->setPopupMode(QToolButton::QToolButton::InstantPopup);
    menuButton->setMenu(m_mainMenu);
    toolbar->addWidget(menuButton);
    toolbar->addSeparator();
    toolbar->addAction(Actions["filesViewDetailsAction"]);
    toolbar->addAction(Actions["filesViewTilesAction"]);
    toolbar->addAction(Actions["filesViewIconsAction"]);
    toolbar->addSeparator();
    ui->verticalLayout->addWidget(toolbar);
    ui->verticalLayout->addSpacing(2);

    toolbar = new DockToolBar(tr("Files Filters"));
    toolbar->addAction(Actions["filesFoldersView"]);
    ui->filtersLayout->addWidget(toolbar);

    auto toolbar2 = new QToolBar(tr("Files Filters"));
    toolbar2->setStyleSheet(
        QStringLiteral("QToolButton:checked { color:palette(highlighted-text); background-color:palette(highlight);}"));
    ui->filtersLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    toolbar2->addActions({Actions["filesFiltersVideo"], Actions["filesFiltersAudio"], Actions["filesFiltersImage"], Actions["filesFiltersOther"]});
    ui->filtersLayout->addWidget(toolbar2);
    auto lineEdit = new LineEditClear(this);
    lineEdit->setToolTip(tr("Only show files whose name contains some text"));
    lineEdit->setPlaceholderText(tr("search"));
    connect(lineEdit, &QLineEdit::textChanged, this, [ = ](const QString & search) {
        m_filesProxyModel->setFilterFixedString(search);
        if (search.isEmpty()) {
            m_view->setRootIndex(m_filesProxyModel->mapFromSource(m_filesModel->index(
                                                                      m_filesModel->rootPath())));
        }
        m_view->scrollToTop();
    });
    ui->filtersLayout->addWidget(lineEdit, 1);

    m_iconsView = new PlaylistIconView(this);
    ui->listView->parentWidget()->layout()->addWidget(m_iconsView);
    // m_iconsView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_iconsView->setIconRole(FilesModel::ThumbnailRole);
    ui->tableView->setModel(m_filesProxyModel);
    ui->listView->setModel(m_filesProxyModel);
    m_iconsView->setModel(m_filesProxyModel);
    ui->tableView->setSelectionModel(m_selectionModel);
    ui->listView->setSelectionModel(m_selectionModel);
    m_iconsView->setSelectionModel(m_selectionModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    connect(ui->tableView, &QAbstractItemView::activated, this, [ = ] (const QModelIndex & index) {
        auto sourceIndex = m_filesProxyModel->mapToSource(index);
        auto filePath = m_filesModel->filePath(sourceIndex);
        m_view->setCurrentIndex(index);
        LOG_DEBUG() << "activated" << filePath;
        emit clipOpened(filePath);
    });
    connect(ui->listView, &QAbstractItemView::activated, ui->tableView, &QAbstractItemView::activated);
    connect(m_iconsView, &QAbstractItemView::activated, ui->tableView, &QAbstractItemView::activated);

    QList<QAbstractItemView *> views;
    views << ui->tableView;
    views << ui->listView;
    views << m_iconsView;
    for (auto view : views) {
        view->setDragDropMode(QAbstractItemView::DragOnly);
        view->setAcceptDrops(false);
        view->setAlternatingRowColors(true);
        connect(view, SIGNAL(customContextMenuRequested(QPoint)),
                SLOT(viewCustomContextMenuRequested(QPoint)));
    }

    if (Settings.viewMode() == kDetailedMode) {
        Actions["filesViewDetailsAction"]->trigger();
    } else if (Settings.viewMode() == kTiledMode) {
        Actions["filesViewTilesAction"]->trigger();
    } else { /* if (Settings.viewMode() == kIconsMode) */
        Actions["filesViewIconsAction"]->trigger();
    }

    LOG_DEBUG() << "end";
}

FilesDock::~FilesDock()
{
    delete ui;
}

int FilesDock::getCacheMediaType(const QString &key)
{
    QMutexLocker<QMutex> m_lock(&m_cacheMutex);
    auto x = m_cache.find(key);
    if (x == m_cache.end())
        return -1;
    return x.value().mediaType;
}

QString FilesDock::getCacheThumbnailKey(const QString &key)
{
    QMutexLocker<QMutex> m_lock(&m_cacheMutex);
    auto x = m_cache.find(key);
    if (x == m_cache.end())
        return QString();
    return x.value().thumbnailKey;
}

void FilesDock::setCacheMediaType(const QString &key, int mediaType)
{
    QMutexLocker<QMutex> m_lock(&m_cacheMutex);
    m_cache[key].mediaType = mediaType;
}

void FilesDock::setCacheThumbnailKey(const QString &key, const QString &thumbnailKey)
{
    QMutexLocker<QMutex> m_lock(&m_cacheMutex);
    m_cache[key].thumbnailKey = thumbnailKey;
}

void FilesDock::setupActions()
{
    QIcon icon;
    QAction *action;
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
    Actions.add("filesViewTilesAction", action);

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
    Actions.add("filesViewIconsAction", action);

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
    Actions.add("filesViewDetailsAction", action);

    action = new QAction(tr("Open"), this);
    action->setToolTip(tr("Open the clip in the Source player"));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &FilesDock::onOpenActionTriggered);
    connect(this, &FilesDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid());
    });
    Actions.add("filesOpenAction", action);

    action = new QAction(tr("Update Thumbnails"), this);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &FilesDock::onUpdateThumbnailsActionTriggered);
    connect(this, &FilesDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_filesProxyModel->rowCount() > 0);
    });
    Actions.add("filesUpdateThumbnailsAction", action);

    action = new QAction(tr("Select All"), this);
    // action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    connect(action, &QAction::triggered, this, &FilesDock::onSelectAllActionTriggered);
    action->setEnabled(m_filesProxyModel->rowCount() > 0);
    Actions.add("filesSelectAllAction", action);

    action = new QAction(tr("Select None"), this);
    // action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_D));
    connect(action, &QAction::triggered, m_selectionModel, &QItemSelectionModel::clearSelection);
    connect(this, &FilesDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_selectionModel->selection().size() > 0);
    });
    Actions.add("filesSelectNoneAction", action);

    action = new QAction(tr("Open Previous"), this);
    // action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        incrementIndex(-1);
        onOpenActionTriggered();
    });
    connect(this, &FilesDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid());
    });
    Actions.add("filesOpenPreviousAction", action);

    action = new QAction(tr("Open Next"), this);
    // action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [ = ]() {
        raise();
        incrementIndex(1);
        onOpenActionTriggered();
    });
    connect(this, &FilesDock::selectionChanged, action, [ = ]() {
        action->setEnabled(m_view->currentIndex().isValid());
    });
    Actions.add("filesOpenNextAction", action);

    action = new QAction(tr("Video"), this);
    action->setToolTip(tr("Show or hide video files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersVideo", action, this->windowTitle());

    action = new QAction(tr("Audio"), this);
    action->setToolTip(tr("Show or hide audio files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersAudio", action, this->windowTitle());

    action = new QAction(tr("Image"), this);
    action->setToolTip(tr("Show or hide image files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersImage", action, this->windowTitle());

    action = new QAction(tr("Other"), this);
    action->setToolTip(tr("Show or hide other kinds of files"));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersOther", action, this->windowTitle());

    action = new QAction(tr("Folders"), this);
    action->setToolTip(tr("Hide or show the list of folders"));
    icon = QIcon::fromTheme("view-choose",
                            QIcon(":/icons/oxygen/32x32/actions/view-choose.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::triggered, this, [ = ](bool checked) {
        ui->treeView->setVisible(checked);
    });
    Actions.add("filesFoldersView", action, windowTitle());
}

void FilesDock::incrementIndex(int step)
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid())
        index = m_filesModel->index(0, 0);
    if (index.isValid()) {
        auto row = qBound(0, index.row() + step, m_filesProxyModel->rowCount(index.parent()) - 1);
        index = m_filesProxyModel->index(row, index.column(), index.parent());
        m_view->setCurrentIndex(index);
    }
}

void FilesDock::onOpenActionTriggered()
{
    auto index = m_view->selectionModel()->selectedIndexes().first();
    if (!index.isValid()) return;
    auto sourceIndex = m_filesProxyModel->mapToSource(index);
    auto filePath = m_filesModel->filePath(sourceIndex);
    m_view->setCurrentIndex(index);
    emit clipOpened(filePath);
}

void FilesDock::viewCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = m_view->currentIndex();
    if (index.isValid()) {
        QMenu menu(this);
        menu.addAction(Actions["filesOpenAction"]);
        menu.addAction(Actions["filesUpdateThumbnailsAction"]);
        menu.exec(mapToGlobal(pos));
    }
}

void FilesDock::updateViewMode()
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
        m_view = ui->tableView;
        ui->tableView->show();
    } else if (mode == kTiledMode) {
        m_view = ui->listView;
        ui->listView->setDragEnabled(true);
        ui->listView->setItemDelegate(new FilesTileDelegate(ui->listView));
        ui->listView->show();
    } else {
        m_view = m_iconsView;
        m_iconsView->show();
    }
    m_view->scrollToTop();
    m_view->setRootIndex(m_filesProxyModel->mapFromSource(m_filesModel->index(
                                                              m_filesModel->rootPath())));
}

void FilesDock::keyPressEvent(QKeyEvent *event)
{
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        event->accept();
    if (!event->isAccepted())
        MAIN.keyPressEvent(event);
}

void FilesDock::keyReleaseEvent(QKeyEvent *event)
{
    QDockWidget::keyReleaseEvent(event);
    if (!event->isAccepted())
        MAIN.keyReleaseEvent(event);
}

void FilesDock::onSelectAllActionTriggered()
{
    show();
    raise();
    m_view->selectionModel()->clearSelection();
    for (int i = 0; i < m_filesProxyModel->rowCount(m_view->rootIndex()); i++) {
        m_view->selectionModel()->select(m_filesProxyModel->index(i, 0, m_view->rootIndex()),
                                         QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void FilesDock::onUpdateThumbnailsActionTriggered()
{
    m_view->selectionModel()->clearSelection();
    for (auto i = 0; i < m_dirsModel.rowCount(); i++) {
        // m_model.updateThumbnails(i);
    }
}

void FilesDock::onMediaTypeClicked()
{
    QList<PlaylistModel::MediaType> types;
    if (Actions["filesFiltersVideo"]->isChecked())
        types << PlaylistModel::Video;
    if (Actions["filesFiltersAudio"]->isChecked())
        types << PlaylistModel::Audio;
    if (Actions["filesFiltersImage"]->isChecked())
        types << PlaylistModel::Image;
    if (Actions["filesFiltersOther"]->isChecked())
        types << PlaylistModel::Other;
    m_filesProxyModel->setMediaTypes(types);
    m_view->scrollToTop();
}

#include "filesdock.moc"
