#include "ui/MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include "ui/HeaderBar.h"
#include "ui/HomePage.h"
#include "ui/MovePage.h"
#include "ui/SafetyInputFilter.h"
#include "ui/Theme.h"

static const int kMaxLogLines = 5000;
static const int kToastShownMs = 4000;
static const int kPageIndexRole = Qt::UserRole;

MainWindow::MainWindow(Session *session, QWidget *parent)
    : QMainWindow(parent), m_session(session)
{
    qApp->installEventFilter(new SafetyInputFilter([this]() { m_session->stopAll(); }, this));

    setWindowTitle("Debugger template");
    resize(1200, 780);

    m_headerBar = new HeaderBar(m_session);

    m_navList = new QListWidget;
    m_navList->setObjectName("navList");
    m_navList->setFixedWidth(200);

    m_pageTitle = new QLabel;
    m_pageTitle->setObjectName("pageTitle");
    m_connectionHint = new QLabel;
    m_connectionHint->setObjectName("connectionHint");
    m_pages = new QStackedWidget;

    auto *pageArea = new QWidget;
    auto *pageAreaLayout = new QVBoxLayout(pageArea);
    pageAreaLayout->setContentsMargins(20, 12, 20, 6);
    pageAreaLayout->setSpacing(4);
    pageAreaLayout->addWidget(m_pageTitle);
    pageAreaLayout->addWidget(m_connectionHint);
    pageAreaLayout->addWidget(m_pages, 1);

    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(pageArea);
    splitter->addWidget(buildLogBar());
    splitter->setStretchFactor(0, 1);
    splitter->setSizes({560, 180});

    auto *body = new QWidget;
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyLayout->addWidget(m_navList);
    bodyLayout->addWidget(splitter, 1);

    auto *root = new QWidget;
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_headerBar);
    rootLayout->addWidget(body, 1);
    setCentralWidget(root);

    buildToast();

    connect(m_navList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        if (item != nullptr) {
            const int pageIndex = item->data(kPageIndexRole).toInt();
            m_pages->setCurrentIndex(pageIndex);
            m_pageTitle->setText(m_pageNames.at(pageIndex));
        }
    });
    connect(m_headerBar, &HeaderBar::themeClicked, this, [this]() {
        applyTheme(!Theme::isDark());
        QSettings().setValue("isDarkTheme", Theme::isDark());
    });
    connect(m_session, &Session::message, this, &MainWindow::handleMessage);
    connect(m_session, &Session::linkStateChanged, this, &MainWindow::updateConnectionHint);
    connect(m_session->poller(), &StatusPoller::liveChanged, this, &MainWindow::updateConnectionHint);

    /* A new tool adds its pages here. */
    addPage("Operate", "Home", "What the machine is doing now.", new HomePage(m_session));
    addPage("Manual", "Move", "Move to a position. The result comes from the live status, not the reply.", new MovePage(m_session));

    applyTheme(QSettings().value("isDarkTheme", false).toBool());
    updateConnectionHint();
}

void MainWindow::addPage(const QString &group, const QString &name, const QString &description, QWidget *page)
{
    /* A page with too little room scrolls, instead of squeezing its labels. */
    auto *contents = new QWidget;
    auto *contentsLayout = new QVBoxLayout(contents);
    contentsLayout->setContentsMargins(0, 0, 8, 0);
    auto *descriptionLabel = new QLabel(description);
    descriptionLabel->setObjectName("pageDescription");
    descriptionLabel->setWordWrap(true);
    contentsLayout->addWidget(descriptionLabel);
    contentsLayout->addWidget(page, 1);

    auto *scroll = new QScrollArea;
    scroll->setWidget(contents);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    const int pageIndex = m_pages->addWidget(scroll);
    m_pageNames << name;

    if (!group.isEmpty()) {
        auto *header = new QListWidgetItem(group, m_navList);
        header->setFlags(Qt::NoItemFlags);
        QFont headerFont = m_navList->font();
        headerFont.setBold(true);
        header->setFont(headerFont);
    }
    auto *item = new QListWidgetItem(name, m_navList);
    item->setData(kPageIndexRole, pageIndex);
    if (m_navList->currentItem() == nullptr) {
        m_navList->setCurrentItem(item);
    }
}

int MainWindow::pageCount() const
{
    return m_pages->count();
}

void MainWindow::showPage(int pageIndex)
{
    for (int row = 0; row < m_navList->count(); row++) {
        QListWidgetItem *item = m_navList->item(row);
        if ((item->flags() & Qt::ItemIsSelectable) && item->data(kPageIndexRole).toInt() == pageIndex) {
            m_navList->setCurrentItem(item);
            return;
        }
    }
}

QStringList MainWindow::saveScreenshots(const QString &folder)
{
    QStringList files;
    QDir().mkpath(folder);
    const bool wasDark = Theme::isDark();
    for (const bool isDark : {false, true}) {
        applyTheme(isDark);
        for (int pageIndex = 0; pageIndex < pageCount(); pageIndex++) {
            showPage(pageIndex);
            QEventLoop settle;
            QTimer::singleShot(150, &settle, &QEventLoop::quit);
            settle.exec();
            const QString name = m_pageNames.at(pageIndex).toLower().replace(' ', '_');
            const QString path = QDir(folder).filePath(QString("%1_%2.png").arg(name, isDark ? "dark" : "light"));
            if (grab().save(path)) {
                files << path;
            }
        }
    }
    applyTheme(wasDark);
    return files;
}

QWidget *MainWindow::buildLogBar()
{
    auto *logBox = new QWidget;
    auto *logLayout = new QVBoxLayout(logBox);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(0);

    auto *bar = new QFrame(logBox);
    bar->setObjectName("logBar");
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(12, 4, 6, 4);
    m_logLevelBox = new QComboBox(bar);
    m_logLevelBox->setObjectName("logLevelBox");
    m_logLevelBox->addItems({"All", "Errors"});
    m_logFilterEdit = new QLineEdit(bar);
    m_logFilterEdit->setObjectName("logFilterEdit");
    m_logFilterEdit->setPlaceholderText("Filter");
    m_logFilterEdit->setClearButtonEnabled(true);
    m_logFilterEdit->setFixedWidth(220);
    auto *clearButton = new QPushButton("Clear log", bar);
    barLayout->addWidget(new QLabel("Log", bar));
    barLayout->addStretch(1);
    barLayout->addWidget(m_logLevelBox);
    barLayout->addWidget(m_logFilterEdit);
    barLayout->addWidget(clearButton);

    m_logView = new QPlainTextEdit(logBox);
    m_logView->setObjectName("logView");
    m_logView->setReadOnly(true);
    logLayout->addWidget(bar);
    logLayout->addWidget(m_logView, 1);

    connect(m_logLevelBox, &QComboBox::currentIndexChanged, this, &MainWindow::refreshLog);
    connect(m_logFilterEdit, &QLineEdit::textChanged, this, &MainWindow::refreshLog);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        m_logLines.clear();
        refreshLog();
    });
    return logBox;
}

void MainWindow::buildToast()
{
    m_toast = new QLabel(this);
    m_toast->setObjectName("toast");
    m_toast->setWordWrap(true);
    m_toast->setMaximumWidth(440);
    m_toast->hide();
    m_toastTimer.setSingleShot(true);
    connect(&m_toastTimer, &QTimer::timeout, m_toast, &QLabel::hide);
}

void MainWindow::applyTheme(bool isDark)
{
    Theme::apply(isDark);
    m_headerBar->setDarkTheme(isDark);
}

void MainWindow::handleMessage(const QString &text, Session::Level level)
{
    const bool isError = level == Session::Level::Error;
    addLogLine(text, isError);
    if (level != Session::Level::Info) {
        showToast(text, isError);
    }
}

void MainWindow::addLogLine(const QString &text, bool isError)
{
    /* "Error:" in words: the log has no color. */
    LogLine line;
    line.text = QDateTime::currentDateTime().toString("HH:mm:ss.zzz") + "  " + (isError ? "Error: " : "") + text;
    line.isError = isError;
    m_logLines.append(line);
    if (m_logLines.size() > kMaxLogLines) {
        m_logLines.removeFirst();
        refreshLog();
    } else if (isLogLineShown(line)) {
        m_logView->appendPlainText(line.text);
    }
}

bool MainWindow::isLogLineShown(const LogLine &line) const
{
    const bool isErrorsOnly = m_logLevelBox->currentIndex() == 1;
    if (isErrorsOnly && !line.isError) {
        return false;
    }
    return line.text.contains(m_logFilterEdit->text(), Qt::CaseInsensitive);
}

void MainWindow::refreshLog()
{
    m_logView->clear();
    for (const LogLine &line : m_logLines) {
        if (isLogLineShown(line)) {
            m_logView->appendPlainText(line.text);
        }
    }
}

void MainWindow::showToast(const QString &text, bool isError)
{
    m_toast->setText(text);
    m_toast->setProperty("isError", isError);
    m_toast->style()->unpolish(m_toast);
    m_toast->style()->polish(m_toast);
    m_toast->adjustSize();
    const int top = m_headerBar->height() + 12;
    m_toast->move(width() - m_toast->width() - 20, top);
    m_toast->raise();
    m_toast->show();
    m_toastTimer.start(kToastShownMs);
}

void MainWindow::updateConnectionHint()
{
    QString hint;
    switch (m_session->linkState()) {
        case Session::LinkState::Disconnected:
            hint = "Not connected. Press Connect at the top of the window.";
            break;
        case Session::LinkState::Connecting:
            hint = "Connecting.";
            break;
        case Session::LinkState::Reconnecting:
            hint = "Link lost. Reconnecting until the board answers, or press Disconnect.";
            break;
        case Session::LinkState::Connected:
            if (!m_session->poller()->isLive()) {
                hint = "Connected. Turn on Live to read the machine.";
            }
            break;
    }
    m_connectionHint->setText(hint);
    m_connectionHint->setVisible(!hint.isEmpty());
}
