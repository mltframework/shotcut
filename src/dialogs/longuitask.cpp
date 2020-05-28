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

#include "longuitask.h"

#include "mainwindow.h"

LongUiTask::LongUiTask(QString title)
    : QProgressDialog(title, QString(), 0, 0, &MAIN)
{
    setWindowTitle(title);
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    setMinimumDuration(2000);
    setRange(0, 0);
}

void LongUiTask::reportProgress(QString text, int value, int max)
{
    setLabelText(text);
    setRange(0, max);
    setValue(value);
    QCoreApplication::processEvents();
}
