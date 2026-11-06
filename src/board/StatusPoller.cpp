#include "board/StatusPoller.h"

#include "board/BoardWorker.h"

namespace
{
struct PollResult {
    BoardReply  reply;
    BoardStatus status;
};
} // namespace

StatusPoller::StatusPoller(BoardWorker *worker, int periodMs, QObject *parent)
    : QObject(parent), m_worker(worker)
{
    m_timer.setInterval(periodMs);
    connect(&m_timer, &QTimer::timeout, this, &StatusPoller::readOnce);
}

void StatusPoller::setLive(bool isLive)
{
    if (m_isLive == isLive) {
        return;
    }
    m_isLive = isLive;
    if (isLive) {
        m_timer.start();
    } else {
        m_timer.stop();
    }
    emit liveChanged(isLive);
}

void StatusPoller::resetWaiting()
{
    m_isReadWaiting = false;
}

void StatusPoller::readOnce()
{
    if (m_isReadWaiting) {
        return;
    }
    m_isReadWaiting = true;
    m_worker->call<PollResult>(
        this,
        [](BoardLink &link) {
            PollResult result;
            result.reply = link.readStatus(result.status);
            return result;
        },
        [this](const PollResult &result) {
            m_isReadWaiting = false;
            if (!m_isLive) {
                return;
            }
            if (result.reply.isOk()) {
                emit statusRead(result.status);
            } else {
                setLive(false);
                emit pollFailed(result.reply);
            }
        });
}
