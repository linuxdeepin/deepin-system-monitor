// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QDebug>

#include "systemmonitorapplet.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("deepin-system-monitor-dde-shell-plugin"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QQmlApplicationEngine engine;

    // Use app as parent so the object is destroyed when QGuiApplication exits.
    // qmlRegisterSingletonInstance does NOT transfer ownership to the QML engine.
    SystemMonitorApplet *applet = new SystemMonitorApplet(&app);
    qmlRegisterSingletonInstance("org.deepin.systemmonitor", 1, 0,
                                "SystemMonitorApplet",
                                applet);

    const QUrl url(QStringLiteral("qrc:/org/deepin/systemmonitor/package/main.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No root objects loaded!";
        return -1;
    }

    return app.exec();
}
