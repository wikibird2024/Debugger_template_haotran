#ifndef BOARDLINK_H
#define BOARDLINK_H

#include <QMetaType>
#include <QString>

enum class ReplyState {
    Ok,
    NoReply, /* nothing came back: the link may be gone */
    Refused  /* the board answered and said no */
};

struct BoardReply {
    ReplyState state = ReplyState::NoReply;
    QString    detail; /* why it was refused */

    bool isOk() const
    {
        return state == ReplyState::Ok;
    }
    QString text() const
    {
        switch (state) {
            case ReplyState::Ok:
                return "OK";
            case ReplyState::NoReply:
                return "no reply from the board";
            case ReplyState::Refused:
                return "refused: " + detail;
        }
        return "unknown reply";
    }
};
Q_DECLARE_METATYPE(BoardReply)

/* What the live poll reads. A new tool replaces these fields with its own. */
struct BoardStatus {
    bool    isBusy = false;
    int     positionSteps = 0;
    QString faultText; /* empty: no fault */
};
Q_DECLARE_METATYPE(BoardStatus)

/* Everything the tool asks of a board. A new tool replaces the commands with
 * its own and gives a real class (serial, USB) next to FakeBoard.
 *
 * Rules for every class behind this interface:
 * - Every call waits for at most its timeout, so STOP ALL is never held up
 *   for longer than one timeout. A long transfer is one job per exchange.
 * - Never send a command twice by itself: a move must not go out twice.
 * - Never reopen by itself: a queued move must not run after a link loss.
 *   Only open() opens the link, and Session decides when to call it.
 * - Created on the worker thread and used only there. */
class BoardLink
{
  public:
    virtual ~BoardLink() = default;

    virtual bool open(QString &whyNot) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual BoardReply readVersion(QString &version) = 0;
    virtual BoardReply readStatus(BoardStatus &status) = 0;
    virtual BoardReply moveTo(int positionSteps) = 0; /* use Session::moveTo: it checks the limits */
    virtual BoardReply stopEverything() = 0;
};

#endif // BOARDLINK_H
