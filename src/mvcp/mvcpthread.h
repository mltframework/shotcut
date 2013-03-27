/*
 * Copyright (c) 2012-2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef MVCPTHREAD_H
#define MVCPTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QStringList>
#include <mvcp.h>

class MvcpThread : public QThread
{
    Q_OBJECT
public:
    explicit MvcpThread(const QString& address, quint16 port, QObject *parent = 0);
    ~MvcpThread();
    void run();
    void uls();
    void cls(QString path, QObject *parent);
    void usta(quint8 unit);

signals:
    void ulsResult(QStringList);  // list of unit names
    void clsResult(QObject* parent, QObjectList* children); // each object has name, full, dir, and size properties
    void ustaResult(QObject*);    // properties named same as mcvp_status

private:
    QString m_address;
    quint16 m_port;
    bool m_quit;
    QMutex m_mutex;
    QWaitCondition m_cond;
    int m_command;
    QObjectList m_commands;

    enum commands {
        COMMAND_INVALID,
        COMMAND_ULS,
        COMMAND_CLS,
        COMMAND_USTA,
    };

    void do_uls(mvcp, QObject*);
    void do_cls(mvcp, QObject*);
    void do_usta(mvcp, QObject*);
};

#endif // MVCPTHREAD_H
