#include "ui/SafetyInputFilter.h"

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QKeyEvent>
#include <QSlider>

static bool isValueBox(QObject *object)
{
    return qobject_cast<QAbstractSpinBox *>(object) != nullptr ||
           qobject_cast<QComboBox *>(object) != nullptr ||
           qobject_cast<QSlider *>(object) != nullptr;
}

SafetyInputFilter::SafetyInputFilter(std::function<void()> stopAll, QObject *parent)
    : QObject(parent), m_stopAll(std::move(stopAll))
{
}

bool SafetyInputFilter::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
        case QEvent::KeyPress: {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() != Qt::Key_Escape) {
                return false;
            }
            if (!keyEvent->isAutoRepeat()) {
                m_stopAll();
            }
            return true;
        }
        case QEvent::Polish:
            /* The default policy takes focus from the wheel, which would let
               the very first wheel turn change the value. */
            if (isValueBox(watched)) {
                static_cast<QWidget *>(watched)->setFocusPolicy(Qt::StrongFocus);
            }
            return false;
        case QEvent::Wheel:
            if (isValueBox(watched) && !static_cast<QWidget *>(watched)->hasFocus()) {
                event->ignore(); /* ignored: Qt passes it on to the scroll area */
                return true;
            }
            return false;
        default:
            return false;
    }
}
