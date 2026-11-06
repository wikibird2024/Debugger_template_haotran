#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtTest>

#include "board/FakeBoard.h"
#include "board/Session.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

static BoardProfile fastProfile()
{
    BoardProfile profile;
    profile.pollPeriodMs = 10;
    profile.moveStartWindowMs = 200;
    profile.moveTimeoutMs = 2000;
    profile.minSteps = 0;
    profile.maxSteps = 1000;
    return profile;
}

struct Rig {
    FakeBoard *fake = new FakeBoard;
    Session    session{[this]() { return std::unique_ptr<BoardLink>(fake); }, fastProfile()};
    MainWindow window{&session};

    Rig()
    {
        window.show();
    }
    template <typename Widget = QWidget>
    Widget *find(const char *objectName)
    {
        return window.findChild<Widget *>(objectName);
    }
    QString logText()
    {
        return find<QPlainTextEdit>("logView")->toPlainText();
    }
    bool connectAndGoLive()
    {
        QTest::mouseClick(find<QPushButton>("connectButton"), Qt::LeftButton);
        if (!QTest::qWaitFor([this]() { return session.isConnected(); }, 2000)) {
            return false;
        }
        QTest::mouseClick(find<QPushButton>("liveButton"), Qt::LeftButton);
        return session.poller()->isLive();
    }
};

static void sendWheel(QWidget *widget)
{
    const QPointF center = QRectF(widget->rect()).center();
    QWheelEvent   wheel(center, widget->mapToGlobal(center), QPoint(0, 0), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(widget, &wheel);
}

class TestUi : public QObject
{
    Q_OBJECT
  private slots:
    void init()
    {
        QSettings().remove("isDarkTheme");
    }

    void stopAllIsVisibleOnEveryPage()
    {
        Rig rig;
        QVERIFY(rig.window.pageCount() >= 2);
        for (int pageIndex = 0; pageIndex < rig.window.pageCount(); pageIndex++) {
            rig.window.showPage(pageIndex);
            auto *button = rig.find<QPushButton>("stopAllButton");
            QVERIFY(button->isVisible());
            QVERIFY(button->isEnabled());
            QCOMPARE(button->text(), QString("STOP ALL (Esc)"));
        }
    }

    void stopAllButton_stopsTheBoard()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        QTest::mouseClick(rig.find<QPushButton>("stopAllButton"), Qt::LeftButton);
        QTRY_VERIFY(rig.fake->commandsSeen().contains("stop"));
    }

    void esc_fromTheWindow_isStopAll()
    {
        Rig        rig;
        QSignalSpy stopSpy(&rig.session, &Session::stopAllRequested);
        QTest::keyClick(&rig.window, Qt::Key_Escape);
        QCOMPARE(stopSpy.count(), 1);
    }

    void esc_whileTypingInABox_isStopAll()
    {
        Rig        rig;
        QSignalSpy stopSpy(&rig.session, &Session::stopAllRequested);
        rig.window.showPage(1);
        auto *box = rig.find<QSpinBox>("moveTargetBox");
        box->setFocus();
        QTest::keyClick(box, Qt::Key_Escape);
        QCOMPARE(stopSpy.count(), 1);
    }

    void esc_withADialogOpen_isStopAll()
    {
        Rig        rig;
        QSignalSpy stopSpy(&rig.session, &Session::stopAllRequested);
        QDialog    dialog(&rig.window);
        dialog.setModal(true);
        dialog.show();
        /* Through the window system, as a real key press arrives. */
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QTest::keyClick(dialog.windowHandle(), Qt::Key_Escape);
        QTRY_COMPARE(stopSpy.count(), 1);
        QVERIFY(dialog.isVisible()); /* Esc only stops: a dialog needs its own Cancel */
    }

    void esc_withAListOpen_isStopAll()
    {
        Rig        rig;
        QSignalSpy stopSpy(&rig.session, &Session::stopAllRequested);
        auto      *list = rig.find<QComboBox>("logLevelBox");
        list->showPopup();
        QWidget *popup = list->view()->window();
        QVERIFY(QTest::qWaitForWindowExposed(popup));
        QTest::keyClick(popup->windowHandle(), Qt::Key_Escape);
        QTRY_COMPARE(stopSpy.count(), 1);
        list->hidePopup();
    }

    void esc_heldDown_stopsOnce()
    {
        Rig        rig;
        QSignalSpy stopSpy(&rig.session, &Session::stopAllRequested);
        QKeyEvent  first(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QKeyEvent  repeat(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier, QString(), true);
        QApplication::sendEvent(&rig.window, &first);
        QApplication::sendEvent(&rig.window, &repeat);
        QCOMPARE(stopSpy.count(), 1);
    }

    void wheel_overABoxNotClicked_doesNotChangeIt()
    {
        Rig rig;
        rig.window.showPage(1);
        auto *box = rig.find<QSpinBox>("moveTargetBox");
        QVERIFY(QTest::qWaitForWindowExposed(&rig.window));
        box->setValue(100);
        box->clearFocus();
        sendWheel(box);
        QCOMPARE(box->value(), 100);
    }

    void wheel_overAClickedBox_changesIt()
    {
        Rig rig;
        rig.window.showPage(1);
        auto *box = rig.find<QSpinBox>("moveTargetBox");
        rig.window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&rig.window));
        box->setValue(100);
        box->setFocus();
        QTRY_VERIFY(box->hasFocus());
        sendWheel(box);
        QCOMPARE(box->value(), 200);
    }

    /* The filter sits on the application, so a box made later is covered too. */
    void wheel_guardCoversABoxMadeLater()
    {
        Rig     rig;
        QDialog dialog(&rig.window);
        auto   *layout = new QVBoxLayout(&dialog);
        auto   *box = new QSpinBox(&dialog);
        box->setRange(0, 1000);
        box->setValue(100);
        layout->addWidget(new QPushButton("Other", &dialog));
        layout->addWidget(box);
        dialog.show();
        QTRY_VERIFY(dialog.isVisible());
        QCOMPARE(box->focusPolicy(), Qt::StrongFocus);
        sendWheel(box);
        QCOMPARE(box->value(), 100);
    }

    void move_judgedFromLiveStatus_reportsDone()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        rig.window.showPage(1);
        rig.find<QSpinBox>("moveTargetBox")->setValue(500);
        auto *moveButton = rig.find<QPushButton>("moveButton");
        QTest::mouseClick(moveButton, Qt::LeftButton);
        QVERIFY(!moveButton->isEnabled());
        QTRY_VERIFY(rig.logText().contains("Move done at 500 steps"));
        QVERIFY(moveButton->isEnabled());
    }

    void move_okButIgnored_isAFailure()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        rig.fake->setIgnoringMoves(true);
        rig.window.showPage(1);
        rig.find<QSpinBox>("moveTargetBox")->setValue(500);
        QTest::mouseClick(rig.find<QPushButton>("moveButton"), Qt::LeftButton);
        QTRY_VERIFY(rig.logText().contains("never started"));
        QVERIFY(rig.find<QPushButton>("moveButton")->isEnabled());
    }

    void move_refusedWithLiveOff()
    {
        Rig rig;
        QTest::mouseClick(rig.find<QPushButton>("connectButton"), Qt::LeftButton);
        QTRY_VERIFY(rig.session.isConnected());
        rig.window.showPage(1);
        QTest::mouseClick(rig.find<QPushButton>("moveButton"), Qt::LeftButton);
        QVERIFY(rig.logText().contains("turn on Live first"));
        QVERIFY(!rig.fake->commandsSeen().contains("move 0"));
    }

    void move_outsideLimits_isRefused()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        rig.window.showPage(1);
        rig.find<QSpinBox>("moveTargetBox")->setValue(5000);
        QTest::mouseClick(rig.find<QPushButton>("moveButton"), Qt::LeftButton);
        QVERIFY(rig.logText().contains("outside the limits"));
        QVERIFY(rig.find<QPushButton>("moveButton")->isEnabled());
    }

    void move_stopAllFreesTheButton()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        rig.fake->setStepsPerStatusRead(1);
        rig.window.showPage(1);
        rig.find<QSpinBox>("moveTargetBox")->setValue(1000);
        auto *moveButton = rig.find<QPushButton>("moveButton");
        QTest::mouseClick(moveButton, Qt::LeftButton);
        QVERIFY(!moveButton->isEnabled());
        rig.session.stopAll();
        QVERIFY(moveButton->isEnabled());
    }

    void header_liveOff_doesNotShowAnOldValue()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        auto *stateLabel = rig.find<QLabel>("stateLabel");
        QTRY_COMPARE(stateLabel->text(), QString("Idle"));
        QTest::mouseClick(rig.find<QPushButton>("liveButton"), Qt::LeftButton);
        QCOMPARE(stateLabel->text(), QString("Live off"));
        QCOMPARE(rig.find<QLabel>("positionLabel")->text(), QString("Position: unknown"));
    }

    void header_faultShowsAsText()
    {
        Rig rig;
        QVERIFY(rig.connectAndGoLive());
        rig.fake->setFaultText("light curtain blocked");
        QTRY_COMPARE(rig.find<QLabel>("stateLabel")->text(), QString("Fault: light curtain blocked"));
        QCOMPARE(rig.find<QLabel>("faultLabel")->text(), QString("Fault: light curtain blocked"));
    }

    void header_stopBlockedIsVisible()
    {
        Rig   rig;
        auto *label = rig.find<QLabel>("stopBlockedLabel");
        QVERIFY(!label->isVisible());
        rig.session.setStopBlocked("firmware update running");
        QVERIFY(label->isVisible());
        QVERIFY(label->text().contains("firmware update running"));
        rig.session.setStopBlocked({});
        QVERIFY(!label->isVisible());
    }

    void hint_followsTheLinkAndLive()
    {
        Rig   rig;
        auto *hint = rig.find<QLabel>("connectionHint");
        QVERIFY(hint->text().contains("Not connected"));
        QTest::mouseClick(rig.find<QPushButton>("connectButton"), Qt::LeftButton);
        QTRY_VERIFY(hint->text().contains("Turn on Live"));
        QTest::mouseClick(rig.find<QPushButton>("liveButton"), Qt::LeftButton);
        QVERIFY(!hint->isVisible());
    }

    void log_filterKeepsErrorsOnly()
    {
        Rig rig;
        rig.session.report("all fine", Session::Level::Info);
        rig.session.report("it broke", Session::Level::Error);
        QVERIFY(rig.logText().contains("all fine"));
        QVERIFY(rig.logText().contains("Error: it broke"));
        rig.find<QComboBox>("logLevelBox")->setCurrentIndex(1);
        QVERIFY(!rig.logText().contains("all fine"));
        QVERIFY(rig.logText().contains("it broke"));
    }

    void toast_showsErrorsNotInfo()
    {
        Rig   rig;
        auto *toast = rig.find<QLabel>("toast");
        rig.session.report("all fine", Session::Level::Info);
        QVERIFY(!toast->isVisible());
        rig.session.report("it broke", Session::Level::Error);
        QVERIFY(toast->isVisible());
        QCOMPARE(toast->text(), QString("it broke"));
    }

    void theme_everyTokenIsFilledIn()
    {
        for (const bool isDark : {false, true}) {
            const QString sheet = Theme::sheet(isDark);
            QVERIFY(!sheet.isEmpty());
            QVERIFY2(!sheet.contains('@'), qPrintable(sheet));
        }
    }

    void screenshots_oneFilePerPageAndTheme()
    {
        Rig           rig;
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        const QStringList files = rig.window.saveScreenshots(folder.path());
        QCOMPARE(files.size(), rig.window.pageCount() * 2);
        for (const QString &path : files) {
            QVERIFY(QFileInfo(path).size() > 0);
        }
    }
};

int main(int argc, char *argv[])
{
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir scratchHome;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, scratchHome.path());
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Debugger template test");
    QApplication::setApplicationName("Debugger template test");
    TestUi test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_ui.moc"
