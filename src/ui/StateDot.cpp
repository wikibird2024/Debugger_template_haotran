#include "ui/StateDot.h"

#include <QPainter>

#include "ui/Theme.h"

StateDot::StateDot(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(14, 14);
}

void StateDot::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        update();
    }
}

void StateDot::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF circle = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    switch (m_state) {
        case State::Off:
            painter.setPen(QPen(Theme::color("state-off"), 2));
            painter.setBrush(Qt::NoBrush);
            break;
        case State::On:
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::color("state-on"));
            break;
        case State::NeedsLook:
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::color("state-look"));
            break;
        case State::Alarm:
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::color("state-alarm"));
            break;
    }
    painter.drawEllipse(circle);
}
