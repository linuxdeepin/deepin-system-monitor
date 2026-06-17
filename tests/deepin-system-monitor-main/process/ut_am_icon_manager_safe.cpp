// Copyright (C) 2026 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 安全版 am_icon_manager 单测：仅覆盖不依赖真实 AM D-Bus 服务的纯逻辑分支。
// 原 ut_am_icon_manager.cpp.disabled 因桩化 DSysInfo / pidfd_open 导致不稳定被禁用，
// 此文件不复用那些桩，仅通过单例 instance() 触发构造，并验证 getIconByAppId 的
// 空输入早退（不触发任何 D-Bus / 缓存查找路径）。

//self
#include "process/am_icon_manager.h"

//gtest
#include <gtest/gtest.h>

//qt
#include <QString>

using namespace core::process;

TEST(UT_AMIconManagerSafe, getIconByAppId_emptyInput_returnsEmpty)
{
    // 空 appId → 直接返回空串，不触发任何 D-Bus 调用
    AMIconManager *mgr = AMIconManager::instance();
    ASSERT_NE(mgr, nullptr);
    QString result = mgr->getIconByAppId("");
    EXPECT_TRUE(result.isEmpty());
}

TEST(UT_AMIconManagerSafe, instance_isSingleton)
{
    AMIconManager *a = AMIconManager::instance();
    AMIconManager *b = AMIconManager::instance();
    EXPECT_EQ(a, b);
}

TEST(UT_AMIconManagerSafe, clearCache_noThrow)
{
    AMIconManager *mgr = AMIconManager::instance();
    EXPECT_NO_THROW(mgr->clearCache());
}
