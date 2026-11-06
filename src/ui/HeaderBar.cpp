#include "ui/HeaderBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "ui/StateDot.h"

HeaderBar::HeaderBar(Session *session, QWidget *parent)
    : QFrame(parent), m_session(session)
{
    setObjectName("headerBar");

    m_connectButton = new QPushButton("Connect", this);
    m_connectButton->setObjectName("connectButton");
    m_linkDot = new StateDot(this);
    m_linkLabel = new QLabel("Not connected", this);
    m_linkLabel->setObjectName("linkLabel");
    m_versionLabel = new QLabel(this);
    m_versionLabel->setObjectName("versionLabel");
    m_stateDot = new StateDot(this);
    m_stateLabel = new QLabel("Live off", this);
    m_stateLabel->setObjectName("stateLabel");
    m_stopBlockedLabel = new QLabel(this);
    m_stopBlockedLabel->setObjectName("stopBlockedLabel");
    m_stopBlockedLabel->hide();
    m_liveButton = new QPushButton("Live", this);
    m_liveButton->setObjectName("liveButton");
    m_liveButton->setCheckable(true);
    m_themeButton = new QPushButton("Dark theme", this);
    m_themeButton->setObjectName("themeButton");

    auto *stopAllButton = new QPushButton("STOP ALL (Esc)", this);
    stopAllButton->setObjectName("stopAllButton");
    stopAllButton->setFocusPolicy(Qt::NoFocus); /* so typing keeps its place in a box */

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);
    layout->addWidget(m_connectButton);
    layout->addWidget(m_linkDot);
    layout->addWidget(m_linkLabel);
    layout->addSpacing(12);
    layout->addWidget(m_versionLabel);
    layout->addStretch(1);
    layout->addWidget(m_stateDot);
    layout->addWidget(m_stateLabel);
    layout->addSpacing(12);
    layout->addWidget(m_stopBlockedLabel);
    layout->addWidget(m_liveButton);
    layout->addWidget(m_themeButton);
    layout->addSpacing(12);
    layout->addWidget(stopAllButton);

    connect(m_connectButton, &QPushButton::clicked, this, [this]() {
        if (m_session->linkState() == Session::LinkState::Disconnected) {
            m_session->connectBoard();
        } else {
            m_session->disconnectBoard();
        }
    });
    connect(stopAllButton, &QPushButton::clicked, m_session, &Session::stopAll);
    connect(m_liveButton, &QPushButton::clicked, m_session, &Session::setLive);
    connect(m_themeButton, &QPushButton::clicked, this, &HeaderBar::themeClicked);

    connect(m_session, &Session::linkStateChanged, this, &HeaderBar::showLinkState);
    connect(m_session, &Session::versionRead, m_versionLabel, &QLabel::setText);
    connect(m_session, &Session::stopBlockedChanged, this, [this](const QString &reason) {
        m_stopBlockedLabel->setText("STOP ALL is not active: " + reason);
        m_stopBlockedLabel->setVisible(!reason.isEmpty());
    });
    connect(m_session->poller(), &StatusPoller::statusRead, this, &HeaderBar::showStatus);
    connect(m_session->poller(), &StatusPoller::liveChanged, this, &HeaderBar::showLive);
}

void HeaderBar::setDarkTheme(bool isDark)
{
    m_themeButton->setText(isDark ? "Light theme" : "Dark theme");
}

void HeaderBar::showLinkState(Session::LinkState state)
{
    switch (state) {
        case Session::LinkState::Disconnected:
            m_linkDot->setState(StateDot::State::Off);
            m_linkLabel->setText("Not connected");
            m_connectButton->setText("Connect");
            m_connectButton->setEnabled(true);
            m_versionLabel->clear();
            break;
        case Session::LinkState::Connecting:
            m_linkDot->setState(StateDot::State::NeedsLook);
            m_linkLabel->setText("Connecting");
            m_connectButton->setEnabled(false);
            break;
        case Session::LinkState::Connected:
            m_linkDot->setState(StateDot::State::On);
            m_linkLabel->setText("Connected");
            m_connectButton->setText("Disconnect");
            m_connectButton->setEnabled(true);
            break;
        case Session::LinkState::Reconnecting:
            m_linkDot->setState(StateDot::State::Alarm);
            m_linkLabel->setText("Link lost: reconnecting");
            m_connectButton->setText("Disconnect");
            m_connectButton->setEnabled(true);
            break;
    }
}

void HeaderBar::showStatus(const BoardStatus &status)
{
    if (!status.faultText.isEmpty()) {
        m_stateDot->setState(StateDot::State::Alarm);
        m_stateLabel->setText("Fault: " + status.faultText);
    } else if (status.isBusy) {
        m_stateDot->setState(StateDot::State::On);
        m_stateLabel->setText("Busy");
    } else {
        m_stateDot->setState(StateDot::State::Off);
        m_stateLabel->setText("Idle");
    }
}

void HeaderBar::showLive(bool isLive)
{
    m_liveButton->setChecked(isLive);
    if (!isLive) {
        m_stateDot->setState(StateDot::State::Off);
        m_stateLabel->setText("Live off");
    }
}
