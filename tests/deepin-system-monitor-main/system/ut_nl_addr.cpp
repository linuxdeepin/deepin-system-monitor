///*
//* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
//*
//* Author:      baohaifeng <baohaifeng@uniontech.com>
//* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
//*
//* This program is free software: you can redistribute it and/or modify
//* it under the terms of the GNU General Public License as published by
//* the Free Software Foundation, either version 3 of the License, or
//* any later version.
//*
//* This program is distributed in the hope that it will be useful,
//* but WITHOUT ANY WARRANTY; without even the implied warranty of
//* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//* GNU General Public License for more details.
//*
//* You should have received a copy of the GNU General Public License
//* along with this program.  If not, see <http://www.gnu.org/licenses/>.
//*/

////self
//#include "system/nl_addr.h"
//#include <netlink/netlink.h>
//#include <netlink/cache.h>
//#include <netlink/addr.h>
//#include <netlink/route/link.h>
//#include <netlink/route/addr.h>
//#include <netlink/addr.h>
//#include <libnl3/netlink/route/addr.h>

////gtest
//#include "stub.h"
//#include <gtest/gtest.h>

//using namespace core::system;

//class UT_NLAddr: public ::testing::Test
//{
//public:
//    UT_NLAddr() : m_tester(nullptr) {}

//public:
//    virtual void SetUp()
//    {
//        rtnl_addr addr;
//        memset(&addr, 0, sizeof (addr));
//        m_tester = new NLAddr(&addr);
//    }

//    virtual void TearDown()
//    {
//        if (m_tester) {
//        delete m_tester;
//        m_tester = nullptr;
//            }
//    }

//protected:
//    NLAddr *m_tester;
//};

//TEST_F(UT_NLAddr, initTest)
//{
//}

//TEST_F(UT_NLAddr, test_ifindex)
//{
//    auto ok = m_tester->ifindex();
//    EXPECT_NE(ok, 0);
//}

//TEST_F(UT_NLAddr, test_family)
//{
//    auto ok = m_tester->family();
//    EXPECT_NE(ok, 0);
//}

//TEST_F(UT_NLAddr, test_prefixlen)
//{
//    auto ok = m_tester->prefixlen();
//    EXPECT_NE(ok, 0);
//}

