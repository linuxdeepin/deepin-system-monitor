/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "environments.h"
#include "accessible.h"
#include "common/common.h"
#include "application.h"
#include "settings.h"
#include "gui/main_window.h"
#include "common/perf.h"

#include <DApplication>
#include <DApplicationSettings>
#include <DGuiApplicationHelper>
#include <DMainWindow>
#include <DWidgetUtil>
#include <DLog>

#include <QApplication>
#include <QDateTime>
#include <QAccessible>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    //
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")){
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }
    PERF_PRINT_BEGIN("POINT-01", "");

    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Application app(argc, argv);
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
        app.setApplicationVersion(VERSION);

        app.setProductIcon(QIcon::fromTheme("deepin-system-monitor"));
        app.setProductName(DApplication::translate("App.About", "System Monitor"));
        app.setApplicationDescription(descriptionText);
        app.setApplicationAcknowledgementPage(acknowledgementLink);

        //app.setWindowIcon(QIcon::fromTheme("deepin-system-monitor")); //耗时40ms

        QAccessible::installFactory(accessibleFactory);

        DApplicationSettings appSettings;

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
        mw.show();

        return app.exec();
    }

    return 0;
}
