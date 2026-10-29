// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - MLogger（deepin-system-monitor-plugin/logger.cpp）
//
// 用例计数声明：
// | 函数          | 用例数 |
// |---------------|--------|
// | MLogger (构造) | 2      |
// | ~MLogger      | 1      |
// | rules         | 3（合并于各构造/追加用例断言） |
// | setRules      | 1      |
// | appendRules   | 4      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：rules()/setRules() 全覆盖，appendRules 经构造路径 + 直接调用覆盖；
// 2 等价类：DConfig 规则 空/非空、追加规则 与现有规则 全重复/部分重复/无重复；
// 3 边界值：空规则串（"" → split 产生单项空串）、重复行精确去重、子串误去重；
// 4 同质分组≥3 用 TEST_P：appendRules 合并语义 4 组（空/追加/去重/拼接）；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：DConfig::value 打桩返回受控规则串、QT_LOGGING_RULES 环境变量注入；
// 7 异常断言：无显式 throw，构造/析构 EXPECT_NO_THROW；
// 8 反向场景：DConfig 返回空串、追加串全部重复、m_rules 为空时首拼；
// 9 强异常安全：setRules 幂等（重复调用结果一致）；
// 10 打桩选型：DConfig::value（外部配置读取）用 stub_ext 打桩，规避真实 dconfig
//    依赖；环境变量经 setTestRulesEnv/restoreRulesEnv 助手注入与还原。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// MLogger::MLogger (构造)：
//   B1 QT_LOGGING_RULES 环境变量读取并暂存 → Logger_Ctor_EnvRulesOnly_KeepsEnvRules
//   B2 qunsetenv 撤销进程级规则 → 同上（构造后进程环境无 QT_LOGGING_RULES）
//   B3 DConfig::value("log_rules") 非空 → appendRules 合并 → Logger_Ctor_MergesDConfigRules
//   B4 DConfig::value 为空 → appendRules 空串早退 → Logger_Ctor_EnvRulesOnly_KeepsEnvRules
//   B5 setRules(m_rules) 应用过滤规则 → 各构造用例 rules() 断言
// ~MLogger：
//   B6 m_config->deleteLater() → Logger_Dtor_DeleteLaterConfig_NoCrash
// setRules：
//   B7 分号 → 换行替换 → Logger_SetRules_ReplacesSemicolons
//   B8 QLoggingCategory::setFilterRules 应用 → 同上（rules() 回读一致）
// appendRules：
//   B9 追加串分号→换行、按行拆分 → Logger_AppendRules_MergeSemantics（TEST_P）
//   B10 m_rules.contains(line) → 行去重 → TEST_P（去重组）+ Logger_AppendRules_DedupsExactLines
//   B11 全部行重复 → tmplist 空 → 早退 → Logger_AppendRules_AllDuplicate_NoChange
//   B12 m_rules 为空 → join 直拼 → TEST_P（空规则组）
//   B13 m_rules 非空 → "\n" + join 追加 → TEST_P（追加组）
//   B14 子串误去重（"format=1" contains "mat=1"）→ Logger_AppendRules_SubstringFalselyDedups
//
// 【备注】appendRules 去重采用 m_rules.contains(整行) 的子串匹配而非整行匹配，
// 存在部分规则串被误去重的隐患（Mode 5 报告记录）；用例 4 按现状断言该行为。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | MLogger/MLogger(析构) | high | env/dconfig/dtor | 4 | 4 |
// | setRules/appendRules | high | normalize/dedup/merge | 8 | 8 |

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "logger.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <QStringList>

#include <DConfig>
DCORE_USE_NAMESPACE

// LoggerTest：MLogger 测试（DConfig 打桩 + 环境变量注入）
class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        origEnv = qgetenv("QT_LOGGING_RULES");
        restoreRulesEnv();
    }
    void TearDown() override
    {
        restoreRulesEnv();
        stub.clear();
    }

    // 注入测试用环境变量（先撤销再注入，文件级 qputenv/qunsetenv 计数平衡）
    static void setTestRulesEnv(const QByteArray &rules)
    {
        qunsetenv("QT_LOGGING_RULES");
        qputenv("QT_LOGGING_RULES", rules);
    }
    // 还原现场：先撤销注入，再按原值恢复
    static void restoreRulesEnv()
    {
        qunsetenv("QT_LOGGING_RULES");
        if (!origEnv.isEmpty())
            qputenv("QT_LOGGING_RULES", origEnv);
    }

    // 打桩 DConfig::value("log_rules") 返回受控规则串
    void stubDConfigValue(const QString &rules)
    {
        stub.set_lamda(static_cast<QVariant (DConfig::*)(const QString &, const QVariant &) const>(
                           &DConfig::value),
                       [rules](const DConfig *, const QString &key, const QVariant &fallback) {
                           Q_UNUSED(key)
                           Q_UNUSED(fallback)
                           return QVariant(QByteArray(rules.toUtf8()));
                       });
    }

    static QByteArray origEnv;
    stub_ext::StubExt stub;
};
QByteArray LoggerTest::origEnv;

// ---- 构造（环境变量路径）----

TEST_F(LoggerTest, Logger_Ctor_EnvRulesOnly_KeepsEnvRules)
{
    // Arrange: 注入 QT_LOGGING_RULES，打桩 DConfig 返回空规则
    setTestRulesEnv("qt.test.env=true");
    stubDConfigValue(QString(""));
    // Act: 构造（B1/B2/B4）
    EXPECT_NO_THROW({
        MLogger logger;
        // Assert: env 规则保留为初始规则；DConfig 空串不追加；进程级环境变量被撤销（B2）
        EXPECT_EQ(logger.rules(), QString("qt.test.env=true"));
        EXPECT_FALSE(qEnvironmentVariableIsSet("QT_LOGGING_RULES"));
    });
}

TEST_F(LoggerTest, Logger_Ctor_MergesDConfigRules)
{
    // Arrange: env 规则 + DConfig 多条规则
    setTestRulesEnv("a=true");
    stubDConfigValue(QString("b=true;c=false"));
    // Act: 构造（B3/B5）
    MLogger logger;
    // Assert: DConfig 规则按行合并到现有规则之后（2 行，分号已归一化）
    EXPECT_EQ(logger.rules(), QString("a=true\nb=true\nc=false"));
    EXPECT_EQ(logger.rules().count('\n'), 2);
}

// ---- 构造（appendRules 合并语义，TEST_P）----

namespace {
struct AppendCase {
    const char *title;
    const char *envRules;     // 构造时的初始规则
    const char *appended;     // 追加规则
    const char *expected;     // 期望结果
};
std::ostream &operator<<(std::ostream &os, const AppendCase &c)
{
    return os << c.title;
}
}

class AppendParamTest : public LoggerTest,
                        public ::testing::WithParamInterface<AppendCase>
{
};

TEST_P(AppendParamTest, Logger_AppendRules_MergeSemantics)
{
    // Arrange: 以 env 规则构造，DConfig 空串
    const AppendCase param = GetParam();
    setTestRulesEnv(QByteArray(param.envRules));
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 追加规则（B9/B10/B12/B13）
    logger.appendRules(QString::fromLatin1(param.appended));
    // Assert: 合并结果符合语义（空规则直拼 / 非空规则换行追加 / 重复行去重）
    EXPECT_EQ(logger.rules(), QString::fromLatin1(param.expected));
    // 幂等复核：重复追加同样结果
    logger.appendRules(QString::fromLatin1(param.appended));
    EXPECT_EQ(logger.rules(), QString::fromLatin1(param.expected));
}

INSTANTIATE_TEST_SUITE_P(AppendMerge, AppendParamTest,
                         ::testing::Values(
                             // 注：base 经构造/appendRules 传入，分号均被归一化为 \n；
                             // 追加串内分号同样被归一化后再去重
                             AppendCase { "EmptyRules_JoinDirect", "", "a=1;b=2", "a=1\nb=2" },
                             AppendCase { "AppendDistinct", "a=1", "b=2;c=3", "a=1\nb=2\nc=3" },
                             AppendCase { "AppendWithDup", "a=1;b=2", "b=2;c=3", "a=1\nb=2\nc=3" },
                             AppendCase { "AppendAllDuplicate", "a=1", "a=1", "a=1" }));

TEST_F(LoggerTest, Logger_AppendRules_DedupsExactLines)
{
    // Arrange: 初始规则含 x=1
    setTestRulesEnv("x=1");
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 追加含重复行的规则串
    logger.appendRules("x=1;y=2");
    // Assert: 精确重复行被去重，仅保留新行，分号归一化为换行（B10）
    EXPECT_EQ(logger.rules(), QString("x=1\ny=2"));
    EXPECT_FALSE(logger.rules().contains(';'));
}

TEST_F(LoggerTest, Logger_AppendRules_AllDuplicate_NoChange)
{
    // Arrange: 初始规则与追加串完全重复
    setTestRulesEnv("only=1");
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 追加全重复规则
    logger.appendRules("only=1");
    // Assert: 早退分支，规则不变且仍为单行（B11）
    EXPECT_EQ(logger.rules(), QString("only=1"));
    EXPECT_EQ(logger.rules().count('\n'), 0);
}

TEST_F(LoggerTest, Logger_AppendRules_SubstringFalselyDedups)
{
    // Arrange: 初始规则 "format=1"，追加串是其子串
    setTestRulesEnv("format=1");
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 追加 "mat=1"（为 "format=1" 的子串）
    logger.appendRules("mat=1");
    // Assert: 子串匹配导致误去重——按现状断言（记录潜在缺陷）：
    // "mat=1" 行因 contains() 子串命中被丢弃，规则仍只有 "format=1" 单行
    EXPECT_EQ(logger.rules(), QString("format=1"));
    EXPECT_EQ(logger.rules().count('\n'), 0);
}

TEST_F(LoggerTest, Logger_AppendRules_EmptyRules_NoChange)
{
    // Arrange: 初始规则非空，追加空串
    setTestRulesEnv("a=1");
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 追加空串（"" → split 出单项空串 → contains("") 命中 → 全被移除 → 早退）
    logger.appendRules("");
    // Assert: 规则不变，仍为单行
    EXPECT_EQ(logger.rules(), QString("a=1"));
    EXPECT_EQ(logger.rules().count('\n'), 0);
}

// ---- setRules ----

TEST_F(LoggerTest, Logger_SetRules_ReplacesSemicolons)
{
    // Arrange: 任意构造
    setTestRulesEnv("");
    stubDConfigValue(QString(""));
    MLogger logger;
    // Act: 设置含分号的规则
    logger.setRules("a=1;b=2");
    // Assert: 分号替换为换行（B7/B8，幂等）
    EXPECT_EQ(logger.rules(), QString("a=1\nb=2"));
    logger.setRules("a=1;b=2");
    EXPECT_EQ(logger.rules(), QString("a=1\nb=2"));
}

// ---- 析构 ----

TEST_F(LoggerTest, Logger_Dtor_DeleteLaterConfig_NoCrash)
{
    // Arrange: 注入环境 + DConfig 打桩
    setTestRulesEnv("dtor=1");
    stubDConfigValue(QString(""));
    // Act: 作用域内构造并析构（B6：m_config->deleteLater()）
    EXPECT_NO_THROW({
        MLogger scoped;
        EXPECT_EQ(scoped.rules(), QString("dtor=1"));
    });
    // Assert: 析构无崩溃（配合 QCoreApplication 处理延迟删除队列）
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    EXPECT_TRUE(true);
}
