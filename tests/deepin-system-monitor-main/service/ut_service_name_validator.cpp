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
    EXPECT_FALSE(isValidServiceName("db;us.service"));  // 分号
    EXPECT_FALSE(isValidServiceName("dbus;"));
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
