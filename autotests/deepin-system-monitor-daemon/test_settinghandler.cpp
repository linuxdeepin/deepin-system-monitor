// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 SettingHandler 单元测试
// 被测类：deepin-system-monitor-daemon/src/settinghandler.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                | level | factors                    | min | actual |
// |-----------------------|-------|----------------------------|-----|--------|
// | SettingHandler (ctor) | low   | -                          | 1   | 1      |
// | ~SettingHandler (dtor)| low   | -                          | 1   | 1      |
// | isCompelted           | mid   | 加载完成/双实例            | 2   | 2      |
// | getOptionValue        | low   | 已知键/未知键              | 1   | 2      |
// | changedOptionValue    | mid   | 合法写/未知键              | 2   | 2      |
// | isVaildValue          | mid   | 三键范围+边界/未知键/类型  | 2   | 7      |
// | getValueRange         | mid   | 三键/未知键                | 2   | 2      |
// | itemKeys              | low   | -                          | 1   | 1      |
//
// 分支清单（源自真实源码 worktree settinghandler.cpp，self-check §2c）：
// - ctor: ConfigLocation+orgName 拼 protection.conf → QSettingBackend + fromJsonFile
//   （qrc :/resources/settings.json 内嵌，mSettings 恒非空）
//     → Ctor_InitializesSettings_CompletedTrue
// - isCompelted: mBackend && mSettings 均非空 → true（加载成功路径）
//     → IsCompelted_AfterConstruction_ReturnsTrue / IsCompelted_AcrossInstances_ReturnsTrue
// - getOptionValue: B1 键存在 → mSettings->getOption(key)（默认 90/90/10）
//     → GetOptionValue_Defaults_ReturnJsonValues
//   B2 键不存在 → 告警 + 返回 QVariant()
//     → GetOptionValue_UnknownKey_ReturnsInvalidVariant
// - changedOptionValue: B1 键存在 → option->setValue + sync（写盘）+ true
//     → ChangedOptionValue_KnownKey_PersistsAndReturnsTrue（含重开实例持久化验证）
//   B2 键不存在 → 告警 + false
//     → ChangedOptionValue_UnknownKey_ReturnsFalse
// - isVaildValue: B1 键在 mValueRange 且类型 ∈ [Bool..Double] → 范围判断
//     → IsVaildValue_CpuRange_BoundariesDetected / IsVaildValue_MemRange_BoundariesDetected /
//       IsVaildValue_IntervalRange_BoundariesDetected
//   B2 键不在范围表 → "no need to check" → true
//     → IsVaildValue_UnknownKey_ReturnsTrue
//   B3 类型超出 [Bool..Double]（如 QString）→ true
//     → IsVaildValue_NonNumericType_ReturnsTrue
// - getValueRange: B1 键存在 → 对应范围 → GetValueRange_KnownKeys_ReturnsRanges
//   B2 键不存在 → (0,0) → GetValueRange_UnknownKey_ReturnsZeroPair
// - itemKeys: 返回 mSettings->keys() → ItemKeys_AllOptionKeys_Contained
//
// 已知源码缺陷（仅标红不修复，详见 defects 报告）：
//   D3 ctor 中 fromJsonFile 之后立即 mSettings->option(...) 解引用，其后的
//      mSettings 空指针检查为死代码（资源缺失时先崩溃）；itemKeys/getOptionValue
//      else 告警分支同样在 mSettings 为空时解引用（不可达，仅记录）
//
// stub 决策（dependency-tracer §3）：
// - QStandardPaths::writableLocation → 指向用例专属 QTemporaryDir（配置文件读写隔离）
// - qrc 资源内嵌于测试目标，DSettings::fromJsonFile 走真实路径（不 stub）
// - 本项目代码不 stub；不触网、不启子进程

#include "settinghandler.h"

#include "stubext.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <gtest/gtest.h>

// 当前用例的配置根目录（SetUp 中指向 QTemporaryDir）
static QString g_configRoot;

class UT_SettingHandler : public ::testing::Test
{
public:
    void SetUp() override
    {
        // Arrange: 每个用例独立的临时配置目录
        ASSERT_TRUE(m_tempDir.isValid());
        g_configRoot = m_tempDir.path();

        stub.set_lamda(static_cast<QString (*)(QStandardPaths::StandardLocation)>(
                           &QStandardPaths::writableLocation),
                       [](QStandardPaths::StandardLocation) -> QString {
                           return g_configRoot;
                       });
    }

    void TearDown() override
    {
        stub.clear();
        g_configRoot.clear();
    }

    stub_ext::StubExt stub;
    QTemporaryDir m_tempDir;
};

// ctor: 设置后端与 DSettings 初始化成功
TEST_F(UT_SettingHandler, Ctor_InitializesSettings_CompletedTrue)
{
    // Arrange: 全新临时配置目录（SetUp 内无既有配置）
    ASSERT_TRUE(m_tempDir.isValid());

    // Act: 构造即完成初始化
    SettingHandler handler;

    // Assert: 加载完成即生效
    EXPECT_TRUE(handler.isCompelted());
    // 默认值来自 :/resources/settings.json
    EXPECT_EQ(handler.getOptionValue(AlarmCpuUsageOptionName).toInt(), 90);
    EXPECT_EQ(handler.getOptionValue(AlarmIntervalOptionName).toInt(), 10);
}

// isCompelted: 构造成功后为 true
TEST_F(UT_SettingHandler, IsCompelted_AfterConstruction_ReturnsTrue)
{
    // Arrange: 全新临时配置目录
    ASSERT_TRUE(m_tempDir.isValid());
    SettingHandler handler;

    // Act: 读取完成标志
    bool completed = handler.isCompelted();

    // Assert: 构造完成 + 键表可用
    EXPECT_EQ(completed, true);
    EXPECT_EQ(handler.itemKeys().size() > 0, true);
}

// isCompelted: 两个独立实例（不同目录）均为 true
TEST_F(UT_SettingHandler, IsCompelted_AcrossInstances_ReturnsTrue)
{
    // Arrange: 同一临时目录下先后构造两个独立实例
    ASSERT_TRUE(m_tempDir.isValid());
    SettingHandler handler1;

    // Act
    SettingHandler handler2;
    bool first = handler1.isCompelted();
    bool second = handler2.isCompelted();

    // Assert: 两实例均完成初始化
    EXPECT_EQ(first, true);
    EXPECT_EQ(second, true);
}

// getOptionValue B1: 三个告警键默认值与 settings.json 一致
TEST_F(UT_SettingHandler, GetOptionValue_Defaults_ReturnJsonValues)
{
    // Arrange
    SettingHandler handler;

    // Act: 读取三个告警键默认值
    int cpuDefault = handler.getOptionValue(AlarmCpuUsageOptionName).toInt();
    int memDefault = handler.getOptionValue(AlarmMemUsageOptionName).toInt();
    int intervalDefault = handler.getOptionValue(AlarmIntervalOptionName).toInt();

    // Assert: 与 settings.json 一致
    EXPECT_EQ(cpuDefault, 90);
    EXPECT_EQ(memDefault, 90);
    EXPECT_EQ(intervalDefault, 10);
    EXPECT_EQ(handler.getOptionValue(AlarmCpuUsageOptionName).toInt(), 90);
    EXPECT_EQ(handler.getOptionValue(AlarmMemUsageOptionName).toInt(), 90);
    EXPECT_EQ(handler.getOptionValue(AlarmIntervalOptionName).toInt(), 10);
    EXPECT_FALSE(handler.getOptionValue(AlarmStatusOptionName).toBool());
    EXPECT_EQ(handler.getOptionValue(AlarmLastTimeOptionName).toLongLong(), 0);
}

// getOptionValue B2: 未知键 → 返回无效 QVariant
TEST_F(UT_SettingHandler, GetOptionValue_UnknownKey_ReturnsInvalidVariant)
{
    // Arrange
    SettingHandler handler;

    // Act
    QVariant value = handler.getOptionValue("setting.bogus.key");

    // Assert: 无效 QVariant（默认构造等价）
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(value, QVariant());
}

// changedOptionValue B1: 合法键写入 → true + 即时可见 + 落盘持久化
TEST_F(UT_SettingHandler, ChangedOptionValue_KnownKey_PersistsAndReturnsTrue)
{
    // Arrange
    SettingHandler handler;

    // Act
    bool ok = handler.changedOptionValue(AlarmCpuUsageOptionName, 55);

    // Assert: 返回值与即时可见性
    EXPECT_TRUE(ok);
    EXPECT_EQ(handler.getOptionValue(AlarmCpuUsageOptionName).toInt(), 55);

    // Assert: 持久化 —— 重新构建实例（同目录）读回已保存值
    SettingHandler reloaded;
    EXPECT_EQ(reloaded.getOptionValue(AlarmCpuUsageOptionName).toInt(), 55);
}

// changedOptionValue B2: 未知键 → false
TEST_F(UT_SettingHandler, ChangedOptionValue_UnknownKey_ReturnsFalse)
{
    // Arrange
    SettingHandler handler;

    // Act
    bool ok = handler.changedOptionValue("setting.bogus.key", 55);

    // Assert: 未知键拒绝写入
    EXPECT_FALSE(ok);
    EXPECT_EQ(handler.getOptionValue("setting.bogus.key").isValid(), false);
}

// isVaildValue B1: CPU 阈值范围 [30,100] 与边界值
TEST_F(UT_SettingHandler, IsVaildValue_CpuRange_BoundariesDetected)
{
    // Arrange
    SettingHandler handler;

    // Act: 依次调用四个边界值校验
    bool loOk = handler.isVaildValue(AlarmCpuUsageOptionName, 30);
    bool hiOk = handler.isVaildValue(AlarmCpuUsageOptionName, 100);
    bool loBad = handler.isVaildValue(AlarmCpuUsageOptionName, 29);
    bool hiBad = handler.isVaildValue(AlarmCpuUsageOptionName, 101);

    // Assert: 边界 30/100 合法，29/101 拒绝
    EXPECT_EQ(loOk, true);
    EXPECT_EQ(hiOk, true);
    EXPECT_EQ(loBad, false);
    EXPECT_EQ(hiBad, false);
}

// isVaildValue B1: 内存阈值范围 [30,100] 与边界值
TEST_F(UT_SettingHandler, IsVaildValue_MemRange_BoundariesDetected)
{
    // Arrange
    SettingHandler handler;

    // Act: 依次调用四个边界值校验
    bool loOk = handler.isVaildValue(AlarmMemUsageOptionName, 30);
    bool hiOk = handler.isVaildValue(AlarmMemUsageOptionName, 100);
    bool loBad = handler.isVaildValue(AlarmMemUsageOptionName, 29);
    bool hiBad = handler.isVaildValue(AlarmMemUsageOptionName, 101);

    // Assert: 边界 30/100 合法，29/101 拒绝
    EXPECT_EQ(loOk, true);
    EXPECT_EQ(hiOk, true);
    EXPECT_EQ(loBad, false);
    EXPECT_EQ(hiBad, false);
}

// isVaildValue B1: 告警间隔范围 [5,60] 与边界值
TEST_F(UT_SettingHandler, IsVaildValue_IntervalRange_BoundariesDetected)
{
    // Arrange
    SettingHandler handler;

    // Act: 依次调用四个边界值校验
    bool loOk = handler.isVaildValue(AlarmIntervalOptionName, 5);
    bool hiOk = handler.isVaildValue(AlarmIntervalOptionName, 60);
    bool loBad = handler.isVaildValue(AlarmIntervalOptionName, 4);
    bool hiBad = handler.isVaildValue(AlarmIntervalOptionName, 61);

    // Assert: 边界 5/60 合法，4/61 拒绝
    EXPECT_EQ(loOk, true);
    EXPECT_EQ(hiOk, true);
    EXPECT_EQ(loBad, false);
    EXPECT_EQ(hiBad, false);
}

// isVaildValue B2: 未知键不在范围表 → 不校验返回 true
TEST_F(UT_SettingHandler, IsVaildValue_UnknownKey_ReturnsTrue)
{
    // Arrange
    SettingHandler handler;

    // Act: 未知键与无范围键逐一调用
    bool bogusKey = handler.isVaildValue("setting.bogus.key", 999);
    bool statusKey = handler.isVaildValue(AlarmStatusOptionName, true);
    bool lastTimeKey = handler.isVaildValue(AlarmLastTimeOptionName, 123456789LL);

    // Assert: 未知键/无范围键不校验，均返回 true
    EXPECT_EQ(bogusKey, true);
    EXPECT_EQ(statusKey, true);
    EXPECT_EQ(lastTimeKey, true);
}

// isVaildValue B3: 非数值类型（QString）→ 不校验返回 true
TEST_F(UT_SettingHandler, IsVaildValue_NonNumericType_ReturnsTrue)
{
    // Arrange
    SettingHandler handler;

    // Act: 传入字符串类型（超出 [Bool..Double] 枚举区间）
    bool strCpu = handler.isVaildValue(AlarmCpuUsageOptionName, QVariant(QString("90")));
    bool strInterval = handler.isVaildValue(AlarmIntervalOptionName, QVariant(QString("30")));

    // Assert: 非数值类型不校验，直接通过
    EXPECT_EQ(strCpu, true);
    EXPECT_EQ(strInterval, true);
}

// getValueRange B1: 三个键的配置范围
TEST_F(UT_SettingHandler, GetValueRange_KnownKeys_ReturnsRanges)
{
    // Arrange
    SettingHandler handler;

    // Act: 读取三个键的配置范围
    auto cpu = handler.getValueRange(AlarmCpuUsageOptionName);
    auto mem = handler.getValueRange(AlarmMemUsageOptionName);
    auto interval = handler.getValueRange(AlarmIntervalOptionName);

    // Assert: 与 settings.json value-range 一致
    EXPECT_DOUBLE_EQ(cpu.first, 30.0);
    EXPECT_DOUBLE_EQ(cpu.second, 100.0);

    EXPECT_DOUBLE_EQ(mem.first, 30.0);
    EXPECT_DOUBLE_EQ(mem.second, 100.0);

    EXPECT_DOUBLE_EQ(interval.first, 5.0);
    EXPECT_DOUBLE_EQ(interval.second, 60.0);
}

// getValueRange B2: 未知键 → (0,0)
TEST_F(UT_SettingHandler, GetValueRange_UnknownKey_ReturnsZeroPair)
{
    // Arrange
    SettingHandler handler;

    // Act
    auto range = handler.getValueRange("setting.bogus.key");

    // Assert
    EXPECT_DOUBLE_EQ(range.first, 0.0);
    EXPECT_DOUBLE_EQ(range.second, 0.0);
}

// itemKeys: 包含全部 5 个告警键与消息设置键
TEST_F(UT_SettingHandler, ItemKeys_AllOptionKeys_Contained)
{
    // Arrange
    SettingHandler handler;

    // Act
    QList<QString> keys = handler.itemKeys();

    // Assert（settings.json 叶子键全量）
    EXPECT_EQ(keys.contains(AlarmStatusOptionName), true);
    EXPECT_TRUE(keys.contains(AlarmCpuUsageOptionName));
    EXPECT_TRUE(keys.contains(AlarmMemUsageOptionName));
    EXPECT_TRUE(keys.contains(AlarmIntervalOptionName));
    EXPECT_TRUE(keys.contains(AlarmLastTimeOptionName));
    EXPECT_TRUE(keys.contains("setting.messgae.message_setting"));
}

// dtor: 作用域销毁（deleteLater）后新实例继续可用
TEST_F(UT_SettingHandler, Dtor_RebuildCycle_DestroysCleanly)
{
    // Arrange
    {
        SettingHandler inner;
        EXPECT_TRUE(inner.isCompelted());
    }

    // Act: 销毁后重建（deferred delete 由 main 末尾冲刷）
    SettingHandler fresh;

    // Assert
    EXPECT_TRUE(fresh.isCompelted());
    EXPECT_EQ(fresh.getOptionValue(AlarmCpuUsageOptionName).toInt(), 90);
}
