// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "logger.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QLoggingCategory>

/***************************************STUB begin*********************************************/
// 无外部依赖打桩需求：MLogger 仅依赖 QLoggingCategory 与 DConfig（读取本机真实 dconfig 元数据，
// 默认 log_rules 为空，测试以受控规则驱动）
/***************************************STUB end***********************************************/

// 分支清单（源自 logger.cpp）:
// 1. ctor: 读取 QT_LOGGING_RULES 环境变量 -> qunsetenv -> m_rules=envRules
//          -> DConfig::create -> appendRules(dconfig log_rules) -> setRules(m_rules)
//          -> connect(DConfig::valueChanged, key=="log_rules" 时刷新规则)
// 2. setRules: m_rules = rules.replace(";", "\n") -> QLoggingCategory::setFilterRules
// 3. appendRules: 规则逐条去重（contains 判定已存在则 removeAt）->
//    tmplist 为空直接 return -> 否则空 m_rules 直接赋值 / 非空则 "\n" 追加
// 4. rules(): 内联返回 m_rules

class MLoggerTest : public ::testing::Test
{
public:
    MLoggerTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        // Arrange: 清理环境变量，确保 DConfig 默认规则（log_rules 默认为空）
        qunsetenv("QT_LOGGING_RULES");

        // Act
        m_tester = new MLogger();
        // Assert: 构造不崩溃
        ASSERT_NE(m_tester, nullptr);
    }

    virtual void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    MLogger *m_tester;
};

// ───────────────────────── ctor ─────────────────────────
TEST_F(MLoggerTest, Construction_WithoutEnvRules_ConsumesEnvironmentVariable)
{
    // Arrange: SetUp 已保证 QT_LOGGING_RULES 未设置
    // Act: 构造 MLogger（m_config 读取真实 DConfig 默认值）
    const bool envStillSet = qEnvironmentVariableIsSet("QT_LOGGING_RULES");

    // Assert: 环境变量被 qunsetenv 消费，构造后不再存在
    EXPECT_FALSE(envStillSet);

    // Act: 再构造一个实例
    MLogger secondInstance;

    // Assert: 两实例均从同一 DConfig 读取，规则一致（不依赖具体机器默认值）
    EXPECT_EQ(m_tester->rules(), secondInstance.rules());
}

TEST_F(MLoggerTest, Construction_WithEnvRules_RulesLoadedFromEnvironment)
{
    // Arrange: 预设 QT_LOGGING_RULES 环境变量
    qputenv("QT_LOGGING_RULES", "qt.widgets.gestures=false");

    // Act: 构造新实例读取环境变量
    MLogger loggerWithEnv;

    // Assert: 规则取自环境变量且环境变量被消费
    EXPECT_NE(loggerWithEnv.rules(), QString(""));
    EXPECT_TRUE(loggerWithEnv.rules().contains("qt.widgets.gestures=false"));
    EXPECT_FALSE(qEnvironmentVariableIsSet("QT_LOGGING_RULES"));
}

// ───────────────────────── setRules ─────────────────────────
TEST_F(MLoggerTest, SetRules_WithSemicolonSeparated_ConvertsToNewlineSeparated)
{
    // Arrange
    const QString semicolonRules = "qt.test.one=false;qt.test.two=true";

    // Act
    m_tester->setRules(semicolonRules);

    // Assert: 分号被替换为换行
    EXPECT_EQ(m_tester->rules(), QString("qt.test.one=false\nqt.test.two=true"));
    EXPECT_FALSE(m_tester->rules().contains(";"));
}

TEST_F(MLoggerTest, SetRules_WithEmptyString_ClearsAllRules)
{
    // Arrange: 先写入规则，再以空串清空
    m_tester->setRules("qt.test.one=false");

    // Act
    m_tester->setRules(QString(""));

    // Assert
    EXPECT_EQ(m_tester->rules(), QString(""));
    EXPECT_TRUE(m_tester->rules().isEmpty());
}

// ───────────────────────── appendRules ─────────────────────────
TEST_F(MLoggerTest, AppendRules_WithNewRules_AppendedWithNewlineSeparator)
{
    // Arrange: 记录构造时 DConfig 基线规则（各机器可能非空）
    const QString baseline = m_tester->rules();

    // Act
    m_tester->appendRules("qt.test.first=false");

    // Assert: 基线为空时直接赋值，非空时以换行追加
    const QString expectedFirst = baseline.isEmpty()
                                      ? QString("qt.test.first=false")
                                      : baseline + "\nqt.test.first=false";
    EXPECT_EQ(m_tester->rules(), expectedFirst);

    // Act: 再追加一条
    m_tester->appendRules("qt.test.second=true");

    // Assert: 非空规则时以换行追加
    EXPECT_EQ(m_tester->rules(), expectedFirst + "\nqt.test.second=true");
}

TEST_F(MLoggerTest, AppendRules_WithExistingRules_DuplicatesAreRemoved)
{
    // Arrange
    m_tester->appendRules("qt.test.dup=false");
    const QString afterDup = m_tester->rules();

    // Act: 追加包含重复项的规则列表（分号分隔自动拆分去重）
    m_tester->appendRules("qt.test.dup=false;qt.test.fresh=true");

    // Assert: 重复项被剔除，仅新增 fresh 规则
    EXPECT_EQ(m_tester->rules(), afterDup + "\nqt.test.fresh=true");
    EXPECT_EQ(m_tester->rules().count("qt.test.dup=false"), 1);
}

TEST_F(MLoggerTest, AppendRules_WhenAllRulesExist_KeepsRulesUnchanged)
{
    // Arrange
    m_tester->appendRules("qt.test.same=false");
    const QString beforeAppend = m_tester->rules();

    // Act: 追加完全相同的规则 -> tmplist 全部被剔除后为空 -> 提前 return
    m_tester->appendRules("qt.test.same=false");

    // Assert
    EXPECT_EQ(m_tester->rules(), beforeAppend);
    EXPECT_EQ(m_tester->rules().count("qt.test.same=false"), 1);
}

TEST_F(MLoggerTest, AppendRules_WithEmptyInput_NoRulesAdded)
{
    // Arrange
    m_tester->appendRules("qt.test.base=false");
    const QString beforeAppend = m_tester->rules();

    // Act: 空输入 split 后为单个空串（不命中 contains）-> 无实际新增
    m_tester->appendRules(QString(""));

    // Assert: 规则集合保持稳定
    EXPECT_EQ(m_tester->rules(), beforeAppend);
    EXPECT_TRUE(m_tester->rules().contains("qt.test.base=false"));
}

// ───────────────────────── rules getter ─────────────────────────
TEST_F(MLoggerTest, RulesGetter_AfterSetRules_ReturnsLatestRules)
{
    // Arrange
    m_tester->setRules("qt.test.getter=false");

    // Act
    const QString current = m_tester->rules();

    // Assert: 内联 getter 返回内部 m_rules 最新值
    EXPECT_EQ(current, QString("qt.test.getter=false"));
    EXPECT_TRUE(current.contains("qt.test.getter=false"));
}
