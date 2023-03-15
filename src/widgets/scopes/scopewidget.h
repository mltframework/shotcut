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

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include <QWidget>
#include <QString>
#include <Logger.h>
#include <QThread>
#include <QFuture>
#include <QMutex>
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
  the paintEvent() implementation will complete quickly to avoid hanging up the
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
    explicit ScopeWidget(const QString &name);

    //! Destructs a ScopeWidget.
    virtual ~ScopeWidget();

    /*!
      Returns the title of the scope to be displayed by the application.
      This virtual function must be implemented by subclasses.
    */
    virtual QString getTitle() = 0;

    /*!
      Sets the preferred orientation on the scope.
      This virtual function may be reimplemented by subclasses.
    */
    virtual void setOrientation(Qt::Orientation) {};

public slots:
    //! Provides a new frame to the scope. Should be called by the application.
    virtual void onNewFrame(const SharedFrame &frame) Q_DECL_FINAL;

signals:
    //! Tells the widget it has been moved. Should be called by the application.
    void moved();

protected:
    /*!
      Triggers refreshScope() to be called in a new thread context.
      Typically requestRefresh would be called from the GUI thread
      (e.g. in resizeEvent()). onNewFrame() also calls requestRefresh().
    */
    virtual void requestRefresh() Q_DECL_FINAL;

    /*!
      Performs the main, CPU intensive, scope drawing in a new thread.

      refreshScope() Shall be implemented by subclasses. Care must be taken to
      protect any members that may be accessed concurrently by the refresh
      thread and the GUI thread.
    */
    virtual void refreshScope(const QSize &size, bool full) = 0;

    /*!
      Stores frames received by onNewFrame().

      Subclasses should check this queue for new frames in the refreshScope()
      implementation.
    */
    DataQueue<SharedFrame> m_queue;

    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
    void changeEvent(QEvent *) Q_DECL_OVERRIDE;

private:
    Q_INVOKABLE virtual void onRefreshThreadComplete() Q_DECL_FINAL;
    virtual void refreshInThread() Q_DECL_FINAL;
    QFuture<void> m_future;
    bool m_refreshPending;

    // Members accessed in multiple threads (mutex protected).
    QMutex m_mutex;
    bool m_forceRefresh;
    QSize m_size;
};

#endif // SCOPEWIDGET_H
