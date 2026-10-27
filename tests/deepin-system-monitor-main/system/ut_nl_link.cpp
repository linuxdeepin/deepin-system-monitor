// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::NLLink (deepin-system-monitor-main/system/nl_link.*).
// Exercises the libnl wrapper against real in-process rtnl_link objects
// created with rtnl_link_alloc + setters (no kernel/netlink traffic).

#include <gtest/gtest.h>

#include <QByteArray>

#include <netlink/object.h>
#include <netlink/route/link.h>

#include "system/nl_link.h"

namespace {

class UT_NLLink : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_link = rtnl_link_alloc();
        ASSERT_NE(nullptr, m_link);
        rtnl_link_set_ifindex(m_link, 2);
        rtnl_link_set_name(m_link, "wlan0");
        rtnl_link_set_mtu(m_link, 1500);
        rtnl_link_set_flags(m_link, 0x1 | 0x2 | 0x40);   // up | broadcast | running
        rtnl_link_set_txqlen(m_link, 1000);
        rtnl_link_set_arptype(m_link, 1);
        rtnl_link_set_carrier(m_link, 1);
        rtnl_link_set_operstate(m_link, 6);              // IF_OPER_UP
        rtnl_link_set_linkmode(m_link, 1);

        struct nl_addr *hw = nullptr;
        ASSERT_EQ(0, nl_addr_parse("aa:bb:cc:dd:ee:ff", AF_LLC, &hw));
        rtnl_link_set_addr(m_link, hw);

        struct nl_addr *bc = nullptr;
        ASSERT_EQ(0, nl_addr_parse("ff:ff:ff:ff:ff:ff", AF_LLC, &bc));
        rtnl_link_set_broadcast(m_link, bc);

        rtnl_link_set_ifalias(m_link, "alias0");

        rtnl_link_set_stat(m_link, RTNL_LINK_RX_PACKETS, 1000);
        rtnl_link_set_stat(m_link, RTNL_LINK_RX_BYTES, 200000);
        rtnl_link_set_stat(m_link, RTNL_LINK_RX_ERRORS, 3);
        rtnl_link_set_stat(m_link, RTNL_LINK_RX_DROPPED, 4);
        rtnl_link_set_stat(m_link, RTNL_LINK_RX_FIFO_ERR, 5);
        rtnl_link_set_stat(m_link, RTNL_LINK_RX_FRAME_ERR, 6);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_PACKETS, 700);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_BYTES, 90000);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_ERRORS, 8);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_DROPPED, 9);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_FIFO_ERR, 10);
        rtnl_link_set_stat(m_link, RTNL_LINK_TX_CARRIER_ERR, 11);
        rtnl_link_set_stat(m_link, RTNL_LINK_COLLISIONS, 12);
    }

    void TearDown() override
    {
        if (m_link)
            rtnl_link_put(m_link);
    }

    struct rtnl_link *m_link = nullptr;
};

// ---------------------------------------------------------------------------
// Wrapper without hold: destruction must not release the caller's reference.
// Branch: ~NLLink with m_hold == false -> no rtnl_link_put.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, Dtor_WithoutHold_ReferenceCountUnchanged)
{
    // Arrange
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_link)));

    // Act
    {
        core::system::NLLink wrapper(m_link, false);
        EXPECT_EQ(2, wrapper.ifindex());   // touch the wrapped object
    }

    // Assert: object still alive, refcount untouched
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_link)));
    EXPECT_STREQ("wlan0", rtnl_link_get_name(m_link));
}

// ---------------------------------------------------------------------------
// Wrapper with hold: destruction releases exactly one reference.
// Branch: ~NLLink with m_hold == true -> rtnl_link_put(m_link).
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, Dtor_WithHold_ReleasesOneReference)
{
    // Arrange: take an extra reference so the wrapper's put does not free
    nl_object_get(reinterpret_cast<struct nl_object *>(m_link));
    EXPECT_EQ(2, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_link)));

    // Act
    {
        core::system::NLLink wrapper(m_link, true);
    }

    // Assert
    EXPECT_EQ(1, nl_object_get_refcnt(reinterpret_cast<struct nl_object *>(m_link)));
}

// ---------------------------------------------------------------------------
// Scalar getters round-trip the values written through the libnl setters.
// Branches: every inline getter delegates to the corresponding rtnl_link_get_*.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, Getters_RoundTripLibnlValues)
{
    // Arrange
    core::system::NLLink wrapper(m_link);

    // Act / Assert
    EXPECT_EQ(2, wrapper.ifindex());
    EXPECT_EQ(QByteArray("wlan0"), wrapper.ifname());
    EXPECT_EQ(1500u, wrapper.mtu());
    EXPECT_EQ(0x43u, wrapper.flags());
    EXPECT_EQ(1u, wrapper.arp_type());
    EXPECT_EQ(1000u, wrapper.txqlen());
    EXPECT_EQ(1u, wrapper.carrier());
    EXPECT_EQ(6u, wrapper.oper_stat());
    EXPECT_EQ(1u, wrapper.link_mode());
    EXPECT_EQ(QByteArray("aa:bb:cc:dd:ee:ff"), wrapper.addr());
    EXPECT_EQ(QByteArray("ff:ff:ff:ff:ff:ff"), wrapper.bcast());
}

// ---------------------------------------------------------------------------
// Statistics getters round-trip the rtnl counters.
// Branches: rx_*/tx_*/collisions inline getters over rtnl_link_get_stat.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, Stats_Getters_ReturnCounterValues)
{
    // Arrange
    core::system::NLLink wrapper(m_link);

    // Act / Assert
    EXPECT_EQ(1000ull, wrapper.rx_packets());
    EXPECT_EQ(200000ull, wrapper.rx_bytes());
    EXPECT_EQ(3ull, wrapper.rx_errors());
    EXPECT_EQ(4ull, wrapper.rx_dropped());
    EXPECT_EQ(5ull, wrapper.rx_fifo());
    EXPECT_EQ(6ull, wrapper.rx_frame());
    EXPECT_EQ(700ull, wrapper.tx_packets());
    EXPECT_EQ(90000ull, wrapper.tx_bytes());
    EXPECT_EQ(8ull, wrapper.tx_errors());
    EXPECT_EQ(9ull, wrapper.tx_dropped());
    EXPECT_EQ(10ull, wrapper.tx_fifo());
    EXPECT_EQ(11ull, wrapper.tx_carrier());
    EXPECT_EQ(12ull, wrapper.collisions());
}

// ---------------------------------------------------------------------------
// carrier_changes: locally built links carry no carrier-change attribute, so
// rtnl_link_get_carrier_changes fails and the getter returns 0 (libnl does
// not offer a setter for this attribute).
// Branch: rtnl_link_get_carrier_changes != 0 -> cnt stays 0.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, CarrierChanges_WithoutAttribute_ReturnsZero)
{
    // Arrange
    core::system::NLLink wrapper(m_link);

    // Act / Assert
    EXPECT_EQ(0u, wrapper.carrier_changes());
}

// ---------------------------------------------------------------------------
// alias(): wraps rtnl_link_get_ifalias.
// Branch: ifalias set -> formatted name returned.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, Alias_ReturnsLibnlAlias)
{
    // Arrange
    core::system::NLLink wrapper(m_link);

    // Act / Assert
    EXPECT_EQ(QByteArray("alias0"), wrapper.alias());
}

// ---------------------------------------------------------------------------
// formatFlags: libnl renders known flag bits as a comma separated lower-case
// list, the wrapper upper-cases it and appends the numeric form.
// Branches: flags -> QByteArray::number(flags) + '<' + flags2str().toUpper() + '>'.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, FormatFlags_KnownBits_NumberPlusUppercaseNames)
{
    // Act
    const QByteArray formatted = core::system::NLLink::formatFlags(0x43);

    // Assert (libnl 3.7: "broadcast,up,running"; buffer is NUL padded to 256)
    EXPECT_TRUE(formatted.startsWith("67<BROADCAST,UP,RUNNING"));
    EXPECT_TRUE(formatted.endsWith('>'));
}

// ---------------------------------------------------------------------------
// formatFlags boundary: zero flags render as "0<>".
// Branch: flags2str(0) writes nothing -> empty upper-case part.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, FormatFlags_ZeroFlags_EmptyNamePart)
{
    // Act
    const QByteArray formatted = core::system::NLLink::formatFlags(0);

    // Assert
    EXPECT_TRUE(formatted.startsWith("0<"));
    EXPECT_TRUE(formatted.endsWith('>'));
}

// ---------------------------------------------------------------------------
// formatCarrier: libnl maps 1 -> "up", 0 -> "down", unknown values to hex
// form. The buffer is 16 bytes NUL padded.
// Branches: carrier2str hit / miss.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, FormatCarrier_KnownValues_UpAndDown)
{
    // Act
    const QByteArray up = core::system::NLLink::formatCarrier(1);
    const QByteArray down = core::system::NLLink::formatCarrier(0);

    // Assert (NUL-padded 16 byte buffers)
    QByteArray expectedUp(16, '\0');
    memcpy(expectedUp.data(), "up", 2);
    EXPECT_EQ(expectedUp, up);

    QByteArray expectedDown(16, '\0');
    memcpy(expectedDown.data(), "down", 4);
    EXPECT_EQ(expectedDown, down);
}

TEST_F(UT_NLLink, FormatCarrier_UnknownValue_FallsBackToHex)
{
    // Act
    const QByteArray unknown = core::system::NLLink::formatCarrier(9);

    // Assert
    QByteArray expected(16, '\0');
    memcpy(expected.data(), "0x9", 3);
    EXPECT_EQ(expected, unknown);
}

// ---------------------------------------------------------------------------
// formatOperState: libnl 3.7 maps 6 -> "up", 0 -> "unknown"; unknown values
// fall back to hex. Buffer is 256 bytes NUL padded.
// Branches: operstate2str hit / miss.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, FormatOperState_Up_MatchesLibnlName)
{
    // Act
    const QByteArray up = core::system::NLLink::formatOperState(6);
    const QByteArray unknownState = core::system::NLLink::formatOperState(0);

    // Assert
    QByteArray expectedUp(256, '\0');
    memcpy(expectedUp.data(), "up", 2);
    EXPECT_EQ(expectedUp, up);

    QByteArray expectedUnknown(256, '\0');
    memcpy(expectedUnknown.data(), "unknown", 7);
    EXPECT_EQ(expectedUnknown, unknownState);
}

TEST_F(UT_NLLink, FormatOperState_UnknownValue_FallsBackToHex)
{
    // Act
    const QByteArray hex = core::system::NLLink::formatOperState(99);

    // Assert
    QByteArray expected(256, '\0');
    memcpy(expected.data(), "0x63", 4);
    EXPECT_EQ(expected, hex);
}

// ---------------------------------------------------------------------------
// formatLinkMode: libnl maps 1 -> "dormant", 0 -> "default". Buffer is 16
// bytes NUL padded.
// Branches: mode2str hit paths.
// ---------------------------------------------------------------------------
TEST_F(UT_NLLink, FormatLinkMode_KnownValues_DormantAndDefault)
{
    // Act
    const QByteArray dormant = core::system::NLLink::formatLinkMode(1);
    const QByteArray deflt = core::system::NLLink::formatLinkMode(0);

    // Assert
    QByteArray expectedDormant(16, '\0');
    memcpy(expectedDormant.data(), "dormant", 7);
    EXPECT_EQ(expectedDormant, dormant);

    QByteArray expectedDefault(16, '\0');
    memcpy(expectedDefault.data(), "default", 7);
    EXPECT_EQ(expectedDefault, deflt);
}

} // namespace
