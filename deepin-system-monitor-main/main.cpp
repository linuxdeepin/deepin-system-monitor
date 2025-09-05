// Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "environments.h"
#include "accessible.h"
#include "common/common.h"
#include "application.h"
#include "settings.h"
#include "gui/main_window.h"
#include "common/perf.h"
#include "dbus/dbus_object.h"
#include "dbus/dbusalarmnotify.h"
#include "3rdparty/dmidecode/dmidecode.h"
#include "ddlog.h"

#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationSettings>
#endif
#include <DGuiApplicationHelper>
#include <DMainWindow>
#include <DWidgetUtil>
#include <DLog>

#include <QApplication>
#include <QDateTime>
#include <QAccessible>
#include <QTimer>
#include <signal.h>
#include "logger.h"
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

using namespace common::init;

static Application *g_app = nullptr;

void signalHandler(int signal)
{
    qCDebug(DDLog::app) << "Received signal:" << signal;
    if (g_app) {
        QTimer::singleShot(0, g_app, &Application::quit);
    }
}

int main(int argc, char *argv[])
{
    MLogger();   // 日志处理要放在app之前，否则QApplication
            // 内部可能进行了日志打印，导致环境变量设置不生效
// 为了兼容性
#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 6, 8, 0))
    Dtk::Core::DLogManager::registerJournalAppender();
#else
    Dtk::Core::DLogManager::registerFileAppender();
#endif
#ifdef QT_DEBUG
    Dtk::Core::DLogManager::registerConsoleAppender();
#endif
    qCDebug(DDLog::app) << "Starting deepin-system-monitor";

    //Judge if Wayland
    WaylandSearchCentered();
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        qCDebug(DDLog::app) << "XDG_CURRENT_DESKTOP is not deepin, setting it.";
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Application app(argc, argv);
    g_app = &app;
    
    // 注册信号处理器以确保优雅退出
    signal(SIGTERM, signalHandler);
    
    qCDebug(DDLog::app) << "Application object created";
    QCommandLineParser parser;
    parser.process(app);
    QStringList allArguments = parser.positionalArguments();
    if (allArguments.size() == 3 && allArguments.first().compare("alarm", Qt::CaseInsensitive) == 0) {
        qCDebug(DDLog::app) << "Alarm command detected, showing notification and exiting.";
        app.loadTranslator();
        DBusAlarmNotify::getInstance().showAlarmNotify(allArguments);
        return 0;
    }

    //=======通知已经打开的进程
    if (!DBusObject::getInstance().registerOrNotify()) {
        qCDebug(DDLog::app) << "Another instance is already running, exiting.";
        return 0;
    }

    //获取dmidecode中CPU频率信息
    qCDebug(DDLog::app) << "Getting CPU info from dmidecode";
    char *const cmd[] = { "dmidecode", "-t", "4" };
    get_cpuinfo_from_dmi(3, cmd);
    PERF_PRINT_BEGIN("POINT-01", "");

    app.setAutoActivateWindows(true);
    //设置单例
    DGuiApplicationHelper::setSingleInstanceInterval(-1);
    if (DGuiApplicationHelper::setSingleInstance(QString("deepin-system-monitor"),
                                                 DGuiApplicationHelper::UserScope)) {
        qCDebug(DDLog::app) << "Successfully set single instance, starting main application";
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
        app.setApplicationVersion(VERSION);

        app.setProductIcon(QIcon::fromTheme("deepin-system-monitor"));
        app.setProductName(DApplication::translate("App.About", "System Monitor"));
        app.setApplicationDescription(descriptionText);
        app.setApplicationAcknowledgementPage(acknowledgementLink);

        //app.setWindowIcon(QIcon::fromTheme("deepin-system-monitor")); //耗时40ms

        QAccessible::installFactory(accessibleFactory);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        DApplicationSettings appSettings;
#endif

        DLogManager::registerConsoleAppender();
        DLogManager::registerFileAppender();

        MainWindow mw;
        gApp->setMainWindow(&mw);
        mw.initDisplay();

        QObject::connect(&app,
                         &DApplication::newInstanceStarted,
                         &mw,
                         &MainWindow::activateWindow);

        Dtk::Widget::moveToCenter(&mw);
        qCDebug(DDLog::app) << "Showing main window";
        mw.show();

        qCDebug(DDLog::app) << "Starting application event loop";
        int result = app.exec();
        g_app = nullptr;
        return result;
    }

    qCDebug(DDLog::app) << "Could not set single instance, exiting.";
    g_app = nullptr;
    return 0;
}
