// SPDX-License-Identifier: MIT
// Compile the actual applet without creating it or invoking device commands.
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <cstdio>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl::fromLocalFile(argv[1]), QQmlComponent::PreferSynchronous);
    if (component.isLoading()) {
        QEventLoop loop;
        QObject::connect(&component, &QQmlComponent::statusChanged, &loop, &QEventLoop::quit);
        QTimer::singleShot(10000, &loop, &QEventLoop::quit);
        loop.exec();
    }
    for (const auto &error : component.errors()) std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
    if (!component.isReady()) return 1;
    std::puts("Actual Plasma applet QML component is ready; no device command executed");
}
