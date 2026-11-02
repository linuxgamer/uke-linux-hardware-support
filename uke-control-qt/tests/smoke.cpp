// SPDX-License-Identifier: MIT
#include "../src/ui.hpp"
#include <QtTest>
#include <unistd.h>
using namespace uq;
class Smoke : public QObject {
  Q_OBJECT
  Fixture fixture;
private slots:
  void initTestCase() {
    fixture = Fixture::load(FIXTURE_PATH);
    QVERIFY(fixture.enabled);
  }
  void fixtureValidation() {
    QTemporaryDir dir;
    QFile bad(dir.filePath("bad.json"));
    QVERIFY(bad.open(QIODevice::WriteOnly));
    bad.write("{\"schema\":\"wrong\",\"results\":{}}");
    bad.close();
    QVERIFY_EXCEPTION_THROWN(Fixture::load(bad.fileName()), std::exception);
    auto link = dir.filePath("link.json");
    QVERIFY(::symlink(FIXTURE_PATH, QFile::encodeName(link).constData()) == 0);
    QVERIFY_EXCEPTION_THROWN(Fixture::load(link), std::exception);
    QCOMPARE(fixture.result("battery.health").exit_status, 69);
  }
  void typedObservation() {
    Observation pane(fixture);
    pane.show();
    pane.start("status");
    QVERIFY(pane.running());
    QTRY_VERIFY_WITH_TIMEOUT(!pane.running(), 1000);
    QCOMPARE(pane.result().exit_status, 0);
    auto *tree = pane.findChild<QTreeWidget *>("observationTree");
    QVERIFY(tree);
    QMap<QString, QString> fields;
    for (int i = 0; i < tree->topLevelItemCount(); ++i)
      fields[tree->topLevelItem(i)->text(0)] = tree->topLevelItem(i)->text(1);
    QCOMPARE(fields["unknown"], QString("Unknown (not reported)"));
    QCOMPARE(fields["cycles"], QString("0"));
    QCOMPARE(fields["current_ma"], QString("-42"));
    QCOMPARE(fields["available"], QString("true"));
    QVERIFY(pane.findChild<QLabel *>("observationState")
                ->text()
                .contains("SIMULATION"));
    QTest::mouseClick(pane.findChild<QPushButton *>("refreshRead"),
                      Qt::LeftButton);
    QVERIFY(pane.running());
    QTRY_VERIFY_WITH_TIMEOUT(!pane.running(), 1000);
  }
  void stopAndUnavailable() {
    Observation pane(fixture);
    pane.start("charge.status");
    QVERIFY(pane.running());
    pane.cancel();
    QVERIFY(!pane.running());
    QVERIFY(pane.result().cancelled);
    QTest::qWait(50);
    QVERIFY(pane.result().cancelled);
    pane.start("battery.health");
    QTRY_VERIFY(!pane.running());
    QCOMPARE(pane.result().exit_status, 69);
    QVERIFY(pane.result().error.find("No command was run") !=
            std::string::npos);
    pane.start("charge.mode", {{"mode", "firmware"}});
    QVERIFY(!pane.running());
    QVERIFY(!pane.result().error.empty());
  }
  void errorsRemainSeparate() {
    Observation pane(fixture);
    pane.start("charge.status");
    QTRY_VERIFY_WITH_TIMEOUT(!pane.running(), 4000);
    QCOMPARE(pane.result().exit_status, 69);
    QVERIFY(pane.result().output.find("synthetic_provider_unavailable") !=
            std::string::npos);
    QVERIFY(pane.findChild<QPlainTextEdit *>("errorOutput")
                ->toPlainText()
                .contains("Synthetic provider error"));
    QVERIFY(!pane.findChild<QPlainTextEdit *>("rawOutput")
                 ->toPlainText()
                 .contains("Synthetic provider error"));
  }
  void navigationAndForms() {
    Window w(fixture, false);
    w.show();
    QVERIFY(w.findChild<KMessageWidget *>("simulationIndicator"));
    auto *nav = w.findChild<QListWidget *>("navigation");
    QCOMPARE(nav->count(), 7);
    for (int i = 0; i < 7; ++i) {
      nav->setCurrentRow(i);
      QCoreApplication::processEvents();
    }
    auto *list = w.findChild<QListWidget *>("advancedActions");
    QCOMPARE(list->count(), int(actions().size()));
    auto *search = w.findChild<QLineEdit *>("advancedSearch");
    search->setText("charge.mode");
    int visible = 0;
    for (int i = 0; i < list->count(); ++i)
      visible += !list->item(i)->isHidden();
    QCOMPARE(visible, 2);
    search->setText("no-match-for-this-action");
    QVERIFY(w.findChild<QLabel *>("advancedEmpty")->isVisible());
    QVERIFY(!w.findChild<QPushButton *>("openAdvancedAction")->isEnabled());
    w.openAction("charge.limit");
    auto *percent = w.findChild<QSpinBox *>("parameter_percent");
    QVERIFY(percent);
    QCOMPARE(percent->minimum(), 1);
    QCOMPARE(percent->maximum(), 100);
    w.openAction("charge.mode");
    QVERIFY(w.findChild<QComboBox *>("parameter_mode"));
    QVERIFY(w.findChild<QPushButton *>("inspectCurrent"));
    QVERIFY(w.findChild<QPushButton *>("previewRequest"));
    w.openAction("charge.status");
    QTest::mouseClick(w.findChild<QPushButton *>("primaryAction"),
                      Qt::LeftButton);
    auto *observation = w.findChild<Observation *>();
    QVERIFY(observation->running());
    w.back();
    QVERIFY(!w.findChild<Observation *>());
  }
  void reviewBlocksSimulation() {
    Window w(fixture, false);
    w.show();
    w.openAction("update.apply");
    bool inspected = false;
    QTimer::singleShot(60, &w, [&] {
      auto *dialog = w.findChild<QDialog *>("requestReview");
      if (!dialog)
        return;
      auto *consent = dialog->findChild<QCheckBox *>("reviewConsent");
      auto *token = dialog->findChild<QLineEdit *>("confirmationToken");
      auto *run = dialog->findChild<QPushButton *>("launchTerminal");
      if (!consent || !token || !run) {
        dialog->reject();
        return;
      }
      consent->setChecked(true);
      token->setText("APPLY");
      inspected = !run->isEnabled();
      dialog->reject();
    });
    QTest::mouseClick(w.findChild<QPushButton *>("primaryAction"),
                      Qt::LeftButton);
    QVERIFY(inspected);
  }
  void reviewRequiresBothApprovals() {
    // Exercise review logic without ever accepting or launching a terminal.
    Window w({}, false);
    w.show();
    w.openAction("update.apply");
    bool checked = false;
    QTimer::singleShot(60, &w, [&] {
      auto *dialog = w.findChild<QDialog *>("requestReview");
      if (!dialog)
        return;
      auto *consent = dialog->findChild<QCheckBox *>("reviewConsent");
      auto *token = dialog->findChild<QLineEdit *>("confirmationToken");
      auto *run = dialog->findChild<QPushButton *>("launchTerminal");
      if (!consent || !token || !run) {
        dialog->reject();
        return;
      }
      const bool initial = !run->isEnabled();
      token->setText("APPLY");
      const bool noConsent = !run->isEnabled();
      consent->setChecked(true);
      const bool approved = run->isEnabled();
      token->setText("apply");
      checked = initial && noConsent && approved && !run->isEnabled();
      dialog->reject();
    });
    QTest::mouseClick(w.findChild<QPushButton *>("primaryAction"),
                      Qt::LeftButton);
    QVERIFY(checked);
  }
  void readOnlyBlocksChanges() {
    Window w({}, true);
    w.show();
    w.openAction("flashlight.state");
    bool blocked = false;
    QTimer::singleShot(60, &w, [&] {
      auto *dialog = w.findChild<QDialog *>("requestReview");
      if (!dialog)
        return;
      auto *consent = dialog->findChild<QCheckBox *>("reviewConsent");
      auto *run = dialog->findChild<QPushButton *>("launchTerminal");
      if (consent && run) {
        consent->setChecked(true);
        blocked = !run->isEnabled();
      }
      dialog->reject();
    });
    QTest::mouseClick(w.findChild<QPushButton *>("primaryAction"),
                      Qt::LeftButton);
    QVERIFY(blocked);
  }
  void closeCancels() {
    auto *w = new Window(fixture, false);
    w->openAction("charge.status");
    QTest::mouseClick(w->findChild<QPushButton *>("primaryAction"),
                      Qt::LeftButton);
    auto *observation = w->findChild<Observation *>();
    QVERIFY(observation->running());
    w->close();
    QVERIFY(observation->result().cancelled);
    delete w;
  }
  void kdeNavigationAndResponsiveForms() {
    Window w(fixture, false);
    w.show();
    w.openAction("charge.mode");
    auto *navigation = w.findChild<QListWidget *>("navigation");
    QCOMPARE(navigation->currentRow(), 1);
    QVERIFY(w.findChild<QAction *>("goBack")->isEnabled());
    QVERIFY(w.findChild<QToolBar *>("navigationToolbar"));
    w.resize(480, 720);
    QCoreApplication::processEvents();
    QVERIFY(w.width() <= 480);
    QVERIFY(!w.findChild<QDockWidget *>("settingsSidebar")->isVisible());
    const auto *notice = w.findChild<KMessageWidget *>("simulationIndicator");
    QTRY_VERIFY(notice->height() >= notice->heightForWidth(notice->width()));
    const auto *primary = w.findChild<QPushButton *>("primaryAction");
    QVERIFY(primary->isVisible());
    QVERIFY(w.rect().contains(primary->mapTo(&w, primary->rect().center())));
    auto *mode = w.findChild<QComboBox *>("parameter_mode");
    QVERIFY(mode);
    QVERIFY(w.rect().contains(mode->mapTo(&w, mode->rect().center())));
    auto *destinations = w.findChild<QAction *>("showNavigation");
    bool menuOpened = false;
    QTimer::singleShot(0, [&] {
      if (auto *menu =
              qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
        menuOpened = true;
        menu->actions().at(3)->trigger();
        menu->close();
      }
    });
    destinations->trigger();
    QVERIFY(menuOpened);
    QCOMPARE(navigation->currentRow(), 3);
    QTimer::singleShot(0, [] {
      if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()))
        QTest::keyClick(menu, Qt::Key_Escape);
    });
    destinations->trigger();
    QCOMPARE(navigation->currentRow(), 3);
    w.resize(1100, 760);
    QCoreApplication::processEvents();
    QVERIFY(w.findChild<QDockWidget *>("settingsSidebar")->isVisible());
    w.back();
    QVERIFY(!w.findChild<QAction *>("goBack")->isEnabled());
    for (int iteration = 0; iteration < 4; ++iteration) {
      w.openAction("status");
      QTest::mouseClick(w.findChild<QPushButton *>("primaryAction"),
                        Qt::LeftButton);
      auto *observation = w.findChild<Observation *>();
      QVERIFY(observation);
      QTRY_VERIFY_WITH_TIMEOUT(!observation->running(), 1000);
      QTest::mouseClick(observation->findChild<QPushButton *>("refreshRead"),
                        Qt::LeftButton);
      w.back();
      QVERIFY(!w.findChild<Observation *>());
    }
  }
  void allGuidesFitNarrowWindow() {
    Window w(fixture, false);
    w.resize(480, 720);
    w.show();
    for (const auto &entry : actions()) {
      w.openAction(entry.id);
      QCoreApplication::processEvents();
      QVERIFY2(w.width() <= 480, entry.id.c_str());
      const auto *primary = w.findChild<QPushButton *>("primaryAction");
      QVERIFY(primary);
      QVERIFY2(w.rect().contains(primary->mapTo(&w, QPoint(0, 0))),
               entry.id.c_str());
      QVERIFY2(
          w.rect().contains(primary->mapTo(&w, primary->rect().bottomRight())),
          entry.id.c_str());
      w.back();
    }
  }
  void narrowInspectionCanBeCancelled() {
    Window w(fixture, false);
    w.resize(480, 720);
    w.show();
    w.openAction("charge.mode");
    bool inspected = false;
    bool readRunning = false;
    QTimer::singleShot(50, [&] {
      auto *dialog = w.findChild<QDialog *>("inspectionDialog");
      if (!dialog)
        return;
      inspected = dialog->width() <= 480;
      if (auto *observation = dialog->findChild<Observation *>())
        readRunning = observation->running();
      QTest::keyClick(dialog, Qt::Key_Escape);
    });
    QTest::mouseClick(w.findChild<QPushButton *>("inspectCurrent"),
                      Qt::LeftButton);
    QVERIFY(inspected);
    QVERIFY(readRunning);
    w.back();
    QVERIFY(!w.findChild<Observation *>());
  }
  void sanitizesControlText() {
    QCOMPARE(safe(std::string("a\x1b[31m")), QString("a\\u001b[31m"));
  }
};
QTEST_MAIN(Smoke)
#include "smoke.moc"
