// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "settings.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

/***************************************STUB begin*********************************************/
// 无外部依赖打桩需求：Settings 仅依赖 QSettings（本地 ini 文件）。
// 为避免污染真实用户配置，将 XDG_CONFIG_HOME 重定向至临时目录后再触发单例创建。
/***************************************STUB end***********************************************/

// 分支清单（源自 settings.cpp）:
// 1. ctor: filepath = ConfigLocation + orgName + appName + "config.conf"，创建 QSettings(IniFormat)
// 2. instance: 静态单例（theInstance()）
// 3. flush: m_settings->sync()
// 4. getOption: beginGroup("settings") -> value(key, defaultValue) -> endGroup
// 5. setOption: beginGroup("settings") -> setValue(key, value) -> endGroup
// 注: Settings 为非 QObject 的普通单例，进程退出时随单例释放

class SettingsTest : public ::testing::Test
{
public:
    static void redirectConfigHome()
    {
        // Arrange: 重定向配置目录到临时路径，隔离真实用户配置
        if (s_tmpDir.isValid())
            qputenv("XDG_CONFIG_HOME", s_tmpDir.path().toUtf8());
    }

    static void SetUpTestSuite() { redirectConfigHome(); }

    virtual void SetUp() override
    {
        // Arrange: 确保配置目录重定向生效后再触发单例（首次 instance() 创建 QSettings）
        redirectConfigHome();
        Settings *inst = Settings::instance();
        // Assert: 实例有效
        ASSERT_NE(inst, nullptr);
    }

    virtual void TearDown() override
    {
        // 单例随进程退出释放；此处仅确认实例仍可用（无泄漏路径）
        EXPECT_NE(Settings::instance(), nullptr);
    }

    static void TearDownTestSuite()
    {
        // 清理重定向，与 SetUpTestSuite 的 qputenv 配对
        qunsetenv("XDG_CONFIG_HOME");
    }

    static QTemporaryDir s_tmpDir;
};

QTemporaryDir SettingsTest::s_tmpDir;

// ───────────────────────── instance ─────────────────────────
TEST_F(SettingsTest, GetInstance_CalledRepeatedly_ReturnsSameInstance)
{
    // Arrange: SetUp 已触发单例创建
    // Act
    Settings *first = Settings::instance();
    Settings *second = Settings::instance();

    // Assert: 静态单例返回同一地址
    EXPECT_EQ(first, second);
    EXPECT_NE(first, nullptr);
}

// ───────────────────────── getOption/setOption ─────────────────────────
TEST_F(SettingsTest, SetOption_WithCustomKey_ValueReadableByGetOption)
{
    // Arrange
    const QString key = "ut_settings_string_key";

    // Act
    Settings::instance()->setOption(key, "ut_settings_value");

    // Assert: 写入后可读回
    EXPECT_EQ(Settings::instance()->getOption(key).toString(), QString("ut_settings_value"));
    EXPECT_TRUE(Settings::instance()->getOption(key).isValid());
}

TEST_F(SettingsTest, GetOption_WithMissingKey_ReturnsDefaultValue)
{
    // Arrange: 使用未写入过的 key
    const QString key = "ut_settings_missing_key";

    // Act
    const QVariant withDefault = Settings::instance()->getOption(key, "default_value");
    const QVariant withoutDefault = Settings::instance()->getOption(key);

    // Assert: 显式默认值生效；未提供默认值时为无效 QVariant
    EXPECT_EQ(withDefault.toString(), QString("default_value"));
    EXPECT_FALSE(withoutDefault.isValid());
}

TEST_F(SettingsTest, SetOption_WithIntAndBool_RoundTripsCorrectly)
{
    // Arrange
    const QString intKey = "ut_settings_int_key";
    const QString boolKey = "ut_settings_bool_key";

    // Act
    Settings::instance()->setOption(intKey, 42);
    Settings::instance()->setOption(boolKey, true);

    // Assert: int 与 bool 类型往返一致
    EXPECT_EQ(Settings::instance()->getOption(intKey).toInt(), 42);
    EXPECT_EQ(Settings::instance()->getOption(boolKey).toBool(), true);
}

TEST_F(SettingsTest, SetOption_WithOverwrite_LastValueWins)
{
    // Arrange
    const QString key = "ut_settings_overwrite_key";
    Settings::instance()->setOption(key, "first");

    // Act: 同 key 二次写入
    Settings::instance()->setOption(key, "second");

    // Assert: 后写覆盖先写
    EXPECT_EQ(Settings::instance()->getOption(key).toString(), QString("second"));
    EXPECT_FALSE(Settings::instance()->getOption(key).toString() == QString("first"));
}

// ───────────────────────── flush（持久化）─────────────────────────
TEST_F(SettingsTest, Flush_AfterSetOption_PersistsToConfigFile)
{
    // Arrange
    const QString key = "ut_settings_flush_key";
    Settings::instance()->setOption(key, "flushed_value");

    // Act: 强制同步到磁盘
    Settings::instance()->flush();

    // Assert: 以相同规则重新构造 QSettings，从磁盘读回
    const QString filepath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                             + "/" + QCoreApplication::organizationName()
                             + "/" + QCoreApplication::applicationName()
                             + "/" + "config.conf";
    QSettings persisted(filepath, QSettings::IniFormat);
    EXPECT_EQ(persisted.value("settings/" + key).toString(), QString("flushed_value"));
    EXPECT_TRUE(persisted.contains("settings/" + key));
}

// ───────────────────────── 预置键常量 ─────────────────────────
TEST_F(SettingsTest, SetOption_WithPresetKey_DisplayModeRoundTrips)
{
    // Arrange: 使用 settings.h 中预置的业务键常量
    const QString key = kSettingKeyDisplayMode;

    // Act
    Settings::instance()->setOption(key, 1);

    // Assert
    EXPECT_EQ(Settings::instance()->getOption(key).toInt(), 1);
    EXPECT_TRUE(Settings::instance()->getOption(key).isValid());
}
