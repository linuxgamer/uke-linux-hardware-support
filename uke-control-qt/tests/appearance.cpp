// SPDX-License-Identifier: MIT
#include <QColor>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

class Appearance : public QObject {
  Q_OBJECT
  struct Probe {
    bool finished = false;
    int exit = -1;
    QJsonObject data;
    QByteArray stderrText;
  };
  Probe probe(const QStringList &arguments = {},
              const QString &configuration = {},
              const QMap<QString, QString> &requestedEnvironment = {}) {
    QTemporaryDir state;
    if (!state.isValid())
      return {};
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto &key :
         {"QT_STYLE_OVERRIDE", "QT_QUICK_CONTROLS_STYLE",
          "QT_QPA_PLATFORMTHEME", "XDG_CURRENT_DESKTOP", "KDE_SESSION_VERSION",
          "KDE_FULL_SESSION", "DBUS_SESSION_BUS_ADDRESS"})
      environment.remove(QString::fromLatin1(key));
    environment.insert("QT_QPA_PLATFORM", "offscreen");
    for (const auto &directory : {"config", "cache", "data", "runtime"})
      QDir().mkpath(state.filePath(directory));
    QFile::setPermissions(state.filePath("runtime"), QFile::ReadOwner |
                                                         QFile::WriteOwner |
                                                         QFile::ExeOwner);
    environment.insert("XDG_CONFIG_HOME", state.filePath("config"));
    environment.insert("XDG_CACHE_HOME", state.filePath("cache"));
    environment.insert("XDG_DATA_HOME", state.filePath("data"));
    environment.insert("XDG_RUNTIME_DIR", state.filePath("runtime"));
    for (auto it = requestedEnvironment.begin();
         it != requestedEnvironment.end(); ++it)
      environment.insert(it.key(), it.value());
    if (!configuration.isEmpty()) {
      QFile file(state.filePath("config/kdeglobals"));
      if (!file.open(QIODevice::WriteOnly))
        return {};
      file.write(configuration.toUtf8());
    }
    QProcess process;
    process.setProcessEnvironment(environment);
    QString application = qEnvironmentVariable("UKE_QT_TEST_APP");
    if (application.isEmpty())
      application = QCoreApplication::applicationDirPath() + "/uke-control-qt";
    process.start(application, QStringList{"--appearance-info"} + arguments);
    Probe result;
    result.finished = process.waitForFinished(15000);
    if (!result.finished) {
      process.kill();
      process.waitForFinished();
    }
    result.exit = process.exitCode();
    result.stderrText = process.readAllStandardError();
    result.data =
        QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    return result;
  }
private slots:
  void nativeBreezeAndIcons() {
    const auto result = probe();
    QVERIFY2(result.finished, result.stderrText.constData());
    QCOMPARE(result.exit, 0);
    QCOMPARE(result.data.value("schema").toString(),
             QString("org.uke.control.qt-appearance.v1"));
    QCOMPARE(result.data.value("quickStyle").toString(),
             QString("org.kde.desktop"));
    const auto style = result.data.value("style").toObject();
    QVERIFY(style.value("breezeAvailable").toBool());
    QCOMPARE(style.value("objectName").toString().toLower(), QString("breeze"));
    QVERIFY(style.value("metaClass")
                .toString()
                .contains("Breeze", Qt::CaseInsensitive));
    const auto icons = result.data.value("icons").toObject();
    QCOMPARE(icons.value("resolvedTheme").toString(), QString("breeze"));
    const auto available = icons.value("available").toObject();
    QVERIFY(!available.isEmpty());
    for (auto it = available.begin(); it != available.end(); ++it)
      QVERIFY2(it.value().toBool(), qPrintable(it.key()));
    const auto font = result.data.value("font").toObject();
    QVERIFY(!font.value("resolvedFamily").toString().isEmpty());
    QVERIFY(font.value("pointSize").toDouble() > 0 ||
            font.value("pixelSize").toInt() > 0);
    const auto palette = result.data.value("palette").toObject();
    for (auto it = palette.begin(); it != palette.end(); ++it)
      QVERIFY(QColor(it.value().toString()).isValid());
  }
  void explicitQtStyleRemainsRespected() {
    const auto result = probe({}, {}, {{"QT_STYLE_OVERRIDE", "Fusion"}});
    QVERIFY2(result.finished, result.stderrText.constData());
    QCOMPARE(result.exit, 0);
    QCOMPARE(result.data.value("style")
                 .toObject()
                 .value("objectName")
                 .toString()
                 .toLower(),
             QString("fusion"));
  }
  void userKdeStyleAndIconPreferencesRemainRespected() {
    const auto result =
        probe({}, "[KDE]\nwidgetStyle=Fusion\n[Icons]\nTheme=breeze-dark\n");
    QVERIFY2(result.finished, result.stderrText.constData());
    QCOMPARE(result.exit, 0);
    QCOMPARE(result.data.value("style")
                 .toObject()
                 .value("objectName")
                 .toString()
                 .toLower(),
             QString("fusion"));
    QCOMPARE(
        result.data.value("icons").toObject().value("resolvedTheme").toString(),
        QString("breeze-dark"));
  }
  void kdePlatformFontAndPaletteRemainRespected() {
    const QDir themes(QLibraryInfo::path(QLibraryInfo::PluginsPath) +
                      "/platformthemes");
    if (themes.entryList({"*KDE*"}, QDir::Files).isEmpty())
      QSKIP(
          "KDE platform-theme plugin is unavailable in this test environment");
    QFont requested("Noto Sans");
    requested.setPointSize(13);
    const auto result = probe(
        {},
        "[General]\nfont=" + requested.toString() +
            "\n[Colors:View]\nBackgroundNormal=255,255,255\nForegroundNormal="
            "35,38,41\n"
            "[Colors:Window]\nBackgroundNormal=34,51,68\nForegroundNormal=240,"
            "240,240\n",
        {{"QT_QPA_PLATFORMTHEME", "kde"}, {"XDG_CURRENT_DESKTOP", "KDE"}});
    QVERIFY2(result.finished, result.stderrText.constData());
    QCOMPARE(result.exit, 0);
    QCOMPARE(result.data.value("font").toObject().value("pointSize").toDouble(),
             13.0);
    const auto palette = result.data.value("palette").toObject();
    QCOMPARE(palette.value("window").toString(), QString("#223344"));
  }
  void metadataDoesNotEnterAnyClientPath() {
    const auto result = probe({"--fixture", "/this-fixture-does-not-exist",
                               "--start", "not.a.real.action"});
    QVERIFY2(result.finished, result.stderrText.constData());
    QCOMPARE(result.exit, 0);
    QCOMPARE(result.data.value("schema").toString(),
             QString("org.uke.control.qt-appearance.v1"));
  }
};
QTEST_GUILESS_MAIN(Appearance)
#include "appearance.moc"
