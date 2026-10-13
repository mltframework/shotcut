

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
