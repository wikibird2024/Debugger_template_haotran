#ifndef STATUSPOLLER_H
#define STATUSPOLLER_H

#include <QObject>
#include <QTimer>

#include "board/BoardLink.h"

class BoardWorker;

/* The live status read. Only one read waits at a time, so a slow link cannot
 * fill the queue. A failed read turns Live off: the screen must not keep
 * showing old values as if they were current. */
class StatusPoller : public QObject
{
    Q_OBJECT
  public:
    StatusPoller(BoardWorker *worker, int periodMs, QObject *parent = nullptr);

    bool isLive() const
    {
        return m_isLive;
    }
    void setLive(bool isLive);
    void resetWaiting(); /* a dropped read never answers */

  signals:
    void liveChanged(bool isLive);
    void statusRead(const BoardStatus &status);
    void pollFailed(const BoardReply &reply);

  private:
    void readOnce();

    BoardWorker *m_worker;
    QTimer       m_timer;
    bool         m_isLive = false;
    bool         m_isReadWaiting = false;
};

#endif // STATUSPOLLER_H
