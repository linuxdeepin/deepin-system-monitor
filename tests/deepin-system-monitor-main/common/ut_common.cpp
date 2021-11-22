/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     xuezifan<xuezifan@uniontech.com>
*
* Maintainer: xuezifan<xuezifan@uniontech.com>
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

//self
#include "common/common.h"
#include "common/hash.h"

//qt
#include <QPainter>
#include <QString>
#include <QtDBus>
#include <QDesktopServices>
#include <QApplication>
#include <QProcess>


//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_any.h"

using namespace common;
using namespace init;

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

TEST(UT_Common, test_displayShortcutHlepDialog_01)
{
    QRect rect;
    displayShortcutHelpDialog(rect);
}

TEST(UT_Common, test_getStatusBarMaxWidth_01)
{
    EXPECT_EQ(getStatusBarMaxWidth(),280);
}

TEST(UT_Common, test_drawLoadingRing_01)
{
    QPainter painter;
    int centerX; int centerY; int radius; int penWidth;
    int loadingAngle; int rotationAngle; QColor foregroundColor;
    double foregroundOpacity; QColor backgroundColor; double backgroundOpacity;
    double percent;
    drawLoadingRing(painter, centerX, centerY, radius, penWidth,
                    loadingAngle, rotationAngle, foregroundColor,
                    foregroundOpacity, backgroundColor, backgroundOpacity,
                    percent);
}

TEST(UT_Common, test_drawRing_01)
{
    QPainter painter;
    int centerX; int centerY; int radius; int penWidth;
    int loadingAngle; int rotationAngle; QColor color; double opacity;
    drawRing(painter, centerX, centerY, radius, penWidth,
             loadingAngle, rotationAngle, color, opacity);

}

TEST(UT_Common, test_startWithhanzi_01)
{
    QString text = "123";
    startWithHanzi(text);
    EXPECT_EQ(text,"123");
}

TEST(UT_Common, test_openFilePathItem_01)
{
    QString path;

    openFilePathItem(path);
}

TEST(UT_Common, test_WaylandSearchCentered_01)
{
    void WaylandSearchCentered();
}

TEST(UT_Common, test_global_init_01)
{
    global_init();
}
