// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_info_db.h"
#include "system/nl_link.h"
#include "system/nl_addr.h"
#include "system/netlink.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>

//system
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/addr.h>
#include <net/if.h>
#include <cstring>

using namespace core::system;

/***************************************STUB begin*********************************************/

// 迭代器桩：用内存假链路/假地址驱动 update_netif_info，隔离真实 netlink 缓存与设备环境
static struct rtnl_link *g_fakeLink = nullptr;   // 复用的假链路对象
static int g_linkSeq = 0;                        // 当前迭代位置
static int g_linkCount = 0;                      // 本轮迭代元素个数
static int g_linkIterCalled = 0;                 // linkIterator 调用计数
static int g_addrIterCalled = 0;                 // addrIterator 调用计数

static struct rtnl_addr *g_fakeAddr = nullptr;
static int g_addrSeq = 0;
static int g_addrCount = 0;

static LinkIterator stub_linkIterator_fake()
{
    g_linkIterCalled++;
    g_linkSeq = 0;
    return LinkIterator(nullptr, nullptr);  // 桩接管 hasNext/next，容器参数不起作用
}

static bool stub_link_hasNext()
{
    return g_linkSeq < g_linkCount;
}

static std::unique_ptr<NLLink> stub_link_next()
{
    g_linkSeq++;
    return std::unique_ptr<NLLink>(new NLLink(g_fakeLink, false));
}

static AddrIterator stub_addrIterator_fake()
{
    g_addrIterCalled++;
    g_addrSeq = 0;
    return AddrIterator(nullptr, nullptr);
}

static bool stub_addr_hasNext()
{
    return g_addrSeq < g_addrCount;
}

static std::unique_ptr<NLAddr> stub_addr_next()
{
    g_addrSeq++;
    return std::unique_ptr<NLAddr>(new NLAddr(g_fakeAddr, false));
}

// socket 桩：updateLinkInfo 内部 wireless/ethtool/hwaddr 探测全部走失败分支
static int stub_socket_fail(int domain, int type, int protocol)
{
    Q_UNUSED(domain);
    Q_UNUSED(type);
    Q_UNUSED(protocol);
    return -1;
}

// 一次打齐迭代器相关桩：Netlink 的两个迭代器入口 + CacheIterator 的 hasNext/next
static void utStubDbIterators(Stub &stub)
{
    stub.set(ADDR(Netlink, linkIterator), stub_linkIterator_fake);
    stub.set(ADDR(Netlink, addrIterator), stub_addrIterator_fake);
    stub.set(ADDR(LinkIterator, hasNext), stub_link_hasNext);
    stub.set(ADDR(LinkIterator, next), stub_link_next);
    stub.set(ADDR(AddrIterator, hasNext), stub_addr_hasNext);
    stub.set(ADDR(AddrIterator, next), stub_addr_next);
}

/***************************************STUB end**********************************************/

static struct rtnl_link *utMakeFakeLink(const char *name, int ifindex)
{
    struct rtnl_link *link = rtnl_link_alloc();
    rtnl_link_set_name(link, name);
    rtnl_link_set_ifindex(link, ifindex);
    rtnl_link_set_mtu(link, 1500);
    rtnl_link_set_flags(link, IFF_UP);
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    struct nl_addr *hw = nl_addr_build(AF_LLC, mac, sizeof(mac));
    rtnl_link_set_addr(link, hw);
    nl_addr_put(hw);
    return link;
}

class UT_NetifInfoDB: public ::testing::Test
{
public:
    UT_NetifInfoDB() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        g_fakeLink = nullptr;
        g_linkSeq = 0;
        g_linkCount = 0;
        g_linkIterCalled = 0;
        g_addrIterCalled = 0;
        g_fakeAddr = nullptr;
        g_addrSeq = 0;
        g_addrCount = 0;
        m_tester = new NetifInfoDB();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifInfoDB *m_tester;
};

// 构造：数据库初始化为空表并持有 netlink 句柄
TEST_F(UT_NetifInfoDB, NetifInfoDB_Construction_EmptyDBWithNetlinkHandle)
{
    // Arrange
    // SetUp 中已构造 NetifInfoDB

    // Act
    QMap<QByteArray, NetifInfoPtr> db = m_tester->infoDB();

    // Assert
    EXPECT_TRUE(db.isEmpty());
    EXPECT_NE(m_tester->m_netlink, nullptr);
}

// infoDB 访问器：返回的是内部表的浅拷贝，外部修改不影响库内数据
TEST_F(UT_NetifInfoDB, InfoDB_Accessor_ShallowCopyOfInternalDB)
{
    // Arrange
    auto info = std::make_shared<NetifInfo>();
    info->set_recv_bps(8.0);
    m_tester->m_infoDB.insert(QByteArray("utkey"), info);

    // Act
    QMap<QByteArray, NetifInfoPtr> db = m_tester->infoDB();

    // Assert
    EXPECT_EQ(db.size(), 1);
    // std::shared_ptr 浅拷贝指向同一对象
    EXPECT_EQ(db.value(QByteArray("utkey"))->recv_bps(), 8.0);
    EXPECT_EQ(m_tester->m_infoDB.size(), 1);
}

// 设备更新：链接/地址迭代器各调用一次且状态保持一致
TEST_F(UT_NetifInfoDB, Update_LinkAndAddrIterators_BothInvokedOnce)
{
    // Arrange
    Stub stub;
    utStubDbIterators(stub);
    stub.set(socket, stub_socket_fail);
    g_linkCount = 0;
    g_addrCount = 0;

    // Act
    m_tester->update();

    // Assert
    EXPECT_EQ(g_linkIterCalled, 1);
    EXPECT_EQ(g_addrIterCalled, 1);
    EXPECT_TRUE(m_tester->m_infoDB.isEmpty());
}

// 设备更新：普通网卡通过迭代器写入数据库，key 为 MAC 地址
TEST_F(UT_NetifInfoDB, Update_NonLoopbackDevice_AddsEntryKeyedByMac)
{
    // Arrange
    g_fakeLink = utMakeFakeLink("utfake0", 42);
    g_linkCount = 1;
    Stub stub;
    utStubDbIterators(stub);
    stub.set(socket, stub_socket_fail);

    // Act
    m_tester->update();

    // Assert
    EXPECT_TRUE(m_tester->m_infoDB.contains(QByteArray("00:11:22:33:44:55")));
    NetifInfoPtr info = m_tester->m_infoDB.value(QByteArray("00:11:22:33:44:55"));
    ASSERT_TRUE(info != nullptr);
    EXPECT_EQ(info->ifname(), QByteArray("utfake0"));
    EXPECT_EQ(info->index(), 42);
    EXPECT_EQ(m_tester->m_infoDB.size(), 1);
}

// 设备更新：回环设备被跳过，不写入数据库
TEST_F(UT_NetifInfoDB, Update_LoopbackDevice_Skipped)
{
    // Arrange
    g_fakeLink = utMakeFakeLink("lo", 1);
    g_linkCount = 1;
    Stub stub;
    utStubDbIterators(stub);
    stub.set(socket, stub_socket_fail);

    // Act
    m_tester->update();

    // Assert
    EXPECT_TRUE(m_tester->m_infoDB.isEmpty());
    EXPECT_EQ(m_tester->m_infoDB.size(), 0);
}

// 速率计算：第二次更新时基于 MAC 命中旧记录计算字节速率
TEST_F(UT_NetifInfoDB, Update_SecondRound_ComputesByteRateFromDiff)
{
    // Arrange
    g_fakeLink = utMakeFakeLink("utfake0", 42);
    g_linkCount = 1;
    Stub stub;
    utStubDbIterators(stub);
    stub.set(socket, stub_socket_fail);
    rtnl_link_set_stat(g_fakeLink, RTNL_LINK_RX_BYTES, 1000);
    rtnl_link_set_stat(g_fakeLink, RTNL_LINK_TX_BYTES, 500);
    m_tester->update();  // 第一轮：建立旧记录

    // Act
    // 第二轮前拉高统计，第二轮 update 内部与旧记录做差算速率
    rtnl_link_set_stat(g_fakeLink, RTNL_LINK_RX_BYTES, 2000);
    rtnl_link_set_stat(g_fakeLink, RTNL_LINK_TX_BYTES, 1500);
    m_tester->update();

    // Assert
    NetifInfoPtr info = m_tester->m_infoDB.value(QByteArray("00:11:22:33:44:55"));
    ASSERT_TRUE(info != nullptr);
    // 差值 1000/500 除以真实间隔(>0)，速率必为正
    EXPECT_GT(info->recv_bps(), 0.0);
    EXPECT_GT(info->sent_bps(), 0.0);
}

// IPv4 地址索引：地址迭代器返回 AF_INET 地址时按 ifindex 入索引
TEST_F(UT_NetifInfoDB, UpdateAddr_AF_INET_IndexedByIfindex)
{
    // Arrange
    struct rtnl_addr *addr = rtnl_addr_alloc();
    ASSERT_NE(addr, nullptr);
    rtnl_addr_set_family(addr, AF_INET);
    rtnl_addr_set_prefixlen(addr, 24);
    rtnl_addr_set_ifindex(addr, 42);
    const unsigned char ip[4] = {192, 168, 1, 10};
    struct nl_addr *local = nl_addr_build(AF_INET, ip, sizeof(ip));
    nl_addr_set_prefixlen(local, 24);  // rtnl_addr_set_local 会从 local 推导 prefixlen
    rtnl_addr_set_local(addr, local);
    nl_addr_put(local);
    const unsigned char bc[4] = {192, 168, 1, 255};
    struct nl_addr *bcast = nl_addr_build(AF_INET, bc, sizeof(bc));
    rtnl_addr_set_broadcast(addr, bcast);
    nl_addr_put(bcast);
    g_fakeAddr = addr;
    g_addrCount = 1;
    Stub stub;
    utStubDbIterators(stub);

    // Act
    m_tester->update_addr();

    // Assert
    QList<INet4Addr> result = m_tester->m_addrIpv4DB.values(42);
    EXPECT_EQ(result.size(), 1);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.first()->addr, QByteArray("192.168.1.10"));
    EXPECT_EQ(result.first()->mask, QByteArray("255.255.255.0"));
    EXPECT_EQ(result.first()->bcast, QByteArray("192.168.1.255"));
    EXPECT_TRUE(m_tester->m_addrIpv6DB.isEmpty());
}

// IPv6 地址索引：地址迭代器返回 AF_INET6 地址时按 ifindex 入索引
TEST_F(UT_NetifInfoDB, UpdateAddr_AF_INET6_IndexedByIfindex)
{
    // Arrange
    struct rtnl_addr *addr = rtnl_addr_alloc();
    ASSERT_NE(addr, nullptr);
    rtnl_addr_set_family(addr, AF_INET6);
    rtnl_addr_set_prefixlen(addr, 64);
    rtnl_addr_set_scope(addr, 2);
    rtnl_addr_set_ifindex(addr, 43);
    const unsigned char ip6[16] = {0xfe, 0x80, 0};
    struct nl_addr *local = nl_addr_build(AF_INET6, ip6, sizeof(ip6));
    nl_addr_set_prefixlen(local, 64);  // rtnl_addr_set_local 会从 local 推导 prefixlen
    rtnl_addr_set_local(addr, local);
    nl_addr_put(local);
    g_fakeAddr = addr;
    g_addrCount = 1;
    Stub stub;
    utStubDbIterators(stub);

    // Act
    m_tester->update_addr();

    // Assert
    QList<INet6Addr> result = m_tester->m_addrIpv6DB.values(43);
    EXPECT_EQ(result.size(), 1);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.first()->addr, QByteArray("fe80::"));
    EXPECT_EQ(result.first()->prefixlen, 64);
    EXPECT_EQ(result.first()->scope, 2);
    EXPECT_TRUE(m_tester->m_addrIpv4DB.isEmpty());
}

// netlink 地址填充：本地地址初始化后 netaddr/netmask 拆分正确
TEST_F(UT_NetifInfoDB, NLAddr_LocalInitialized_SplitsNetaddrAndNetmask)
{
    // Arrange
    struct rtnl_addr *addr = rtnl_addr_alloc();
    ASSERT_NE(addr, nullptr);
    rtnl_addr_set_family(addr, AF_INET);
    rtnl_addr_set_prefixlen(addr, 24);
    rtnl_addr_set_ifindex(addr, 42);
    rtnl_addr_set_scope(addr, 0);
    const unsigned char ip[4] = {192, 168, 1, 10};
    struct nl_addr *local = nl_addr_build(AF_INET, ip, sizeof(ip));
    nl_addr_set_prefixlen(local, 24);  // rtnl_addr_set_local 会从 local 推导 prefixlen
    rtnl_addr_set_local(addr, local);
    nl_addr_put(local);

    // Act
    NLAddr nlAddr(addr, false);
    nlAddr.local();

    // Assert
    EXPECT_EQ(nlAddr.ifindex(), 42);
    EXPECT_EQ(nlAddr.family(), AF_INET);
    EXPECT_EQ(nlAddr.prefixlen(), 24);
    EXPECT_EQ(nlAddr.scope(), 0);
    EXPECT_EQ(nlAddr.netaddr(), QByteArray("192.168.1.10"));
    EXPECT_EQ(nlAddr.netmask(), QByteArray("255.255.255.0"));
}

// netlink 地址填充：迭代器桩返回的假地址可被正常消费
TEST_F(UT_NetifInfoDB, Update_AddrIteratorFakes_DrivenIntoAddrIndex)
{
    // Arrange
    struct rtnl_addr *addr = rtnl_addr_alloc();
    rtnl_addr_set_family(addr, AF_INET);
    rtnl_addr_set_prefixlen(addr, 24);
    rtnl_addr_set_ifindex(addr, 42);
    const unsigned char ip[4] = {10, 0, 0, 5};
    struct nl_addr *local = nl_addr_build(AF_INET, ip, sizeof(ip));
    rtnl_addr_set_local(addr, local);
    nl_addr_put(local);
    g_fakeAddr = addr;
    g_addrCount = 1;
    Stub stub;
    utStubDbIterators(stub);
    stub.set(socket, stub_socket_fail);
    g_linkCount = 0;

    // Act
    m_tester->update();

    // Assert
    QList<INet4Addr> result = m_tester->m_addrIpv4DB.values(42);
    EXPECT_EQ(result.size(), 1);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.first()->addr, QByteArray("10.0.0.5"));
}
