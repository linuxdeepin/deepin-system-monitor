/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
#include "gui/mainwindow.h"
#include "dde-dock/constants.h"
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
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell");
    }

    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    DGuiApplicationHelper::setColorCompositingEnabled(true);

    DApplication *app = DApplication::globalApplication(argc, argv);

    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Application ac(argc, argv);
    ac.setAutoActivateWindows(true);

    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-system-monitor-plugin-popup");
    app->setApplicationDisplayName("deepin-system-monitor-plugin-popup");
    app->setApplicationVersion("1.0");


    if (!DGuiApplicationHelper::setSingleInstance(QString("deepin-system-monitor-plugin-popup"))) {
        qDebug() << "set single instance failed!";
        return -1;
    }

    app->loadTranslator();

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
    return app->exec();
}
