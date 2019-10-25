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

    DApplication::loadDXcbPlugin();

    DApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    if (app.setSingleInstance(QString("deepin-system-monitor"), DApplication::UserScope)) {
        app.loadTranslator();

        const QString descriptionText = DApplication::tr(
            "Deepin System Monitor is an intuitive and powerful system monitor. It can monitor the "
            "process CPU, memory, network, disk and other status.");

        const QString acknowledgementLink =
            "https://www.deepin.org/acknowledgments/deepin-system-monitor#thanks";

        app.setOrganizationName("deepin");
        app.setApplicationName("deepin-system-monitor");
        app.setApplicationDisplayName(DApplication::tr("Deepin System Monitor"));
        app.setApplicationVersion("1.4.2");

        app.setProductIcon(QIcon(Utils::getQrcPath("logo_96.svg")));
        app.setProductName(DApplication::tr("Deepin System Monitor"));
        app.setApplicationDescription(descriptionText);
        app.setApplicationAcknowledgementPage(acknowledgementLink);

        app.setWindowIcon(QIcon(Utils::getQrcPath("logo_96.svg")));

        Settings *settings = Settings::instance();

        std::thread nethogs_monitor_thread(&NetworkTrafficFilter::nethogsMonitorThreadProc);
        nethogs_monitor_thread.detach();

        MainWindow *window = MainWindow::instance();
        window->initDisplay();

        QObject::connect(&app, &DApplication::newInstanceStarted, window,
                         &MainWindow::activateWindow);
        QObject::connect(&app, &QCoreApplication::aboutToQuit, window, [=]() {
            if (settings)
                settings->flush();
            window->deleteLater();
        });

        window->setMinimumSize(QSize(Constant::WINDOW_MIN_WIDTH, Constant::WINDOW_MIN_HEIGHT));
        Dtk::Widget::moveToCenter(window);
        window->show();
        //        window.adjustStatusBarWidth();

        int rc = app.exec();
        if (settings)
            settings->flush();

        return rc;
    }

    return 0;
}
