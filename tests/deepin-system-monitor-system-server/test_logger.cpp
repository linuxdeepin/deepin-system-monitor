// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 MLogger（system-server 进程）单元测试
// 被测类：deepin-system-monitor-system-server/src/logger.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method              | level | factors                          | min | actual |
// |---------------------|-------|----------------------------------|-----|--------|
// | MLogger (ctor)      | mid   | env有值/env空                    | 2   | 2      |
// | ~MLogger            | low   | 正常析构                         | 1   | 1      |
// | setRules            | mid   | 分号替换/全局生效                | 2   | 2      |
// | appendRules         | mid   | 空表直拼/去重/全重复/空输入      | 4   | 4      |
// | valueChanged 槽     | mid   | log_rules键生效/其他键忽略       | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: B1 读取 QT_LOGGING_RULES → qunsetenv; B2 m_rules=env 值;
//         B3 DConfig::create + value("log_rules") → appendRules; B4 setRules(m_rules);
//         B5 connect DConfig::valueChanged → key=="log_rules" 时 setRules(m_config->value)
// - setRules: B1 ';'→'\n' 替换; B2 m_rules 赋值 + QLoggingCategory::setFilterRules
// - appendRules: B1 tmpRules ';'→'\n' 拆分; B2 m_rules.contains(entry) 子串剔除
//         （仅与既有 m_rules 比对，批次内部重复项保留）; B3 全部剔除 → 提前 return;
//         B4 原规则为空 → 直接拼接，否则 '\n' 连接
// - valueChanged 槽: B1 key=="log_rules" → setRules(value(key)); B2 其他 key → 忽略

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QVariant>

#include <DConfig>

#include "stubext.h"
#include "logger.h"

DCORE_USE_NAMESPACE

// ═══════════════════ DConfig 打桩 ═══════════════════

namespace {

// DConfig::value 打桩返回值（构造与 valueChanged 槽共用）
QVariant g_dconfigValue;
// 记录 DConfig::value 调用次数
int g_dconfigValueCalls = 0;

using DCValueFn = QVariant (DConfig::*)(const QString &, const QVariant &) const;
// 打桩函数签名兼容 DConfig::value 成员函数指针
QVariant fakeDConfigValue(DConfig *, const QString &, const QVariant &)
{
    ++g_dconfigValueCalls;
    return g_dconfigValue;
}

void installDConfigStubs(stub_ext::StubExt &stub)
{
    g_dconfigValueCalls = 0;
    g_dconfigValue = QVariant();
    stub.set_lamda((DCValueFn)&DConfig::value,
                   [](DConfig *self, const QString &key, const QVariant &def) -> QVariant {
                       return fakeDConfigValue(self, key, def);
                   });
}

}  // namespace

// ═══════════════════ 测试夹具 ═══════════════════

class MLoggerSysServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        g_dconfigValueCalls = 0;
        g_dconfigValue = QVariant();
        installDConfigStubs(stub);
    }

    void TearDown() override
    {
        // 还原全局日志过滤规则与打桩
        QLoggingCategory::setFilterRules(QString());
        stub.clear();
    }

    stub_ext::StubExt stub;
};

// ═══════════════════ 构造 / 析构 ═══════════════════

TEST_F(MLoggerSysServerTest, Ctor_EnvRulesSet_CapturedIntoState)
{
    // Arrange: 预置环境变量 QT_LOGGING_RULES（DConfig 桩返回空 → 不追加）
    qputenv("QT_LOGGING_RULES", "env.category.debug=true");
    // Act
    MLogger logger;
    // Assert: env 值进入 m_rules，随后环境变量被清除
    EXPECT_EQ(logger.rules(), QStringLiteral("env.category.debug=true"));
    EXPECT_TRUE(qgetenv("QT_LOGGING_RULES").isEmpty());
}

TEST_F(MLoggerSysServerTest, Ctor_EnvAbsent_EmptyRulesFromDConfigDefault)
{
    // Arrange: 显式清除 env，DConfig 桩返回空值
    qunsetenv("QT_LOGGING_RULES");
    // Act
    MLogger logger;
    // Assert: 空规则；DConfig::value 在构造中被调用
    EXPECT_EQ(logger.rules(), QString());
    EXPECT_EQ(g_dconfigValueCalls, 1);
}

TEST_F(MLoggerSysServerTest, Dtor_ExplicitDelete_DoesNotCrash)
{
    // Arrange: 堆上构造 logger（内部挂 DConfig 子对象）
    MLogger *logger = new MLogger();
    ASSERT_NE(logger, nullptr);
    ASSERT_NE(logger->m_config, nullptr);
    // Act: 显式销毁
    delete logger;
    // Assert: 析构未崩溃，进程事件循环对象仍有效
    EXPECT_NE(QCoreApplication::instance(), nullptr);
    EXPECT_TRUE(qgetenv("QT_LOGGING_RULES").isEmpty());
}

// ═══════════════════ setRules ═══════════════════

TEST_F(MLoggerSysServerTest, SetRules_SemicolonInput_ReplacedWithNewlines)
{
    // Arrange
    MLogger logger;
    // Act
    logger.setRules("a;b;c");
    // Assert: ';' → '\n'
    EXPECT_EQ(logger.rules(), QStringLiteral("a\nb\nc"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

TEST_F(MLoggerSysServerTest, SetRules_NewRules_AppliedGlobally)
{
    // Arrange
    MLogger logger;
    // Act
    logger.setRules("test.dsmcat.debug=false");
    // Assert: 全局过滤器被同步设置
    EXPECT_EQ(logger.rules(), QStringLiteral("test.dsmcat.debug=false"));
    EXPECT_FALSE(QLoggingCategory("test.dsmcat").isDebugEnabled());
}

// ═══════════════════ appendRules ═══════════════════

TEST_F(MLoggerSysServerTest, AppendRules_ToEmptyRules_JoinsDirectly)
{
    // Arrange: 空规则表
    MLogger logger;
    ASSERT_TRUE(logger.rules().isEmpty());
    // Act
    logger.appendRules("r1;r2;r3");
    // Assert: 原规则为空 → 直接拼接
    EXPECT_EQ(logger.rules(), QStringLiteral("r1\nr2\nr3"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

TEST_F(MLoggerSysServerTest, AppendRules_ExistingDuplicates_Removed)
{
    // Arrange: 已有 r1、r2
    MLogger logger;
    logger.setRules("r1\nr2");
    // Act: 追加 r2、r3（r2 已存在被剔除，r3 保留）
    logger.appendRules("r2;r3");
    // Assert: 去重仅与既有 m_rules 比对
    EXPECT_EQ(logger.rules(), QStringLiteral("r1\nr2\nr3"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

TEST_F(MLoggerSysServerTest, AppendRules_AllDuplicates_NoChange)
{
    // Arrange: 已有 r1、r2
    MLogger logger;
    logger.setRules("r1\nr2");
    const QString before = logger.rules();
    // Act: 追加内容全部已存在
    logger.appendRules("r1;r2");
    // Assert: 全部重复 → 提前 return，规则不变
    EXPECT_EQ(logger.rules(), before);
    EXPECT_EQ(before, QStringLiteral("r1\nr2"));
}

TEST_F(MLoggerSysServerTest, AppendRules_EmptyInput_NoChange)
{
    // Arrange: 已有规则
    MLogger logger;
    logger.setRules("r1");
    const QString before = logger.rules();
    // Act: 空输入（"" contains 于任何规则 → 全部剔除 → 提前 return）
    logger.appendRules("");
    // Assert: 规则不变
    EXPECT_EQ(logger.rules(), before);
    EXPECT_EQ(before, QStringLiteral("r1"));
}

// ═══════════════════ DConfig::valueChanged 槽 ═══════════════════

TEST_F(MLoggerSysServerTest, DConfigValueChanged_LogRules_UpdatesRules)
{
    // Arrange: 构造（桩值空 → 规则空），随后变更桩值
    MLogger logger;
    ASSERT_TRUE(logger.rules().isEmpty());
    g_dconfigValue = QVariant("dc.newrule=true");
    // Act: DConfig 发出 valueChanged("log_rules")，槽经 value() 读取新值
    emit logger.m_config->valueChanged(QStringLiteral("log_rules"));
    // Assert: 槽按 key 过滤后应用新规则
    EXPECT_EQ(logger.rules(), QStringLiteral("dc.newrule=true"));
    EXPECT_EQ(g_dconfigValueCalls, 2);   // ctor 一次 + 槽一次
}

TEST_F(MLoggerSysServerTest, DConfigValueChanged_OtherKey_Ignored)
{
    // Arrange: 构造已有规则
    MLogger logger;
    logger.setRules("keep.me=true");
    g_dconfigValue = QVariant("should.not.apply");
    // Act: 非 log_rules 键的变更
    emit logger.m_config->valueChanged(QStringLiteral("other_key"));
    // Assert: 规则保持不变（value 未被再次调用）
    EXPECT_EQ(logger.rules(), QStringLiteral("keep.me=true"));
    EXPECT_EQ(g_dconfigValueCalls, 1);   // 仅 ctor 一次
}
