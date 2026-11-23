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

TEST(UT_Common, test_formatHz_normal)
{
    // 2400MHz 进位显示为 2.40GHz
    EXPECT_EQ(QString("2.40GHz"), format::formatHz(2400.0, format::MHz));
    // 恰好 1000 不触发进位（条件为 > 1000）
    EXPECT_EQ(QString("1000.00MHz"), format::formatHz(1000.0, format::MHz));
    // 2400000MHz 连进两级到 THz
    EXPECT_EQ(QString("2.40THz"), format::formatHz(2400000.0, format::MHz));
}

TEST(UT_Common, test_formatHz_negative)
{
    // 负频率无实际意义，必须显示 0 且不得触发浮点转整型的未定义行为
    EXPECT_EQ(QString("0.00MHz"), format::formatHz(-1200.0, format::MHz));
}

TEST(UT_Common, test_formatHz_nonFinite)
{
    // NaN 与正负无穷均按 0 处理
    const qreal nan = qQNaN();
    const qreal inf = qInf();
    EXPECT_EQ(QString("0.00MHz"), format::formatHz(nan, format::MHz));
    EXPECT_EQ(QString("0.00MHz"), format::formatHz(inf, format::MHz));
    EXPECT_EQ(QString("0.00MHz"), format::formatHz(-inf, format::MHz));
}

TEST(UT_Common, test_formatHz_overflowSaturate)
{
    // 超出 quint32 上限的值饱和处理，单位索引不得越过 HzUnitSuffix 边界。
    // quint32 上限 4294967295KHz 换算后约 4.29PHz。
    const qreal huge = 1e30;
    const QString result = format::formatHz(huge, format::MHz);
    EXPECT_FALSE(result.isEmpty());
    // 单位后缀必须是数组内的合法项，不能是越界读到的内容
    bool suffixValid = false;
    for (const char *suffix : format::HzUnitSuffix) {
        if (result.endsWith(QString::fromLatin1(suffix)))
            suffixValid = true;
    }
    EXPECT_TRUE(suffixValid);
}
