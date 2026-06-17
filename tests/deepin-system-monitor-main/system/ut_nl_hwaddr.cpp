// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/nl_hwaddr.h"
#include <net/if_arp.h>
#include <sys/ioctl.h>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB end**********************************************/

int stub_ioctl_hwaddr(int __fd, unsigned long int __request, ...)
{
    return 0;
}

/***************************************STUB end**********************************************/

class UT_NLHWAddr: public ::testing::Test
{
public:
    UT_NLHWAddr() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        QByteArray ifname;
        m_tester = new NLHWAddr(ifname);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NLHWAddr *m_tester;
};

TEST_F(UT_NLHWAddr, initTest)
{
}

TEST_F(UT_NLHWAddr, test_initData_01)
{
    m_tester->initData();
//    EXPECT_NE(m_tester->m_sa_family, 0);
}

// 扩展测试：使用空接口名
TEST_F(UT_NLHWAddr, test_initData_emptyIfname)
{
    // 短于 IFNAMSIZ 的接口名
    QByteArray shortName = "lo";
    NLHWAddr addr(shortName);
    // initData 会被构造函数调用
}

TEST_F(UT_NLHWAddr, test_initData_validIfname)
{
    // 有效的接口名
    QByteArray ethName = "eth0";
    NLHWAddr addr(ethName);
}

TEST_F(UT_NLHWAddr, test_initData_wlanIfname)
{
    // WLAN 接口名
    QByteArray wlanName = "wlan0";
    NLHWAddr addr(wlanName);
}

TEST_F(UT_NLHWAddr, test_initData_longIfname)
{
    // 超过 IFNAMSIZ-1 的接口名（会被截断）
    QByteArray longName = "verylonginterfacename12345";
    NLHWAddr addr(longName);
}

