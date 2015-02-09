/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "scopewidget.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

ScopeWidget::ScopeWidget(const QString& name)
  : QWidget()
  , m_queue(3, DataQueue<SharedFrame>::OverflowModeDiscardOldest)
  , m_future()
  , m_refreshPending(false)
{
    qDebug() << "begin" << m_future.isFinished();
    setObjectName(name);
    qDebug() << "end";
}

ScopeWidget::~ScopeWidget()
{
}

void ScopeWidget::onNewFrame(const SharedFrame& frame)
{
    m_queue.push(frame);
    requestRefresh();
}

void ScopeWidget::requestRefresh()
{
    if (m_future.isFinished()) {
        m_future = QtConcurrent::run(this, &ScopeWidget::refreshInThread);
    } else {
        m_refreshPending = true;
    }
}

void ScopeWidget::refreshInThread()
{
    m_refreshPending = false;
    refreshScope();
    // Tell the GUI thread that the refresh is complete.
    QMetaObject::invokeMethod(this, "onRefreshThreadComplete", Qt::QueuedConnection);
}

void ScopeWidget::onRefreshThreadComplete()
{
    update();
    if (m_refreshPending) {
        requestRefresh();
    }
}
