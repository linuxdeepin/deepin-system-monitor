// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | ctor | low | c:1 | 1 | 1 |
 * | updateCache | mid | c:2 | 1 | 3 |
 */

/*****************************************************************************************
 * 测试目标：core::process::TrayAppsCache
 * 用例说明：托盘应用 pid 缓存：初始为空 / 更新（整体替换语义）/ 查询
 * 覆盖方法：TrayAppsCache / updateCache / isTrayApp（内联，作行为补充验证）
 * 打桩说明：纯内存列表缓存，无外部依赖，无需打桩。
 * 异常场景：空列表替换 / 未注册 pid 查询
 * 用例计数：4 个 TEST_F
 *****************************************************************************************/

//self
#include "process/tray_apps_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::process;

/**************************** 测试夹具 ****************************/
class UT_TrayAppsCache : public ::testing::Test
{
public:
    UT_TrayAppsCache() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        m_tester = new TrayAppsCache();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    TrayAppsCache *m_tester;
};

/**************************** 用例 ****************************/

// 构造：初始缓存为空，任意 pid 均不是托盘应用
TEST_F(UT_TrayAppsCache, ctor_InitiallyEmpty_NoTrayApp)
{
    // Arrange：SetUp 已构造
    ASSERT_TRUE(m_tester != nullptr);
    // Act
    bool isTray = m_tester->isTrayApp(1000);

    // Assert
    EXPECT_EQ(isTray, false);
    EXPECT_FALSE(m_tester->isTrayApp(-1));
}

// 更新后：列表内 pid 命中，列表外未命中
TEST_F(UT_TrayAppsCache, updateCache_WithPids_MembershipUpdated)
{
    // Arrange
    QList<pid_t> pids{ 1000, 1001, 1002 };

    // Act
    m_tester->updateCache(pids);

    // Assert
    EXPECT_EQ(m_tester->isTrayApp(1000), true);
    EXPECT_TRUE(m_tester->isTrayApp(1001));
    EXPECT_TRUE(m_tester->isTrayApp(1002));
    EXPECT_FALSE(m_tester->isTrayApp(999));
}

// 更新为空列表：整体替换而非追加，原条目全部失效
TEST_F(UT_TrayAppsCache, updateCache_EmptyList_ReplacesInsteadOfAppend)
{
    // Arrange：先注入非空缓存
    m_tester->updateCache(QList<pid_t>{ 2000, 2001 });
    ASSERT_TRUE(m_tester->isTrayApp(2000));

    // Act：更新为空
    m_tester->updateCache(QList<pid_t>());

    // Assert
    EXPECT_EQ(m_tester->isTrayApp(2000), false);
    EXPECT_FALSE(m_tester->isTrayApp(2001));
}

// 重复更新：新列表整体替换旧列表
TEST_F(UT_TrayAppsCache, updateCache_SecondUpdate_ReplacesPreviousList)
{
    // Arrange
    m_tester->updateCache(QList<pid_t>{ 3000 });

    // Act：更新为不含 3000 的新列表
    m_tester->updateCache(QList<pid_t>{ 3001, 3002 });

    // Assert
    EXPECT_EQ(m_tester->isTrayApp(3000), false);
    EXPECT_TRUE(m_tester->isTrayApp(3001));
    EXPECT_TRUE(m_tester->isTrayApp(3002));
}
