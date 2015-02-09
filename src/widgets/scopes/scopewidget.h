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

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include <QWidget>
#include <QString>
#include <QDebug>
#include <QThread>
#include <QFuture>
#include "sharedframe.h"
#include "dataqueue.h"

/*!
  \class ScopeWidget
  \brief The ScopeWidget provides a common interface for all scopes in Shotcut.

  ScopeWidget is a QWidget that provides some additional functionality that is
  common to all scopes. One common function is a queue that can receive and
  store frames until they can be processed by the scope. Another common function
  is the ability to trigger the "heavy lifting" to be done in a worker thread.

  Frames are received by the onNewFrame() slot. The ScopeWidget automatically
  places new frames in the DataQueue (m_queue). Subclasses shall implement the
  refreshScope() function and can check for new frames in m_queue.

  refreshScope() is run from a separate thread. Therefore, any members that are
  accessed by both the worker thread (refreshScope) and the GUI thread
  (paintEvent(), resizeEvent(), etc) must be protected by a mutex. After the
  refreshScope() function returns, the ScopeWidget will automatically request
  the GUI thread to update(). A well implemented ScopeWidget will be designed
  such that most of the CPU intensive work will be done in refreshScope() and
  the paintEvent() implementation will complete quicly to avoid hanging up the
  GUI thread.

  Subclasses shall also implement getTitle() so that the application can display
  an appropriate title for the scope.
*/

class ScopeWidget : public QWidget
{
    Q_OBJECT
    
public:
    /*!
      Constructs an ScopeWidget.

      The \a name will be set as the objectName and should be initialized by
      subclasses.
    */
    explicit ScopeWidget(const QString& name);

    //! Destructs a ScopeWidget.
    virtual ~ScopeWidget();

    /*!
      Returns the title of the scope to be displayed by the application.
      This virtual function must be implemented by subclasses.
    */
    virtual QString getTitle() = 0;

public slots:
    //! Provides a new frame to the scope. Should be called by the application.
    void onNewFrame(const SharedFrame& frame) Q_DECL_FINAL;

protected:
    /*!
      Triggers refreshScope() to be called in a new thread context.
      Typically requestRefresh would be called from the GUI thread
      (e.g. in resizeEvent()). onNewFrame() also calls requestRefresh().
    */
    void requestRefresh() Q_DECL_FINAL;

    /*!
      Performs the main, CPU intensive, scope drawing in a new thread.

      refreshScope() Shall be implemented by subclasses. Care must be taken to
      protect any members that may be accessed concurrently by the refresh
      thread and the GUI thread.
    */
    virtual void refreshScope() = 0;

    /*!
      Stores frames received by onNewFrame().

      Subclasses should check this queue for new frames in the refreshScope()
      implementation.
    */
    DataQueue<SharedFrame> m_queue;

private:
    Q_INVOKABLE void onRefreshThreadComplete() Q_DECL_FINAL;
    void refreshInThread() Q_DECL_FINAL;
    QFuture<void> m_future;
    bool m_refreshPending;
};

#endif // SCOPEWIDGET_H
