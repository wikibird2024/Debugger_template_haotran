#ifndef SESSION_H
#define SESSION_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "board/BoardProfile.h"
#include "board/BoardWorker.h"
#include "board/StatusPoller.h"
#include "logic/MoveLimits.h"
#include "logic/StopCheck.h"

/* The shared state of one connection: link state, STOP ALL, reconnect, and the
 * only way to send a move. No widgets, so all of it runs in the tests. */
class Session : public QObject
{
    Q_OBJECT
  public:
    enum class LinkState { Disconnected,
                           Connecting,
                           Connected,
                           Reconnecting };
    Q_ENUM(LinkState)

    enum class Level { Info,
                       Warning,
                       Error };
    Q_ENUM(Level)

    Session(BoardWorker::MakeLink makeLink, const BoardProfile &profile, QObject *parent = nullptr);

    BoardWorker *worker()
    {
        return &m_worker;
    }
    StatusPoller *poller()
    {
        return &m_poller;
    }
    const BoardProfile &profile() const
    {
        return m_profile;
    }
    LinkState linkState() const
    {
        return m_linkState;
    }
    bool isConnected() const
    {
        return m_linkState == LinkState::Connected;
    }

    void connectBoard();
    void disconnectBoard();
    void setLive(bool isLive);

    /* Ends every loop, throws away waiting jobs, sends the stop next, then
       checks the live status: the board may answer OK and keep moving. This is
       a software stop over the link, not an emergency stop. */
    void stopAll();

    /* A long job that cannot take a stop (firmware update) names itself here.
       STOP ALL then says it was not sent, instead of doing nothing silently.
       An empty reason ends it. */
    void    setStopBlocked(const QString &reason);
    QString stopBlockedReason() const
    {
        return m_stopBlockedReason;
    }

    /* Checks the limits, then queues the move. False: refused, already reported.
       `done` does not run if STOP ALL came first. */
    bool moveTo(int positionSteps, QObject *context, std::function<void(const BoardReply &)> done);

    void report(const QString &text, Level level);

  signals:
    void linkStateChanged(Session::LinkState state);
    void versionRead(const QString &version);
    void stopAllRequested();
    void stopBlockedChanged(const QString &reason);
    void message(const QString &text, Session::Level level);

    /* Every waiting flag on a page resets here: STOP ALL, dropped jobs,
       disconnect and link loss all come through this one signal. */
    void workEnded();

  private:
    void openLink(LinkState stateWhileOpening);
    void setLinkState(LinkState state);
    void startReconnect(const QString &why);
    void tryReconnect();
    void endStopCheck();
    void handleStopAttempt(const StopAttempt &attempt);
    void checkStopAgain();

    BoardProfile  m_profile;
    BoardWorker   m_worker;
    StatusPoller  m_poller;
    MoveLimits    m_limits;
    LinkState     m_linkState = LinkState::Disconnected;
    QString       m_stopBlockedReason;
    QTimer        m_reconnectTimer;
    bool          m_isReconnectWaiting = false;
    QTimer        m_stopCheckTimer;
    QElapsedTimer m_stopCheckClock;
    bool          m_isStopChecking = false;
    int           m_stopCount = 0; /* a reply from before the last STOP ALL is ignored */
};

#endif // SESSION_H
