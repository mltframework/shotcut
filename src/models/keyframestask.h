/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef KEYFRAMESTASK_H
#define KEYFRAMESTASK_H

#include <MltProducer.h>
#include <climits>
#include <QList>
#include <QMap>
#include <QRunnable>
#include <QVector>

// Stored in a stream's keyframe vector to mean "this stream is intra-only;
// every frame is a keyframe, so two-phase seek can be skipped."
static constexpr int kIntraOnlySentinel = INT_MAX;

class KeyframesTask : public QRunnable
{
public:
    explicit KeyframesTask(Mlt::Producer &producer);
    virtual ~KeyframesTask();
    static void start(Mlt::Producer &producer, bool force = false);
    static void closeAll();
    bool operator==(const KeyframesTask &b) const;

protected:
    void run() override;

private:
    void setProperty(const QMap<int, QVector<int>> &keyframes);
    QString cacheKey() const;

    QList<Mlt::Producer *> m_producers;
    bool m_isCanceled;
    bool m_isForce;
};

#endif // KEYFRAMESTASK_H
