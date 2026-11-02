// SPDX-License-Identifier: MIT
#include "../src/bridge.hpp"
#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KStyleManager>
#include <QAccessible>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtTest>
using namespace uq;
static QQuickItem *visualItem(QQuickItem *parent, const QString &name) {
  if (parent->objectName() == name)
    return parent;
  for (auto *child : parent->childItems())
    if (auto *found = visualItem(child, name))
      return found;
  return nullptr;
}
class KirigamiTests : public QObject {
  Q_OBJECT
  Fixture fixture;
private slots:
  void initTestCase() { fixture = Fixture::load(FIXTURE_PATH); }
  void typedReadsAndCancellation() {
    Bridge bridge(fixture, false);
    QVERIFY(bridge.openAction("status"));
    bridge.submit();
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    QMap<QString, QString> values;
    for (const auto &entry : bridge.observation().value("rows").toList()) {
      const auto row = entry.toMap();
      values[row.value("key").toString()] = row.value("value").toString();
    }
    QCOMPARE(values["unknown"], "Unknown (not reported)");
    QCOMPARE(values["cycles"], "0");
    QCOMPARE(values["current_ma"], "-42");
    QVERIFY(bridge.openAction("charge.status"));
    bridge.submit();
    QVERIFY(bridge.observation().value("running").toBool());
    bridge.cancelRead();
    QVERIFY(bridge.observation().value("cancelled").toBool());
    bridge.refresh();
    QVERIFY(bridge.observation().value("running").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!bridge.observation().value("running").toBool(),
                             4000);
    QCOMPARE(bridge.observation().value("exitStatus").toInt(), 69);
    QVERIFY(bridge.observation().value("error").toString().contains(
        "Synthetic provider error"));
    QVERIFY(!bridge.observation().value("raw").toString().contains(
        "Synthetic provider error"));
    for (int i = 0; i < 4; ++i) {
      bridge.refresh();
      bridge.leaveGuide();
      QVERIFY(bridge.observation().isEmpty());
    }
  }
  void frozenReviewAndAdmission() {
    Bridge bridge(fixture, false);
    QVERIFY(!bridge.confirmReview(true, ""));
    QVERIFY(bridge.openAction("charge.mode"));
    bridge.submit();
    QVERIFY(bridge.review().value("blocked").toBool());
    QVERIFY(
        !bridge.confirmReview(true, bridge.review().value("token").toString()));
    bridge.setDraft("mode", "firmware");
    QVERIFY(bridge.review().isEmpty());
    bridge.submit();
    QVERIFY(bridge.review().value("command").toString().contains("firmware"));
    bridge.setDraft("mode", "not-a-mode");
    bridge.submit();
    QVERIFY(!bridge.message().isEmpty());
    QVERIFY(bridge.review().isEmpty());
    QVERIFY(!bridge.confirmReview(true, ""));
    Bridge readOnly({}, true);
    QVERIFY(readOnly.openAction("charge.mode"));
    readOnly.submit();
    QVERIFY(readOnly.review().value("blocked").toBool());
    QVERIFY(!readOnly.confirmReview(true, ""));
    QVERIFY(bridge.openAction("update.apply"));
    bridge.submit();
    QCOMPARE(bridge.review().value("token").toString(), "APPLY");
    QVERIFY(!bridge.confirmReview(true, "apply"));
    QVERIFY(!bridge.confirmReview(false, "APPLY"));
    QVERIFY(!bridge.confirmReview(
        true, "APPLY")); // fixture remains blocked even with exact consent
  }
  void startupGuideRemainsVisible() {
    Bridge bridge(fixture, false);
    QVERIFY(bridge.openAction("charge.mode"));
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &bridge);
    engine.rootContext()->setContextProperty("styleNotice", QString());
    engine.loadFromModule("org.uke.ControlQtTest", "Main");
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window);
    QTest::qWait(300);
    QCOMPARE(bridge.guide().value("id").toString(), QString("charge.mode"));
    auto *guide = visualItem(window->contentItem(), "guidePage");
    QVERIFY(guide);
    QVERIFY(guide->isVisible());
    window->close();
  }
  void qmlNavigationAndEveryGuide() {
    Bridge bridge(fixture, false);
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &bridge);
    engine.rootContext()->setContextProperty("styleNotice", QString());
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, this,
            [&](const QList<QQmlError> &errors) {
              for (const auto &error : errors)
                warnings << error.toString();
            });
    engine.loadFromModule("org.uke.ControlQtTest", "Main");
    QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window);
    window->resize(480, 720);
    window->show();
    QTest::qWait(100);
    QCOMPARE(bridge.search("").size(), int(actions().size()));
    QCOMPARE(bridge.search("no-such-action-98765").size(), 0);
    for (const auto &entry : actions()) {
      QVERIFY(QMetaObject::invokeMethod(
          window, "openGuide",
          Q_ARG(QVariant, QString::fromStdString(entry.id))));
      QCoreApplication::processEvents();
      QTest::qWait(20);
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
      QCOMPARE(bridge.guide().value("id").toString(),
               QString::fromStdString(entry.id));
      QCOMPARE(window->width(), 480);
      const auto guides = window->findChildren<QQuickItem *>("guidePage");
      QVERIFY2(!guides.isEmpty(), entry.id.c_str());
      QVERIFY2(guides.size() <= 2,
               qPrintable(QString("%1 guide pages retained for %2")
                              .arg(guides.size())
                              .arg(QString::fromStdString(entry.id))));
      bool inScene = false;
      for (const auto *guide : guides)
        if (guide->window() == window && guide->isVisible())
          inScene = true;
      QVERIFY2(inScene, entry.id.c_str());
    }
    QVERIFY(QMetaObject::invokeMethod(window, "showArea", Q_ARG(QVariant, 6)));
    QCoreApplication::processEvents();
    QCOMPARE(bridge.section(), 6);
    QVERIFY(bridge.guide().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(window, "openGuide",
                                      Q_ARG(QVariant, QString("charge.mode"))));
    bridge.submit();
    QCoreApplication::processEvents();
    auto *review = window->findChild<QObject *>("requestReview");
    QVERIFY(review);
    QVERIFY(review->property("visible").toBool());
    QQuickItem *mode = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(
        (mode = visualItem(window->contentItem(), "parameter_mode")), 2000);
    auto *accessible = QAccessible::queryAccessibleInterface(mode);
    QVERIFY(accessible);
    QVERIFY(!accessible->text(QAccessible::Name).isEmpty());
    auto *launch = review->findChild<QObject *>("launchTerminal");
    QVERIFY(launch);
    QVERIFY(!launch->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(review, "reject"));
    QCoreApplication::processEvents();
    QVERIFY(bridge.review().isEmpty());
    window->resize(1100, 760);
    QTest::qWait(100);
    window->close();
    bridge.cancelRead();
    // PageRow creates a page under its nonvisual pagesLogic object before
    // inserting it into ColumnView (upstream PageRow::initPage). Qt may warn
    // about that temporary parent. Every guide above must actually enter this
    // window's scene; all other QML warnings remain failures.
    warnings.removeIf([](const QString &warning) {
      return warning.contains("Main.qml:") &&
             (warning.contains("QML GuidePage:") ||
              warning.contains("QML AreaPage:")) &&
             warning.endsWith("Created graphical object was not placed in the "
                              "graphics scene.");
    });
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
  }
};
int main(int argc, char **argv) {
  KIconTheme::initTheme();
  QApplication app(argc, argv);
  KStyleManager::initStyle();
  QQuickStyle::setStyle("org.kde.desktop");
  KirigamiTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "kirigami.moc"
