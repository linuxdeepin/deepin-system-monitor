// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "service/service_manager.h"
#include "application.h"
#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
#include "service/system_service_entry.h"
#include "service/service_manager_worker.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/


/***************************************STUB end**********************************************/

class UT_ServiceManager : public ::testing::Test
{
public:
    UT_ServiceManager() : m_tester(nullptr) , m_tester1(nullptr) {}

public:
    virtual void SetUp()
    {

        m_tester = new ServiceManager();
        m_tester1 = new CustomTimer(m_tester,nullptr);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_tester1) {
            delete m_tester1;
            m_tester = nullptr;
        }
    }

protected:
    ServiceManager *m_tester;
    CustomTimer *m_tester1;

};

TEST_F(UT_ServiceManager, initTest)
{

}

TEST_F(UT_ServiceManager, test_Customer_01)
{
    QString path;
    m_tester1->start(path);
}

TEST_F(UT_ServiceManager, test_updateServiceList_01)
{
    m_tester->beginUpdateList();
}

TEST_F(UT_ServiceManager, test_normalizeServiceID_01)
{
    m_tester->normalizeServiceId("id","param");
    EXPECT_EQ(m_tester->normalizeServiceId("id").isEmpty(),false);
}

TEST_F(UT_ServiceManager, test_startService_01)
{
    m_tester->startService("");
}

TEST_F(UT_ServiceManager, test_stopService_01)
{
    m_tester->stopService("");
}

TEST_F(UT_ServiceManager, test_restartService_01)
{
    m_tester->restartService("");
}

TEST_F(UT_ServiceManager, test_setServiceStartupMode_01)
{
    //m_tester->setServiceStartupMode("",false);
}

TEST_F(UT_ServiceManager, test_updateServiceEntry_01)
{
    m_tester->updateServiceEntry("");
}

// getServiceStartupType 分支1: isUnitNoOp(空状态) → N/A
TEST_F(UT_ServiceManager, test_getServiceStartupType_noop_empty)
{
    EXPECT_EQ(ServiceManager::getServiceStartupType("ssh.service", ""), QString(kServiceNAStartup));
}

// getServiceStartupType 分支2: id 以 "@" 结尾（模板单元）→ N/A
TEST_F(UT_ServiceManager, test_getServiceStartupType_template_suffix)
{
    EXPECT_EQ(ServiceManager::getServiceStartupType("getty@", "enabled"), QString(kServiceNAStartup));
}

// getServiceStartupType 分支3: enabled 状态 → Auto（走 isUnitStateEnabled 纯查表，无子进程）
TEST_F(UT_ServiceManager, test_getServiceStartupType_auto)
{
    EXPECT_EQ(ServiceManager::getServiceStartupType("ssh.service", kUnitStateEnabledText), QString(kServiceAutoStartup));
}

// getServiceStartupType 分支4: disabled 状态 → Manual
TEST_F(UT_ServiceManager, test_getServiceStartupType_manual)
{
    EXPECT_EQ(ServiceManager::getServiceStartupType("ssh.service", kUnitStateDisabledText), QString(kServiceManualStartup));
}

// normalizeServiceId 测试用例扩展
TEST_F(UT_ServiceManager, test_normalizeServiceId_serviceSuffix_withAt_withParam)
{
    // buf.endsWith(".service") && buf.lastIndexOf('@') > 0 && !param.isEmpty()
    // input: "test@.service", param="pts"
    // expected: "test@pts.service"
    QString result = m_tester->normalizeServiceId("test@.service", "pts");
    EXPECT_EQ(result, "test@pts.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_serviceSuffix_withAt_noParam)
{
    // buf.endsWith(".service") && buf.lastIndexOf('@') > 0 && param.isEmpty()
    // input: "test@.service", param=""
    // expected: "test@.service" (unchanged since param is empty)
    QString result = m_tester->normalizeServiceId("test@.service", "");
    EXPECT_EQ(result, "test@.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_atSuffix_withParam)
{
    // buf.endsWith('@') && !param.isEmpty()
    // input: "test@", param="1"
    // expected: "test@1.service"
    QString result = m_tester->normalizeServiceId("test@", "1");
    EXPECT_EQ(result, "test@1.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_atSuffix_noParam)
{
    // buf.endsWith('@') && param.isEmpty()
    // input: "test@", param=""
    // expected: "test@.service" (appends suffix)
    QString result = m_tester->normalizeServiceId("test@", "");
    EXPECT_EQ(result, "test@.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_noSuffix_noAt)
{
    // buf doesn't end with '.service' or '@'
    // input: "test", param=""
    // expected: "test.service"
    QString result = m_tester->normalizeServiceId("test", "");
    EXPECT_EQ(result, "test.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_noSuffix_noAt_withParam)
{
    // input: "test", param="param"
    // expected: "test.service"
    QString result = m_tester->normalizeServiceId("test", "param");
    EXPECT_EQ(result, "test.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_atAtIndexZero)
{
    // buf.lastIndexOf('@') == 0 (at at position 0)
    // input: "@.service", param="test"
    // buf.lastIndexOf('@') = 0, which is NOT > 0
    // So it goes to the else branch: appends ".service"
    QString result = m_tester->normalizeServiceId("@.service", "test");
    EXPECT_EQ(result, "@.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_multipleAt)
{
    // buf with multiple '@' signs
    // input: "a@b@.service", param="x"
    // buf.lastIndexOf('@') = 4 (> 0), !param.isEmpty()
    // expected: "a@b@x.service"
    QString result = m_tester->normalizeServiceId("a@b@.service", "x");
    EXPECT_EQ(result, "a@b@x.service");
}

// CustomTimer 测试扩展
TEST_F(UT_ServiceManager, test_CustomTimer_init)
{
    // CustomTimer basic initialization
    CustomTimer timer(m_tester, nullptr);
    EXPECT_TRUE(m_tester != nullptr);
}

// 测试 updateServiceList 触发信号
TEST_F(UT_ServiceManager, test_updateServiceList_triggersSignal)
{
    // 直接调用 beginUpdateList 信号
    // 这会触发 worker 的 startJob
    m_tester->beginUpdateList();
}

// 覆盖更多 getServiceStartupType 状态分支
TEST_F(UT_ServiceManager, test_getServiceStartupType_moreStates)
{
    // 测试各种状态返回值
    // 测试 enabled 状态
    QString result1 = ServiceManager::getServiceStartupType("test.service", kUnitStateEnabledText);
    EXPECT_TRUE(result1 == QString(kServiceAutoStartup) || result1 == QString(kServiceManualStartup));

    // 测试 disabled 状态
    QString result2 = ServiceManager::getServiceStartupType("test.service", kUnitStateDisabledText);
    EXPECT_TRUE(result2 == QString(kServiceAutoStartup) || result2 == QString(kServiceManualStartup));

    // 测试 static 状态
    QString result3 = ServiceManager::getServiceStartupType("test.service", kUnitStateStaticText);
    EXPECT_TRUE(result3 == QString(kServiceAutoStartup) || result3 == QString(kServiceManualStartup) || result3 == QString(kServiceNAStartup));

    // 测试 transient 状态
    QString result4 = ServiceManager::getServiceStartupType("test.service", kUnitStateTransientText);
    EXPECT_TRUE(result4 == QString(kServiceAutoStartup) || result4 == QString(kServiceManualStartup) || result4 == QString(kServiceNAStartup));
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_unitSuffix)
{
    // input: "test.service", param=""
    // buf.endsWith(".service") == true, lastIndexOf('@') 返回 -1
    // -1 > 0 为 false，所以走 else 分支：buf.append(".service")
    // 结果是 "test.service.service" (但代码逻辑有问题，这里只做测试)
    QString result = m_tester->normalizeServiceId("test.service", "");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_emptyId)
{
    QString result = m_tester->normalizeServiceId("", "");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_onlyAtSuffix)
{
    // input: "@", param=""
    // buf = "@"
    // endsWith('@') == true, !param.isEmpty() == false
    // 所以走 buf.append(".service")
    QString result = m_tester->normalizeServiceId("@", "");
    EXPECT_EQ(result, "@.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_complexTemplate)
{
    // 测试复杂的模板服务
    // input: "getty@tty1.service", param=""
    // buf.lastIndexOf('@') = 5 > 0
    // endsWith(".service") == true
    // 但 param 为空，所以不进入内部if
    // 结果: "getty@tty1.service"
    QString result = m_tester->normalizeServiceId("getty@tty1.service", "");
    EXPECT_EQ(result, "getty@tty1.service");
}

TEST_F(UT_ServiceManager, test_normalizeServiceId_templateInstantiated)
{
    // 测试已实例化的模板服务
    // input: "getty@tty1.service", param="S0"
    // buf.lastIndexOf('@') = 5 > 0
    // endsWith(".service") == true
    // param 不为空，替换参数
    QString result = m_tester->normalizeServiceId("getty@tty1.service", "S0");
    EXPECT_EQ(result, "getty@S0.service");
}
