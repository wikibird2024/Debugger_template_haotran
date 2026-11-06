#include <QElapsedTimer>
#include <QSignalSpy>
#include <QtTest>

#include "board/BoardWorker.h"
#include "board/FakeBoard.h"

/* The worker owns the fake; the test only uses it while the worker lives. */
struct Rig {
    FakeBoard  *fake = new FakeBoard;
    BoardWorker worker{[this]() { return std::unique_ptr<BoardLink>(fake); }};

    bool open(QObject *context)
    {
        bool isDone = false;
        worker.open(context, [&isDone](const BoardWorker::OpenResult &) { isDone = true; });
        return QTest::qWaitFor([&isDone]() { return isDone; }, 2000);
    }
    void queueMove(QObject *context, int positionSteps, int *doneCount = nullptr)
    {
        worker.call<BoardReply>(
            context,
            [positionSteps](BoardLink &link) { return link.moveTo(positionSteps); },
            [doneCount](const BoardReply &) {
                if (doneCount != nullptr) {
                    (*doneCount)++;
                }
            });
    }
    void queueStopFirst(QObject *context, bool *isDone)
    {
        worker.callFirst<BoardReply>(
            context,
            [](BoardLink &link) { return link.stopEverything(); },
            [isDone](const BoardReply &) { *isDone = true; });
    }
};

class TestWorker : public QObject
{
    Q_OBJECT
  private slots:
    void jobsRunInOrder()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        int doneCount = 0;
        rig.queueMove(this, 1, &doneCount);
        rig.queueMove(this, 2, &doneCount);
        rig.queueMove(this, 3, &doneCount);
        QTRY_COMPARE(doneCount, 3);
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 1", "move 2", "move 3"}));
    }

    void stopThrowsAwayWaitingJobsAndTheirDoneNeverRuns()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        QSignalSpy droppedSpy(&rig.worker, &BoardWorker::jobsDropped);
        rig.fake->setMoveReplyDelayMs(100);
        int doneCount = 0;
        rig.queueMove(this, 1, &doneCount);
        QTest::qWait(30); /* move 1 is now in progress */
        rig.queueMove(this, 2, &doneCount);
        rig.queueMove(this, 3, &doneCount);
        bool isStopDone = false;
        rig.queueStopFirst(this, &isStopDone);
        QTRY_VERIFY(isStopDone);
        QTest::qWait(150);
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 1", "stop"}));
        QCOMPARE(doneCount, 1); /* only the job already running */
        QCOMPARE(droppedSpy.count(), 1);
        QCOMPARE(droppedSpy.at(0).at(0).toInt(), 2);
    }

    /* The board never answers the move in progress: the stop must still go out
       as soon as that one timeout ends, ahead of everything queued. */
    void stopWaitsForAtMostOneTimeout()
    {
        const int kReplyTimeoutMs = 300;
        const int kMarginMs = 200;
        Rig       rig;
        QVERIFY(rig.open(this));
        rig.fake->setMoveReplyDelayMs(kReplyTimeoutMs);
        for (int i = 0; i < 5; i++) {
            rig.queueMove(this, i);
        }
        QTest::qWait(20);
        QElapsedTimer clock;
        clock.start();
        bool isStopDone = false;
        rig.queueStopFirst(this, &isStopDone);
        QTRY_VERIFY_WITH_TIMEOUT(isStopDone, 3000);
        QVERIFY2(clock.elapsed() < kReplyTimeoutMs + kMarginMs, qPrintable(QString("took %1 ms").arg(clock.elapsed())));
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 0", "stop"}));
    }

    void doneDoesNotRunAfterItsContextIsGone()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        int  doneCount = 0;
        auto context = std::make_unique<QObject>();
        rig.queueMove(context.get(), 1, &doneCount);
        context.reset();
        bool isLaterDone = false;
        rig.queueStopFirst(this, &isLaterDone);
        QTRY_VERIFY(isLaterDone);
        QCoreApplication::processEvents();
        QCOMPARE(doneCount, 0);
    }

    /* A waiting move must not run after the link comes back. */
    void linkLossThrowsAwayWaitingJobs()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        QSignalSpy lostSpy(&rig.worker, &BoardWorker::connectionLost);
        QSignalSpy droppedSpy(&rig.worker, &BoardWorker::jobsDropped);
        rig.fake->setMoveReplyDelayMs(100);
        rig.queueMove(this, 1);
        QTest::qWait(30);
        rig.queueMove(this, 2);
        rig.queueMove(this, 3);
        rig.fake->setUnplugged(true);
        QTRY_COMPARE(lostSpy.count(), 1);
        QCOMPARE(droppedSpy.count(), 1);
        QCOMPARE(droppedSpy.at(0).at(0).toInt(), 2);
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 1"}));
    }

    void askedCloseIsNotALinkLoss()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        QSignalSpy lostSpy(&rig.worker, &BoardWorker::connectionLost);
        rig.worker.close();
        QTRY_VERIFY(!rig.fake->isOpen());
        QTest::qWait(20);
        QCOMPARE(lostSpy.count(), 0);
    }

    void closeThrowsAwayWaitingJobs()
    {
        Rig rig;
        QVERIFY(rig.open(this));
        QSignalSpy droppedSpy(&rig.worker, &BoardWorker::jobsDropped);
        rig.fake->setMoveReplyDelayMs(100);
        rig.queueMove(this, 1);
        QTest::qWait(30);
        rig.queueMove(this, 2);
        rig.worker.close();
        QCOMPARE(droppedSpy.count(), 1);
        QTRY_VERIFY(!rig.fake->isOpen());
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 1"}));
    }
};

QTEST_GUILESS_MAIN(TestWorker)
#include "test_worker.moc"
