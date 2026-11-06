#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>

#include "board/BoardLink.h"

class QLabel;
class Session;
class StateDot;

/* Example page: what the machine is doing, read from the live status. */
class HomePage : public QWidget
{
    Q_OBJECT
  public:
    explicit HomePage(Session *session, QWidget *parent = nullptr);

  private:
    void showStatus(const BoardStatus &status);
    void showUnknown();

    StateDot *m_motionDot;
    QLabel   *m_motionLabel;
    StateDot *m_faultDot;
    QLabel   *m_faultLabel;
    QLabel   *m_positionLabel;
};

#endif // HOMEPAGE_H
