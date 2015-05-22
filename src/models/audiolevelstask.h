/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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

#ifndef AUDIOLEVELSTASK_H
#define AUDIOLEVELSTASK_H

#include "multitrackmodel.h"
#include <QRunnable>
#include <QPersistentModelIndex>
#include <MltProducer.h>

class AudioLevelsTask : public QRunnable
{
public:
    AudioLevelsTask(Mlt::Producer& producer, MultitrackModel* model, const QModelIndex& index);
    virtual ~AudioLevelsTask();
    static void start(Mlt::Producer& producer, MultitrackModel* model, const QModelIndex& index);

protected:
    void run();

private:
    Mlt::Producer* tempProducer();
    QString cacheKey();

    Mlt::Producer m_producer;
    MultitrackModel* m_model;
    QPersistentModelIndex m_index;
    Mlt::Producer* m_tempProducer;
};

#endif // AUDIOLEVELSTASK_H
