// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 MLogger 单元测试
// 被测类：deepin-system-monitor-daemon/src/logger.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method             | level | factors                 | min | actual |
// |--------------------|-------|-------------------------|-----|--------|
// | MLogger (ctor)     | low   | env/dconfig 两来源      | 1   | 3      |
// | ~MLogger (dtor)    | low   | -                       | 1   | 1      |
// | setRules           | low   | ';' 替换/空串           | 1   | 2      |
// | appendRules        | mid   | linear_scan_in_loop:2   | 2   | 6      |
// | rules              | low   | -                       | 1   | 1      |
//
// 分支清单（源自真实源码 worktree logger.cpp，self-check §2c）：
// - ctor: 读取 QT_LOGGING_RULES 环境变量并回收
//      → Ctor_GrabsEnvRules_ThenUnsets / Ctor_WithoutEnv_EmptyRules
//   DConfig::value("log_rules") → appendRules（dconfig 注入路径）
//      → Ctor_AppendsDconfigRules_FromDconfig
//   connect(DConfig::valueChanged → setRules) 监听
//      → Ctor_WithoutEnv_EmptyRules 中 m_config 非空断言
// - ~MLogger: m_config->deleteLater()
//      → Dtor_RepeatedCycles_DestroysCleanly
// - setRules: ';' → '\n' 替换 + QLoggingCategory::setFilterRules(m_rules)
//      → SetRules_SemicolonInput_TranslatedToNewlines / SetRules_EmptyString_ClearsRules
// - appendRules（去重循环）:
//   B1 逐条 contains 判重移除（含 i-- 回退）
//      → AppendRules_ExistingRule_Deduped
//   B2 tmplist 全部去重后为空 → 提前 return（m_rules 不变）
//      → AppendRules_AllDuplicates_NoChange / AppendRules_EmptyInput_NoChange
//   B3 m_rules 为空 → 直接 join 赋值（三元第一分支）
//      → AppendRules_EmptyStart_DirectAssign
//   B4 m_rules 非空 → 前置 '\n' 再 join（三元第二分支）
//      → AppendRules_NewRules_Concatenated
// - rules: 返回 m_rules   → rules() 断言贯穿上述用例
//
// 已知源码缺陷（仅标红不修复，详见 defects 报告）：
//   D1 appendRules 用 contains() 子串判重：追加 "core=true" 时若 m_rules 已含
//      "debug.core=true" 会被误判为重复规则（AppendRules_SubstringOverlap_WronglyDeduped 锁定）
//   D2 同批次内重复规则不互相去重（AppendRules_IntraBatchDup_Kept 锁定）
//
// stub 决策（dependency-tracer §3）：
// - DConfig::value → 受控返回（DConfig 元数据在本机已安装，stub 消除环境差异）
// - 环境变量 QT_LOGGING_RULES 每用例精确设置/回收（进程级副作用，串行安全）

#include "logger.h"

#include "stubext.h"

#include <DConfig>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <gtest/gtest.h>

DCORE_USE_NAMESPACE

typedef QVariant (*DConfigValuePtr)(DConfig *, const QString &, const QVariant &);

// DConfig::value 受控返回：仅 log_rules 键返回 g_dconfigLogRules，其余返回空串
// 环境变量包装（进程级副作用集中管理，串行安全）
static void setRulesEnv(const QByteArray &value) { qputenv("QT_LOGGING_RULES", value); }
static void unsetRulesEnv() { qunsetenv("QT_LOGGING_RULES"); }

static QString g_dconfigLogRules;

static QVariant stub_DConfig_value(DConfig *, const QString &key, const QVariant &)
{
    if (key == "log_rules")
        return QVariant(g_dconfigLogRules);
    return QVariant(QString(""));
}

// DConfig::value 补丁（成员函数走 stub.set + 静态函数，仓库既有模式）
static void stubDconfigRules(stub_ext::StubExt &stub, const QString &rules)
{
    g_dconfigLogRules = rules;
    stub.set((DConfigValuePtr)((QVariant (DConfig::*)(const QString &, const QVariant &) const)
                                   ADDR(DConfig, value)),
             stub_DConfig_value);
}

class UT_MLogger : public ::testing::Test
{
public:
    void SetUp() override
    {
        unsetRulesEnv();
    }

    void TearDown() override
    {
        stub.clear();
        // setRules 会进程级改动全局过滤规则，用例后复位
        QLoggingCategory::setFilterRules(QString());
        unsetRulesEnv();
    }

    stub_ext::StubExt stub;
};

// ctor: 从环境变量取得初始规则并回收环境变量
TEST_F(UT_MLogger, Ctor_GrabsEnvRules_ThenUnsets)
{
    // Arrange
    setRulesEnv("ut.rule=true");
    auto envGuard = qScopeGuard([] { unsetRulesEnv(); });
    stubDconfigRules(stub, QString(""));

    // Act
    MLogger logger;

    // Assert
    EXPECT_EQ(logger.rules(), QString("ut.rule=true"));
    EXPECT_FALSE(qEnvironmentVariableIsSet("QT_LOGGING_RULES"));
}

// ctor: 无环境变量且 DConfig 规则为空 → 规则为空，m_config 非空（监听连接已建立）
TEST_F(UT_MLogger, Ctor_WithoutEnv_EmptyRules)
{
    // Arrange: SetUp 已回收环境变量
    stubDconfigRules(stub, QString(""));

    // Act
    MLogger logger;

    // Assert
    EXPECT_TRUE(logger.rules().isEmpty());
    // 直接访问私有成员（仓库 -fno-access-control）：DConfig 创建成功
    EXPECT_NE(logger.m_config, nullptr);
}

// ctor: DConfig log_rules 注入路径（appendRules 生效）
TEST_F(UT_MLogger, Ctor_AppendsDconfigRules_FromDconfig)
{
    // Arrange
    stubDconfigRules(stub, QString("dc.rule=true"));

    // Act
    MLogger logger;

    // Assert: 空规则起点 + dconfig 注入 → 直接赋值（三元第一分支）
    EXPECT_EQ(logger.rules(), QString("dc.rule=true"));
    // DConfig 创建成功（监听连接已建立）
    EXPECT_NE(logger.m_config, nullptr);
}

// setRules: ';' 替换为 '\n' 并应用全局过滤规则
TEST_F(UT_MLogger, SetRules_SemicolonInput_TranslatedToNewlines)
{
    // Arrange
    MLogger logger;

    // Act
    logger.setRules("a=true;b=false");

    // Assert: 分号翻译为换行
    EXPECT_EQ(logger.rules(), QString("a=true\nb=false"));
    EXPECT_EQ(logger.rules().contains(';'), false);
}

// setRules: 空串 → 清空规则
TEST_F(UT_MLogger, SetRules_EmptyString_ClearsRules)
{
    // Arrange
    MLogger logger;
    logger.setRules("a=true");

    // Act
    logger.setRules(QString(""));

    // Assert: 清空生效
    EXPECT_EQ(logger.rules(), QString(""));
    EXPECT_EQ(logger.rules().contains("a"), false);
}

// appendRules B4: 既有规则非空 → 新规则以 '\n' 前置拼接（分号自动转换）
TEST_F(UT_MLogger, AppendRules_NewRules_Concatenated)
{
    // Arrange
    MLogger logger;
    logger.setRules("a");

    // Act
    logger.appendRules("b;c");

    // Assert: 换行拼接生效
    EXPECT_EQ(logger.rules(), QString("a\nb\nc"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

// appendRules B1: 逐条判重，已存在的规则被移除
TEST_F(UT_MLogger, AppendRules_ExistingRule_Deduped)
{
    // Arrange
    MLogger logger;
    logger.setRules("a\nb");

    // Act
    logger.appendRules("b\nc");

    // Assert: "b" 已存在被剔除，"c" 追加（总数不变）
    EXPECT_EQ(logger.rules(), QString("a\nb\nc"));
    EXPECT_NE(logger.rules().indexOf("c"), -1);
}

// appendRules B2: 全部重复 → 提前 return，规则不变
TEST_F(UT_MLogger, AppendRules_AllDuplicates_NoChange)
{
    // Arrange
    MLogger logger;
    logger.setRules("x");

    // Act
    logger.appendRules("x;x");

    // Assert: 两条均被判重 → tmplist 为空 → 不变
    EXPECT_EQ(logger.rules(), QString("x"));
    EXPECT_EQ(logger.rules().count('\n'), 0);
}

// appendRules B2 边界: 空输入 → split 产出 [""]（任何串都含空串）→ 剔除后为空 → 不变
TEST_F(UT_MLogger, AppendRules_EmptyInput_NoChange)
{
    // Arrange
    MLogger logger;
    logger.setRules("x");

    // Act
    logger.appendRules(QString(""));

    // Assert: 空输入不破坏既有规则
    EXPECT_EQ(logger.rules(), QString("x"));
    EXPECT_NE(logger.rules().indexOf("x"), -1);
}

// appendRules B3: 规则为空起点 → 直接赋值
TEST_F(UT_MLogger, AppendRules_EmptyStart_DirectAssign)
{
    // Arrange: 无环境变量、DConfig 规则为空 → m_rules 为空
    stubDconfigRules(stub, QString(""));
    MLogger logger;

    // Act
    logger.appendRules("p;q");

    // Assert: 空起点直接赋值
    EXPECT_EQ(logger.rules(), QString("p\nq"));
    EXPECT_EQ(logger.rules().count('\n'), 1);
}

// [缺陷锁定 D1] appendRules 子串判重误伤：新规则 "core=true" 并非重复，
// 但 m_rules 中 "debug.core=true" 包含其子串 → 被误剔（仅标红不修复）
TEST_F(UT_MLogger, AppendRules_SubstringOverlap_WronglyDeduped)
{
    // Arrange
    MLogger logger;
    logger.setRules("debug.core=true");

    // Act
    logger.appendRules("core=true");

    // Assert: 期望应为 "debug.core=true\ncore=true"，实际因子串判重保持不变（缺陷行为）
    EXPECT_EQ(logger.rules(), QString("debug.core=true"));
    EXPECT_EQ(logger.rules().count('\n'), 0);
}

// [缺陷锁定 D2] 同批次内重复规则不互相去重（仅标红不修复）
TEST_F(UT_MLogger, AppendRules_IntraBatchDup_Kept)
{
    // Arrange
    MLogger logger;
    logger.setRules("a");

    // Act
    logger.appendRules("b;b");

    // Assert: 期望 "a\nb"，实际 "a\nb\nb"（缺陷行为）
    EXPECT_EQ(logger.rules(), QString("a\nb\nb"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

// dtor: m_config->deleteLater，作用域销毁不崩溃且新实例继续可用
TEST_F(UT_MLogger, Dtor_RepeatedCycles_DestroysCleanly)
{
    // Arrange
    stubDconfigRules(stub, QString(""));
    MLogger *inner = new MLogger();
    inner->setRules("inner.rule=true");
    EXPECT_EQ(inner->rules(), QString("inner.rule=true"));

    // Act: 作用域销毁（deleteLater 事件由 main 末尾冲刷）
    delete inner;

    // Assert: 销毁后新实例功能正常
    MLogger fresh;
    fresh.setRules("fresh.rule=true");
    EXPECT_EQ(fresh.rules(), QString("fresh.rule=true"));
}
