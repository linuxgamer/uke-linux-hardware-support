// SPDX-License-Identifier: MIT
#include "bridge.hpp"
#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KStyleManager>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtWidgets>
#include <unistd.h>
int main(int argc, char **argv) {
  KIconTheme::initTheme();
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("uke-control-qt");
  QCoreApplication::setApplicationVersion("1.0.0");
  QCoreApplication::setOrganizationName("Uke");
  QGuiApplication::setDesktopFileName("org.uke.ControlQt");
  KStyleManager::initStyle();
  KLocalizedString::setApplicationDomain("uke-control-qt");
  if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Native Kirigami interface to the Uke Control client. Run as a normal "
      "session user.");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(
      {"fixture",
       "Run without any commands, using a bounded simulation fixture.",
       "file"});
  parser.addOption({"read-only", "Disable mutations and local file creation."});
  parser.addOption({"start", "Open a known action guide without executing it.",
                    "action-id"});
  parser.addOption({"appearance-info",
                    "Print actual Qt/KDE appearance metadata without "
                    "opening a window or querying devices."});
  parser.process(app);
  QString notice;
  const auto styles = QStyleFactory::keys();
  bool breeze = false;
  for (const auto &style : styles)
    if (style.compare("Breeze", Qt::CaseInsensitive) == 0) {
      breeze = true;
      break;
    }
  if (!breeze)
    notice = "Breeze Qt style is unavailable; using the installed native " +
             app.style()->objectName() +
             " style. Install plasma-breeze-qt6 for Breeze.";
  if (parser.isSet("appearance-info")) {
    // Finish deferred icon/color-scheme startup before reporting actual values.
    // No frontend window or client query has been constructed on this path.
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    const auto font = app.font();
    const QFontInfo resolvedFont(font);
    QJsonObject icons;
    for (const auto *name :
         {"dialog-information", "battery", "video-display", "network-wired",
          "audio-card", "system-software-update", "preferences-system",
          "go-previous", "view-refresh", "dialog-warning"})
      icons.insert(QString::fromLatin1(name),
                   QIcon::hasThemeIcon(QString::fromLatin1(name)));
    QJsonObject environment;
    for (const auto *key : {"XDG_CURRENT_DESKTOP", "KDE_SESSION_VERSION",
                            "QT_QPA_PLATFORMTHEME", "QT_STYLE_OVERRIDE"})
      environment.insert(QString::fromLatin1(key), qEnvironmentVariable(key));
    QJsonObject palette;
    for (const auto &role :
         std::initializer_list<std::pair<const char *, QPalette::ColorRole>>{
             {"window", QPalette::Window},
             {"base", QPalette::Base},
             {"windowText", QPalette::WindowText},
             {"highlight", QPalette::Highlight},
             {"highlightedText", QPalette::HighlightedText}})
      palette.insert(QString::fromLatin1(role.first),
                     app.palette().color(role.second).name());
    const QJsonObject result{
        {"schema", "org.uke.control.qt-appearance.v1"},
        {"style",
         QJsonObject{{"objectName", app.style()->objectName()},
                     {"metaClass", QString::fromLatin1(
                                       app.style()->metaObject()->className())},
                     {"breezeAvailable", breeze},
                     {"availableStyles", QJsonArray::fromStringList(styles)}}},
        {"icons", QJsonObject{{"theme", QIcon::themeName()},
                              {"resolvedTheme", KIconTheme::current()},
                              {"fallbackTheme", QIcon::fallbackThemeName()},
                              {"available", icons}}},
        {"font", QJsonObject{{"family", font.family()},
                             {"resolvedFamily", resolvedFont.family()},
                             {"pointSize", font.pointSizeF()},
                             {"pixelSize", font.pixelSize()},
                             {"weight", int(font.weight())}}},
        {"palette", palette},
        {"platform", QGuiApplication::platformName()},
        {"quickStyle", QQuickStyle::name()},
        {"environment", environment},
        {"notice", notice}};
    QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact)
                        << Qt::endl;
    return 0;
  }
  try {
    uq::Fixture fixture;
    if (parser.isSet("fixture"))
      fixture = uq::Fixture::load(parser.value("fixture"));
    if (geteuid() == 0 && !fixture.enabled) {
      qCritical("Run Uke Control as a normal graphical session user; root GUI "
                "is refused.");
      return 77;
    }
    uq::Bridge backend(fixture, parser.isSet("read-only"));
    if (parser.isSet("start"))
      if (!backend.openAction(parser.value("start")))
        throw std::runtime_error(backend.message().toStdString());
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("styleNotice", notice);
    engine.loadFromModule("org.uke.ControlQt", "Main");
    if (engine.rootObjects().isEmpty())
      return 70;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &backend,
                     &uq::Bridge::cancelRead);
    return app.exec();
  } catch (const std::exception &e) {
    qCritical("%s", e.what());
    return 64;
  }
}
