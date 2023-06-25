/*
 * Copyright (c) 2023 Meltytech, LLC
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

#ifndef TRANSCODER_H
#define TRANSCODER_H

#include "dialogs/transcodedialog.h"

#include <MltProducer.h>

#include <QList>
#include <QObject>

class Transcoder : public QObject
{
    Q_OBJECT

public:

    explicit Transcoder() : QObject() {}
    void setProducers(QList<Mlt::Producer> &producers);
    void addProducer(Mlt::Producer &producer);
    void addProducer(Mlt::Producer *producer);
    void convert(TranscodeDialog &dialog);

private:
    void convertProducer(Mlt::Producer *producer, TranscodeDialog &dialog);
    QList<Mlt::Producer> m_producers;
};

#endif // TRANSCODER_H
