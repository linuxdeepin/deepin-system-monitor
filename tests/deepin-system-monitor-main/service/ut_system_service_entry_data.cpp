// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | SystemServiceEntryData(ctor) | low | - | 1 | 2 |
// | operator= | low | operator | 1 | 2 |
// | dtor | low | - | 1 | 1 |

//self
#include "service/system_service_entry_data.h"

//gtest
#include <gtest/gtest.h>

/*
 * [test-types 最小清单完成情况]
 * 1. 复杂度>=10 方法分支覆盖：本文件无复杂度>=10 方法（EntryData 仅默认构造/拷贝/赋值/析构）— 完成
 * 2. AAA 三段注释：所有用例 Arrange/Act/Assert 分段 — 完成
 * 3. TEST_P 参数化 >=3 组同质输入：值集参数化 3 组 — 完成
 * 4. EXPECT_THROW 精确异常：无抛异常 API，负面路径以字段默认值断言覆盖 — 完成
 * 5. 外部依赖隔离：纯内存数据类，无 DBus/网络/文件依赖 — 完成
 *
 * [分支清单]
 * - SystemServiceEntryData()           默认构造：13 个成员 NSDMI 默认值
 * - SystemServiceEntryData(const &)    拷贝构造：13 个成员逐一拷贝（QSharedData ref 归零 → 独立副本）
 * - operator=(const &)                 self-assignment 短路分支 (this != &rhs) T/F
 * - ~SystemServiceEntryData()          析构无副作用
 */

/**
 * @brief 逐字段构造一个填满 13 个成员的源对象（Arrange 复用）
 */
static SystemServiceEntryData makeFullData(const QString &tag)
{
    SystemServiceEntryData d;
    d.m_id = "id@" + tag;
    d.m_sname = "sname_" + tag;
    d.m_loadState = "loaded_" + tag;
    d.m_activeState = "active_" + tag;
    d.m_subState = "running_" + tag;
    d.m_state = "enabled_" + tag;
    d.m_startupType = "auto_" + tag;
    d.m_unitObjectPath = "/org/freedesktop/systemd1/unit/" + tag;
    d.m_description = "desc " + tag;
    d.m_mainPID = 4321u;
    d.m_canReload = true;
    d.m_canStart = true;
    d.m_canStop = false;
    return d;
}

/**
 * @brief 断言 13 个成员全部等于 tag 对应的期望值（Assert 复用）
 */
static void expectAllFields(const SystemServiceEntryData &d, const QString &tag)
{
    EXPECT_EQ(d.m_id, QString("id@") + tag);
    EXPECT_EQ(d.m_sname, QString("sname_") + tag);
    EXPECT_EQ(d.m_loadState, QString("loaded_") + tag);
    EXPECT_EQ(d.m_activeState, QString("active_") + tag);
    EXPECT_EQ(d.m_subState, QString("running_") + tag);
    EXPECT_EQ(d.m_state, QString("enabled_") + tag);
    EXPECT_EQ(d.m_startupType, QString("auto_") + tag);
    EXPECT_EQ(d.m_unitObjectPath, QString("/org/freedesktop/systemd1/unit/") + tag);
    EXPECT_EQ(d.m_description, QString("desc ") + tag);
    EXPECT_EQ(d.m_mainPID, 4321u);
    EXPECT_TRUE(d.m_canReload);
    EXPECT_TRUE(d.m_canStart);
    EXPECT_FALSE(d.m_canStop);
}

class UT_SystemServiceEntryData : public ::testing::Test
{
public:
    UT_SystemServiceEntryData() : m_tester(nullptr) {}

    void SetUp() override
    {
        // Arrange：被测对象默认构造
        m_tester = new SystemServiceEntryData();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    SystemServiceEntryData *m_tester;
};

// 默认构造：13 个成员全部为 NSDMI 默认值（负面/边界基线）
TEST_F(UT_SystemServiceEntryData, default_ctor_all_fields_initialized)
{
    // Act：m_tester 已在 SetUp 中默认构造
    // Assert：QString 默认空、PID 0、能力位 false、QSharedData 引用未共享
    EXPECT_TRUE(m_tester->m_id.isEmpty());
    EXPECT_TRUE(m_tester->m_sname.isEmpty());
    EXPECT_TRUE(m_tester->m_loadState.isEmpty());
    EXPECT_TRUE(m_tester->m_activeState.isEmpty());
    EXPECT_TRUE(m_tester->m_subState.isEmpty());
    EXPECT_TRUE(m_tester->m_state.isEmpty());
    EXPECT_TRUE(m_tester->m_startupType.isEmpty());
    EXPECT_TRUE(m_tester->m_unitObjectPath.isEmpty());
    EXPECT_TRUE(m_tester->m_description.isEmpty());
    EXPECT_EQ(m_tester->m_mainPID, 0u);
    EXPECT_FALSE(m_tester->m_canReload);
    EXPECT_FALSE(m_tester->m_canStart);
    EXPECT_FALSE(m_tester->m_canStop);
    EXPECT_EQ(m_tester->ref.loadRelaxed(), 0);
}

// 拷贝构造：13 个成员逐一拷贝，且 QSharedData 拷贝基类引用计数归零 → 独立副本（改副本不影响源）
TEST_F(UT_SystemServiceEntryData, copy_ctor_deep_copies_all_fields)
{
    // Arrange
    SystemServiceEntryData src = makeFullData("src");

    // Act
    SystemServiceEntryData copy(src);

    // Assert：字段一致 + ref 独立
    expectAllFields(copy, "src");
    EXPECT_EQ(copy.ref.loadRelaxed(), 0);
    // Act：修改副本
    copy.m_id = "changed";
    copy.m_mainPID = 999u;
    // Assert：源不受影响
    EXPECT_EQ(src.m_id, QString("id@src"));
    EXPECT_EQ(src.m_mainPID, 4321u);
}

// 赋值运算符：目标字段被源覆盖
TEST_F(UT_SystemServiceEntryData, assign_overwrites_target_fields)
{
    // Arrange
    SystemServiceEntryData src = makeFullData("src");

    // Act
    *m_tester = src;

    // Assert
    expectAllFields(*m_tester, "src");
    // 源未被破坏
    EXPECT_EQ(src.m_id, QString("id@src"));
}

// 赋值运算符：self-assignment 短路分支（this == &rhs 直接返回，字段保持不变）
TEST_F(UT_SystemServiceEntryData, assign_self_keeps_fields)
{
    // Arrange
    *m_tester = makeFullData("self");

    // Act
    m_tester->operator=(*m_tester);

    // Assert：字段未被破坏
    expectAllFields(*m_tester, "self");
}

// 析构：栈对象离开作用域 / 堆对象 delete，均无副作用
TEST_F(UT_SystemServiceEntryData, dtor_no_side_effect)
{
    // Arrange & Act：栈对象
    {
        SystemServiceEntryData stackObj = makeFullData("stack");
        expectAllFields(stackObj, "stack");
    }
    // Act：堆对象
    {
        SystemServiceEntryData *heapObj = new SystemServiceEntryData();
        delete heapObj;
    }
    // Assert：执行到此处即无崩溃
    SUCCEED();
}

// TEST_P：三组同质输入分别经 赋值→拷贝构造 往返，字段完整保真
namespace {
struct DataRoundTripCase
{
    QString tag;
    quint32 pid;
};
}  // namespace

class UT_SystemServiceEntryData_RoundTrip : public ::testing::TestWithParam<DataRoundTripCase>
{
};

TEST_P(UT_SystemServiceEntryData_RoundTrip, assign_then_copy_preserves_fields)
{
    // Arrange
    const DataRoundTripCase &cs = GetParam();
    SystemServiceEntryData src = makeFullData(cs.tag);
    src.m_mainPID = cs.pid;

    // Act：先赋值，再拷贝构造
    SystemServiceEntryData assigned;
    assigned = src;
    SystemServiceEntryData copied(assigned);

    // Assert：三个对象字段一致
    expectAllFields(assigned, cs.tag);
    expectAllFields(copied, cs.tag);
    EXPECT_EQ(assigned.m_mainPID, cs.pid);
    EXPECT_EQ(copied.m_mainPID, cs.pid);
}

INSTANTIATE_TEST_SUITE_P(ValueSets, UT_SystemServiceEntryData_RoundTrip,
                         ::testing::Values(DataRoundTripCase{"alpha", 100u},
                                           DataRoundTripCase{"beta", 0u},
                                           DataRoundTripCase{"gamma", 4294967295u}));
