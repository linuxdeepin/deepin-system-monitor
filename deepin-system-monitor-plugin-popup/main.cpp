// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gui/mainwindow.h"
#include "dde-dock/constants.h"
#include "application.h"
#include "ddlog.h"

//#include "clipboard_adaptor.h"
#include "logger.h"
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <unistd.h>
#include <QDBusInterface>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

using namespace DDLog;

int main(int argc, char *argv[])
{
    qCInfo(app) << "Starting deepin-system-monitor-plugin-popup application";
#ifdef DTKCORE_CLASS_DConfigFile
    MLogger();   // 日志处理要放在app之前，否则QApplication
            // 内部可能进行了日志打印，导致环境变量设置不生效
#endif

// 为了兼容性
#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 6, 8, 0))
    Dtk::Core::DLogManager::registerJournalAppender();
#else
    Dtk::Core::DLogManager::registerFileAppender();
#endif
#ifdef QT_DEBUG
    Dtk::Core::DLogManager::registerConsoleAppender();
#endif

    // 前置wayland环境变量
    if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        qCDebug(app) << "Wayland display detected, setting Wayland platform";
        qputenv("QT_QPA_PLATFORM", "wayland");
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qCDebug(app) << "Setting Qt5 specific GUI attributes";
    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    DGuiApplicationHelper::setColorCompositingEnabled(true);
#else
    qCDebug(app) << "Setting Qt6 specific GUI attributes";
    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::UseInactiveColorGroup, false);
    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::ColorCompositing, true);
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
    Application ac(argc, argv);
    ac.setAutoActivateWindows(true);

    ac.setOrganizationName("deepin");
    ac.setApplicationName("deepin-system-monitor-plugin-popup");
    ac.setApplicationDisplayName("deepin-system-monitor-plugin-popup");
    ac.setApplicationVersion("1.0");

    if (!DGuiApplicationHelper::setSingleInstance(QString("deepin-system-monitor-plugin-popup"))) {
        qCWarning(app) << "Failed to set single instance, another instance may be running";
        return -1;
    }

    ac.loadTranslator();

    MainWindow w;
    gApp->setMainWindow(&w);

    qCDebug(app) << "Checking DBus interface";
    QDBusInterface interface("com.deepin.SystemMonitorPluginPopup", "/com/deepin/SystemMonitorPluginPopup",
                             "com.deepin.SystemMonitorPluginPopup",
                             QDBusConnection::sessionBus());
    if (!interface.isValid()) {
        qCDebug(app) << "DBus interface not found, starting loader";
        w.startLoader();
    } else {
        qCDebug(app) << "DBus interface found, skipping loader";
    }

#ifdef QT_DEBUG
    w.showAni();
#endif
    return ac.exec();
}
