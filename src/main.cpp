#include <QApplication>
#include <QCommandLineParser>
#include <QFont>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>
#include <memory>

#include "MakeBoardLink.h"
#include "board/FakeBoard.h"
#include "board/Session.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    /* Seen on the bench with Dekit Grader v2 (not reproduced here): Qt's
       accessibility bridge crashed the tool on the first setEnabled. */
    qputenv("QT_ACCESSIBILITY", "0");

    /* Wayland alone aborts when its display is gone (a terminal left over from
       an old session); add X11 as a fallback. */
    if (qgetenv("QT_QPA_PLATFORM") == "wayland") {
        qputenv("QT_QPA_PLATFORM", "wayland;xcb");
    }

    /* --screenshot must not touch the user's settings. */
    QTemporaryDir scratchHome;
    bool          isScreenshotRun = false;
    for (int i = 1; i < argc; i++) {
        if (QByteArray(argv[i]) == "--screenshot") {
            isScreenshotRun = true;
            QStandardPaths::setTestModeEnabled(true);
            QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, scratchHome.path());
        }
    }

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Debugger template");
    QApplication::setApplicationName("Debugger template");
    QApplication::setApplicationVersion(TOOL_VERSION);
    /* Fusion draws the same on every desktop, so the sheet looks the same too. */
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QFont font("Noto Sans");
    font.setStyleHint(QFont::SansSerif);
    font.setPointSize(10);
    QApplication::setFont(font);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption screenshotOption("screenshot", "Save a PNG of every page, light and dark, into <folder> and quit. Uses the fake board.", "folder");
    parser.addOption(screenshotOption);
    parser.process(app);

    /* A screenshot run connects and turns Live on by itself: it must never reach a machine. */
    BoardWorker::MakeLink makeLink = isScreenshotRun
                                         ? BoardWorker::MakeLink([]() { return std::unique_ptr<BoardLink>(std::make_unique<FakeBoard>()); })
                                         : BoardWorker::MakeLink(makeBoardLink);
    Session               session(makeLink, BoardProfile{});
    MainWindow            window(&session);
    window.show();

    if (isScreenshotRun) {
        QObject::connect(&session, &Session::linkStateChanged, &window, [&](Session::LinkState state) {
            if (state != Session::LinkState::Connected) {
                return;
            }
            session.setLive(true);
            QTimer::singleShot(500, &window, [&]() {
                const QStringList files = window.saveScreenshots(parser.value(screenshotOption));
                std::printf("Saved %lld files\n", static_cast<long long>(files.size()));
                QApplication::exit(files.isEmpty() ? 1 : 0);
            });
        });
        session.connectBoard();
    }

    return app.exec();
}
