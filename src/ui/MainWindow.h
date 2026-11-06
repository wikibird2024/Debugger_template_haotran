#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>

#include "board/Session.h"

class HeaderBar;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QStackedWidget;

/* The shell: header, sidebar, page title, shared hint, log bar and toast.
 * A new tool adds its pages in the constructor and leaves the rest alone. */
class MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    explicit MainWindow(Session *session, QWidget *parent = nullptr);

    /* An empty group continues the group above. */
    void addPage(const QString &group, const QString &name, const QString &description, QWidget *page);
    int  pageCount() const;
    void showPage(int pageIndex);

    /* Saves <page>_light.png and <page>_dark.png for every page; works offscreen.
       Does not change the saved theme. Returns the files written. */
    QStringList saveScreenshots(const QString &folder);

  private:
    struct LogLine {
        QString text;
        bool    isError = false;
    };

    QWidget *buildLogBar();
    void     buildToast();
    void     applyTheme(bool isDark);
    void     handleMessage(const QString &text, Session::Level level);
    void     addLogLine(const QString &text, bool isError);
    bool     isLogLineShown(const LogLine &line) const;
    void     refreshLog();
    void     showToast(const QString &text, bool isError);
    void     updateConnectionHint();

    Session         *m_session;
    HeaderBar       *m_headerBar;
    QListWidget     *m_navList;
    QStackedWidget  *m_pages;
    QLabel          *m_pageTitle;
    QLabel          *m_connectionHint;
    QPlainTextEdit  *m_logView;
    QComboBox       *m_logLevelBox;
    QLineEdit       *m_logFilterEdit;
    QLabel          *m_toast;
    QTimer           m_toastTimer;
    QVector<LogLine> m_logLines;
    QStringList      m_pageNames;
};

#endif // MAINWINDOW_H
