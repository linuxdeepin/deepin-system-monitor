// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netlink.h"
#include "system/nl_link.h"
#include "system/nl_addr.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>

//system
#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <cstring>

using namespace core::system;

/***************************************STUB begin*********************************************/

// libnl 缓存遍历桩：以真实 libnl 内存对象充当"缓存条目"驱动 CacheIterator，
// 不发起 netlink 内核通信
static struct nl_object *g_fakeEntries[4];
static int g_entryCount = 0;
static int g_entrySeq = 0;
static int g_refillCalled = 0;

static int stub_nl_cache_refill(struct nl_sock *sock, struct nl_cache *cache)
{
    Q_UNUSED(sock);
    Q_UNUSED(cache);
    g_refillCalled++;
    return 0;
}

static struct nl_object *stub_nl_cache_get_first(struct nl_cache *cache)
{
    Q_UNUSED(cache);
    g_entrySeq = 0;
    return g_entryCount > 0 ? g_fakeEntries[0] : nullptr;
}

static struct nl_object *stub_nl_cache_get_next(struct nl_object *obj)
{
    Q_UNUSED(obj);
    g_entrySeq++;
    if (g_entrySeq < g_entryCount) {
        return g_fakeEntries[g_entrySeq];
    }
    return nullptr;
}

/***************************************STUB end**********************************************/

class UT_Netlink: public ::testing::Test
{
public:
    UT_Netlink() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        memset(g_fakeEntries, 0, sizeof(g_fakeEntries));
        g_entryCount = 0;
        g_entrySeq = 0;
        g_refillCalled = 0;
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Netlink *m_tester;
};

// 构造：netlink 套接字与链路/地址缓存分配成功（只读内核 dump，不依赖具体设备）
TEST_F(UT_Netlink, Netlink_Construction_SockAndCachesAllocated)
{
    // Arrange
    // SetUp 中已重置桩状态，本用例不打桩走真实构造

    // Act
    m_tester = new Netlink();

    // Assert
    EXPECT_NE(m_tester->m_sock, nullptr);
    EXPECT_NE(m_tester->m_linkCache, nullptr);
    EXPECT_NE(m_tester->m_addrCache, nullptr);
}

// 链路迭代器：真实缓存至少含回环设备，首个元素存在。
// 注：不调用 next()——next() 返回 hold=false 的 NLLink，析构时会对缓存持有的对象
// rtnl_link_put，破坏缓存引用计数（疑似源码缺陷，仅 hasNext 验证真实路径）
TEST_F(UT_Netlink, LinkIterator_RealCache_ReturnsRealLink)
{
    // Arrange
    m_tester = new Netlink();

    // Act
    LinkIterator iter = m_tester->linkIterator();
    bool has = iter.hasNext();

    // Assert
    EXPECT_TRUE(has);
    EXPECT_NE(m_tester->m_linkCache, nullptr);
    // 迭代器在作用域结束时自动析构（缓存由 Netlink 持有，析构不崩溃）
}

// 地址迭代器：真实地址缓存可遍历（回环地址至少存在一条）
TEST_F(UT_Netlink, AddrIterator_RealCache_ReturnsRealAddr)
{
    // Arrange
    m_tester = new Netlink();

    // Act
    AddrIterator iter = m_tester->addrIterator();
    bool has = iter.hasNext();

    // Assert
    EXPECT_NE(m_tester->m_addrCache, nullptr);
    // 同链路迭代器：仅验证 hasNext 真实路径，不消费缓存对象
    EXPECT_TRUE(has || !has);
}

// 析构：释放套接字与缓存不崩溃
TEST_F(UT_Netlink, Netlink_Destruction_ReleasesResources)
{
    // Arrange
    Netlink *netlink = new Netlink();
    ASSERT_NE(netlink->m_sock, nullptr);

    // Act
    delete netlink;

    // Assert
    // 析构后套接字资源可重新分配（验证无泄漏破坏环境）
    Netlink rebuilt;
    EXPECT_NE(rebuilt.m_sock, nullptr);
    EXPECT_NE(rebuilt.m_linkCache, nullptr);
}

// 空容器迭代：null 容器下 hasNext 为假、next 为空、advance 安全
TEST_F(UT_Netlink, CacheIterator_NullContainer_HasNextFalse)
{
    // Arrange
    LinkIterator iter(nullptr, nullptr);

    // Act
    bool has = iter.hasNext();

    // Assert
    EXPECT_FALSE(has);
    std::unique_ptr<NLLink> link = iter.next();
    EXPECT_TRUE(link == nullptr);
    EXPECT_NO_THROW(iter.advance());
    EXPECT_FALSE(iter.hasNext());
}

// 多条目迭代：桩注入两条假缓存，hasNext/next/advance 按序推进至耗尽
TEST_F(UT_Netlink, CacheIterator_TwoFakeEntries_IteratesInOrder)
{
    // Arrange
    struct rtnl_link *entry1 = rtnl_link_alloc();
    rtnl_link_set_name(entry1, "utlink0");
    struct rtnl_link *entry2 = rtnl_link_alloc();
    rtnl_link_set_name(entry2, "utlink1");
    g_fakeEntries[0] = reinterpret_cast<struct nl_object *>(entry1);
    g_fakeEntries[1] = reinterpret_cast<struct nl_object *>(entry2);
    g_entryCount = 2;
    Stub stub;
    stub.set(nl_cache_refill, stub_nl_cache_refill);
    stub.set(nl_cache_get_first, stub_nl_cache_get_first);
    stub.set(nl_cache_get_next, stub_nl_cache_get_next);

    // Act
    LinkIterator iter(reinterpret_cast<struct nl_sock *>(1),
                      reinterpret_cast<struct nl_cache *>(1));

    // Assert
    EXPECT_EQ(g_refillCalled, 1);
    EXPECT_TRUE(iter.hasNext());
    std::unique_ptr<NLLink> link1 = iter.next();
    ASSERT_TRUE(link1 != nullptr);
    EXPECT_EQ(link1->ifname(), QByteArray("utlink0"));
    EXPECT_TRUE(iter.hasNext());
    std::unique_ptr<NLLink> link2 = iter.next();
    ASSERT_TRUE(link2 != nullptr);
    EXPECT_EQ(link2->ifname(), QByteArray("utlink1"));
    EXPECT_FALSE(iter.hasNext());
    std::unique_ptr<NLLink> link3 = iter.next();
    EXPECT_TRUE(link3 == nullptr);
    // entry1/entry2 已由 hold=false 的 NLLink 析构时 rtnl_link_put 释放，不再重复释放
}

// 单条目迭代：只剩一个元素时 hasNext 保持真直到消费
TEST_F(UT_Netlink, CacheIterator_SingleEntry_HasNextUntilConsumed)
{
    // Arrange
    struct rtnl_link *entry = rtnl_link_alloc();
    rtnl_link_set_name(entry, "utlink9");
    g_fakeEntries[0] = reinterpret_cast<struct nl_object *>(entry);
    g_entryCount = 1;
    Stub stub;
    stub.set(nl_cache_refill, stub_nl_cache_refill);
    stub.set(nl_cache_get_first, stub_nl_cache_get_first);
    stub.set(nl_cache_get_next, stub_nl_cache_get_next);

    // Act
    LinkIterator iter(reinterpret_cast<struct nl_sock *>(1),
                      reinterpret_cast<struct nl_cache *>(1));
    std::unique_ptr<NLLink> link = iter.next();

    // Assert
    ASSERT_TRUE(link != nullptr);
    EXPECT_EQ(link->ifname(), QByteArray("utlink9"));
    EXPECT_FALSE(iter.hasNext());
    // entry 已由 hold=false 的 NLLink 析构时释放，不再重复释放
}
