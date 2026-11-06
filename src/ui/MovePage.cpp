#include "ui/MovePage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "board/Session.h"

MovePage::MovePage(Session *session, QWidget *parent)
    : QWidget(parent), m_session(session)
{
    auto *box = new QGroupBox("Move", this);
    auto *form = new QFormLayout(box);
    m_targetBox = new QSpinBox(box);
    m_targetBox->setObjectName("moveTargetBox");
    /* Wider than the limits on purpose: Session::moveTo is what refuses. */
    m_targetBox->setRange(-1000000, 1000000);
    m_targetBox->setSingleStep(100);
    m_targetBox->setSuffix(" steps");
    m_targetBox->setToolTip(QString("Limits: %1 to %2 steps").arg(session->profile().minSteps).arg(session->profile().maxSteps));
    m_moveButton = new QPushButton("Start move", box);
    m_moveButton->setObjectName("moveButton");
    form->addRow("Target:", m_targetBox);
    form->addRow(m_moveButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box);
    layout->addStretch(1);

    connect(m_moveButton, &QPushButton::clicked, this, &MovePage::startMove);
    connect(m_session->poller(), &StatusPoller::statusRead, this, &MovePage::handleStatus);
    connect(m_session, &Session::workEnded, this, &MovePage::endMove);
    connect(m_session->poller(), &StatusPoller::liveChanged, this, [this](bool isLive) {
        if (!isLive && m_isWaiting) {
            endMove();
            m_session->report("Move result unknown: Live went off.", Session::Level::Error);
        }
    });
}

void MovePage::startMove()
{
    if (!m_session->poller()->isLive()) {
        m_session->report("Move refused: turn on Live first. A move is judged from the live status.", Session::Level::Warning);
        return;
    }
    const bool isQueued = m_session->moveTo(m_targetBox->value(), this, [this](const BoardReply &reply) { handleReply(reply); });
    if (isQueued) {
        m_isWaiting = true;
        m_moveButton->setEnabled(false);
    }
}

void MovePage::handleReply(const BoardReply &reply)
{
    if (!m_isWaiting) {
        return;
    }
    if (!reply.isOk()) {
        endMove();
        m_session->report("Move failed: " + reply.text() + ".", Session::Level::Error);
        return;
    }
    m_moveClock.start();
    m_watch.start(m_session->profile().moveStartWindowMs, m_session->profile().moveTimeoutMs);
}

void MovePage::handleStatus(const BoardStatus &status)
{
    if (!m_watch.isWaiting()) {
        return;
    }
    switch (m_watch.update(status.isBusy, m_moveClock.elapsed())) {
        case MoveWatch::State::Done:
            endMove();
            m_session->report(QString("Move done at %1 steps.").arg(status.positionSteps), Session::Level::Info);
            break;
        case MoveWatch::State::NotStarted:
            endMove();
            m_session->report("Move failed: the board answered OK but never started.", Session::Level::Error);
            break;
        case MoveWatch::State::TimedOut:
            endMove();
            m_session->report("Move failed: still busy after the time limit. Press STOP ALL and check the machine.", Session::Level::Error);
            break;
        case MoveWatch::State::Idle:
        case MoveWatch::State::Waiting:
            break;
    }
}

void MovePage::endMove()
{
    m_isWaiting = false;
    m_watch.cancel();
    m_moveButton->setEnabled(true);
}
