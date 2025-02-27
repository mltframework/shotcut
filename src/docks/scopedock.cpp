/*
 * Copyright (c) 2015-2023 Meltytech, LLC
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

#include "scopedock.h"

#include "Logger.h"
#include "controllers/scopecontroller.h"
#include "mltcontroller.h"

#include <QAction>
#include <QtWidgets/QScrollArea>

ScopeDock::ScopeDock(ScopeController *scopeController, ScopeWidget *scopeWidget)
    : QDockWidget()
    , m_scopeController(scopeController)
    , m_scopeWidget(scopeWidget)
{
    LOG_DEBUG() << "begin";
    setObjectName(m_scopeWidget->objectName() + "Dock");
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_scopeWidget);
    QDockWidget::setWidget(scrollArea);
    QDockWidget::setWindowTitle(m_scopeWidget->getTitle());

    connect(toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(onActionToggled(bool)));
    connect(this, &QDockWidget::dockLocationChanged, m_scopeWidget, &ScopeWidget::moved);
    LOG_DEBUG() << "end";
}

void ScopeDock::resizeEvent(QResizeEvent *e)
{
    if (width() > height()) {
        m_scopeWidget->setOrientation(Qt::Horizontal);
    } else {
        m_scopeWidget->setOrientation(Qt::Vertical);
    }
    QDockWidget::resizeEvent(e);
}

void ScopeDock::onActionToggled(bool checked)
{
    if (checked) {
        connect(m_scopeController,
                SIGNAL(newFrame(const SharedFrame &)),
                m_scopeWidget,
                SLOT(onNewFrame(const SharedFrame &)));
        MLT.refreshConsumer();
    } else {
        disconnect(m_scopeController,
                   SIGNAL(newFrame(const SharedFrame &)),
                   m_scopeWidget,
                   SLOT(onNewFrame(const SharedFrame &)));
    }
}
