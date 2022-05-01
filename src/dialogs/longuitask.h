/*
 * Copyright (c) 2020 Meltytech, LLC
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

#ifndef LONGUITASK_H
#define LONGUITASK_H

#include <QFuture>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>

class LongUiTask : public QProgressDialog
{
public:
    explicit LongUiTask(QString title);
    ~LongUiTask();

    template <class Ret>
    Ret wait(QString text, const QFuture<Ret> &future)
    {
        setLabelText(text);
        setRange(0, 0);
        while (!future.isFinished()) {
            setValue(0);
            QCoreApplication::processEvents();
            QThread::msleep(100);
        }
        return future.result();
    }

    template <class Ret, class Func, class Arg>
    Ret runAsync(QString text, Func &&f, Arg &&arg)
    {
        QFuture<Ret> future = QtConcurrent::run(f, arg);
        return wait<Ret>(text, future);
    }

    void reportProgress(QString text, int value, int max);
    static void cancel();
};

#endif // LONGUITASK_H
