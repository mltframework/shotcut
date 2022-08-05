/*
 * Copyright (c) 2022 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your tbOption) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "docktoolbar.h"

#include "settings.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolBar>

#include <QDebug>

DockToolBar::DockToolBar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
    , m_area(Qt::TopToolBarArea)
{
    setMovable(false);
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setFloatable(false);
    setProperty("Movable", QVariant(false));
    updateStyle();
    connect(&Settings, SIGNAL(smallIconsChanged()), SLOT(updateStyle()));
}

void DockToolBar::setAreaHint(Qt::ToolBarArea area)
{
    m_area = area;
}

void DockToolBar::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QLinearGradient gradient = QLinearGradient(rect().left(), rect().center().y(),
                                               rect().right(), rect().center().y());
    gradient.setColorAt(0, palette().window().color().lighter(104));
    gradient.setColorAt(1, palette().window().color());
    p.fillRect(rect(), gradient);
    if (m_area == Qt::TopToolBarArea) {
        // Apply the same styling that is applied to main window toolbars.
        // This creates extra lines of separation between the toolbar and the
        // dock contents.
        QColor light = QColor(255, 255, 255, 90);
        QColor shadow = QColor(0, 0, 0, 60);
        p.setPen(shadow);
        p.drawLine(rect().bottomLeft(), rect().bottomRight());
        p.setPen(light);
        p.drawLine(rect().topLeft(), rect().topRight());
    }
}

void DockToolBar::updateStyle()
{
    int height = 33;
    if (Settings.smallIcons()) {
        height = 25;
    }
    setFixedHeight(height);
    setIconSize(QSize(height - 9, height - 9));
    QString styleSheet = QString::fromUtf8( "   \
         QToolButton {                          \
           width:%1px;                          \
           height:%1px;                         \
         }                                      \
         QToolButton:checked {                  \
           color:palette(highlighted-text);     \
           background-color:palette(highlight); \
         }                                      \
         QToolBar {                             \
           spacing:3px;                         \
           padding:1px;                         \
         }                                      \
        ").arg(height - 9);
    setStyleSheet(styleSheet);
}
