// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gui/mainwindow.h"
//#include "dde-dock/constants.h"
#include "application.h"

//#include "clipboard_adaptor.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <unistd.h>
#include <QDBusInterface>



DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    // 前置wayland环境变量
    if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", "wayland");
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");
    }

    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    DGuiApplicationHelper::setColorCompositingEnabled(true);

//    DApplication *app = DApplication::globalApplication(argc, argv);

    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Application ac(argc, argv);
    ac.setAutoActivateWindows(true);

    ac.setOrganizationName("deepin");
    ac.setApplicationName("deepin-system-monitor-plugin-popup");
    ac.setApplicationDisplayName("deepin-system-monitor-plugin-popup");
    ac.setApplicationVersion("1.0");


    if (!DGuiApplicationHelper::setSingleInstance(QString("deepin-system-monitor-plugin-popup"))) {
        qDebug() << "set single instance failed!";
        return -1;
    }

    ac.loadTranslator();

    MainWindow w;
    gApp->setMainWindow(&w);

    QDBusInterface interface("com.deepin.SystemMonitorPluginPopup", "/com/deepin/SystemMonitorPluginPopup",
                                 "com.deepin.SystemMonitorPluginPopup",
                                 QDBusConnection::sessionBus());
    if (!interface.isValid()) {
        qDebug() << "start loader...";
        w.startLoader();
    }
#ifdef QT_DEBUG
    w.showAni();
#endif
    return ac.exec();
}
