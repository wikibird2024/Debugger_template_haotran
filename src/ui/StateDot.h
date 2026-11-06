#ifndef STATEDOT_H
#define STATEDOT_H

#include <QWidget>

/* A state light. Off is hollow and the others are filled, so color is never
 * the only signal; always put text beside it. */
class StateDot : public QWidget
{
    Q_OBJECT
  public:
    enum class State { Off,
                       On,
                       NeedsLook,
                       Alarm };

    explicit StateDot(QWidget *parent = nullptr);
    void setState(State state);

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    State m_state = State::Off;
};

#endif // STATEDOT_H
