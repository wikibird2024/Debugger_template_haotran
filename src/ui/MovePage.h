#ifndef MOVEPAGE_H
#define MOVEPAGE_H

#include <QElapsedTimer>
#include <QWidget>

#include "board/BoardLink.h"
#include "logic/MoveWatch.h"

class QPushButton;
class QSpinBox;
class Session;

/* Example page to copy: a command judged from the live status, with its
 * waiting state reset on Session::workEnded. */
class MovePage : public QWidget
{
    Q_OBJECT
  public:
    explicit MovePage(Session *session, QWidget *parent = nullptr);

  private:
    void startMove();
    void handleReply(const BoardReply &reply);
    void handleStatus(const BoardStatus &status);
    void endMove();

    Session      *m_session;
    QSpinBox     *m_targetBox;
    QPushButton  *m_moveButton;
    MoveWatch     m_watch;
    QElapsedTimer m_moveClock;
    bool          m_isWaiting = false; /* from the click until the move is judged */
};

#endif // MOVEPAGE_H
