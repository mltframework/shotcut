/*
 * Copyright (c) 2024-2026 Meltytech, LLC
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

#include "Logger.h"
#include "actions.h"
#include "database.h"
#include "dialogs/listselectiondialog.h"
#include "mainwindow.h"
#include "models/playlistmodel.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "util.h"
#include "widgets/docktoolbar.h"
#include "widgets/lineeditclear.h"
#include "widgets/playlisticonview.h"
#include "widgets/playlistlistview.h"
#include "widgets/playlisttable.h"

#include <QActionGroup>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMutexLocker>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QRunnable>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QThreadPool>
#include <QToolButton>

static const auto kTilePaddingPx = 10;
static const auto kTreeViewWidthPx = 150;
static const auto kDetailedMode = QLatin1String("detailed");
static const auto kIconsMode = QLatin1String("icons");
static const auto kTiledMode = QLatin1String("tiled");
static const QSet<QString> kAudioExtensions{
    QLatin1String("m4a"),
    QLatin1String("wav"),
    QLatin1String("mp3"),
    QLatin1String("ac3"),
    QLatin1String("flac"),
    QLatin1String("oga"),
    QLatin1String("opus"),
    QLatin1String("wma"),
    QLatin1String("mka"),
};
static const QSet<QString> kImageExtensions{
    QLatin1String("jpg"),
    QLatin1String("jpeg"),
    QLatin1String("png"),
    QLatin1String("bmp"),
    QLatin1String("tif"),
    QLatin1String("tiff"),
    QLatin1String("svg"),
    QLatin1String("webp"),
    QLatin1String("gif"),
    QLatin1String("tga"),
};
static const QSet<QString> kOtherExtensions{
    QLatin1String("mlt"),   QLatin1String("xml"), QLatin1String("txt"),      QLatin1String("pdf"),
    QLatin1String("doc"),   QLatin1String("gpx"), QLatin1String("rawr"),     QLatin1String("stab"),
    QLatin1String("srt"),   QLatin1String("so"),  QLatin1String("dll"),      QLatin1String("exe"),
    QLatin1String("zip"),   QLatin1String("edl"), QLatin1String("kdenlive"), QLatin1String("osp"),
    QLatin1String("blend"), QLatin1String("swf"), QLatin1String("cube"),     QLatin1String("json"),
};
static const QSet<QString> kVideoExtensions{
    QLatin1String("mp4"),
    QLatin1String("m4v"),
    QLatin1String("avi"),
    QLatin1String("mpg"),
    QLatin1String("mpeg"),
    QLatin1String("ts"),
    QLatin1String("mts"),
    QLatin1String("m2ts"),
    QLatin1String("mkv"),
    QLatin1String("ogv"),
    QLatin1String("webm"),
    QLatin1String("dv"),
    QLatin1String("lrv"),
    QLatin1String("360"),
    QLatin1String("flv"),
    QLatin1String("wmv"),
};

static void cacheMediaType(FilesModel *model,
                           const QString &filePath,
                           int mediaType,
                           const QModelIndex &index);
static void cacheThumbnail(FilesModel *model,
                           const QString &filePath,
                           QImage &image,
                           const QModelIndex &index);

class FilesMediaTypeTask : public QRunnable
{
    FilesModel *m_model;
    QString m_filePath;
    QModelIndex m_index;

public:
    FilesMediaTypeTask(FilesModel *model, const QString &filePath, const QModelIndex &index)
        : QRunnable()
        , m_model(model)
        , m_filePath(filePath)
        , m_index(index)
    {}

public:
    void run()
    {
        static Mlt::Profile profile{"atsc_720p_60"};
        Mlt::Producer producer(profile, m_filePath.toUtf8().constData());
        auto mediaType = PlaylistModel::Other;
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
        LOG_DEBUG() << "Mlt::Producer" << m_filePath << mediaType;
        cacheMediaType(m_model, m_filePath, mediaType, m_index);
    }
};

class FilesThumbnailTask : public QRunnable
{
    FilesModel *m_model;
    QString m_filePath;
    QModelIndex m_index;

public:
    FilesThumbnailTask(FilesModel *model, const QString &filePath, const QModelIndex &index)
        : QRunnable()
        , m_model(model)
        , m_filePath(filePath)
        , m_index(index)
    {}

    static QString cacheKey(const QString &filePath)
    {
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(filePath.toUtf8());
        return hash.result().toHex();
    }

private:
    bool isValidService(Mlt::Producer &producer) const
    {
        if (producer.is_valid()) {
            auto service = QString::fromLatin1(producer.get("mlt_service"));
            return (service.startsWith("avformat") || service == "qimage" || service == "pixbuf"
                    || service == "glaxnimate");
        }
        return false;
    }

public:
    void run()
    {
        LOG_DEBUG() << "Mlt::Producer" << m_filePath;
        QImage image;
        static Mlt::Profile profile{"atsc_720p_60"};
        Mlt::Producer producer(profile, "abnormal", m_filePath.toUtf8().constData());
        if (isValidService(producer)) {
            Mlt::Filter scaler(profile, "swscale");
            Mlt::Filter padder(profile, "resize");
            Mlt::Filter converter(profile, "avcolor_space");
            producer.attach(scaler);
            producer.attach(padder);
            producer.attach(converter);

            auto width = PlaylistModel::THUMBNAIL_WIDTH * 2;
            auto height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
            image = MLT.image(producer, 0, width, height);
        }
        if (!image.isNull()) {
            cacheThumbnail(m_model, m_filePath, image, m_index);
        }
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
    {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const auto info = fileInfo(index);
        const auto isDir = info.isDir();
        if (MediaTypeStringRole == role || (index.column() == 2 && Qt::DisplayRole == role)) {
            QString names[] = {
                tr("Video"),
                tr("Image"),
                tr("Audio"),
                tr("Other"),
                QLatin1String(""),
            };
            auto i = isDir ? 4 : mediaType(index);
            return names[i];
        }
        switch (role) {
        case Qt::ToolTipRole:
            return QDir::toNativeSeparators(info.filePath());
        case DateRole:
            return info.lastModified();
        case MediaTypeRole:
            return isDir ? PlaylistModel::Other : mediaType(index);
        case ThumbnailRole: {
            const auto path = info.filePath();
            const auto thumbnailKey = FilesThumbnailTask::cacheKey(path);
            auto image = DB.getThumbnail(thumbnailKey);
            if (image.isNull()) {
                ::cacheThumbnail(const_cast<FilesModel *>(this), path, image, index);
                if (!path.endsWith(QStringLiteral(".mlt"), Qt::CaseInsensitive)
                    && !info.isShortcut())
                    QThreadPool::globalInstance()->start(
                        new FilesThumbnailTask(const_cast<FilesModel *>(this), path, index));
            }
            return image;
        }
        default:
            break;
        }

        return QFileSystemModel::data(index, role);
    }

    void updateThumbnails(const QModelIndex &index)
    {
        const auto path = filePath(index);
        if (!path.endsWith(QStringLiteral(".mlt"), Qt::CaseInsensitive))
            QThreadPool::globalInstance()->start(
                new FilesThumbnailTask(const_cast<FilesModel *>(this), path, index));
    }

private:
    FilesDock *m_dock;

    int mediaType(const QModelIndex &index) const
    {
        auto path = filePath(index);
        auto mediaType = m_dock->getCacheMediaType(path);

        if (mediaType < 0) {
            const auto info = QFileInfo(path);
            const auto ext = info.suffix().toLower();
            if (info.isDir() || kOtherExtensions.contains(ext)) {
                m_dock->setCacheMediaType(path, mediaType);
                return PlaylistModel::Other;
            }
            if (kAudioExtensions.contains(ext)) {
                m_dock->setCacheMediaType(path, mediaType);
                return PlaylistModel::Audio;
            }
            if (kImageExtensions.contains(ext)) {
                m_dock->setCacheMediaType(path, mediaType);
                return PlaylistModel::Image;
            }
            if (kVideoExtensions.contains(ext)) {
                m_dock->setCacheMediaType(path, mediaType);
                return PlaylistModel::Video;
            }
            mediaType = PlaylistModel::Pending;
            m_dock->setCacheMediaType(path, mediaType);
            QThreadPool::globalInstance()->start(
                new FilesMediaTypeTask(const_cast<FilesModel *>(this), path, index));
        }

        return mediaType;
    }

public:
    void cacheMediaType(const QString &filePath, int mediaType, const QModelIndex &index)
    {
        m_dock->setCacheMediaType(filePath, mediaType);
        emit dataChanged(index, index);
    }

    void cacheThumbnail(const QString &filePath, QImage &image, const QModelIndex &index)
    {
        bool updateModel = !image.isNull();
        if (image.isNull()) {
            image = QImage(64, 64, QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            if (index.isValid()) {
                const auto pixmap = QFileSystemModel::data(index, Qt::DecorationRole)
                                        .value<QIcon>()
                                        .pixmap({16, 16}, m_dock->devicePixelRatioF());
                QPainter painter(&image);
                QIcon(pixmap).paint(&painter, image.rect());
            }
        }
        auto key = FilesThumbnailTask::cacheKey(filePath);
        DB.putThumbnail(key, image);
        if (updateModel)
            emit dataChanged(index, index);
    }
};

static void cacheMediaType(FilesModel *model,
                           const QString &filePath,
                           int mediaType,
                           const QModelIndex &index)
{
    model->cacheMediaType(filePath, mediaType, index);
}

static void cacheThumbnail(FilesModel *model,
                           const QString &filePath,
                           QImage &image,
                           const QModelIndex &index)
{
    model->cacheThumbnail(filePath, image, index);
}

class FilesTileDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    FilesTileDelegate(QAbstractItemView *view, QWidget *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_view(view)
    {
        connect(&Settings, SIGNAL(playlistThumbnailsChanged()), SLOT(emitSizeHintChanged()));
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const QImage thumb = index.data(FilesModel::ThumbnailRole).value<QImage>();
        const int lineHeight = painter->fontMetrics().height();
        const auto fileInfo = QFileInfo(index.data(Qt::ToolTipRole).toString());
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
        if (thumbRect.width() > 0) {
            const float width = qRound(16.f / 9.f * option.rect.height());
            thumbRect = QRect(0, 0, width, qRound(width * thumbRect.height() / thumbRect.width()));
            thumbRect.moveCenter(option.rect.center());
            thumbRect.moveLeft(0);
            painter->drawImage(thumbRect, thumb);
        }
        auto textRect = option.rect;
        textRect.setHeight(lineHeight * 3 + kTilePaddingPx);
        textRect.moveCenter(option.rect.center());
        textRect.setLeft(thumbRect.width() + kTilePaddingPx);

        QPoint textPoint = textRect.topLeft();
        textPoint.setY(textPoint.y() + lineHeight);
        painter->setFont(boldFont);
        auto name = fileInfo.isShortcut() ? fileInfo.baseName() : fileInfo.fileName();
        name = painter->fontMetrics().elidedText(name, Qt::ElideMiddle, textRect.width());
        painter->drawText(textPoint, name);
        painter->setFont(oldFont);

        textPoint.setY(textPoint.y() + lineHeight);
        painter->drawText(textPoint,
                          tr("Date: %1")
                              .arg(index.data(FilesModel::DateRole)
                                       .toDateTime()
                                       .toString("yyyy-MM-dd HH:mm:ss")));
        textPoint.setY(textPoint.y() + lineHeight);
        if (fileInfo.isFile()) {
            // Get the text of the second (size) column
            auto myindex = index.model()->index(index.row(), 1, index.parent());
            auto size = myindex.data(Qt::DisplayRole).toString();
            painter->drawText(textPoint, tr("Size: %1").arg(size));
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(m_view->viewport()->width(), PlaylistModel::THUMBNAIL_HEIGHT + kTilePaddingPx);
    }

private slots:
    void emitSizeHintChanged() { emit sizeHintChanged(QModelIndex()); }

private:
    QAbstractItemView *m_view;
};

class FilesProxyModel : public QSortFilterProxyModel
{
public:
    explicit FilesProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {}

    void setMediaTypes(QList<PlaylistModel::MediaType> types)
    {
        m_mediaTypes = types;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        const auto index = sourceModel()->index(row, 0, parent);
        const auto model = qobject_cast<const QFileSystemModel *>(sourceModel());

        // Media types
        if (m_mediaTypes.size() > 0 && m_mediaTypes.size() < 4 && !model->isDir(index)) {
            if (!m_mediaTypes.contains(index.data(FilesModel::MediaTypeRole)))
                return false;
        }

        // Text search
        return index.data(QFileSystemModel::FileNameRole)
            .toString()
            .contains(filterRegularExpression());
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        const auto model = qobject_cast<const QFileSystemModel *>(sourceModel());
        if (model->isDir(left) && model->isDir(right)) {
            if (left.column() == 3)
                return model->lastModified(left) < model->lastModified(right);
            return left.data().toString().toLower() < right.data().toString().toLower();
        }
        if (model->isDir(left) || model->isDir(right))
            return model->isDir(left);
        // file size
        if (left.column() == 1)
            return model->size(left) < model->size(right);
        // file date
        if (left.column() == 3)
            return model->lastModified(left) < model->lastModified(right);
        return QSortFilterProxyModel::lessThan(left, right);
    }

private:
    QList<PlaylistModel::MediaType> m_mediaTypes;
};

FilesDock::FilesDock(QWidget *parent)
    : QDockWidget(parent)
    , ui(new Ui::FilesDock)
{
    LOG_DEBUG() << "begin";
    ui->setupUi(this);
    QIcon icon = QIcon::fromTheme("system-file-manager",
                                  QIcon(":/icons/oxygen/32x32/apps/system-file-manager.png"));
    toggleViewAction()->setIcon(icon);

    const auto ls = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    ui->locationsCombo->addItem(tr("Home", "The user's home folder in the file system"), ls.first());
    ui->locationsCombo->addItem(tr("Current Project"), "");
    ui->locationsCombo->addItem(tr("Documents"),
                                QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)
                                    .constFirst());
#if defined(Q_OS_MAC)
    ui->locationsCombo
        ->addItem(tr("Movies", "The system-provided videos folder called Movies on macOS"),
                  QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).constFirst());
#endif
    ui->locationsCombo
        ->addItem(tr("Music"),
                  QStandardPaths::standardLocations(QStandardPaths::MusicLocation).constFirst());
    ui->locationsCombo
        ->addItem(tr("Pictures", "The system-provided photos folder"),
                  QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).constFirst());
#if defined(Q_OS_MAC)
    ui->locationsCombo->addItem(
        tr("Volumes",
           "The macOS file system location where external drives and network shares are mounted"),
        "/Volumes");
#else
    ui->locationsCombo
        ->addItem(tr("Videos"),
                  QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).first());
#endif
    ui->removeLocationButton->setDisabled(true);
    auto n = ui->locationsCombo->count();
    connect(ui->locationsCombo, &QComboBox::currentIndexChanged, this, [=](int index) {
        ui->removeLocationButton->setEnabled(index >= n);
    });

    // Add from Settings
    auto locations = Settings.filesLocations();
    for (const auto &name : locations) {
        auto path = Settings.filesLocationPath(name);
        ui->locationsCombo->addItem(name, path);
    }
    ui->locationsCombo->setEditText(QDir::toNativeSeparators(Settings.filesCurrentDir()));
    connect(ui->locationsCombo->lineEdit(),
            &QLineEdit::editingFinished,
            this,
            &FilesDock::onLocationsEditingFinished);

    m_filesModel = new FilesModel(this);
    m_filesModel->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    m_filesModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    m_filesModel->setReadOnly(true);
    m_filesModel->setRootPath(Settings.filesCurrentDir());
    ui->locationsCombo->setToolTip(Settings.filesCurrentDir());
    m_filesProxyModel = new FilesProxyModel(this);
    m_filesProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filesProxyModel->setSourceModel(m_filesModel);
    m_filesProxyModel->setFilterRole(QFileSystemModel::FileNameRole);
    m_filesProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filesProxyModel->setRecursiveFilteringEnabled(true);
    m_selectionModel = new QItemSelectionModel(m_filesProxyModel, this);
    connect(m_selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &FilesDock::selectionChanged);

    m_dirsModel.reset(new QFileSystemModel);
    m_dirsModel->setReadOnly(true);
    m_dirsModel->setRootPath(QString());
    m_dirsModel->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    m_dirsModel->setOption(QFileSystemModel::DontWatchForChanges);
    m_dirsModel->setFilter(QDir::Drives | QDir::Dirs | QDir::NoDotAndDotDot);

    const int width = qRound(kTreeViewWidthPx * devicePixelRatio());
    ui->splitter->setSizes({width, this->width() - width});
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
    ui->treeView->setModel(m_dirsModel.get());
    ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    for (int i = 1; i < m_dirsModel->columnCount(); ++i)
        ui->treeView->hideColumn(i);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    const auto homeIndex = m_dirsModel->index(Settings.filesCurrentDir());
    ui->treeView->setExpanded(homeIndex, true);
    ui->treeView->scrollTo(homeIndex);
    ui->treeView->setCurrentIndex(homeIndex);
    QTimer::singleShot(0, this, [=]() { ui->treeView->setVisible(Settings.filesFoldersOpen()); });
    connect(ui->treeView, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        QMenu menu(this);
        menu.exec(mapToGlobal(pos));
    });
    connect(ui->treeView, &QAbstractItemView::clicked, this, [=](const QModelIndex &index) {
        auto filePath = m_dirsModel->filePath(index);
        LOG_DEBUG() << "clicked" << filePath;
        auto sourceIndex = m_filesModel->setRootPath(filePath);
        Settings.setFilesCurrentDir(m_filesModel->rootPath());
        changeFilesDirectory(m_filesProxyModel->mapFromSource(sourceIndex));
    });

    m_mainMenu = new QMenu(tr("Files"), this);
    setupActions();

    m_mainMenu->addAction(Actions["filesOpenAction"]);
    m_mainMenu->addAction(Actions["filesGoUp"]);
    m_mainMenu->addAction(Actions["filesOpenPreviousAction"]);
    m_mainMenu->addAction(Actions["filesOpenNextAction"]);
    m_mainMenu->addAction(Actions["filesUpdateThumbnailsAction"]);
    m_mainMenu->addAction(Actions["filesShowInFolder"]);
    m_mainMenu->addAction(Actions["filesRefreshFolders"]);
    QMenu *selectMenu = m_mainMenu->addMenu(tr("Select"));
    selectMenu->addAction(Actions["filesSelectAllAction"]);
    selectMenu->addAction(Actions["filesSelectNoneAction"]);

    DockToolBar *toolbar = new DockToolBar(tr("Files Controls"));
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    QToolButton *menuButton = new QToolButton();
    menuButton->setIcon(
        QIcon::fromTheme("show-menu", QIcon(":/icons/oxygen/32x32/actions/show-menu.png")));
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
    m_label = new QLabel(toolbar);
    toolbar->addWidget(m_label);
    connect(m_filesModel,
            &QFileSystemModel::directoryLoaded,
            this,
            &FilesDock::updateStatus,
            Qt::QueuedConnection);
    connect(m_filesProxyModel,
            &QAbstractItemModel::modelAboutToBeReset,
            this,
            &FilesDock::clearStatus);
    connect(m_filesProxyModel, &QAbstractItemModel::rowsInserted, this, &FilesDock::updateStatus);
    connect(m_filesProxyModel, &QAbstractItemModel::rowsRemoved, this, &FilesDock::updateStatus);
    ui->verticalLayout->addWidget(toolbar);
    ui->verticalLayout->addSpacing(2);

    toolbar = new DockToolBar(tr("Files Filters"));
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    toolbar->addAction(Actions["filesFoldersView"]);
    ui->filtersLayout->addWidget(toolbar);

    toolbar = new DockToolBar(ui->label->text());
    toolbar->setAreaHint(Qt::BottomToolBarArea);
    toolbar->addAction(Actions["filesRefreshFolders"]);
    toolbar->addAction(Actions["filesGoUp"]);
    delete ui->label;
    ui->locationsLayout->insertWidget(1, toolbar);

    auto toolbar2 = new QToolBar(tr("Files Filters"));
    QString styleSheet = QStringLiteral("QToolButton {"
                                        "    background-color: palette(background);"
                                        "    border-style: solid;"
                                        "    border-width: 1px;"
                                        "    border-radius: 3px;"
                                        "    border-color: palette(shadow);"
                                        "    color: palette(button-text);"
                                        "}"
                                        "QToolButton:checked {"
                                        "    color:palette(highlighted-text);"
                                        "    background-color:palette(highlight);"
                                        "    border-color: palette(highlight);"
                                        "}");
    toolbar2->setStyleSheet(styleSheet);
    ui->filtersLayout->addItem(
        new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    toolbar2->addActions({Actions["filesFiltersVideo"],
                          Actions["filesFiltersAudio"],
                          Actions["filesFiltersImage"],
                          Actions["filesFiltersOther"]});
    ui->filtersLayout->addWidget(toolbar2);
    m_searchField = new LineEditClear(this);
    m_searchField->setToolTip(tr("Only show files whose name contains some text"));
    m_searchField->setPlaceholderText(tr("search"));
    connect(m_searchField, &QLineEdit::textChanged, this, [=](const QString &search) {
        m_filesProxyModel->setFilterFixedString(search);
        if (search.isEmpty()) {
            changeFilesDirectory(
                m_filesProxyModel->mapFromSource(m_filesModel->index(m_filesModel->rootPath())));
        }
        m_view->scrollToTop();
    });
    ui->filtersLayout->addWidget(m_searchField, 1);

    m_iconsView = new PlaylistIconView(this);
    ui->listView->parentWidget()->layout()->addWidget(m_iconsView);
    m_iconsView->setIconRole(FilesModel::ThumbnailRole);
    ui->tableView->setModel(m_filesProxyModel);
    ui->listView->setModel(m_filesProxyModel);
    m_iconsView->setModel(m_filesProxyModel);
    ui->tableView->setSelectionModel(m_selectionModel);
    ui->listView->setSelectionModel(m_selectionModel);
    m_iconsView->setSelectionModel(m_selectionModel);
    ui->tableView->setColumnHidden(2, true);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    ui->tableView->setColumnWidth(1, 100);
    connect(ui->tableView, &QAbstractItemView::activated, this, [=](const QModelIndex &index) {
        const auto sourceIndex = m_filesProxyModel->mapToSource(index);
        auto filePath = m_filesModel->filePath(sourceIndex);
        auto info = m_filesModel->fileInfo(sourceIndex);
        if (info.isSymLink()) {
            filePath = info.symLinkTarget();
            info = QFileInfo(filePath);
        }
        LOG_DEBUG() << "activated" << filePath + (info.isDir() ? "/" : "");
        if (info.isDir()) {
            changeDirectory(filePath);
            return;
        }
        m_selectionModel->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
        if (!MAIN.open(filePath))
            openClip(filePath);
    });
    connect(ui->tableView->horizontalHeader(),
            &QHeaderView::sortIndicatorChanged,
            this,
            [=](int column, Qt::SortOrder order) {
                LOG_DEBUG() << "sort by column" << column;
                ui->tableView->sortByColumn(column, order);
            });
    connect(ui->listView,
            &QAbstractItemView::activated,
            ui->tableView,
            &QAbstractItemView::activated);
    connect(m_iconsView,
            &QAbstractItemView::activated,
            ui->tableView,
            &QAbstractItemView::activated);

    QList<QAbstractItemView *> views;
    views << ui->tableView;
    views << ui->listView;
    views << m_iconsView;
    for (auto view : views) {
        view->setDragDropMode(QAbstractItemView::DragOnly);
        view->setAcceptDrops(false);
        view->setAlternatingRowColors(true);
        connect(view,
                SIGNAL(customContextMenuRequested(QPoint)),
                SLOT(viewCustomContextMenuRequested(QPoint)));
    }

    if (Settings.filesViewMode() == kDetailedMode) {
        Actions["filesViewDetailsAction"]->trigger();
    } else if (Settings.filesViewMode() == kTiledMode) {
        Actions["filesViewTilesAction"]->trigger();
    } else { /* if (Settings.filesViewMode() == kIconsMode) */
        Actions["filesViewIconsAction"]->trigger();
    }
    Actions.loadFromMenu(m_mainMenu);

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

void FilesDock::setCacheMediaType(const QString &key, int mediaType)
{
    QMutexLocker<QMutex> m_lock(&m_cacheMutex);
    m_cache[key].mediaType = mediaType;
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
    connect(action, &QAction::triggered, this, [&]() {
        Settings.setFilesViewMode(kTiledMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    Actions.add("filesViewTilesAction", action, m_mainMenu->title());

    action = new QAction(tr("Icons"), this);
    action->setToolTip(tr("View as icons"));
    icon = QIcon::fromTheme("view-list-icons",
                            QIcon(":/icons/oxygen/32x32/actions/view-list-icons.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&]() {
        Settings.setFilesViewMode(kIconsMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    Actions.add("filesViewIconsAction", action, m_mainMenu->title());

    action = new QAction(tr("Details"), this);
    action->setToolTip(tr("View as details"));
    icon = QIcon::fromTheme("view-list-text",
                            QIcon(":/icons/oxygen/32x32/actions/view-list-text.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, [&]() {
        Settings.setFilesViewMode(kDetailedMode);
        updateViewMode();
    });
    modeGroup->addAction(action);
    Actions.add("filesViewDetailsAction", action, m_mainMenu->title());

    action = new QAction(tr("Open In Shotcut"), this);
    action->setToolTip(tr("Open the clip in the Source player"));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &FilesDock::onOpenActionTriggered);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesOpenAction", action);

    action = new QAction(tr("System Default"), this);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [=]() {
        auto filePath = firstSelectedFilePath();
        if (filePath.isEmpty())
            filePath = m_filesModel->rootPath();
        LOG_DEBUG() << filePath;
        openClip(filePath);
    });
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesOpenDefaultAction", action);

#ifdef EXTERNAL_LAUNCHERS
    action = new QAction(tr("Other..."), this);
    action->setEnabled(false);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    connect(action, &QAction::triggered, this, &FilesDock::onOpenOtherAdd);
    Actions.add("filesOpenWithOtherAction", action);

    action = new QAction(tr("Remove..."), this);
    action->setEnabled(false);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    connect(action, &QAction::triggered, this, &FilesDock::onOpenOtherRemove);
    Actions.add("filesOpenWithRemoveAction", action);
#endif

    action = new QAction(tr("Show In File Manager"), this);
    connect(action, &QAction::triggered, this, [=]() {
        auto filePath = firstSelectedFilePath();
        if (filePath.isEmpty())
            filePath = m_filesModel->rootPath();
        LOG_DEBUG() << filePath;
        Util::showInFolder(filePath);
    });
    Actions.add("filesShowInFolder", action);

    action = new QAction(tr("Update Thumbnails"), this);
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, &FilesDock::onUpdateThumbnailsActionTriggered);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesUpdateThumbnailsAction", action);

    action = new QAction(tr("Select All"), this);
    // action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_A));
    connect(action, &QAction::triggered, this, &FilesDock::onSelectAllActionTriggered);
    connect(m_filesProxyModel, &QAbstractItemModel::rowsInserted, this, [=]() {
        action->setEnabled(m_filesProxyModel->rowCount() > 0);
    });
    connect(m_filesProxyModel, &QAbstractItemModel::rowsRemoved, this, [=]() {
        action->setEnabled(m_filesProxyModel->rowCount() > 0);
    });
    Actions.add("filesSelectAllAction", action);

    action = new QAction(tr("Select None"), this);
    // action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_D));
    connect(action, &QAction::triggered, this, [=]() {
        m_view->setCurrentIndex(QModelIndex());
        m_selectionModel->clearSelection();
    });
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesSelectNoneAction", action);

    action = new QAction(tr("Open Previous"), this);
    // action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [=]() {
        raise();
        incrementIndex(-1);
    });
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesOpenPreviousAction", action);

    action = new QAction(tr("Open Next"), this);
    // action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [=]() {
        raise();
        incrementIndex(1);
    });
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, action, [=]() {
        action->setEnabled(!m_selectionModel->selection().isEmpty());
    });
    Actions.add("filesOpenNextAction", action);

    action = new QAction(tr("Video"), this);
    action->setToolTip(tr("Show or hide video files"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersVideo", action, this->windowTitle());

    action = new QAction(tr("Audio"), this);
    action->setToolTip(tr("Show or hide audio files"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersAudio", action, this->windowTitle());

    action = new QAction(tr("Image"), this);
    action->setToolTip(tr("Show or hide image files"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersImage", action, this->windowTitle());

    action = new QAction(tr("Other"), this);
    action->setToolTip(tr("Show or hide other kinds of files"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, &FilesDock::onMediaTypeClicked);
    Actions.add("filesFiltersOther", action, this->windowTitle());

    action = new QAction(tr("Folders"), this);
    action->setToolTip(tr("Hide or show the list of folders"));
    icon = QIcon::fromTheme("view-choose", QIcon(":/icons/oxygen/32x32/actions/view-choose.png"));
    action->setIcon(icon);
    action->setCheckable(true);
    action->setChecked(Settings.filesFoldersOpen());
    connect(action, &QAction::triggered, this, [=](bool checked) {
        ui->treeView->setVisible(checked);
        Settings.setFilesFoldersOpen(checked);
    });
    Actions.add("filesFoldersView", action, windowTitle());

    action = new QAction(tr("Go Up"), this);
    action->setToolTip(tr("Show the parent folder"));
    action->setShortcut({Qt::ALT | Qt::Key_Backspace});
    icon = QIcon::fromTheme("lift", QIcon(":/icons/oxygen/32x32/actions/lift.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [=]() {
        auto dir = QDir(m_filesModel->rootPath());
        dir.cdUp();
        const auto filePath = dir.absolutePath();
        const auto index = m_filesModel->setRootPath(filePath);
        Settings.setFilesCurrentDir(filePath);
        changeFilesDirectory(m_filesProxyModel->mapFromSource(index));
        const auto dirsIndex = m_dirsModel->index(filePath);
        ui->treeView->setExpanded(dirsIndex, true);
        ui->treeView->scrollTo(dirsIndex);
        ui->treeView->setCurrentIndex(dirsIndex);
    });
    Actions.add("filesGoUp", action);

    action = new QAction(tr("Refresh Folders"), this);
    icon = QIcon::fromTheme("view-refresh", QIcon(":/icons/oxygen/32x32/actions/view-refresh.png"));
    action->setIcon(icon);
    connect(action, &QAction::triggered, this, [=]() {
        const auto cd = m_dirsModel->filePath(ui->treeView->currentIndex());
        m_dirsModel.reset(new QFileSystemModel);
        m_dirsModel->setReadOnly(true);
        m_dirsModel->setRootPath(QString());
        m_dirsModel->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
        m_dirsModel->setOption(QFileSystemModel::DontWatchForChanges);
        m_dirsModel->setFilter(QDir::Drives | QDir::Dirs | QDir::NoDotAndDotDot);
        ui->treeView->setModel(m_dirsModel.get());
        for (int i = 1; i < m_dirsModel->columnCount(); ++i)
            ui->treeView->hideColumn(i);
        const auto index = m_dirsModel->index(cd);
        ui->treeView->setExpanded(index, true);
        ui->treeView->scrollTo(index);
        ui->treeView->setCurrentIndex(index);
    });
    Actions.add("filesRefreshFolders", action);

    action = new QAction(tr("Search"), this);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    connect(action, &QAction::triggered, this, [=]() {
        setVisible(true);
        raise();
        m_searchField->setFocus();
    });
    Actions.add("filesSearch", action, m_mainMenu->title());
}

void FilesDock::incrementIndex(int step)
{
    QModelIndex index = m_view->currentIndex();
    if (!index.isValid())
        index = m_filesModel->index(0, 0);
    if (index.isValid()) {
        auto row = qBound(0, index.row() + step, m_filesProxyModel->rowCount(index.parent()) - 1);
        index = m_filesProxyModel->index(row, index.column(), index.parent());
        emit m_view->activated(index);
    }
}

void FilesDock::addOpenWithMenu(QMenu *menu)
{
    auto subMenu = menu->addMenu(tr("Open With"));
    subMenu->addAction(Actions["filesOpenDefaultAction"]);
#ifdef EXTERNAL_LAUNCHERS
    subMenu->addSeparator();
    // custom options
    auto programs = Settings.filesOpenOther(firstSelectedMediaType());
    for (const auto &program : programs) {
        auto action = subMenu->addAction(QFileInfo(program).baseName(), this, [=]() {
            const auto filePath = firstSelectedFilePath();
            LOG_DEBUG() << program << filePath;
            Util::startDetached(program, {QDir::toNativeSeparators(filePath)});
        });
        action->setObjectName(program);
    }
    subMenu->addSeparator();
    // custom options actions
    subMenu->addAction(Actions["filesOpenWithOtherAction"]);
    subMenu->addAction(Actions["filesOpenWithRemoveAction"]);
#endif
}

QString FilesDock::firstSelectedFilePath()
{
    QString result;
    if (!m_view->selectionModel()->selectedIndexes().isEmpty()) {
        const auto index = m_view->selectionModel()->selectedIndexes().first();
        if (index.isValid()) {
            auto sourceIndex = m_filesProxyModel->mapToSource(index);
            result = m_filesModel->filePath(sourceIndex);
            const auto info = m_filesModel->fileInfo(sourceIndex);
            if (info.isSymLink())
                result = info.symLinkTarget();
        }
    }
    return result;
}

QString FilesDock::firstSelectedMediaType()
{
    QString result;
    if (!m_view->selectionModel()->selectedIndexes().isEmpty()) {
        const auto index = m_view->selectionModel()->selectedIndexes().first();
        if (index.isValid()) {
            auto sourceIndex = m_filesProxyModel->mapToSource(index);
            switch (sourceIndex.data(FilesModel::MediaTypeRole).toInt()) {
            case PlaylistModel::Audio:
                result = QLatin1String("audio");
                break;
            case PlaylistModel::Image:
                result = QLatin1String("image");
                break;
            case PlaylistModel::Video:
                result = QLatin1String("video");
                break;
            default:
                result = QLatin1String("other");
                break;
            }
        }
    }
    return result;
}

void FilesDock::openClip(const QString &filePath)
{
#if defined(Q_OS_WIN)
    const auto scheme = QLatin1String("file:///");
#else
    const auto scheme = QLatin1String("file://");
#endif
    Util::openUrl({scheme + filePath, QUrl::TolerantMode});
}

void FilesDock::onOpenActionTriggered()
{
    const auto filePath = firstSelectedFilePath();
    if (!filePath.isEmpty()) {
        const auto index = m_filesProxyModel->mapFromSource(m_filesModel->index(filePath));
        m_view->setCurrentIndex(index);
        MAIN.open(filePath);
    }
}

void FilesDock::changeDirectory(const QString &filePath, bool updateLocation)
{
    LOG_DEBUG() << filePath;
    QFileInfo info(filePath);
    auto path = info.isDir() ? filePath : info.path();
    auto index = m_dirsModel->index(path);
    ui->treeView->setExpanded(index, true);
    ui->treeView->scrollTo(index);
    ui->treeView->setCurrentIndex(index);
    if (!QFile::exists(path)) {
        MAIN.showStatusMessage("!? " + QDir::toNativeSeparators(path));
        const auto ls = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        path = ls.first();
    }
    index = m_filesModel->setRootPath(path);
    Settings.setFilesCurrentDir(path);
    path = QDir::toNativeSeparators(path);
    ui->locationsCombo->setToolTip(path);
    if (updateLocation && path != ui->locationsCombo->currentText())
        ui->locationsCombo->setCurrentText(path);
    m_view->setRootIndex(m_filesProxyModel->mapFromSource(index));
    m_iconsView->updateSizes();
    if (info.isDir()) {
        m_view->scrollToTop();
    } else {
        QTimer::singleShot(2000, this, [=]() {
            const auto index = m_filesProxyModel->mapFromSource(m_filesModel->index(filePath));
            m_view->scrollTo(index);
            m_view->setCurrentIndex(index);
        });
    }
}

void FilesDock::changeFilesDirectory(const QModelIndex &index)
{
    m_view->setRootIndex(index);
    m_iconsView->updateSizes();
    auto path = QDir::toNativeSeparators(m_filesModel->rootPath());
    ui->locationsCombo->setCurrentText(path);
    ui->locationsCombo->setToolTip(path);
    m_view->scrollToTop();
}

void FilesDock::viewCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = m_view->currentIndex();
    if (index.isValid()) {
        QMenu menu(this);
        menu.addAction(Actions["filesOpenAction"]);
        menu.addAction(Actions["filesUpdateThumbnailsAction"]);
        menu.addAction(Actions["filesShowInFolder"]);
        addOpenWithMenu(&menu);
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

    QString mode = Settings.filesViewMode();
    if (mode == kDetailedMode) {
        m_view = ui->tableView;
    } else if (mode == kTiledMode) {
        m_view = ui->listView;
        if (!ui->listView->itemDelegate())
            ui->listView->setItemDelegate(new FilesTileDelegate(ui->listView));
    } else {
        m_view = m_iconsView;
    }
    m_view->setRootIndex(
        m_filesProxyModel->mapFromSource(m_filesModel->index(m_filesModel->rootPath())));
    m_iconsView->updateSizes();
    m_view->show();
}

void FilesDock::keyPressEvent(QKeyEvent *event)
{
#if defined(Q_OS_MAC)
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!m_view->selectionModel()->selectedIndexes().isEmpty()) {
            const auto index = m_view->selectionModel()->selectedIndexes().first();
            if (index.isValid()) {
                const auto sourceIndex = m_filesProxyModel->mapToSource(index);
                auto filePath = m_filesModel->filePath(sourceIndex);
                QFileInfo info(filePath);
                if (info.isSymLink()) {
                    filePath = info.symLinkTarget();
                    info = QFileInfo(filePath);
                }
                LOG_DEBUG() << "activated" << filePath + (info.isDir() ? "/" : "");
                if (info.isDir()) {
                    changeDirectory(filePath);
                    return;
                }
                if (!MAIN.open(filePath))
                    openClip(filePath);
            }
        }
        event->accept();
        return;
    }
#endif
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
    for (auto &index : m_view->selectionModel()->selectedIndexes()) {
        auto sourceIndex = m_filesProxyModel->mapToSource(index);
        if (sourceIndex.isValid())
            m_filesModel->updateThumbnails(sourceIndex);
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

void FilesDock::onOpenOtherAdd()
{
    LOG_DEBUG();
    const auto filePath = firstSelectedFilePath();
    if (filePath.isEmpty())
        return;

    const auto program = Util::getExecutable(&MAIN);
    if (!program.isEmpty()) {
        if (Util::startDetached(program, {QDir::toNativeSeparators(filePath)})) {
            Settings.setFilesOpenOther(firstSelectedMediaType(), program);
        }
    }
}

void FilesDock::onOpenOtherRemove()
{
    const auto mediaType = firstSelectedMediaType();
    LOG_DEBUG() << mediaType;
    auto ls = Settings.filesOpenOther(mediaType);
    ls.sort(Qt::CaseInsensitive);
    QStringList programs;
    std::for_each(ls.begin(), ls.end(), [&](const QString &s) {
        programs << QDir::toNativeSeparators(s);
    });
    ListSelectionDialog dialog(programs, this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setWindowTitle(tr("Remove From Open With"));
    if (QDialog::Accepted == dialog.exec()) {
        for (auto program : dialog.selection()) {
            program = QDir::fromNativeSeparators(program);
            Settings.removeFilesOpenOther(mediaType, program);
            // Remove menu item
            delete m_mainMenu->findChild<QAction *>(program);
        }
    }
}

void FilesDock::clearStatus()
{
    m_label->setText("...");
}

void FilesDock::updateStatus()
{
    if (!m_view->rootIndex().isValid())
        return;
    auto n = m_filesProxyModel->rowCount(m_view->rootIndex());
    m_label->setText(tr("%n item(s)", nullptr, n));
}

void FilesDock::onLocationsEditingFinished()
{
    auto path = ui->locationsCombo->currentText();
    LOG_DEBUG() << path;
    if (!QFile::exists(path))
        return;
#if defined(Q_OS_WIN)
    if (QLatin1String("/") == path)
        path = QStringLiteral("C:/");
#endif
    changeDirectory(path);
}

void FilesDock::on_locationsCombo_activated(int)
{
    auto path = ui->locationsCombo->currentData().toString();
    if (path.isEmpty() && !MAIN.fileName().isEmpty())
        path = QFileInfo(MAIN.fileName()).absolutePath();
    if (path.isEmpty())
        return;
    ui->locationsCombo->clearFocus();
    if (!QFile::exists(path))
        return;
#if defined(Q_OS_WIN)
    if (QLatin1String("/") == path)
        path = QStringLiteral("C:/");
#endif
    changeDirectory(path, false /* updateLocation */);
}

void FilesDock::on_addLocationButton_clicked()
{
    const auto path = m_filesProxyModel->mapToSource(m_view->rootIndex())
                          .data(QFileSystemModel::FilePathRole)
                          .toString();
    if (path.isEmpty())
        return;
    QInputDialog dialog(this);
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setWindowTitle(tr("Add Location"));
    dialog.setLabelText(tr("Name") + QStringLiteral(" ").repeated(80));
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setTextValue(QDir::toNativeSeparators(path));
    if (QDialog::Accepted == dialog.exec()) {
        auto name = dialog.textValue();
        if (name.isEmpty())
            name = path;
        Settings.setFilesLocation(name, path);
        ui->locationsCombo->addItem(name, path);
    }
}

void FilesDock::on_removeLocationButton_clicked()
{
    const auto &location = ui->locationsCombo->currentText();
    if (location.isEmpty())
        return;
    QMessageBox dialog(QMessageBox::Question,
                       tr("Delete Location"),
                       tr("Are you sure you want to remove %1?").arg(location),
                       QMessageBox::No | QMessageBox::Yes,
                       this);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (QMessageBox::Yes == dialog.exec()) {
        Settings.removeFilesLocation(location);
        const auto index = ui->locationsCombo->findText(location);
        if (index > -1)
            ui->locationsCombo->removeItem(index);
    }
}

#include "filesdock.moc"
