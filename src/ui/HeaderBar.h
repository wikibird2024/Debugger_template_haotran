#ifndef HEADERBAR_H
#define HEADERBAR_H

#include <QFrame>

#include "board/Session.h"

class QLabel;
class QPushButton;
class StateDot;

/* On every page: link, version, machine state and fault, Live, theme, STOP ALL.
 * With Live off the state says "Live off", never the last value it read. */
class HeaderBar : public QFrame
{
    Q_OBJECT
  public:
    explicit HeaderBar(Session *session, QWidget *parent = nullptr);
    void setDarkTheme(bool isDark);

  signals:
    void themeClicked();

  private:
    void showLinkState(Session::LinkState state);
    void showStatus(const BoardStatus &status);
    void showLive(bool isLive);

    Session     *m_session;
    QPushButton *m_connectButton;
    StateDot    *m_linkDot;
    QLabel      *m_linkLabel;
    QLabel      *m_versionLabel;
    StateDot    *m_stateDot;
    QLabel      *m_stateLabel;
    QLabel      *m_stopBlockedLabel;
    QPushButton *m_liveButton;
    QPushButton *m_themeButton;
};

#endif // HEADERBAR_H
