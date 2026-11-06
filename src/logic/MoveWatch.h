#ifndef MOVEWATCH_H
#define MOVEWATCH_H

#include <QtGlobal>

/* Judges one move from the live status, never from the reply: firmware can
 * answer OK and ignore the command. Done only after busy was seen, then not
 * busy. A move shorter than one poll period looks like NotStarted, so a tool
 * with short moves needs a faster poll or a move counter from the board.
 *
 * With no live status update() is never called: the owner must cancel() when
 * Live goes off. */
class MoveWatch
{
  public:
    enum class State { Idle,
                       Waiting,
                       Done,
                       NotStarted, /* no busy within the start window */
                       TimedOut };

    void start(int startWindowMs, int timeoutMs)
    {
        m_startWindowMs = startWindowMs;
        m_timeoutMs = timeoutMs;
        m_hasSeenBusy = false;
        m_state = State::Waiting;
    }
    void cancel()
    {
        m_state = State::Idle;
    }
    bool isWaiting() const
    {
        return m_state == State::Waiting;
    }
    State update(bool isBusy, qint64 elapsedMs)
    {
        if (m_state != State::Waiting) {
            return m_state;
        }
        if (isBusy) {
            m_hasSeenBusy = true;
            if (elapsedMs >= m_timeoutMs) {
                m_state = State::TimedOut;
            }
        } else if (m_hasSeenBusy) {
            m_state = State::Done;
        } else if (elapsedMs >= m_startWindowMs) {
            m_state = State::NotStarted;
        }
        return m_state;
    }

  private:
    State m_state = State::Idle;
    bool  m_hasSeenBusy = false;
    int   m_startWindowMs = 0;
    int   m_timeoutMs = 0;
};

#endif // MOVEWATCH_H
