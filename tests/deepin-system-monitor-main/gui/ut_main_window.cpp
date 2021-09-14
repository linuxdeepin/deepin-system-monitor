/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define private public
#define protected public
#include "main_window.h"
#include "application.h"
#include "process_page_widget.h"
#include "system_service_page_widget.h"
#include "toolbar.h"
#include "common/common.h"
#include "settings.h"
#include "common/perf.h"
#include "detailwidgetmanager.h"
#include "gui/dialog/systemprotectionsetting.h"

#include <DSettingsWidgetFactory>
#include <DApplicationHelper>
#include <DTitlebar>
#include <QKeyEvent>
#include <QTimer>
#include <QDesktopWidget>
#include <QDBusConnection>
#undef private
#undef protected

#include "ut_main_window.h"

Ut_MainWindow::Ut_MainWindow()
{

}

TEST(UT_MainWindow_initUI, UT_MainWindow_initUI_001)
{
    MainWindow *mainwindow = new MainWindow;
    gApp->setMainWindow(mainwindow);
    mainwindow->initUI();

    EXPECT_GE(mainwindow->width(), 1080);
    EXPECT_GE(mainwindow->height(), 760);

    mainwindow->deleteLater();
}

TEST(UT_MainWindow_initDisplay, UT_MainWindow_initDisplay_001)
{
    MainWindow *mainwindow = new MainWindow;
    gApp->setMainWindow(mainwindow);
    mainwindow->initDisplay();

    EXPECT_NE(mainwindow->toolbar(), nullptr);

    mainwindow->deleteLater();
}

