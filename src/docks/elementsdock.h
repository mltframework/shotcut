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

#ifndef ELEMENTSDOCK_H
#define ELEMENTSDOCK_H

#include <QColor>
#include <QDir>
#include <QDockWidget>
#include <QIcon>
#include <QModelIndex>
#include <QSize>

#include <MltConsumer.h>

#include <memory>

namespace Mlt {
class Producer;
}

class QActionGroup;
class QMimeData;
class QMovie;
class QSortFilterProxyModel;
class QStackedWidget;
class QTimer;
class QWidget;

class ElementsModel;
class PlaylistIconView;

class ElementsDock : public QDockWidget
{
    Q_OBJECT

public:
    enum class Page {
        Emojis = 0,
        Sounds = 1,
        Text = 2,
        Transitions = 3,
        Graphics = 4,
    };

    explicit ElementsDock(QWidget *parent = nullptr);
    ~ElementsDock();

    /// Build QMimeData for dragging: copies the file to the project dir and
    /// wraps the resulting MLT producer as Mlt::XmlMimeType. Returns nullptr
    /// on failure. Caller takes ownership of the returned object.
    QMimeData *buildMimeData(const QString &sourcePath,
                             const QString &categoryName,
                             bool autoFilter,
                             bool isSoundEffect = false,
                             bool makeUnique = false,
                             QString *destPath = nullptr,
                             bool *didCreate = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onActivated(const QModelIndex &proxyIndex, Page page);

private:
    struct CategoryPage
    {
        QString name;
        bool autoFilter; ///< true for emojis and text categories
        ElementsModel *model{nullptr};
        QSortFilterProxyModel *proxyModel{nullptr};
        PlaylistIconView *view{nullptr};
    };

    void setupCategoryPage(
        Page page, const QString &name, const QIcon &icon, bool autoFilter, const QDir &dir);
    void updateToolbarStyle();
    void stopHoverPreview();
    void stopAudioPreview();
    void startAudioPreview(const QString &filePath);
    void startHoverAnimation(const QModelIndex &sourceIndex, const QString &webpPath);
    QString destPath(const QString &sourcePath, const QString &categoryName) const;
    Mlt::Producer *copyAndCreateProducer(const QString &sourcePath,
                                         const QString &categoryName,
                                         bool autoFilter,
                                         bool isSoundEffect = false,
                                         bool makeUnique = false,
                                         QString *destPath = nullptr,
                                         bool *didCreate = nullptr);
    static void attachSizeFilter(Mlt::Producer *producer, QSize fixedSize = QSize());
    static QIcon makeTextIcon(const QString &text, const QColor &color = QColor());

    QWidget *m_categoryToolbar{nullptr};
    QActionGroup *m_categoryGroup{nullptr};
    QStackedWidget *m_stack{nullptr};
    QList<CategoryPage> m_pages;

    QMovie *m_activeMovie{nullptr};
    QTimer *m_previewTimer{nullptr};
    QString m_previewFilePath;
    QPersistentModelIndex m_hoveredProxyIndex;
    QPersistentModelIndex m_hoveredSourceIndex;
    int m_hoveredPageIndex{-1};
    QPoint m_dragStartPos;
    int m_dragPageIndex{-1};
    std::unique_ptr<Mlt::Consumer> m_audioConsumer;
};

#endif // ELEMENTSDOCK_H
