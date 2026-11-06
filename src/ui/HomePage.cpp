#include "ui/HomePage.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include "board/Session.h"
#include "ui/StateDot.h"

HomePage::HomePage(Session *session, QWidget *parent)
    : QWidget(parent)
{
    auto *box = new QGroupBox("Machine state", this);
    auto *grid = new QGridLayout(box);
    m_motionDot = new StateDot(box);
    m_motionLabel = new QLabel(box);
    m_faultDot = new StateDot(box);
    m_faultLabel = new QLabel(box);
    m_faultLabel->setObjectName("faultLabel");
    m_positionLabel = new QLabel(box);
    m_positionLabel->setObjectName("positionLabel");
    grid->addWidget(m_motionDot, 0, 0);
    grid->addWidget(m_motionLabel, 0, 1);
    grid->addWidget(m_faultDot, 1, 0);
    grid->addWidget(m_faultLabel, 1, 1);
    grid->addWidget(m_positionLabel, 2, 1);
    grid->setColumnStretch(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box);
    layout->addStretch(1);

    connect(session->poller(), &StatusPoller::statusRead, this, &HomePage::showStatus);
    connect(session->poller(), &StatusPoller::liveChanged, this, [this](bool isLive) {
        if (!isLive) {
            showUnknown();
        }
    });
    showUnknown();
}

void HomePage::showStatus(const BoardStatus &status)
{
    m_motionDot->setState(status.isBusy ? StateDot::State::On : StateDot::State::Off);
    m_motionLabel->setText(status.isBusy ? "Moving" : "Not moving");
    const bool hasFault = !status.faultText.isEmpty();
    m_faultDot->setState(hasFault ? StateDot::State::Alarm : StateDot::State::Off);
    m_faultLabel->setText(hasFault ? "Fault: " + status.faultText : "No fault");
    m_positionLabel->setText(QString("Position: %1 steps").arg(status.positionSteps));
}

void HomePage::showUnknown()
{
    m_motionDot->setState(StateDot::State::Off);
    m_motionLabel->setText("Motion: unknown");
    m_faultDot->setState(StateDot::State::Off);
    m_faultLabel->setText("Fault: unknown");
    m_positionLabel->setText("Position: unknown");
}
