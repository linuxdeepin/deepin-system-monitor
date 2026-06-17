// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
void stub_Common_startDetached(void* obj, qint64*)
{
    return;
}
/***************************************STUB end**********************************************/

TEST(UT_Common, test_displayShortcutHlepDialog_01)
{
    QRect rect;
    Stub stub;
    stub.set((bool(QProcess::*)(qint64*))ADDR(QProcess, startDetached), stub_Common_startDetached);
    displayShortcutHelpDialog(rect);
}

TEST(UT_Common, test_getStatusBarMaxWidth_01)
{
    EXPECT_EQ(getStatusBarMaxWidth(),280);
}

TEST(UT_Common, test_drawLoadingRing_01)
{
    QPixmap pixmap;
    QPainter painter(&pixmap);
    int centerX = 0;
    int centerY = 0;
    int radius = 0;
    int penWidth = 0;
    int loadingAngle = 0;
    int rotationAngle = 0;
    QColor foregroundColor;
    double foregroundOpacity = 0.0;
    QColor backgroundColor;
    double backgroundOpacity = 0.0;
    double percent = 0.0;
    drawLoadingRing(painter, centerX, centerY, radius, penWidth,
                    loadingAngle, rotationAngle, foregroundColor,
                    foregroundOpacity, backgroundColor, backgroundOpacity,
                    percent);
}

TEST(UT_Common, test_drawRing_01)
{
    QPixmap pixmap;
    QPainter painter(&pixmap);
    int centerX = 0;
    int centerY = 0;
    int radius = 0;
    int penWidth = 0;
    int loadingAngle = 0;
    int rotationAngle = 0;
    QColor color;
    double opacity = 0.0;
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
    Stub stub;
    stub.set((bool(QProcess::*)(qint64*))ADDR(QProcess, startDetached), stub_Common_startDetached);
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

// ========== 新增：覆盖 format::formatUnit_memory_disk / formatUnit_net 各分支 ==========
#include "common/common.h"
using namespace common::format;

TEST(UT_Common, test_formatUnit_memory_disk_invalidType_returnsEmpty)
{
    // 不可转为 Double 的类型 → 返回空
    QVariant invalid = QVariant::fromValue<QObject*>(nullptr);
    QString r = formatUnit_memory_disk(invalid);
    EXPECT_TRUE(r.isEmpty());
}

TEST(UT_Common, test_formatUnit_memory_disk_smallValue)
{
    // 小于 1024 不进位
    QString r = formatUnit_memory_disk(QVariant(512.0), B, 1, false);
    EXPECT_FALSE(r.isEmpty());
    EXPECT_TRUE(r.contains("B"));
}

TEST(UT_Common, test_formatUnit_memory_disk_largeValue_unitEscalation)
{
    // 大于 1024 多次 → 单位递进到 KB/MB/GB
    QString r = formatUnit_memory_disk(QVariant(1024.0 * 1024 * 1024 * 5), B, 2, false);
    EXPECT_FALSE(r.isEmpty());
    EXPECT_TRUE(r.contains("GB"));
}

TEST(UT_Common, test_formatUnit_memory_disk_speedFlag)
{
    QString r = formatUnit_memory_disk(QVariant(2048.0), B, 1, true);
    EXPECT_TRUE(r.contains("/s"));
}

TEST(UT_Common, test_formatUnit_net_invalidType_returnsEmpty)
{
    QVariant invalid = QVariant::fromValue<QObject*>(nullptr);
    QString r = formatUnit_net(invalid);
    EXPECT_TRUE(r.isEmpty());
}

TEST(UT_Common, test_formatUnit_net_smallValue)
{
    QString r = formatUnit_net(QVariant(100.0), B, 1, false);
    EXPECT_FALSE(r.isEmpty());
}

TEST(UT_Common, test_formatUnit_net_largeValue_speedFlag)
{
    QString r = formatUnit_net(QVariant(1024.0 * 1024 * 2), B, 1, true);
    EXPECT_FALSE(r.isEmpty());
    EXPECT_TRUE(r.contains("/s"));
}
