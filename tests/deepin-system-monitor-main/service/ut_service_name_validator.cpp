// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// self
#include "service_name_validator.h"

// gtest
#include <gtest/gtest.h>

class UT_ServiceNameValidator : public ::testing::Test
{
};

// ---------------- isValidServiceName ----------------

TEST_F(UT_ServiceNameValidator, ValidNames)
{
    EXPECT_TRUE(isValidServiceName("dbus.service"));
    EXPECT_TRUE(isValidServiceName("cron.service"));
    EXPECT_TRUE(isValidServiceName("systemd-journald.service"));
    EXPECT_TRUE(isValidServiceName("getty@.service"));
    EXPECT_TRUE(isValidServiceName("getty@tty1.service"));
    EXPECT_TRUE(isValidServiceName("a-b_c.d@e.service"));
    EXPECT_TRUE(isValidServiceName("foo\\x20bar.service"));
    EXPECT_TRUE(isValidServiceName("foo\\x3bbar.service"));
    EXPECT_TRUE(isValidServiceName("foo\\x5cbar.service"));
    EXPECT_TRUE(isValidServiceName("dbus.s\\x65rvice.service"));
}

TEST_F(UT_ServiceNameValidator, InvalidEmptyAndSuffix)
{
    EXPECT_FALSE(isValidServiceName(""));          // 空
    EXPECT_FALSE(isValidServiceName("dbus"));     // 缺 .service 后缀
    EXPECT_FALSE(isValidServiceName("dbus.servicex")); // 后缀错误
    EXPECT_FALSE(isValidServiceName(".service")); // 缺主体
}

TEST_F(UT_ServiceNameValidator, InvalidControlChars)
{
    EXPECT_FALSE(isValidServiceName("db us.service"));  // 空格
    EXPECT_FALSE(isValidServiceName("db\tus.service")); // 制表符
    EXPECT_FALSE(isValidServiceName("db\nus.service"));  // 换行
    EXPECT_FALSE(isValidServiceName("dbus.service\n"));  // 尾部换行
    EXPECT_FALSE(isValidServiceName("dbus.service\r"));  // 尾部回车
    EXPECT_FALSE(isValidServiceName("db;us.service"));  // 分号
    EXPECT_FALSE(isValidServiceName("dbus;"));
    EXPECT_FALSE(isValidServiceName("db/us.service"));   // 路径分隔符
    EXPECT_FALSE(isValidServiceName("\\x2f.service"));   // 转义路径分隔符
    EXPECT_FALSE(isValidServiceName("\\x2F.service"));   // 转义路径分隔符
    EXPECT_FALSE(isValidServiceName("db\\x2fus.service"));
    EXPECT_FALSE(isValidServiceName("db\\us.service"));  // 非 systemd 转义的反斜杠
    EXPECT_FALSE(isValidServiceName("db\\\\us.service")); // 非 systemd 转义的反斜杠
    EXPECT_FALSE(isValidServiceName("db\\xZZ.service")); // 非法 systemd 转义
    EXPECT_FALSE(isValidServiceName("dbus.\\x73ervice")); // 后缀必须是字面 .service
}

TEST_F(UT_ServiceNameValidator, InvalidTooLong)
{
    QString longName;
    longName.fill('a', SHRT_MAX);          // 主体超长
    longName += ".service";
    EXPECT_FALSE(isValidServiceName(longName));
}

// ---------------- serviceExistsInList ----------------

static const char *kSampleList =
    "UNIT FILE                                  STATE         PRESET\n"
    "cron.service                               enabled       enabled\n"
    "dbus.service                               static        -\n"
    "getty@.service                             enabled       enabled\n"
    "\n"
    "3 unit files listed.\n";

TEST_F(UT_ServiceNameValidator, ExistsExactMatch)
{
    EXPECT_TRUE(serviceExistsInList("dbus.service", kSampleList));
    EXPECT_TRUE(serviceExistsInList("cron.service", kSampleList));
    EXPECT_TRUE(serviceExistsInList("getty@.service", kSampleList));
}

TEST_F(UT_ServiceNameValidator, ExistsWithTabSeparatedColumns)
{
    const QByteArray list =
        "UNIT FILE\tSTATE\tPRESET\n"
        "dbus.service\tstatic\t-\n";

    EXPECT_TRUE(serviceExistsInList("dbus.service", list));
}

TEST_F(UT_ServiceNameValidator, NotExistsSubstringOrSuffix)
{
    // 子串不应命中
    EXPECT_FALSE(serviceExistsInList("dbu.service", kSampleList));   // 与 dbus.service 有子串关系但不同
    EXPECT_FALSE(serviceExistsInList("dbus", kSampleList));         // 缺后缀
    EXPECT_FALSE(serviceExistsInList("cron.servicex", kSampleList));
    // 头行/尾行不应命中
    EXPECT_FALSE(serviceExistsInList("UNIT", kSampleList));
    EXPECT_FALSE(serviceExistsInList("listed.", kSampleList));
}

TEST_F(UT_ServiceNameValidator, ExistsEmptyList)
{
    EXPECT_FALSE(serviceExistsInList("dbus.service", ""));
    EXPECT_FALSE(serviceExistsInList("dbus.service", QByteArray()));
}
