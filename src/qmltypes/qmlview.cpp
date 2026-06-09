/*
 * Copyright (c) 2014-2026 Meltytech, LLC
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

#include "qmlview.h"

#include "Logger.h"

#include <QWidget>

/*!
    \qmltype QmlView
    \inqmlmodule org.shotcut.qml
    \brief Provides access to the host widget geometry, available via the \c view context property.

    \c view is injected as a context property into every dock's QML view.
    Use it when you need the screen coordinates of the host widget
    (e.g. to position pop-up elements absolutely on screen).

    \code
    var widgetPos = view.pos()
    \endcode
*/

QmlView::QmlView(QWidget *qview)
    : QObject(qview)
    , m_qview(qview)
{}

/*!
    \qmlmethod point QmlView::pos()
    \brief Returns the top-left position of the host QWidget in screen coordinates.
*/

QPoint QmlView::pos()
{
    return m_qview->mapToGlobal(QPoint(0, 0));
}
