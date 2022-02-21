/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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
#include <Logger.h>
#include <QtConcurrent/QtConcurrent>

ScopeWidget::ScopeWidget(const QString& name)
  : QWidget()
  , m_queue(3, DataQueue<SharedFrame>::OverflowModeDiscardOldest)
  , m_thread(nullptr)
  , m_mutex(QMutex::NonRecursive)
  , m_forceRefresh(false)
  , m_size(0, 0)
{
    LOG_DEBUG() << "begin";
    setObjectName(name);
    m_thread = QThread::create([&]{ refreshInThread(); });
    m_thread->start(QThread::LowPriority);
    LOG_DEBUG() << "end";
}

ScopeWidget::~ScopeWidget()
{
    m_thread->requestInterruption();
    m_refreshSempaphore.release();
    m_thread->wait(1000);
    delete m_thread;
}

void ScopeWidget::onNewFrame(const SharedFrame& frame)
{
    m_queue.push(frame);
    requestRefresh();
}

void ScopeWidget::requestRefresh()
{
    if (!m_refreshSempaphore.available()) {
        m_refreshSempaphore.release();
    }
}

void ScopeWidget::refreshInThread()
{
    while (true) {
        m_refreshSempaphore.acquire();

        if (m_thread->isInterruptionRequested()) {
            break;
        }

        m_mutex.lock();
        if (m_size.isEmpty()) {
            m_mutex.unlock();
            continue;
        }
        QSize size = m_size;
        bool full = m_forceRefresh;
        m_forceRefresh = false;
        m_mutex.unlock();

        refreshScope(size, full);
        // Tell the GUI thread that the refresh is complete.
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }
    m_thread->quit();
}

void ScopeWidget::resizeEvent(QResizeEvent*)
{
    m_mutex.lock();
    m_size = size();
    m_mutex.unlock();
    if (isVisible()) {
        requestRefresh();
    }
}

void ScopeWidget::changeEvent(QEvent*)
{
    m_mutex.lock();
    m_forceRefresh = true;
    m_mutex.unlock();
    if (isVisible()) {
        requestRefresh();
    }
}
