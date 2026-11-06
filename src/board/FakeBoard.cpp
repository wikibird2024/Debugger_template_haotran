#include "board/FakeBoard.h"

#include <QThread>
#include <cstdlib>

bool FakeBoard::open(QString &whyNot)
{
    int delayMs = 0;
    {
        QMutexLocker lock(&m_mutex);
        delayMs = m_openDelayMs;
    }
    if (delayMs > 0) {
        QThread::msleep(delayMs);
    }
    QMutexLocker lock(&m_mutex);
    if (m_isUnplugged) {
        whyNot = "no board found";
        return false;
    }
    m_isOpen = true;
    m_commandsSeen << "open";
    return true;
}

void FakeBoard::close()
{
    QMutexLocker lock(&m_mutex);
    m_isOpen = false;
}

bool FakeBoard::isOpen() const
{
    QMutexLocker lock(&m_mutex);
    return m_isOpen;
}

bool FakeBoard::isAnswering()
{
    if (m_isUnplugged) {
        m_isOpen = false;
    }
    return m_isOpen && !m_isSilent;
}

BoardReply FakeBoard::readVersion(QString &version)
{
    QMutexLocker lock(&m_mutex);
    m_versionReadCount++;
    if (!isAnswering()) {
        return {ReplyState::NoReply, {}};
    }
    version = "Fake board 1.0";
    return {ReplyState::Ok, {}};
}

BoardReply FakeBoard::readStatus(BoardStatus &status)
{
    QMutexLocker lock(&m_mutex);
    m_statusReadCount++;
    if (!isAnswering()) {
        return {ReplyState::NoReply, {}};
    }
    const int distance = m_targetSteps - m_positionSteps;
    if (std::abs(distance) <= m_stepsPerStatusRead) {
        m_positionSteps = m_targetSteps;
    } else {
        m_positionSteps += distance > 0 ? m_stepsPerStatusRead : -m_stepsPerStatusRead;
    }
    status.isBusy = m_positionSteps != m_targetSteps;
    status.positionSteps = m_positionSteps;
    status.faultText = m_faultText;
    return {ReplyState::Ok, {}};
}

BoardReply FakeBoard::moveTo(int positionSteps)
{
    int delayMs = 0;
    {
        QMutexLocker lock(&m_mutex);
        delayMs = m_moveReplyDelayMs;
        m_commandsSeen << QString("move %1").arg(positionSteps);
    }
    if (delayMs > 0) {
        QThread::msleep(delayMs); /* a real link waiting for its timeout */
    }
    QMutexLocker lock(&m_mutex);
    if (!isAnswering() || delayMs > 0) {
        return {ReplyState::NoReply, {}};
    }
    if (!m_isIgnoringMoves) {
        m_targetSteps = positionSteps;
    }
    return {ReplyState::Ok, {}};
}

BoardReply FakeBoard::stopEverything()
{
    QMutexLocker lock(&m_mutex);
    m_commandsSeen << "stop";
    if (!isAnswering()) {
        return {ReplyState::NoReply, {}};
    }
    if (!m_isIgnoringStop) {
        m_targetSteps = m_positionSteps;
    }
    return {ReplyState::Ok, {}};
}

void FakeBoard::setUnplugged(bool isUnplugged)
{
    QMutexLocker lock(&m_mutex);
    m_isUnplugged = isUnplugged;
}

void FakeBoard::setSilent(bool isSilent)
{
    QMutexLocker lock(&m_mutex);
    m_isSilent = isSilent;
}

void FakeBoard::setIgnoringMoves(bool isIgnoring)
{
    QMutexLocker lock(&m_mutex);
    m_isIgnoringMoves = isIgnoring;
}

void FakeBoard::setIgnoringStop(bool isIgnoring)
{
    QMutexLocker lock(&m_mutex);
    m_isIgnoringStop = isIgnoring;
}

void FakeBoard::setMoveReplyDelayMs(int delayMs)
{
    QMutexLocker lock(&m_mutex);
    m_moveReplyDelayMs = delayMs;
}

void FakeBoard::setOpenDelayMs(int delayMs)
{
    QMutexLocker lock(&m_mutex);
    m_openDelayMs = delayMs;
}

void FakeBoard::setFaultText(const QString &faultText)
{
    QMutexLocker lock(&m_mutex);
    m_faultText = faultText;
}

void FakeBoard::setStepsPerStatusRead(int stepsPerRead)
{
    QMutexLocker lock(&m_mutex);
    m_stepsPerStatusRead = stepsPerRead;
}

QStringList FakeBoard::commandsSeen() const
{
    QMutexLocker lock(&m_mutex);
    return m_commandsSeen;
}

int FakeBoard::positionSteps() const
{
    QMutexLocker lock(&m_mutex);
    return m_positionSteps;
}

int FakeBoard::statusReadCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_statusReadCount;
}

int FakeBoard::versionReadCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_versionReadCount;
}
