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

#include "elementsdock.h"

#include "Logger.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "models/elementsmodel.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "widgets/playlisticonview.h"

#include <MltConsumer.h>
#include <MltProducer.h>

#include "widgets/flowlayout.h"
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDrag>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QMimeData>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// Minimum dock width (px) at which the toolbar shows text under the icon.
static constexpr int kTextUnderIconThreshold = 310;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QIcon ElementsDock::makeTextIcon(const QString &text, const QColor &color)
{
    const int size = 24;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    QFont f = p.font();
    f.setPixelSize(size);
    p.setFont(f);
    p.setPen(color.isValid() ? color : QGuiApplication::palette().color(QPalette::WindowText));
    p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, text);
    return QIcon(pixmap);
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

ElementsDock::ElementsDock(QWidget *parent)
    : QDockWidget(tr("Elements"), parent)
{
    LOG_DEBUG() << "begin";
    setObjectName("ElementsDock");
    QIcon icon = QIcon::fromTheme("fire", QIcon(":/icons/oxygen/32x32/fire.png"));
    toggleViewAction()->setIcon(icon);
    setWhatsThis("https://shotcut.org");

    auto *mainWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Category toolbar (wrapping flow of tool buttons)
    m_categoryToolbar = new QWidget(mainWidget);
    auto *flowLayout = new FlowLayout(m_categoryToolbar, 2, 2, 2);
    m_categoryToolbar->setLayout(flowLayout);
    m_categoryGroup = new QActionGroup(this);
    m_categoryGroup->setExclusive(true);
    layout->addWidget(m_categoryToolbar);

    // Stacked content area
    m_stack = new QStackedWidget(mainWidget);
    layout->addWidget(m_stack);

    setWidget(mainWidget);

    // Locate installed resource directories
    auto dir = QmlApplication::dataDir();
    dir.cd("shotcut");
    dir.cd("elements");

    // Set up one page per category
    setupCategoryPage(Page::Emojis,
                      tr("Emojis"),
                      makeTextIcon("\U0001F600"), // grin emoji
                      true,
                      QDir(dir.filePath("emojis")));
    setupCategoryPage(Page::Sounds,
                      tr("Sounds"),
                      makeTextIcon("\U0001F50A"), // speaker emoji
                      false,
                      QDir(dir.filePath("sounds")));
    setupCategoryPage(Page::Text,
                      tr("Text"),
                      makeTextIcon("Sh", QColor("#E05050")),
                      true,
                      QDir(dir.filePath("text")));
    setupCategoryPage(Page::Transitions,
                      tr("Transitions"),
                      makeTextIcon("\u25E7", QColor("#9370DB")),
                      false,
                      QDir(dir.filePath("transitions")));
    setupCategoryPage(Page::Graphics,
                      tr("Graphics"),
                      makeTextIcon("\u2726", QColor("#39FF14")),
                      true,
                      QDir(dir.filePath("graphics")));

    // Activate first category
    updateToolbarStyle();
    if (!m_categoryGroup->actions().isEmpty())
        m_categoryGroup->actions().first()->trigger();

    // Stop audio when switching pages
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int) {
        stopHoverPreview();
        stopAudioPreview();
    });

    LOG_DEBUG() << "end";
}

ElementsDock::~ElementsDock()
{
    stopHoverPreview();
    stopAudioPreview();
}

// ---------------------------------------------------------------------------
// Setup helpers
// ---------------------------------------------------------------------------

void ElementsDock::setupCategoryPage(
    Page page, const QString &name, const QIcon &icon, bool autoFilter, const QDir &dir)
{
    const int pageIndex = static_cast<int>(page);
    // Category action
    auto *action = new QAction(icon, tr(name.toUtf8().constData()), this);
    action->setCheckable(true);
    m_categoryGroup->addAction(action);
    auto *btn = new QToolButton(m_categoryToolbar);
    btn->setDefaultAction(action);
    btn->setIconSize(QSize(24, 24));
    m_categoryToolbar->layout()->addWidget(btn);
    connect(action, &QAction::triggered, this, [this, pageIndex]() {
        m_stack->setCurrentIndex(pageIndex);
    });

    // Model and proxy
    auto *model = new ElementsModel(dir, this);
    auto *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);

    // View
    auto *view = new PlaylistIconView(this);
    view->setModel(proxyModel);
    view->setIconRole(ElementsModel::ThumbnailRole);
    view->setElideMode(Qt::ElideRight);
    view->setMouseTracking(true);
    view->viewport()->setMouseTracking(true);
    view->viewport()->installEventFilter(this);

    connect(view, &QAbstractItemView::activated, this, [this, page](const QModelIndex &idx) {
        onActivated(idx, page);
    });

    if (page == Page::Sounds) {
        // Play on selection change
        connect(view->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                [this, proxyModel](const QModelIndex &current, const QModelIndex &) {
                    if (!current.isValid()) {
                        stopAudioPreview();
                        return;
                    }
                    const auto srcIdx = proxyModel->mapToSource(current);
                    const QString fp = srcIdx.data(ElementsModel::FilePathRole).toString();
                    startAudioPreview(fp);
                });
        // Re-play when clicking an already-current item (currentChanged doesn't fire)
        connect(view,
                &QAbstractItemView::clicked,
                this,
                [this, proxyModel](const QModelIndex &current) {
                    if (!current.isValid())
                        return;
                    const auto srcIdx = proxyModel->mapToSource(current);
                    const QString fp = srcIdx.data(ElementsModel::FilePathRole).toString();
                    startAudioPreview(fp);
                });
    }

    // Wrap in a page widget
    auto *pageWidget = new QWidget(this);
    auto *pageLayout = new QVBoxLayout(pageWidget);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    if (page == Page::Emojis) {
        static const QList<QPair<QString, QString>> kEmojiCategories = {
            {tr("Smileys & Emotion"), QStringLiteral("standard")},
            {tr("People & Body"), QStringLiteral("person")},
            {tr("Animals & Nature"), QStringLiteral("nature")},
            {tr("Food & Drink"), QStringLiteral("food")},
            {tr("Travel & Places"), QStringLiteral("travel")},
            {tr("Activities"), QStringLiteral("activity")},
            {tr("Objects"), QStringLiteral("object")},
            {tr("Symbols"), QStringLiteral("symbol")},
        };
        auto *combo = new QComboBox(pageWidget);
        for (const auto &cat : kEmojiCategories)
            combo->addItem(cat.first, cat.second);
        pageLayout->addWidget(combo);

        // Load the first sub-category immediately
        model->setDir(QDir(dir.filePath(kEmojiCategories.first().second)));

        connect(combo, &QComboBox::currentIndexChanged, this, [model, dir, combo](int index) {
            const QString subFolder = combo->itemData(index).toString();
            model->setDir(QDir(dir.filePath(subFolder)));
        });
    }

    pageLayout->addWidget(view);
    m_stack->addWidget(pageWidget);

    m_pages.append({name, autoFilter, model, proxyModel, view});
}

// ---------------------------------------------------------------------------
// Resize → adaptive toolbar style
// ---------------------------------------------------------------------------

void ElementsDock::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);
    updateToolbarStyle();
}

void ElementsDock::updateToolbarStyle()
{
    const bool textUnder = width() >= kTextUnderIconThreshold;
    const auto style = textUnder ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;
    const auto buttons = m_categoryToolbar->findChildren<QToolButton *>();
    for (auto *btn : buttons) {
        if (btn->toolButtonStyle() != style)
            btn->setToolButtonStyle(style);
        if (textUnder) {
            // Account for FlowLayout margins (2px each side) and inter-button spacing (2px * gaps)
            const int n = qMax(1, buttons.count());
            const int btnWidth = (width() - 4 - (n - 1) * 2) / n;
            btn->setFixedWidth(btnWidth);
        } else {
            btn->setFixedWidth(btn->sizeHint().width());
        }
    }
}

// ---------------------------------------------------------------------------
// Event filter — hover preview + drag initiation
// ---------------------------------------------------------------------------

bool ElementsDock::eventFilter(QObject *watched, QEvent *event)
{
    // Identify which page the event belongs to
    int pageIndex = -1;
    for (int i = 0; i < m_pages.count(); ++i) {
        if (watched == m_pages[i].view->viewport()) {
            pageIndex = i;
            break;
        }
    }
    if (pageIndex < 0)
        return QDockWidget::eventFilter(watched, event);

    const auto &page = m_pages[pageIndex];

    if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent *>(event);

        if (me->buttons() == Qt::NoButton) {
            // ---- Hover logic ----
            const auto proxyIdx = page.view->indexAt(me->pos());
            if (QPersistentModelIndex(proxyIdx) != m_hoveredProxyIndex) {
                stopHoverPreview();
                m_hoveredProxyIndex = QPersistentModelIndex(proxyIdx);

                if (proxyIdx.isValid()) {
                    const auto sourceIdx = page.proxyModel->mapToSource(proxyIdx);
                    m_hoveredSourceIndex = QPersistentModelIndex(sourceIdx);
                    m_hoveredPageIndex = pageIndex;
                    const QString filePath = sourceIdx.data(ElementsModel::FilePathRole).toString();

                    // Animated .webp companion
                    if (sourceIdx.data(ElementsModel::HasAnimationRole).toBool()) {
                        const QString webpPath = QFileInfo(filePath).dir().filePath(
                            QFileInfo(filePath).completeBaseName() + ".webp");
                        startHoverAnimation(sourceIdx, webpPath);
                    }
                }
            }

        } else if (me->buttons() & Qt::LeftButton) {
            // ---- Drag initiation ----
            if (!m_dragStartPos.isNull()
                && (me->pos() - m_dragStartPos).manhattanLength()
                       >= QApplication::startDragDistance()) {
                const auto idx = page.view->indexAt(m_dragStartPos);
                if (idx.isValid()) {
                    const auto srcIdx = page.proxyModel->mapToSource(idx);
                    const QString filePath = srcIdx.data(ElementsModel::FilePathRole).toString();
                    const bool isSound = pageIndex == static_cast<int>(Page::Sounds);
                    const bool makeUnique = (pageIndex != static_cast<int>(Page::Emojis)
                                             && !isSound);
                    QString dest;
                    bool didCreate = false;
                    auto *mimeData = buildMimeData(filePath,
                                                   page.name,
                                                   page.autoFilter,
                                                   isSound,
                                                   makeUnique,
                                                   &dest,
                                                   &didCreate);
                    if (mimeData) {
                        stopAudioPreview();
                        auto *drag = new QDrag(page.view);
                        drag->setMimeData(mimeData);
                        const Qt::DropAction action = drag->exec(Qt::CopyAction);
                        if (action != Qt::CopyAction && didCreate)
                            QFile::remove(dest);
                    }
                }
                m_dragStartPos = QPoint();
                return true;
            }
        }

    } else if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            m_dragStartPos = me->pos();
            m_dragPageIndex = pageIndex;
        }

    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_dragStartPos = QPoint();
        m_dragPageIndex = -1;

    } else if (event->type() == QEvent::Leave) {
        stopHoverPreview();
        m_dragStartPos = QPoint();
        m_dragPageIndex = -1;
    }

    return QDockWidget::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------
// Hover preview helpers
// ---------------------------------------------------------------------------

void ElementsDock::stopHoverPreview()
{
    if (m_previewTimer)
        m_previewTimer->stop();

    if (m_activeMovie) {
        m_activeMovie->stop();
        // Restore the DB thumbnail by clearing the override
        if (m_hoveredSourceIndex.isValid() && m_hoveredPageIndex >= 0
            && m_hoveredPageIndex < m_pages.count() && m_pages[m_hoveredPageIndex].model) {
            m_pages[m_hoveredPageIndex].model->setData(m_hoveredSourceIndex,
                                                       QImage(),
                                                       ElementsModel::ThumbnailRole);
        }
        delete m_activeMovie;
        m_activeMovie = nullptr;
    }

    m_hoveredProxyIndex = QPersistentModelIndex();
    m_hoveredSourceIndex = QPersistentModelIndex();
    m_hoveredPageIndex = -1;
}

void ElementsDock::startHoverAnimation(const QModelIndex &sourceIndex, const QString &webpPath)
{
    m_activeMovie = new QMovie(webpPath, QByteArray(), this);
    if (!m_activeMovie->isValid() || m_activeMovie->frameCount() <= 1) {
        delete m_activeMovie;
        m_activeMovie = nullptr;
        return;
    }

    m_activeMovie->setScaledSize(QSize(128, 72));

    if (m_hoveredPageIndex < 0 || m_hoveredPageIndex >= m_pages.count()
        || !m_pages[m_hoveredPageIndex].model) {
        delete m_activeMovie;
        m_activeMovie = nullptr;
        return;
    }
    ElementsModel *sourceModel = m_pages[m_hoveredPageIndex].model;

    QPersistentModelIndex persistentSource(sourceIndex);
    connect(m_activeMovie, &QMovie::frameChanged, this, [this, sourceModel, persistentSource]() {
        if (persistentSource.isValid() && m_activeMovie) {
            sourceModel->setData(persistentSource,
                                 QVariant::fromValue(m_activeMovie->currentImage()),
                                 ElementsModel::ThumbnailRole);
        }
    });

    m_activeMovie->start();
}

// ---------------------------------------------------------------------------
// Activated (double-click / Enter)
// ---------------------------------------------------------------------------

void ElementsDock::stopAudioPreview()
{
    if (m_audioConsumer && m_audioConsumer->is_valid())
        m_audioConsumer->stop();
}

void ElementsDock::startAudioPreview(const QString &filePath)
{
    stopAudioPreview();
    Mlt::Producer producer(MLT.profile(), filePath.toUtf8().constData());
    if (!producer.is_valid())
        return;
    m_audioConsumer.reset(new Mlt::Consumer(MLT.profile(), "rtaudio"));
    if (!m_audioConsumer || !m_audioConsumer->is_valid()) {
        m_audioConsumer.reset();
        return;
    }
    m_audioConsumer->connect(producer);
    m_audioConsumer->set("terminate_on_pause", 1);
    m_audioConsumer->set("video_off", 1);
    m_audioConsumer->start();
}

void ElementsDock::onActivated(const QModelIndex &proxyIndex, Page page)
{
    const int pageIndex = static_cast<int>(page);
    if (!proxyIndex.isValid() || pageIndex < 0 || pageIndex >= m_pages.count())
        return;
    const auto &pg = m_pages[pageIndex];
    const auto sourceIdx = pg.proxyModel->mapToSource(proxyIndex);
    const QString filePath = sourceIdx.data(ElementsModel::FilePathRole).toString();

    const bool makeUnique = (page != Page::Emojis && page != Page::Sounds);
    auto *producer
        = copyAndCreateProducer(filePath, pg.name, pg.autoFilter, page == Page::Sounds, makeUnique);
    if (producer)
        MAIN.open(producer);
}

// ---------------------------------------------------------------------------
// Copy + create producer
// ---------------------------------------------------------------------------

Mlt::Producer *ElementsDock::copyAndCreateProducer(const QString &sourcePath,
                                                   const QString &categoryName,
                                                   bool autoFilter,
                                                   bool isSoundEffect,
                                                   bool makeUnique,
                                                   QString *actualDest,
                                                   bool *didCreate)
{
    QString dest = destPath(sourcePath, categoryName);
    if (dest.isEmpty())
        return nullptr;

    if (makeUnique && QFile::exists(dest)) {
        const QFileInfo info(dest);
        const QString dir = info.absolutePath();
        const QString base = info.completeBaseName();
        const QString ext = info.suffix();
        for (unsigned i = 1; QFile::exists(dest); ++i)
            dest = QDir(dir).filePath(
                QStringLiteral("%1-%2.%3").arg(base).arg(i, 3, 10, QChar('0')).arg(ext));
    }

    QDir().mkpath(QFileInfo(dest).absolutePath());

    const auto alreadyExists = QFile::exists(dest);
    if (!alreadyExists && !QFile::copy(sourcePath, dest)) {
        LOG_WARNING() << "Failed to copy" << sourcePath << "to" << dest;
        return nullptr;
    }

    if (actualDest)
        *actualDest = dest;
    if (didCreate)
        *didCreate = !alreadyExists;

    auto *producer = new Mlt::Producer(MLT.profile(), dest.toUtf8().constData());
    if (!producer->is_valid()) {
        LOG_WARNING() << "Failed to create producer for" << dest;
        if (!alreadyExists)
            QFile::remove(dest);
        delete producer;
        return nullptr;
    }

    if (isSoundEffect) {
        producer->set("video_index", -1);
        Mlt::Filter volumeFilter(MLT.profile(), "volume");
        if (volumeFilter.is_valid()) {
            volumeFilter.set("level", 0.0);
            volumeFilter.set(kShotcutFilterProperty, "audioGain");
            producer->attach(volumeFilter);
        }
    }

    if (autoFilter) {
        QSize mediaSize;
        const int pw = MLT.profile().width();
        const int ph = MLT.profile().height();
        if (categoryName == "emojis") {
            // Emojis are square; fit within project dimensions preserving 1:1
            const int side = qMin(512, qMin(pw, ph));
            mediaSize = QSize(side, side);
        } else {
            const int w = producer->get_int("meta.media.width");
            const int h = producer->get_int("meta.media.height");
            if (w > 0 && h > 0) {
                // Scale down to fit within project dimensions, preserving aspect ratio; never upscale
                QSize orig(w, h);
                QSize scaled = orig.scaled(pw, ph, Qt::KeepAspectRatio);
                mediaSize = (w > pw || h > ph) ? scaled : orig;
            }
        }
        attachSizeFilter(producer, mediaSize);
    }

    return producer;
}

QMimeData *ElementsDock::buildMimeData(const QString &sourcePath,
                                       const QString &categoryName,
                                       bool autoFilter,
                                       bool isSoundEffect,
                                       bool makeUnique,
                                       QString *destPath,
                                       bool *didCreate)
{
    auto *producer = copyAndCreateProducer(sourcePath,
                                           categoryName,
                                           autoFilter,
                                           isSoundEffect,
                                           makeUnique,
                                           destPath,
                                           didCreate);
    if (!producer)
        return nullptr;

    const QString dest = QString::fromUtf8(producer->get("resource"));
    if (destPath)
        *destPath = dest;
    auto *mimeData = new QMimeData;
    mimeData->setData(Mlt::XmlMimeType, MLT.XML(producer).toUtf8());
    mimeData->setUrls({QUrl::fromLocalFile(dest)});
    delete producer;
    return mimeData;
}

void ElementsDock::attachSizeFilter(Mlt::Producer *producer, QSize fixedSize)
{
    const char *service = Settings.playerGPU() ? "movit.rect" : "affine";
    const char *filterName = Settings.playerGPU() ? "movitSizePosition" : "affineSizePosition";
    Mlt::Filter f(MLT.profile(), service);
    const int pw = MLT.profile().width();
    const int ph = MLT.profile().height();
    QString rectStr;
    if (fixedSize.isValid()) {
        const int x = (pw - fixedSize.width()) / 2;
        const int y = (ph - fixedSize.height()) / 2;
        rectStr = QStringLiteral("%1 %2 %3 %4 1.0")
                      .arg(x)
                      .arg(y)
                      .arg(fixedSize.width())
                      .arg(fixedSize.height());
    } else {
        rectStr = QStringLiteral("0 0 %1 %2 1.0").arg(pw).arg(ph);
    }
    if (Settings.playerGPU()) {
        f.set("rect", rectStr.toLatin1().constData());
        f.set("fill", 1);
        f.set("distort", 0);
        f.set("valign", "middle");
        f.set("halign", "center");
    } else {
        f.set("transition.rect", rectStr.toLatin1().constData());
        f.set("transition.fill", 1);
        f.set("transition.distort", 0);
        f.set("transition.valign", "middle");
        f.set("transition.halign", "center");
        f.set("transition.threads", 0);
        f.set("background", "color:#00000000");
    }
    f.set(kShotcutFilterProperty, filterName);
    producer->attach(f);
}

QString ElementsDock::destPath(const QString &sourcePath, const QString &categoryName) const
{
    QString baseDir;
    const auto &currentFile = MAIN.fileName();
    if (!currentFile.isEmpty()) {
        baseDir = QFileInfo(currentFile).absolutePath();
    } else {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return QDir(baseDir).filePath(categoryName + "/" + QFileInfo(sourcePath).fileName());
}
