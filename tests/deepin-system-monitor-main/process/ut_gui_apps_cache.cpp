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
 * 测试目标：core::process::GuiAppsCache
 * 用例说明：GUI 应用 pid 缓存：初始为空 / 更新（整体替换语义）/ 查询
 * 覆盖方法：GuiAppsCache / updateCache / isGuiApp（内联，作行为补充验证）
 * 打桩说明：纯内存列表缓存，无外部依赖，无需打桩。
 * 异常场景：空列表替换 / 未注册 pid 查询
 * 用例计数：4 个 TEST_F
 *****************************************************************************************/

//self
#include "process/gui_apps_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::process;

/**************************** 测试夹具 ****************************/
class UT_GuiAppsCache : public ::testing::Test
{
public:
    UT_GuiAppsCache() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        m_tester = new GuiAppsCache();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    GuiAppsCache *m_tester;
};

/**************************** 用例 ****************************/

// 构造：初始缓存为空，任意 pid 均不是 GUI 应用
TEST_F(UT_GuiAppsCache, ctor_InitiallyEmpty_NoGuiApp)
{
    // Arrange：SetUp 已构造
    ASSERT_TRUE(m_tester != nullptr);
    // Act
    bool isGui = m_tester->isGuiApp(1000);

    // Assert
    EXPECT_EQ(isGui, false);
    EXPECT_FALSE(m_tester->isGuiApp(-1));
}

// 更新后：列表内 pid 命中，列表外未命中
TEST_F(UT_GuiAppsCache, updateCache_WithPids_MembershipUpdated)
{
    // Arrange
    QList<pid_t> pids{ 500, 600 };

    // Act
    m_tester->updateCache(pids);

    // Assert
    EXPECT_EQ(m_tester->isGuiApp(500), true);
    EXPECT_TRUE(m_tester->isGuiApp(600));
    EXPECT_FALSE(m_tester->isGuiApp(700));
}

// 更新为空列表：整体替换，原条目全部失效
TEST_F(UT_GuiAppsCache, updateCache_EmptyList_ReplacesInsteadOfAppend)
{
    // Arrange
    m_tester->updateCache(QList<pid_t>{ 800 });
    ASSERT_TRUE(m_tester->isGuiApp(800));

    // Act
    m_tester->updateCache(QList<pid_t>());

    // Assert：原条目 800 失效；pid 域内其他 pid 亦未注册
    EXPECT_EQ(m_tester->isGuiApp(800), false);
    EXPECT_EQ(m_tester->isGuiApp(801), false);
}

// 重复更新：新列表整体替换旧列表
TEST_F(UT_GuiAppsCache, updateCache_SecondUpdate_ReplacesPreviousList)
{
    // Arrange
    m_tester->updateCache(QList<pid_t>{ 900 });

    // Act
    m_tester->updateCache(QList<pid_t>{ 901 });

    // Assert
    EXPECT_EQ(m_tester->isGuiApp(900), false);
    EXPECT_TRUE(m_tester->isGuiApp(901));
}
