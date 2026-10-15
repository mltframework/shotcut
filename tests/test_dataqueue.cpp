/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "dataqueue.h"

#include <QtTest/QtTest>

class TestDataQueue : public QObject
{
    Q_OBJECT

private slots:
    void newQueueIsEmpty()
    {
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeDiscardOldest);
        QCOMPARE(queue.count(), 0);
    }

    void pushIncreasesCount()
    {
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeDiscardOldest);
        queue.push(10);
        QCOMPARE(queue.count(), 1);
        queue.push(20);
        QCOMPARE(queue.count(), 2);
    }

    void popReturnsItemsInOrder()
    {
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeDiscardOldest);
        queue.push(1);
        queue.push(2);
        queue.push(3);
        QCOMPARE(queue.pop(), 1);
        QCOMPARE(queue.pop(), 2);
        QCOMPARE(queue.pop(), 3);
        QCOMPARE(queue.count(), 0);
    }

    void discardOldestDropsFirstItem()
    {
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeDiscardOldest);
        queue.push(1);
        queue.push(2);
        queue.push(3);
        queue.push(4); // the queue is full, so 1 is dropped
        QCOMPARE(queue.count(), 3);
        QCOMPARE(queue.pop(), 2);
        QCOMPARE(queue.pop(), 3);
        QCOMPARE(queue.pop(), 4);
    }

    void discardNewestDropsLastItem()
    {
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeDiscardNewest);
        queue.push(1);
        queue.push(2);
        queue.push(3);
        queue.push(4); // the queue is full, so 4 is dropped
        QCOMPARE(queue.count(), 3);
        QCOMPARE(queue.pop(), 1);
        QCOMPARE(queue.pop(), 2);
        QCOMPARE(queue.pop(), 3);
    }

    void waitModeKeepsItemsWhenNotFull()
    {
        // Only tests the case where the queue is not full.
        // A full queue would block and freeze the test.
        DataQueue<int> queue(3, DataQueue<int>::OverflowModeWait);
        queue.push(1);
        queue.push(2);
        QCOMPARE(queue.count(), 2);
        QCOMPARE(queue.pop(), 1);
        QCOMPARE(queue.count(), 1);
        QCOMPARE(queue.pop(), 2);
        QCOMPARE(queue.count(), 0);
    }
};

QTEST_APPLESS_MAIN(TestDataQueue)

#include "test_dataqueue.moc"
