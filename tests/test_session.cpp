#include <QSignalSpy>
#include <QThread>
#include <QtTest>

#include "board/FakeBoard.h"
#include "board/Session.h"

static BoardProfile fastProfile()
{
    BoardProfile profile;
    profile.pollPeriodMs = 10;
    profile.reconnectRetryMs = 30;
    profile.stopConfirmWindowMs = 100;
    profile.moveStartWindowMs = 100;
    profile.moveTimeoutMs = 1000;
    profile.minSteps = 0;
    profile.maxSteps = 1000;
    return profile;
}

struct Rig {
    FakeBoard  *fake = new FakeBoard;
    Session     session{[this]() { return std::unique_ptr<BoardLink>(fake); }, fastProfile()};
    QStringList messages;

    Rig()
    {
        QObject::connect(&session, &Session::message, &session, [this](const QString &text, Session::Level) { messages << text; });
    }
    bool connectBoard()
    {
        session.connectBoard();
        return QTest::qWaitFor([this]() { return session.isConnected(); }, 2000);
    }
    bool hasMessage(const QString &part) const
    {
        for (const QString &text : messages) {
            if (text.contains(part)) {
                return true;
            }
        }
        return false;
    }
};

class TestSession : public QObject
{
    Q_OBJECT
  private slots:
    void stopAll_goesAheadOfWaitingMoves()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setMoveReplyDelayMs(100);
        QVERIFY(rig.session.moveTo(1, this, nullptr));
        QTest::qWait(30);
        QVERIFY(rig.session.moveTo(2, this, nullptr));
        QVERIFY(rig.session.moveTo(3, this, nullptr));
        rig.session.stopAll();
        QTRY_VERIFY(rig.hasMessage("reports stopped"));
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open", "move 1", "stop"}));
    }

    void stopAll_signalsEveryPageToReset()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        QSignalSpy loopSpy(&rig.session, &Session::stopAllRequested);
        QSignalSpy workSpy(&rig.session, &Session::workEnded);
        rig.session.stopAll();
        QCOMPARE(loopSpy.count(), 1);
        QVERIFY(workSpy.count() >= 1);
    }

    /* Firmware can answer OK and keep moving. */
    void stopAll_boardAnswersOkButKeepsMoving_isReported()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setStepsPerStatusRead(1);
        rig.fake->setIgnoringStop(true);
        bool isMoveAccepted = false; /* a move still queued would be dropped, not stopped */
        QVERIFY(rig.session.moveTo(1000, this, [&isMoveAccepted](const BoardReply &) { isMoveAccepted = true; }));
        QTRY_VERIFY(isMoveAccepted);
        rig.session.stopAll();
        QTRY_VERIFY_WITH_TIMEOUT(rig.hasMessage("still reports busy"), 2000);
        QVERIFY(!rig.hasMessage("reports stopped"));
    }

    void stopAll_noReply_isReported()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setSilent(true);
        rig.session.stopAll();
        QTRY_VERIFY(rig.hasMessage("no reply from the board"));
        QVERIFY(rig.hasMessage("E-stop"));
    }

    void stopAll_notConnected_saysItWasNotSent()
    {
        Rig rig;
        rig.session.stopAll();
        QVERIFY(rig.hasMessage("not sent"));
        QVERIFY(!rig.fake->commandsSeen().contains("stop"));
    }

    void stopAll_blocked_saysItWasNotSent()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        QSignalSpy blockedSpy(&rig.session, &Session::stopBlockedChanged);
        rig.session.setStopBlocked("firmware update running");
        QCOMPARE(blockedSpy.count(), 1);
        rig.session.stopAll();
        QVERIFY(rig.hasMessage("firmware update running"));
        QTest::qWait(30);
        QVERIFY(!rig.fake->commandsSeen().contains("stop"));
        rig.session.setStopBlocked({});
        rig.session.stopAll();
        QTRY_VERIFY(rig.fake->commandsSeen().contains("stop"));
    }

    void move_replyFromBeforeStop_isIgnored()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setMoveReplyDelayMs(50);
        bool isDoneCalled = false;
        QVERIFY(rig.session.moveTo(1, this, [&isDoneCalled](const BoardReply &) { isDoneCalled = true; }));
        QTest::qWait(10);
        rig.session.stopAll();
        QTRY_VERIFY(rig.hasMessage("reports stopped"));
        QVERIFY(!isDoneCalled);
    }

    void move_outsideLimits_neverLeavesTheTool()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        QVERIFY(!rig.session.moveTo(1001, this, nullptr));
        QVERIFY(!rig.session.moveTo(-1, this, nullptr));
        QVERIFY(rig.hasMessage("outside the limits"));
        QTest::qWait(20);
        QCOMPARE(rig.fake->commandsSeen(), QStringList({"open"}));
    }

    void move_refusedWhileStopIsBeingConfirmed()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setStepsPerStatusRead(1);
        rig.fake->setIgnoringStop(true);
        QVERIFY(rig.session.moveTo(1000, this, nullptr));
        rig.session.stopAll();
        QVERIFY(!rig.session.moveTo(5, this, nullptr));
        QVERIFY(rig.hasMessage("still being confirmed"));
    }

    void reconnect_retriesAndLiveStaysOff()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.setLive(true);
        QVERIFY(rig.session.poller()->isLive());
        rig.fake->setUnplugged(true);
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        QVERIFY(!rig.session.poller()->isLive());
        QTest::qWait(100); /* several attempts fail while it stays unplugged */
        QCOMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        rig.fake->setUnplugged(false);
        QTRY_VERIFY(rig.session.isConnected());
        QVERIFY(!rig.session.poller()->isLive());
        QVERIFY(rig.hasMessage("Live stays off"));
    }

    void reconnect_waitingMoveDoesNotRunAfterwards()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setMoveReplyDelayMs(50);
        QVERIFY(rig.session.moveTo(1, this, nullptr));
        QTest::qWait(10);
        QVERIFY(rig.session.moveTo(2, this, nullptr));
        rig.fake->setUnplugged(true);
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        rig.fake->setMoveReplyDelayMs(0);
        rig.fake->setUnplugged(false);
        QTRY_VERIFY(rig.session.isConnected());
        QTest::qWait(30);
        QVERIFY(!rig.fake->commandsSeen().contains("move 2"));
    }

    /* A slow job ends in link loss after a retry was queued, which drops that
       retry. Its `done` never runs, so nothing else may be left waiting for it. */
    void reconnect_retryDroppedByLinkLoss_stillReconnects()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.fake->setSilent(true);
        rig.fake->setMoveReplyDelayMs(400);
        /* Holds the worker so the queue becomes: failing read, then slow move. */
        rig.session.worker()->call<int>(this, [](BoardLink &) {
            QThread::msleep(60);
            return 0;
        });
        rig.session.setLive(true);
        QTest::qWait(30);
        QVERIFY(rig.session.moveTo(1, this, nullptr));
        QTRY_COMPARE_WITH_TIMEOUT(rig.session.linkState(), Session::LinkState::Reconnecting, 300);
        rig.fake->setUnplugged(true); /* the slow move ends with the link closed */
        QTest::qWait(600);            /* the slow move has ended and dropped the retry */
        rig.fake->setMoveReplyDelayMs(0);
        rig.fake->setSilent(false);
        rig.fake->setUnplugged(false);
        QTRY_VERIFY_WITH_TIMEOUT(rig.session.isConnected(), 1000);
    }

    /* Opening is slower than the retry time. Every open job reads the version,
       so an extra read means attempts were stacked behind the first. */
    void reconnect_slowOpen_neverStacksAttempts()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.setLive(true);
        rig.fake->setUnplugged(true);
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        const int versionReadsBefore = rig.fake->versionReadCount();
        rig.fake->setOpenDelayMs(150);
        rig.fake->setUnplugged(false);
        QTRY_VERIFY_WITH_TIMEOUT(rig.session.isConnected(), 2000);
        QTest::qWait(100); /* stacked attempts would run now */
        QCOMPARE(rig.fake->versionReadCount(), versionReadsBefore + 1);
    }

    void reconnect_disconnectEndsIt()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.setLive(true);
        rig.fake->setUnplugged(true);
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        rig.session.disconnectBoard();
        QCOMPARE(rig.session.linkState(), Session::LinkState::Disconnected);
        const int opensBefore = rig.fake->commandsSeen().count("open");
        rig.fake->setUnplugged(false);
        QTest::qWait(100);
        QCOMPARE(rig.session.linkState(), Session::LinkState::Disconnected);
        QCOMPARE(rig.fake->commandsSeen().count("open"), opensBefore);
    }

    /* The link stays open but nothing answers. */
    void silentBoard_livePollStartsReconnect()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.setLive(true);
        rig.fake->setSilent(true);
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Reconnecting);
        QVERIFY(rig.hasMessage("Live stopped"));
        rig.fake->setSilent(false);
        QTRY_VERIFY(rig.session.isConnected());
    }

    void connect_noBoard_saysWhatToDo()
    {
        Rig rig;
        rig.fake->setUnplugged(true);
        rig.session.connectBoard();
        QTRY_COMPARE(rig.session.linkState(), Session::LinkState::Disconnected);
        QVERIFY(rig.hasMessage("Check the cable"));
    }

    void live_refusedWhenNotConnected()
    {
        Rig rig;
        rig.session.setLive(true);
        QVERIFY(!rig.session.poller()->isLive());
        QVERIFY(rig.hasMessage("Not connected"));
    }

    /* The link is busy for 20 poll periods, then Live goes off, so any read that
       runs after that had been waiting in the queue. */
    void live_slowLink_onlyOneReadWaits()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.worker()->call<int>(this, [](BoardLink &) {
            QThread::msleep(200);
            return 0;
        });
        rig.session.setLive(true);
        QTest::qWait(150);
        rig.session.setLive(false);
        const int readsBefore = rig.fake->statusReadCount();
        QTest::qWait(150);
        QVERIFY(rig.fake->statusReadCount() - readsBefore <= 1);
    }

    void live_keepsReadingAfterStopAll()
    {
        Rig rig;
        QVERIFY(rig.connectBoard());
        rig.session.setLive(true);
        QSignalSpy statusSpy(rig.session.poller(), &StatusPoller::statusRead);
        QTRY_VERIFY(statusSpy.count() > 0);
        rig.session.stopAll();
        statusSpy.clear();
        QTRY_VERIFY(statusSpy.count() > 0);
    }
};

QTEST_GUILESS_MAIN(TestSession)
#include "test_session.moc"
