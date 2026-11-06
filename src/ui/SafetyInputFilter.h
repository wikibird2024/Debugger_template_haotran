#ifndef SAFETYINPUTFILTER_H
#define SAFETYINPUTFILTER_H

#include <QObject>
#include <functional>

/* Sits on the whole application, so it also covers dialogs, open lists and
 * boxes made later: a page cannot forget it.
 *
 * Esc is STOP ALL everywhere and does nothing else. A plain window shortcut
 * misses Esc while a modal dialog is open: the dialog just closes. So Esc does
 * not close a dialog or a list in this tool: give dialogs a Cancel button. A
 * native file dialog is another program and still takes its own Esc.
 *
 * The wheel only changes a number box, list box or slider that was clicked
 * first. Otherwise it scrolls the page: scrolling past a step box must not
 * turn 1 mm into 10 mm for the next jog. */
class SafetyInputFilter : public QObject
{
    Q_OBJECT
  public:
    explicit SafetyInputFilter(std::function<void()> stopAll, QObject *parent = nullptr);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    std::function<void()> m_stopAll;
};

#endif // SAFETYINPUTFILTER_H
