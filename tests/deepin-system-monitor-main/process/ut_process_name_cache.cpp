// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | insert | mid | c:2 | 1 | 2 |
 * | processName/remove/contains/clear | mid | c:2 | 1 | 4 |
 */

/*****************************************************************************************
 * 测试目标：core::process::ProcessNameCache
 * 用例说明：进程名缓存单例：插入/查询/移除/清空/容量淘汰
 * 覆盖方法：ProcessNameCache / instance / insert / processName / contains / remove / clear
 * 打桩说明：QCache 纯内存语义，无外部依赖，无需打桩；缓存接管插入对象所有权
 *           （remove/clear/淘汰时自动删除），用例插入后不得手动 delete。
 *           单例由 instance() 提供，用例间用 clear() 清场，夹具不拥有单例所有权。
 * 异常场景：未注册 pid 查询 / 超过默认 maxCost(100) 淘汰
 * 用例计数：7 个 TEST_F
 *****************************************************************************************/

//self
#include "process/process_name_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//project
#include "process/process_name.h"

using namespace core::process;

/**************************** 测试夹具 ****************************/
class UT_ProcessNameCache : public ::testing::Test
{
public:
    UT_ProcessNameCache() = default;

    virtual void SetUp()
    {
        m_tester = ProcessNameCache::instance();
        m_tester->clear();
    }

    virtual void TearDown()
    {
        // 清场但不删除单例：m_instance 为静态，删除会导致悬垂
        m_tester->clear();
        m_tester = nullptr;
    }

protected:
    ProcessNameCache *m_tester = nullptr;
};

/**************************** 用例 ****************************/

// 单例语义：多次获取同一实例
TEST_F(UT_ProcessNameCache, instance_RepeatedCalls_ReturnsSamePointer)
{
    // Arrange：SetUp 已获取实例
    ASSERT_TRUE(m_tester != nullptr);
    // Act
    ProcessNameCache *again = ProcessNameCache::instance();
    // Assert
    EXPECT_EQ(again, m_tester);
    EXPECT_NE(m_tester, nullptr);
    EXPECT_NE(again, nullptr);
}

// 插入后可查询且返回同一对象指针
TEST_F(UT_ProcessNameCache, insert_NewPid_ProcessNameReturnsSameObject)
{
    // Arrange
    pid_t pid = 100;
    ProcessName *name = new ProcessName();

    // Act
    m_tester->insert(pid, name);

    // Assert
    EXPECT_TRUE(m_tester->contains(pid));
    EXPECT_EQ(m_tester->processName(pid), name);
}

// 未注册 pid：查询返回空指针
TEST_F(UT_ProcessNameCache, processName_MissingPid_ReturnsNull)
{
    // Arrange：缓存为空
    ASSERT_FALSE(m_tester->contains(999));
    // Act
    const ProcessName *name = m_tester->processName(999);
    // Assert
    EXPECT_EQ(name, nullptr);
    EXPECT_EQ(m_tester->contains(999), false);
}

// 移除后不可再查询
TEST_F(UT_ProcessNameCache, remove_RegisteredPid_BecomesMissing)
{
    // Arrange
    m_tester->insert(200, new ProcessName());
    ASSERT_TRUE(m_tester->contains(200));

    // Act
    m_tester->remove(200);

    // Assert
    EXPECT_FALSE(m_tester->contains(200));
    EXPECT_EQ(m_tester->processName(200), nullptr);
}

// clear：清空全部条目
TEST_F(UT_ProcessNameCache, clear_MultipleEntries_EmptiesCache)
{
    // Arrange
    m_tester->insert(301, new ProcessName());
    m_tester->insert(302, new ProcessName());
    m_tester->insert(303, new ProcessName());
    ASSERT_TRUE(m_tester->contains(301));
    ASSERT_TRUE(m_tester->contains(303));

    // Act
    m_tester->clear();

    // Assert
    EXPECT_EQ(m_tester->contains(301), false);
    EXPECT_FALSE(m_tester->contains(302));
    EXPECT_FALSE(m_tester->contains(303));
}

// contains：未注册 pid 直接返回 false
TEST_F(UT_ProcessNameCache, contains_MissingPid_ReturnsFalse)
{
    // Arrange：空缓存
    ASSERT_EQ(m_tester->processName(404), nullptr);
    // Act
    bool exists = m_tester->contains(404);
    // Assert
    EXPECT_EQ(exists, false);
    EXPECT_EQ(m_tester->contains(404), false);
}

// 容量淘汰：默认 maxCost=100，超出后最早插入的条目被淘汰
TEST_F(UT_ProcessNameCache, insert_MaxCostExceeded_EvictsOldestEntry)
{
    // Arrange：空缓存（无历史条目）
    ASSERT_FALSE(m_tester->contains(0));
    // Act：连续插入 150 个条目（代价 1/个，超过默认 100）
    for (int i = 0; i < 150; ++i) {
        m_tester->insert(i, new ProcessName());
    }

    // Assert：前 50 个被淘汰，末尾条目保留
    EXPECT_EQ(m_tester->contains(0), false);
    EXPECT_EQ(m_tester->contains(49), false);
    EXPECT_TRUE(m_tester->contains(50));
    EXPECT_TRUE(m_tester->contains(149));
}
