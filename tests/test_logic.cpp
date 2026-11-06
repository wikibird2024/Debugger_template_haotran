#include <QtTest>

#include "logic/MoveLimits.h"
#include "logic/MoveWatch.h"
#include "logic/StopCheck.h"

class TestLogic : public QObject
{
    Q_OBJECT
  private slots:
    void moveWatch_doneOnlyAfterBusyWasSeen()
    {
        MoveWatch watch;
        watch.start(100, 1000);
        QCOMPARE(watch.update(false, 10), MoveWatch::State::Waiting);
        QCOMPARE(watch.update(true, 20), MoveWatch::State::Waiting);
        QCOMPARE(watch.update(false, 30), MoveWatch::State::Done);
    }
    void moveWatch_okButIgnoredIsNotStarted()
    {
        MoveWatch watch;
        watch.start(100, 1000);
        QCOMPARE(watch.update(false, 99), MoveWatch::State::Waiting);
        QCOMPARE(watch.update(false, 100), MoveWatch::State::NotStarted);
    }
    void moveWatch_busyTooLongTimesOut()
    {
        MoveWatch watch;
        watch.start(100, 1000);
        QCOMPARE(watch.update(true, 999), MoveWatch::State::Waiting);
        QCOMPARE(watch.update(true, 1000), MoveWatch::State::TimedOut);
    }
    void moveWatch_cancelEndsIt()
    {
        MoveWatch watch;
        watch.start(100, 1000);
        watch.cancel();
        QVERIFY(!watch.isWaiting());
        QCOMPARE(watch.update(true, 10), MoveWatch::State::Idle);
    }
    void moveLimits_edgesAreInside()
    {
        const MoveLimits limits(0, 1000);
        QString          whyNot;
        QVERIFY(limits.check(0, whyNot));
        QVERIFY(limits.check(1000, whyNot));
        QVERIFY(whyNot.isEmpty());
    }
    void moveLimits_outsideSaysWhy()
    {
        const MoveLimits limits(0, 1000);
        QString          whyNot;
        QVERIFY(!limits.check(-1, whyNot));
        QVERIFY(whyNot.contains("-1"));
        QVERIFY(!limits.check(1001, whyNot));
    }
    void judgeStop_everyOutcome()
    {
        StopAttempt attempt;
        QCOMPARE(judgeStop(attempt), StopOutcome::NoReply);
        attempt.stopReply.state = ReplyState::Refused;
        QCOMPARE(judgeStop(attempt), StopOutcome::Refused);
        attempt.stopReply.state = ReplyState::Ok;
        QCOMPARE(judgeStop(attempt), StopOutcome::StatusUnknown);
        attempt.isStatusRead = true;
        attempt.isBusy = true;
        QCOMPARE(judgeStop(attempt), StopOutcome::StillBusy);
        attempt.isBusy = false;
        QCOMPARE(judgeStop(attempt), StopOutcome::Stopped);
    }
};

QTEST_GUILESS_MAIN(TestLogic)
#include "test_logic.moc"
