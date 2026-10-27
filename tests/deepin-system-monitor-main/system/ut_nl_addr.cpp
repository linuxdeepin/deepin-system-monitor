// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::NLAddr (deepin-system-monitor-main/system/nl_addr.*).
// Exercises the libnl address wrapper against real in-process rtnl_addr
// objects created with rtnl_addr_alloc + setters (no kernel traffic).
//
// Known implementation defect (not exercised here): NLAddr::local() computes
// the netmask via LMOVE(0xffffffff, 32 - n); for prefix length 0 this
// performs a 32-bit shift by 32, which is undefined behaviour. Prefix /0 is
// therefore deliberately not fed to local().

#include <gtest/gtest.h>

#include <QByteArray>

#include <netlink/addr.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>

#include "system/nl_addr.h"

namespace {

class UT_NLAddr : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_addr = rtnl_addr_alloc();
        ASSERT_NE(nullptr, m_addr);

        // scalar attributes first: libnl binds peer/broadcast to the
        // prefixlen/family at setter time
        rtnl_addr_set_ifindex(m_addr, 2);
        rtnl_addr_set_family(m_addr, AF_INET);
        rtnl_addr_set_prefixlen(m_addr, 24);
        rtnl_addr_set_scope(m_addr, 0);   // RT_SCOPE_UNIVERSE

        struct nl_addr *local = nullptr;
        ASSERT_EQ(0, nl_addr_parse("192.168.1.100/24", AF_INET, &local));
        rtnl_addr_set_local(m_addr, local);

        struct nl_addr *peer = nullptr;
        // peer must carry the interface prefix: rtnl_addr_set_peer adopts
        // the peer address's prefixlen into the rtnl_addr
        ASSERT_EQ(0, nl_addr_parse("192.168.1.1/24", AF_INET, &peer));
        rtnl_addr_set_peer(m_addr, peer);

        struct nl_addr *bc = nullptr;
        ASSERT_EQ(0, nl_addr_parse("192.168.1.255", AF_INET, &bc));
        rtnl_addr_set_broadcast(m_addr, bc);

        // multicast/anycast attributes are only accepted on IPv6 addresses
        m_addr6 = rtnl_addr_alloc();
        ASSERT_NE(nullptr, m_addr6);
        rtnl_addr_set_ifindex(m_addr6, 2);
        rtnl_addr_set_family(m_addr6, AF_INET6);
        rtnl_addr_set_prefixlen(m_addr6, 64);

        struct nl_addr *local6 = nullptr;
        ASSERT_EQ(0, nl_addr_parse("2001:db8::1/64", AF_INET6, &local6));
        rtnl_addr_set_local(m_addr6, local6);

        struct nl_addr *mc = nullptr;
        ASSERT_EQ(0, nl_addr_parse("ff02::1", AF_INET6, &mc));
        rtnl_addr_set_multicast(m_addr6, mc);

        struct nl_addr *ac = nullptr;
        ASSERT_EQ(0, nl_addr_parse("2001:db8::9", AF_INET6, &ac));
        rtnl_addr_set_anycast(m_addr6, ac);
    }

    void TearDown() override
    {
        if (m_addr)
            rtnl_addr_put(m_addr);
        if (m_addr6)
            rtnl_addr_put(m_addr6);
    }

    struct rtnl_addr *m_addr = nullptr;
    struct rtnl_addr *m_addr6 = nullptr;
};

// ---------------------------------------------------------------------------
// Wrapper without hold: destruction keeps the caller's reference alive.
// Branch: ~NLAddr with m_hold == false -> no rtnl_addr_put.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Dtor_WithoutHold_ReferenceCountUnchanged)
{
    // Arrange
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_addr)));

    // Act
    {
        core::system::NLAddr wrapper(m_addr);
        EXPECT_EQ(2, wrapper.ifindex());   // touch the wrapped object
    }

    // Assert
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_addr)));
    EXPECT_EQ(24, rtnl_addr_get_prefixlen(m_addr));
}

// ---------------------------------------------------------------------------
// Wrapper with hold: destruction releases exactly one reference.
// Branch: ~NLAddr with m_hold == true -> rtnl_addr_put(m_addr).
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Dtor_WithHold_ReleasesOneReference)
{
    // Arrange
    nl_object_get(reinterpret_cast<struct nl_object *>(m_addr));
    EXPECT_EQ(2, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_addr)));

    // Act
    {
        core::system::NLAddr wrapper(m_addr, true);
    }

    // Assert
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_addr)));
}

// ---------------------------------------------------------------------------
// Scalar getters round-trip libnl values.
// Branches: ifindex/family/prefixlen/scope inline getters.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Getters_RoundTripLibnlValues)
{
    // Arrange
    core::system::NLAddr wrapper(m_addr);

    // Act / Assert
    EXPECT_EQ(2, wrapper.ifindex());
    EXPECT_EQ(AF_INET, wrapper.family());
    EXPECT_EQ(24, wrapper.prefixlen());
    EXPECT_EQ(0, wrapper.scope());
}

// ---------------------------------------------------------------------------
// local(): splits "192.168.1.100/24" into netaddr "192.168.1.100" and
// netmask "255.255.255.0".
// Branch: split('/') size == 2 -> mask computed via LMOVE/RMOVE + sprintf.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Local_Ipv4WithPrefix_SplitsNetaddrAndNetmask)
{
    // Arrange
    core::system::NLAddr wrapper(m_addr);

    // Act
    wrapper.local();

    // Assert
    EXPECT_EQ(QByteArray("192.168.1.100"), wrapper.netaddr());
    EXPECT_EQ(QByteArray("255.255.255.0"), wrapper.netmask());
}

// ---------------------------------------------------------------------------
// local() boundary: /32 host route. libnl's nl_addr2str omits the prefix
// suffix for a full-length prefix, so the implementation's split('/') finds
// no '/' and the netmask member stays empty while netaddr keeps the full
// string.
// Branch: arrayBuffer.size() == 2 false -> netAddr = buffer, netMask untouched.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Local_Prefix32_HostRoute_NetmaskStaysEmpty)
{
    // Arrange
    struct rtnl_addr *addr32 = rtnl_addr_alloc();
    ASSERT_NE(nullptr, addr32);
    rtnl_addr_set_family(addr32, AF_INET);
    rtnl_addr_set_prefixlen(addr32, 32);
    struct nl_addr *local = nullptr;
    ASSERT_EQ(0, nl_addr_parse("10.20.30.40/32", AF_INET, &local));
    rtnl_addr_set_local(addr32, local);

    core::system::NLAddr wrapper(addr32);

    // Act
    wrapper.local();

    // Assert
    EXPECT_EQ(QByteArray("10.20.30.40"), wrapper.netaddr());
    EXPECT_EQ(QByteArray(), wrapper.netmask());

    rtnl_addr_put(addr32);
}

// ---------------------------------------------------------------------------
// local() boundary: /16 -> mask 255.255.0.0.
// Branch: n == 16 -> addr == 0xffff0000.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Local_Prefix16_ClassfulNetmask)
{
    // Arrange
    struct rtnl_addr *addr16 = rtnl_addr_alloc();
    ASSERT_NE(nullptr, addr16);
    struct nl_addr *local = nullptr;
    ASSERT_EQ(0, nl_addr_parse("172.16.5.9/16", AF_INET, &local));
    rtnl_addr_set_local(addr16, local);

    core::system::NLAddr wrapper(addr16);

    // Act
    wrapper.local();

    // Assert
    EXPECT_EQ(QByteArray("172.16.5.9"), wrapper.netaddr());
    EXPECT_EQ(QByteArray("255.255.0.0"), wrapper.netmask());

    rtnl_addr_put(addr16);
}

// ---------------------------------------------------------------------------
// local() without a prefix: nl_addr2str yields no '/' -> split size != 2 ->
// netaddr keeps the full string and netmask stays empty.
// Branch: arrayBuffer.size() == 2 false -> netAddr = buffer, netMask untouched.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Local_NoPrefix_NetmaskStaysEmpty)
{
    // Arrange
    struct rtnl_addr *plain = rtnl_addr_alloc();
    ASSERT_NE(nullptr, plain);
    struct nl_addr *local = nullptr;
    ASSERT_EQ(0, nl_addr_parse("10.0.0.5", AF_INET, &local));
    rtnl_addr_set_local(plain, local);

    core::system::NLAddr wrapper(plain);

    // Act
    wrapper.local();

    // Assert
    EXPECT_EQ(QByteArray("10.0.0.5"), wrapper.netaddr());
    EXPECT_EQ(QByteArray(), wrapper.netmask());

    rtnl_addr_put(plain);
}

// ---------------------------------------------------------------------------
// netaddr()/netmask() before local(): both empty (members never written).
// Branch: fresh wrapper -> empty QByteArray members.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Netaddr_Netmask_BeforeLocal_StayEmpty)
{
    // Arrange
    core::system::NLAddr wrapper(m_addr);

    // Act / Assert
    EXPECT_EQ(QByteArray(), wrapper.netaddr());
    EXPECT_EQ(QByteArray(), wrapper.netmask());
}

// ---------------------------------------------------------------------------
// peer()/broadcast() (IPv4) and multicast()/anycast() (IPv6, the only family
// libnl accepts these attributes on): formatted via nl_addr2str.
// Branches: non-null nl_addr -> buffer filled with canonical form.
// ---------------------------------------------------------------------------
TEST_F(UT_NLAddr, Peer_Broadcast_Multicast_Anycast_RoundTrip)
{
    // Arrange
    core::system::NLAddr wrapper(m_addr);
    core::system::NLAddr wrapper6(m_addr6);

    // Act / Assert
    EXPECT_EQ(QByteArray("192.168.1.1/24"), wrapper.peer());
    EXPECT_EQ(QByteArray("192.168.1.255"), wrapper.broadcast());
    EXPECT_EQ(QByteArray("ff02::1"), wrapper6.multicast());
    EXPECT_EQ(QByteArray("2001:db8::9"), wrapper6.anycast());
}

} // namespace
