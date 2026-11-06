#ifndef STOPCHECK_H
#define STOPCHECK_H

#include "board/BoardLink.h"

/* What came back from one STOP ALL attempt. A typed result: Grader v2 decides
 * the same thing by searching the reply text, which breaks when the wording
 * changes. */
struct StopAttempt {
    BoardReply stopReply;
    bool       isStatusRead = false;
    bool       isBusy = false;
};

enum class StopOutcome {
    Stopped,      /* answered, and the status says not busy */
    StillBusy,    /* answered, but the status says busy: maybe slowing down, maybe ignored */
    NoReply,      /* the stop got no answer */
    Refused,      /* the board said no */
    StatusUnknown /* answered, but the status could not be read */
};

inline StopOutcome judgeStop(const StopAttempt &attempt)
{
    if (attempt.stopReply.state == ReplyState::NoReply) {
        return StopOutcome::NoReply;
    }
    if (attempt.stopReply.state == ReplyState::Refused) {
        return StopOutcome::Refused;
    }
    if (!attempt.isStatusRead) {
        return StopOutcome::StatusUnknown;
    }
    return attempt.isBusy ? StopOutcome::StillBusy : StopOutcome::Stopped;
}

#endif // STOPCHECK_H
