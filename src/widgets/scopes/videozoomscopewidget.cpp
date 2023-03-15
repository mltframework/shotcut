/*
 * Copyright (c) 2019-2023 Meltytech, LLC
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

#include "videozoomscopewidget.h"

#include "videowidget.h"
#include "videozoomwidget.h"

#include <QLabel>
#include <QToolButton>
#include <QToolTip>
#include <QGridLayout>
#include <QHBoxLayout>

#include <math.h>

QWidget *getSeparator()
{
    // Create a 1 pixel wide line separator with a contrasting color
    QWidget *separator = new QWidget();
    separator->setGeometry(0, 0, 300, 300);
    separator->setMinimumSize(1, 1);
    QPalette pal = separator->palette();
//    pal.setColor(QPalette::Background, pal.color(QPalette::WindowText));
    separator->setAutoFillBackground(true);
    separator->setPalette(pal);
    return separator;
}

QRect getPlayerBoundingRect(Mlt::VideoWidget *videoWidget)
{
    // Get the global rectangle of the player that contains image.
    // This function assumes that the player is zoomed to best fit the image
    // so that all of the image is available and it fills the widget in one
    // direction.
    QRect rect;
    double widgetAr = (double)videoWidget->width() / (double)videoWidget->height();
    double vidAr = MLT.profile().dar();
    if (widgetAr > vidAr) {
        double width = (double)videoWidget->height() * vidAr;
        rect.setX((round((double)videoWidget->width() - width) / 2));
        rect.setY(0);
        rect.setWidth(round(width));
        rect.setHeight(videoWidget->height());
    } else {
        double height = videoWidget->width() / vidAr;
        rect.setX(0);
        rect.setY((round((double)videoWidget->height() - height) / 2));
        rect.setWidth(videoWidget->width());
        rect.setHeight(round(height));
    }
    return QRect(videoWidget->mapToGlobal(rect.topLeft()), rect.size());
}

QPoint pixelToPlayerPos(const QRect &playerRect, const QPoint &pixel)
{
    // Convert a pixel index to the corresponding global screen position of that
    // pixel in the player.
    double xOffset = (double)playerRect.width() * (double)pixel.x() / (double)MLT.profile().width();
    double yOffset = (double)playerRect.height() * (double)pixel.y() / (double)MLT.profile().height();
    return playerRect.topLeft() + QPoint(round(xOffset), round(yOffset));
}

QPoint playerPosToPixel(const QRect &playerRect, const QPoint &pos)
{
    // Convert the global position of a point in the player to the corresponding
    // pixel index.
    QPoint offset = pos - playerRect.topLeft();
    double xOffset = (double)MLT.profile().width() * (double)offset.x() / (double)playerRect.width();
    double yOffset = (double)MLT.profile().height() * (double)offset.y() / (double)playerRect.height();
    return QPoint(round(xOffset), round(yOffset));
}

VideoZoomScopeWidget::VideoZoomScopeWidget()
    : ScopeWidget("VideoZoom")
    , m_zoomWidget(new VideoZoomWidget())
    , m_zoomLabel(new QLabel(this))
    , m_pixelXLabel(new QLabel(this))
    , m_pixelYLabel(new QLabel(this))
    , m_rLabel(new QLabel(this))
    , m_gLabel(new QLabel(this))
    , m_bLabel(new QLabel(this))
    , m_yLabel(new QLabel(this))
    , m_uLabel(new QLabel(this))
    , m_vLabel(new QLabel(this))
    , m_lockButton(new QToolButton(this))
{
    LOG_DEBUG() << "begin";
    QFont font = QWidget::font();
    int fontSize = font.pointSize() - (font.pointSize() > 10 ? 2 : (font.pointSize() > 8 ? 1 : 0));
    font.setPointSize(fontSize);
    QWidget::setFont(font);

    QHBoxLayout *hlayout = new QHBoxLayout(this);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    QGridLayout *glayout = new QGridLayout();
    glayout->setContentsMargins(5, 5, 2, 0);
    glayout->setHorizontalSpacing(0);
    glayout->setVerticalSpacing(2);

    // Add labels
    glayout->addWidget(m_zoomLabel, 0, 0, 1, 2);
    glayout->addWidget(getSeparator(), 1, 0, 1, 2);
    glayout->addWidget(new QLabel(tr("x")), 2, 0, Qt::AlignLeft);
    glayout->addWidget(m_pixelXLabel, 2, 1, Qt::AlignRight);
    glayout->addWidget(new QLabel(tr("y")), 3, 0, Qt::AlignLeft);
    glayout->addWidget(m_pixelYLabel, 3, 1, Qt::AlignRight);
    glayout->addWidget(getSeparator(), 4, 0, 1, 2);
    glayout->addWidget(new QLabel(tr("R")), 5, 0, Qt::AlignLeft);
    glayout->addWidget(m_rLabel, 5, 1, Qt::AlignRight);
    glayout->addWidget(new QLabel(tr("G")), 6, 0, Qt::AlignLeft);
    glayout->addWidget(m_gLabel, 6, 1, Qt::AlignRight);
    glayout->addWidget(new QLabel(tr("B")), 7, 0, Qt::AlignLeft);
    glayout->addWidget(m_bLabel, 7, 1, Qt::AlignRight);
    glayout->addWidget(getSeparator(), 8, 0, 1, 2);
    glayout->addWidget(new QLabel(tr("Y")), 9, 0, Qt::AlignLeft);
    glayout->addWidget(m_yLabel, 9, 1, Qt::AlignRight);
    glayout->addWidget(new QLabel(tr("U")), 10, 0, Qt::AlignLeft);
    glayout->addWidget(m_uLabel, 10, 1, Qt::AlignRight);
    glayout->addWidget(new QLabel(tr("V")), 11, 0, Qt::AlignLeft);
    glayout->addWidget(m_vLabel, 11, 1, Qt::AlignRight);
    glayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 12, 0, 1, 2);
    updateLabels();
    onZoomChanged(m_zoomWidget->getZoom());

    // Add HBoxLayout for tool buttons
    QHBoxLayout *toolLayout = new QHBoxLayout();
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(0);
    glayout->addLayout(toolLayout, 13, 0, 1, 2);

    // Add pixel picker button
    QToolButton *pickButton = new QToolButton(this);
    pickButton->setToolTip(tr("Pick a pixel from the source player"));
    pickButton->setIcon(QIcon::fromTheme("zoom-select",
                                         QIcon(":/icons/oxygen/32x32/actions/zoom-select")));
    toolLayout->addWidget(pickButton);
    connect(pickButton, SIGNAL(clicked()), this, SLOT(onScreenSelectStarted()));

    // Add pixel lock button
    m_lockButton->setToolTip(tr("Lock/Unlock the selected pixel"));
    m_lockButton->setIcon(QIcon::fromTheme("object-unlocked",
                                           QIcon(":/icons/oxygen/32x32/status/object-unlocked")));
    m_lockButton->setCheckable(true);
    m_lockButton->setChecked(false);
    toolLayout->addWidget(m_lockButton);
    connect(m_lockButton, SIGNAL(toggled(bool)), this, SLOT(onLockToggled(bool)));

    toolLayout->addStretch();

    // Set minimum size for the grid layout.
    QRect col1Size = fontMetrics().boundingRect("X ");
    glayout->setColumnMinimumWidth(0, col1Size.width());
    QRect col2Size = fontMetrics().boundingRect("9999");
    glayout->setColumnMinimumWidth(1, col2Size.width());

    hlayout->addLayout(glayout);
    hlayout->addWidget(m_zoomWidget);

    connect(m_zoomWidget, SIGNAL(pixelSelected(const QPoint &)), this,
            SLOT(onPixelSelected(const QPoint &)));
    connect(m_zoomWidget, SIGNAL(zoomChanged(int)), this, SLOT(onZoomChanged(int)));
    connect(&m_selector, SIGNAL(screenSelected(const QRect &)), this,
            SLOT(onScreenRectSelected(const QRect &)));
    connect(&m_selector, SIGNAL(pointSelected(const QPoint &)), this,
            SLOT(onScreenPointSelected(const QPoint &)));
    LOG_DEBUG() << "end";
}

void VideoZoomScopeWidget::onScreenSelectStarted()
{
    if (!MLT.producer() || !MLT.producer()->is_valid()) {
        return;
    }

    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    // Toggle the zoom off on the player so that the entire image is displayed
    // in the player. The user can toggle it back on if they want.
    videoWidget->toggleZoom(false);

    // Get the global rectangle in the player that has the image in it.
    QRect boundingRect = getPlayerBoundingRect(videoWidget);
    m_selector.setBoundingRect(boundingRect);

    // Calculate the size of the zoom window to show over the image.
    QSize selectionSize;
    selectionSize.setWidth((double)boundingRect.width() * (((double)m_zoomWidget->width() /
                                                            (double)m_zoomWidget->getZoom()) / (double)MLT.profile().width()));
    selectionSize.setHeight((double)boundingRect.height() * (((double)m_zoomWidget->height() /
                                                              (double)m_zoomWidget->getZoom()) / (double)MLT.profile().height()));
    m_selector.setFixedSize(selectionSize);

    // Calculate the global position of the zoom window.
    QRect zoomRect = m_zoomWidget->getPixelRect();
    QRect selectedRect;
    selectedRect.setTopLeft(pixelToPlayerPos(boundingRect, zoomRect.topLeft()));
    selectedRect.setBottomRight(pixelToPlayerPos(boundingRect, zoomRect.bottomRight()));
    m_selector.setSelectedRect(selectedRect);

    // Calculate the global position of the selected pixel.
    QPoint startPoint = pixelToPlayerPos(boundingRect, m_zoomWidget->getSelectedPixel());
    m_selector.startSelection(startPoint);
}

void VideoZoomScopeWidget::onLockToggled(bool enabled)
{
    m_zoomWidget->lock(enabled);
    if (enabled) {
        m_lockButton->setIcon(QIcon::fromTheme("object-locked",
                                               QIcon(":/icons/oxygen/32x32/status/object-locked")));
    } else {
        m_lockButton->setIcon(QIcon::fromTheme("object-unlocked",
                                               QIcon(":/icons/oxygen/32x32/status/object-unlocked")));
    }
}

void VideoZoomScopeWidget::onScreenRectSelected(const QRect &rect)
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    QRect boundingRect = getPlayerBoundingRect(videoWidget);
    QPoint pixel = playerPosToPixel(boundingRect, rect.topLeft());
    m_zoomWidget->setOffset(pixel);
}

void VideoZoomScopeWidget::onScreenPointSelected(const QPoint &point)
{
    auto *videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget());
    QRect boundingRect = getPlayerBoundingRect(videoWidget);
    QPoint pixel = playerPosToPixel(boundingRect, point);
    m_zoomWidget->setSelectedPixel(pixel);
}

void VideoZoomScopeWidget::onPixelSelected(const QPoint &pixel)
{
    Q_UNUSED(pixel);
    updateLabels();
}

void VideoZoomScopeWidget::onZoomChanged(int zoom)
{
    m_zoomLabel->setText(tr("%1x").arg(zoom));
}

void VideoZoomScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(size)
    Q_UNUSED(full)

    SharedFrame frame;

    while (m_queue.count() > 0) {
        frame = m_queue.pop();
    }

    if (frame.is_valid()) {
        m_zoomWidget->putFrame(frame);
        QMetaObject::invokeMethod(this, "updateLabels", Qt::QueuedConnection);
    }
}

void VideoZoomScopeWidget::updateLabels()
{
    QPoint selectedPixel = m_zoomWidget->getSelectedPixel();

    if (selectedPixel.x() >= 0) {
        VideoZoomWidget::PixelValues values = m_zoomWidget->getPixelValues(selectedPixel);
        m_pixelXLabel->setText(QString::number(selectedPixel.x() + 1));
        m_pixelYLabel->setText(QString::number(selectedPixel.y() + 1));
        m_rLabel->setText(QString::number(values.r));
        m_gLabel->setText(QString::number(values.g));
        m_bLabel->setText(QString::number(values.b));
        m_yLabel->setText(QString::number(values.y));
        m_uLabel->setText(QString::number(values.u));
        m_vLabel->setText(QString::number(values.v));
    } else {
        m_pixelXLabel->setText("");
        m_pixelYLabel->setText("");
        m_rLabel->setText("---");
        m_gLabel->setText("---");
        m_bLabel->setText("---");
        m_yLabel->setText("---");
        m_uLabel->setText("---");
        m_vLabel->setText("---");
    }
}

QString VideoZoomScopeWidget::getTitle()
{
    return tr("Video Zoom");
}
