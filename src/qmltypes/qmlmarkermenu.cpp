/*
 * Copyright (c) 2022-2023 Meltytech, LLC
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

#include "qmlmarkermenu.h"

#include "actions.h"
#include "docks/timelinedock.h"
#include "qmltypes/qmlapplication.h"

#include <QColorDialog>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>

QmlMarkerMenu::QmlMarkerMenu(QObject *parent)
    : QObject(parent)
    , m_timeline(nullptr)
    , m_index(-1)
{
}

QObject *QmlMarkerMenu::target()
{
    return m_timeline;
}

void QmlMarkerMenu::setTarget(QObject *target)
{
    m_timeline = dynamic_cast<TimelineDock *>(target);
}

int QmlMarkerMenu::index()
{
    return m_index;
}
void QmlMarkerMenu::setIndex(int index)
{
    m_index = index;
}

void QmlMarkerMenu::popup()
{
    if (!m_timeline || m_index < 0)
        return;

    QMenu menu;
    Markers::Marker marker = m_timeline->markersModel()->getMarker(m_index);

    QAction editAction(tr("Edit..."));
    editAction.setShortcut(Actions["timelineMarkerAction"]->shortcut());
    connect(&editAction, &QAction::triggered, this, [&]() {
        m_timeline->editMarker(m_index);
    });
    menu.addAction(&editAction);

    QAction deleteAction(tr("Delete"));
    deleteAction.setShortcut(Actions["timelineDeleteMarkerAction"]->shortcut());
    connect(&deleteAction, &QAction::triggered, this, [&]() {
        m_timeline->deleteMarker(m_index);
    });
    menu.addAction(&deleteAction);

    QAction colorAction(tr("Choose Color..."));
    connect(&colorAction, &QAction::triggered, this, [&]() {
        QColor markerColor = m_timeline->markersModel()->getMarker(m_index).color;
        QColorDialog colorDialog(markerColor);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        colorDialog.setOptions(QColorDialog::DontUseNativeDialog);
#endif
        colorDialog.setModal(QmlApplication::dialogModality());
        if (colorDialog.exec() == QDialog::Accepted) {
            m_timeline->markersModel()->setColor(m_index, colorDialog.currentColor());
        }
    });
    menu.addAction(&colorAction);

    QMenu *recentColorMenu = menu.addMenu(tr("Choose Recent Color"));
    QStringList colors = m_timeline->markersModel()->recentColors();
    QString highlightColor = QApplication::palette().highlight().color().name();
    for (int c = 0; c < colors.size(); c++) {
        QWidgetAction *widgetAction = new QWidgetAction(recentColorMenu);
        QToolButton *colorButton = new QToolButton();
        colorButton->setText(colors[c]);
        QString textColor = QmlApplication::contrastingColor(colors[c]).name();
        QString styleSheet = QString(
                                 "QToolButton {"
                                 "    background-color: %1;"
                                 "    border-style: solid;"
                                 "    border-width: 3px;"
                                 "    border-color: %1;"
                                 "    color: %2"
                                 "}"
                                 "QToolButton:hover {"
                                 "    background-color: %1;"
                                 "    border-style: solid;"
                                 "    border-width: 3px;"
                                 "    border-color: %3;"
                                 "    color: %2"
                                 "}"
                             ).arg(colors[c]).arg(textColor).arg(highlightColor);
        colorButton->setStyleSheet(styleSheet);
        connect(colorButton, &QToolButton::clicked, this, [ &, colorButton]() {
            m_timeline->markersModel()->setColor(m_index, colorButton->text());
            menu.close();
        });
        widgetAction->setDefaultWidget(colorButton);
        recentColorMenu->addAction(widgetAction);
    }

    QAction loopAction(tr("Loop/Unloop Marker"));
    loopAction.setShortcut(Actions["timelineToggleLoopMarkerAction"]->shortcut());
    loopAction.setEnabled(marker.start != marker.end);
    connect(&loopAction, &QAction::triggered, this, [&]() {
        m_timeline->toggleLoopMarker(m_index);
    });
    menu.addAction(&loopAction);

    menu.exec(QCursor::pos());
}
