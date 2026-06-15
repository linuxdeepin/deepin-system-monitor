// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// eventlogutils 原先 0% 覆盖。单例 + QLibrary 动态加载 libdeepin-event-log.so。
// 本测试覆盖 get() 单例（首次构造 + 复用）与 writeLogs（库已加载走 writeEventLog 调用）。

#include "common/eventlogutils.h"

#include <gtest/gtest.h>
#include <QJsonObject>
#include <QJsonDocument>

class UT_EventLogUtils : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// get() 单例：首次调用构造实例（覆盖私有构造函数 + QLibrary::resolve + init 调用），
// 第二次调用走 m_instance != nullptr 复用分支
TEST_F(UT_EventLogUtils, Get_ReturnsSameSingletonInstance)
{
    EventLogUtils &inst1 = EventLogUtils::get();
    EventLogUtils &inst2 = EventLogUtils::get();
    EXPECT_EQ(&inst1, &inst2);
}

// writeLogs：库已加载（writeEventLog 非空）→ 调用 writeEventLog 写入 JSON；不应崩溃
TEST_F(UT_EventLogUtils, WriteLogs_ValidData_DoesNotCrash)
{
    EventLogUtils &inst = EventLogUtils::get();
    QJsonObject data;
    data["tid"] = EventLogUtils::OpeningTime;
    data["ts"] = 0;
    inst.writeLogs(data);
}

// writeLogs 空对象也不应崩溃（覆盖 JSON 序列化路径）
TEST_F(UT_EventLogUtils, WriteLogs_EmptyObject_DoesNotCrash)
{
    EventLogUtils &inst = EventLogUtils::get();
    QJsonObject empty;
    inst.writeLogs(empty);
}
