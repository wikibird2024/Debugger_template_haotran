#ifndef FAKEBOARD_H
#define FAKEBOARD_H

#include <QMutex>
#include <QStringList>

#include "board/BoardLink.h"

/* A board in memory, for the tests, for --screenshot and for trying the shell.
 * It can also misbehave the way real boards have. The position moves once per
 * status read, so tests do not depend on the clock. Every setter is safe to
 * call from any thread. */
class FakeBoard : public BoardLink
{
  public:
    bool open(QString &whyNot) override;
    void close() override;
    bool isOpen() const override;

    BoardReply readVersion(QString &version) override;
    BoardReply readStatus(BoardStatus &status) override;
    BoardReply moveTo(int positionSteps) override;
    BoardReply stopEverything() override;

    void setUnplugged(bool isUnplugged);    /* the link dies at the next call */
    void setSilent(bool isSilent);          /* stays open, nothing answers */
    void setIgnoringMoves(bool isIgnoring); /* answers OK to a move and does nothing */
    void setIgnoringStop(bool isIgnoring);  /* answers OK to a stop and keeps moving */
    void setMoveReplyDelayMs(int delayMs);  /* the next moves take this long, then no reply */
    void setOpenDelayMs(int delayMs);       /* opening a real port can take seconds */
    void setFaultText(const QString &faultText);
    void setStepsPerStatusRead(int stepsPerRead);

    QStringList commandsSeen() const; /* "open", "move 100", "stop", ... (status reads left out) */
    int         positionSteps() const;
    int         statusReadCount() const;
    int         versionReadCount() const;

  private:
    bool isAnswering(); /* call with m_mutex held */

    mutable QMutex m_mutex;
    bool           m_isOpen = false;
    bool           m_isUnplugged = false;
    bool           m_isSilent = false;
    bool           m_isIgnoringMoves = false;
    bool           m_isIgnoringStop = false;
    int            m_moveReplyDelayMs = 0;
    int            m_openDelayMs = 0;
    int            m_statusReadCount = 0;
    int            m_versionReadCount = 0;
    int            m_stepsPerStatusRead = 100;
    int            m_positionSteps = 0;
    int            m_targetSteps = 0;
    QString        m_faultText;
    QStringList    m_commandsSeen;
};

#endif // FAKEBOARD_H
