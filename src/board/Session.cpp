#include "board/Session.h"

Session::Session(BoardWorker::MakeLink makeLink, const BoardProfile &profile, QObject *parent)
    : QObject(parent),
      m_profile(profile),
      m_worker(std::move(makeLink)),
      m_poller(&m_worker, profile.pollPeriodMs),
      m_limits(profile.minSteps, profile.maxSteps)
{
    connect(&m_worker, &BoardWorker::jobsDropped, this, [this]() {
        m_isReconnectWaiting = false; /* a dropped open never answers: reconnect would hang */
        emit workEnded();
    });
    connect(&m_worker, &BoardWorker::connectionLost, this, [this]() { startReconnect("Lost the board."); });
    connect(this, &Session::workEnded, &m_poller, &StatusPoller::resetWaiting);

    connect(&m_poller, &StatusPoller::pollFailed, this, [this](const BoardReply &reply) {
        report("Live stopped: " + reply.text() + ".", Level::Error);
        if (reply.state == ReplyState::NoReply) {
            startReconnect("The board stopped answering.");
        }
    });

    m_reconnectTimer.setInterval(m_profile.reconnectRetryMs);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &Session::tryReconnect);

    m_stopCheckTimer.setSingleShot(true);
    connect(&m_stopCheckTimer, &QTimer::timeout, this, &Session::checkStopAgain);
}

void Session::report(const QString &text, Level level)
{
    emit message(text, level);
}

void Session::setLinkState(LinkState state)
{
    if (m_linkState != state) {
        m_linkState = state;
        emit linkStateChanged(state);
    }
}

void Session::openLink(LinkState stateWhileOpening)
{
    setLinkState(stateWhileOpening);
    m_worker.open(this, [this, stateWhileOpening](const BoardWorker::OpenResult &result) {
        m_isReconnectWaiting = false;
        if (m_linkState != stateWhileOpening) {
            return; /* the user pressed Disconnect meanwhile */
        }
        if (result.isOpen) {
            m_reconnectTimer.stop();
            setLinkState(LinkState::Connected);
            emit versionRead(result.version);
            if (stateWhileOpening == LinkState::Reconnecting) {
                report("Board back. Live stays off until you turn it on.", Level::Warning);
            } else {
                report("Connected to " + result.version + ".", Level::Info);
            }
        } else if (stateWhileOpening == LinkState::Connecting) {
            setLinkState(LinkState::Disconnected);
            report("Cannot connect: " + result.whyNot + ". Check the cable and press Connect.", Level::Error);
        }
    });
}

void Session::connectBoard()
{
    if (m_linkState == LinkState::Disconnected) {
        openLink(LinkState::Connecting);
    }
}

void Session::disconnectBoard()
{
    if (m_linkState == LinkState::Disconnected) {
        return;
    }
    m_reconnectTimer.stop();
    m_isReconnectWaiting = false;
    m_poller.setLive(false);
    endStopCheck();
    setLinkState(LinkState::Disconnected);
    emit workEnded();
    m_worker.close();
    report("Disconnected. Anything already running on the board keeps running.", Level::Warning);
}

void Session::setLive(bool isLive)
{
    if (isLive && !isConnected()) {
        report("Not connected. Press Connect.", Level::Warning);
        emit m_poller.liveChanged(false); /* the header button goes back to off */
        return;
    }
    m_poller.setLive(isLive);
}

void Session::startReconnect(const QString &why)
{
    if (m_linkState != LinkState::Connected) {
        return;
    }
    m_poller.setLive(false);
    if (m_isStopChecking) {
        endStopCheck();
        report("STOP ALL not confirmed: the link is gone. Check the machine.", Level::Error);
    }
    setLinkState(LinkState::Reconnecting);
    emit workEnded();
    m_worker.close();
    report(why + " Reconnecting until it answers, or press Disconnect.", Level::Error);
    m_reconnectTimer.start();
}

void Session::tryReconnect()
{
    if (m_isReconnectWaiting) {
        return; /* opening can take seconds: never stack attempts */
    }
    m_isReconnectWaiting = true;
    openLink(LinkState::Reconnecting);
}

void Session::endStopCheck()
{
    m_isStopChecking = false;
    m_stopCheckTimer.stop();
}

void Session::stopAll()
{
    m_stopCount++;
    endStopCheck();
    emit stopAllRequested();
    emit workEnded();

    if (!m_stopBlockedReason.isEmpty()) {
        report("STOP ALL not sent: " + m_stopBlockedReason + ". Use the hardware E-stop.", Level::Error);
        return;
    }
    if (!isConnected()) {
        report("STOP ALL not sent: not connected. Use the hardware E-stop if the machine is moving.", Level::Warning);
        return;
    }

    m_isStopChecking = true;
    m_stopCheckClock.start();
    const int stopCount = m_stopCount;
    m_worker.callFirst<StopAttempt>(
        this,
        [](BoardLink &link) {
            StopAttempt attempt;
            attempt.stopReply = link.stopEverything();
            if (attempt.stopReply.isOk()) {
                BoardStatus status;
                attempt.isStatusRead = link.readStatus(status).isOk();
                attempt.isBusy = status.isBusy;
            }
            return attempt;
        },
        [this, stopCount](const StopAttempt &attempt) {
            if (stopCount == m_stopCount && m_isStopChecking) {
                handleStopAttempt(attempt);
            }
        });
}

void Session::handleStopAttempt(const StopAttempt &attempt)
{
    switch (judgeStop(attempt)) {
        case StopOutcome::Stopped:
            endStopCheck();
            report("STOP ALL: the board reports stopped.", Level::Warning);
            break;
        case StopOutcome::StillBusy:
            if (m_stopCheckClock.elapsed() < m_profile.stopConfirmWindowMs) {
                m_stopCheckTimer.start(m_profile.pollPeriodMs);
            } else {
                endStopCheck();
                report("STOP ALL sent, but the board still reports busy. Use the hardware E-stop.", Level::Error);
            }
            break;
        case StopOutcome::NoReply:
            endStopCheck();
            report("STOP ALL: no reply from the board. Use the hardware E-stop.", Level::Error);
            break;
        case StopOutcome::Refused:
            endStopCheck();
            report("STOP ALL " + attempt.stopReply.text() + ". Use the hardware E-stop.", Level::Error);
            break;
        case StopOutcome::StatusUnknown:
            endStopCheck();
            report("STOP ALL answered, but the status cannot be read. Check the machine.", Level::Error);
            break;
    }
}

void Session::checkStopAgain()
{
    const int stopCount = m_stopCount;
    m_worker.call<StopAttempt>(
        this,
        [](BoardLink &link) {
            StopAttempt attempt;
            attempt.stopReply.state = ReplyState::Ok; /* already answered: this only reads */
            BoardStatus status;
            attempt.isStatusRead = link.readStatus(status).isOk();
            attempt.isBusy = status.isBusy;
            return attempt;
        },
        [this, stopCount](const StopAttempt &attempt) {
            if (stopCount == m_stopCount && m_isStopChecking) {
                handleStopAttempt(attempt);
            }
        });
}

void Session::setStopBlocked(const QString &reason)
{
    if (m_stopBlockedReason != reason) {
        m_stopBlockedReason = reason;
        emit stopBlockedChanged(reason);
    }
}

bool Session::moveTo(int positionSteps, QObject *context, std::function<void(const BoardReply &)> done)
{
    if (!isConnected()) {
        report("Move refused: not connected. Press Connect.", Level::Warning);
        return false;
    }
    if (m_isStopChecking) {
        report("Move refused: STOP ALL is still being confirmed.", Level::Warning);
        return false;
    }
    QString whyNot;
    if (!m_limits.check(positionSteps, whyNot)) {
        report("Move refused: " + whyNot + ".", Level::Error);
        return false;
    }
    const int stopCount = m_stopCount;
    m_worker.call<BoardReply>(
        context,
        [positionSteps](BoardLink &link) { return link.moveTo(positionSteps); },
        [this, stopCount, done](const BoardReply &reply) {
            if (stopCount == m_stopCount && done) {
                done(reply);
            }
        });
    return true;
}
