/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <unistd.h>
#include <DApplication>
#include <DApplicationSettings>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DMainWindow>
#include <DWidgetUtil>
#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <iostream>
#include <thread>

#include "constant.h"
#include "gui/main_window.h"
#include "network_traffic_filter.h"
#include "service/service_manager.h"
#include "settings.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

void defaultMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString buf = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    QByteArray local = buf.toLocal8Bit();
    const char *ts = local.constData();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "%s [Debug]: %s (%s:%u, %s)\n", ts, localMsg.constData(), context.file,
                context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "%s [Info]: %s (%s:%u, %s)\n", ts, localMsg.constData(), context.file,
                context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "%s [Warning]: %s (%s:%u, %s)\n", ts, localMsg.constData(),
                context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s [Critical]: %s (%s:%u, %s)\n", ts, localMsg.constData(),
                context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s [Fatal]: %s (%s:%u, %s)\n", ts, localMsg.constData(), context.file,
                context.line, context.function);
        break;
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(defaultMessageOutput);

    //    DApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    DApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    DApplication::loadDXcbPlugin();

    DApplication app(argc, argv);
    app.setAutoActivateWindows(true);

    if (DGuiApplicationHelper::setSingleInstance(QString("deepin-system-monitor"),
                                                 DGuiApplicationHelper::UserScope)) {
        app.loadTranslator();

        const QString descriptionText = DApplication::translate(
                                            "App.About",
                                            "System Monitor is a tool to monitor realtime system load, "
                                            "view and control processes and services running on your system.");

        const QString acknowledgementLink =
            "https://www.deepin.org/acknowledgments/deepin-system-monitor#thanks";

        app.setOrganizationName("deepin");
        app.setApplicationName("deepin-system-monitor");
        app.setApplicationDisplayName(DApplication::translate("App.About", "System Monitor"));
        app.setApplicationVersion(DApplication::buildVersion("1.0"));

        app.setProductIcon(QIcon::fromTheme("deepin-system-monitor"));
        app.setProductName(DApplication::translate("App.About", "System Monitor"));
        app.setApplicationDescription(descriptionText);
        app.setApplicationAcknowledgementPage(acknowledgementLink);

        app.setWindowIcon(QIcon::fromTheme("deepin-system-monitor"));

        DApplicationSettings appSettings;

        std::thread nethogs_monitor_thread(&NetworkTrafficFilter::nethogsMonitorThreadProc);
        nethogs_monitor_thread.detach();

        MainWindow *window = MainWindow::instance();
        window->initDisplay();

        QObject::connect(&app, &DApplication::newInstanceStarted, window,
                         &MainWindow::activateWindow);
        QObject::connect(&app, &QCoreApplication::aboutToQuit, window,
        [ = ]() { window->deleteLater(); });

        window->setMinimumSize(QSize(Constant::WINDOW_MIN_WIDTH, Constant::WINDOW_MIN_HEIGHT));
        Dtk::Widget::moveToCenter(window);
        window->show();

        return app.exec();
    }

    return 0;
}
