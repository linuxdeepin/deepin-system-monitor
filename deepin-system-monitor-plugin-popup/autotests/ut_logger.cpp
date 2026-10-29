// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// MLogger(deepin-system-monitor-plugin-popup/logger.cpp)单元测试
//
// 外部依赖打桩:DConfig 配置后端(create/value)——不读取真实 DConfig 元数据;
// 值通过 knob(g_dconfigLogRules)注入,TearDown 由 Stub RAII 自动还原。
// 注意:MLogger 构造/设置规则会调用 QLoggingCategory::setFilterRules(进程级
// 副作用),用例规则仅使用 ut.* 前缀的具体类别,不影响其他用例的日志行为。

#include <gtest/gtest.h>

#include <DConfig>

#include <QLoggingCategory>
#include <QString>
#include <QVariant>
#include <cstdlib>

#include "logger.h"
#include "../../tests/stub.h"

DCORE_USE_NAMESPACE

namespace {

// ---- knob:注入给 DConfig::value("log_rules") 的返回值 ----
QString g_dconfigLogRules;
// 最近一次 create 桩构造的 DConfig 实例(用于触发 valueChanged 信号)
DConfig *g_lastDConfig = nullptr;

DConfig *stub_DConfig_create(const QString &appId, const QString &name,
                             const QString &subpath, QObject *parent)
{
    Q_UNUSED(appId)
    Q_UNUSED(name)
    // 使用不存在的配置名,内部回落为隐藏配置(仅告警,不崩溃);
    // value() 已被桩接管,不触碰真实元数据
    g_lastDConfig = new DConfig(QStringLiteral("__ut_stub_nonexistent__"), subpath, parent);
    return g_lastDConfig;
}

QVariant stub_DConfig_value(const DConfig *self, const QString &key, const QVariant &fallback)
{
    Q_UNUSED(self)
    if (key == QStringLiteral("log_rules"))
        return QVariant(g_dconfigLogRules);
    return fallback;
}

}  // namespace

class UT_Logger : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_dconfigLogRules.clear();
        g_lastDConfig = nullptr;
        qunsetenv("QT_LOGGING_RULES");
        // 用例级局部桩(RAII:用例结束自动还原)
        m_stub.set((DConfig *(*)(const QString &, const QString &, const QString &, QObject *))&DConfig::create,
                   stub_DConfig_create);
        m_stub.set((QVariant (DConfig::*)(const QString &, const QVariant &) const)ADDR(DConfig, value),
                   stub_DConfig_value);
    }

    void TearDown() override
    {
        qunsetenv("QT_LOGGING_RULES");
    }

    Stub m_stub;
};

// 用例名称: construction_emptyDconfig_readsEnvRules
// 测试类型: 正常场景
// 覆盖函数: MLogger::MLogger(环境变量读取分支 + appendRules 空串提前返回分支)
// 操作步骤: 设置 QT_LOGGING_RULES 环境变量,dconfig log_rules 为空,构造 MLogger
// 预期结果: rules() 等于环境变量规则(dconfig 空串不追加)
TEST_F(UT_Logger, construction_emptyDconfig_readsEnvRules)
{
    // Arrange: 环境变量规则,dconfig 为空
    qputenv("QT_LOGGING_RULES", "ut.env.rule=true");
    // Act: 构造 MLogger(读取环境变量并 qunsetenv)
    MLogger logger;
    // Assert: 仅含环境变量规则
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.env.rule=true"));
}

// 用例名称: construction_dconfigRules_appendedAfterEnvRules
// 测试类型: 正常场景
// 覆盖函数: MLogger::MLogger → appendRules(分号替换换行 + 追加分支) → setRules
// 操作步骤: 环境变量规则 + dconfig 分号分隔规则,构造 MLogger
// 预期结果: 分号被替换为换行,追加到环境变量规则之后
TEST_F(UT_Logger, construction_dconfigRules_appendedAfterEnvRules)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "ut.env.rule=true");
    g_dconfigLogRules = QStringLiteral("ut.dc.a=true;ut.dc.b=false");
    // Act
    MLogger logger;
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.env.rule=true\nut.dc.a=true\nut.dc.b=false"));
}

// 用例名称: construction_noEnv_dconfigRulesBecomesRules
// 测试类型: 边界场景
// 覆盖函数: MLogger::MLogger → appendRules(m_rules 为空直接赋值分支)
// 操作步骤: 无环境变量规则,dconfig 提供规则,构造 MLogger
// 预期结果: rules() 直接等于 dconfig 规则
TEST_F(UT_Logger, construction_noEnv_dconfigRulesBecomesRules)
{
    // Arrange
    g_dconfigLogRules = QStringLiteral("ut.dc.a=true");
    // Act
    MLogger logger;
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.dc.a=true"));
}

// 用例名称: construction_dconfigDuplicateRule_deduplicated
// 测试类型: 边界场景
// 覆盖函数: MLogger::MLogger → appendRules(已包含条目去重分支)
// 操作步骤: dconfig 规则中一条与环境变量规则重复,一条新增
// 预期结果: 重复条目被去重,仅新增条目追加
TEST_F(UT_Logger, construction_dconfigDuplicateRule_deduplicated)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "ut.env.rule=true");
    g_dconfigLogRules = QStringLiteral("ut.env.rule=true;ut.dc.a=true");
    // Act
    MLogger logger;
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.env.rule=true\nut.dc.a=true"));
}

// 用例名称: construction_dconfigAllDuplicate_appendSkipped
// 测试类型: 边界场景
// 覆盖函数: MLogger::MLogger → appendRules(去重后列表为空提前返回分支)
// 操作步骤: dconfig 规则与环境变量规则完全重复
// 预期结果: 规则保持不变,不产生多余换行
TEST_F(UT_Logger, construction_dconfigAllDuplicate_appendSkipped)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "ut.env.rule=true");
    g_dconfigLogRules = QStringLiteral("ut.env.rule=true");
    // Act
    MLogger logger;
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.env.rule=true"));
}

// 用例名称: setRules_semicolonReplacedByNewline
// 测试类型: 正常场景
// 覆盖函数: MLogger::setRules(分号替换 + 整体替换语义)
// 操作步骤: 先设置含分号的规则,再设置单条规则
// 预期结果: 分号替换为换行;再次设置整体替换而非追加
TEST_F(UT_Logger, setRules_semicolonReplacedByNewline)
{
    // Arrange
    MLogger logger;
    // Act: 设置含分号的规则
    logger.setRules(QStringLiteral("ut.a=true;ut.b=false"));
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.a=true\nut.b=false"));
    // Act: 再次设置,验证整体替换
    logger.setRules(QStringLiteral("ut.c=true"));
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.c=true"));
}

// 用例名称: valueChanged_logRulesKey_updatesRules
// 测试类型: 正常场景
// 覆盖函数: MLogger::MLogger 中 DConfig::valueChanged lambda(log_rules 分支)
// 操作步骤: 构造后注入新规则值,通过公开信号方法触发 valueChanged("log_rules")
// 预期结果: lambda 拉取新值并调用 setRules 整体替换
TEST_F(UT_Logger, valueChanged_logRulesKey_updatesRules)
{
    // Arrange
    MLogger logger;
    ASSERT_NE(g_lastDConfig, nullptr);
    g_dconfigLogRules = QStringLiteral("ut.new.rule=true");
    // Act: 触发 dconfig 变更通知(lambda 同线程直连,同步执行)
    g_lastDConfig->valueChanged(QStringLiteral("log_rules"));
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.new.rule=true"));
}

// 用例名称: valueChanged_otherKey_rulesUnchanged
// 测试类型: 边界场景
// 覆盖函数: MLogger::MLogger 中 DConfig::valueChanged lambda(非 log_rules 分支)
// 操作步骤: 触发 valueChanged("other_key")
// 预期结果: 规则保持不变
TEST_F(UT_Logger, valueChanged_otherKey_rulesUnchanged)
{
    // Arrange
    qputenv("QT_LOGGING_RULES", "ut.env.rule=true");
    MLogger logger;
    ASSERT_NE(g_lastDConfig, nullptr);
    // Act: 非 log_rules 键不触发更新
    g_lastDConfig->valueChanged(QStringLiteral("other_key"));
    // Assert
    EXPECT_EQ(logger.rules(), QStringLiteral("ut.env.rule=true"));
}

// 用例名称: destructor_noCrash
// 测试类型: 边界场景
// 覆盖函数: MLogger::~MLogger(m_config deleteLater)
// 操作步骤: 作用域内构造并析构 MLogger
// 预期结果: 析构无崩溃(deleteLater 由事件循环接管)
TEST_F(UT_Logger, destructor_noCrash)
{
    {
        MLogger logger;
        Q_UNUSED(logger)
    }
    SUCCEED();
}
