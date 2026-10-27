// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::wireless (deepin-system-monitor-main/system/wireless.cpp).
// The NetworkManager DBus world (org.freedesktop.NetworkManager) is fully
// simulated in-process: no system bus traffic, no real network devices.

#include <gtest/gtest.h>

#include "ut_stub_helpers.h"
#include "system/wireless.h"

using stub_ext::StubExt;
using ut_helpers::DBusState;

namespace {

class UT_wireless : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_dbus = DBusState();
        ut_helpers::installDBusStubs(m_stub, m_dbus);
    }

    void TearDown() override
    {
    }

    StubExt m_stub;
    DBusState m_dbus;
};

// ---------------------------------------------------------------------------
// Default constructor: no interface name, nothing read yet.
// Branch: wireless() init list (m_bwireless=false, qualities 0); read_wireless_info
// is NOT invoked from the default ctor.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Construction_DefaultObject_FieldsAtDefaults)
{
    // Act
    core::system::wireless w;

    // Assert
    EXPECT_FALSE(w.is_wireless());
    EXPECT_TRUE(w.essid().isEmpty());
    EXPECT_EQ(0u, w.link_quality());
    EXPECT_EQ(0u, w.signal_levle());
    EXPECT_EQ(0u, w.noise_level());
    // default ctor must not have touched NetworkManager
    EXPECT_TRUE(m_dbus.calledMethods.isEmpty());
}

// ---------------------------------------------------------------------------
// Null interface name: read_wireless_info bails out before any DBus call.
// Branch: m_ifname.isNull() == true -> warn + m_bwireless=false + return false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Construction_WithNullIfname_ReadFailsEarly)
{
    // Act
    core::system::wireless w(QByteArray{});

    // Assert
    EXPECT_FALSE(w.is_wireless());
    EXPECT_EQ(0u, w.link_quality());
    EXPECT_TRUE(w.essid().isEmpty());
    // early return must not reach the NetworkManager interface
    EXPECT_TRUE(m_dbus.calledMethods.isEmpty());
}

// ---------------------------------------------------------------------------
// Empty but non-null interface name passes the null guard and reaches DBus;
// with an invalid NetworkManager the read fails.
// Branch: isNull() false -> QDBusInterface constructed -> isValid() false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Construction_WithEmptyIfname_NMInvalid_ReturnsFalse)
{
    // Arrange
    m_dbus.valid = false;

    // Act
    core::system::wireless w(QByteArray(""));

    // Assert
    EXPECT_FALSE(w.is_wireless());
    EXPECT_EQ(0u, w.link_quality());
    EXPECT_TRUE(m_dbus.calledMethods.isEmpty());   // fails before GetDevices
}

// ---------------------------------------------------------------------------
// read_wireless_info with invalid NM interface.
// Branch: nm.isValid() == false -> m_bwireless = false -> return false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_NMInvalid_ReturnsFalse)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.valid = false;

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(w.is_wireless());
}

// ---------------------------------------------------------------------------
// read_wireless_info when GetDevices reply is a DBus error.
// Branch: devices.isValid() == false -> m_bwireless=false -> return false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_GetDevicesFails_ReturnsFalse)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.nmGetDevicesOk = false;

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(w.is_wireless());
    EXPECT_TRUE(m_dbus.calledMethods.contains("GetDevices"));
}

// ---------------------------------------------------------------------------
// No device matches the requested interface name.
// Branch: property("Interface") != m_ifname -> continue loop -> fallthrough
// m_bwireless = false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_NoMatchingDevice_ReturnsFalse)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.devicePaths = QStringList { "/org/freedesktop/NetworkManager/Devices/0" };
    m_dbus.deviceInterface = "eth9";

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(w.is_wireless());
    EXPECT_TRUE(m_dbus.calledMethods.contains("GetDevices"));
    EXPECT_FALSE(m_dbus.calledMethods.contains("GetAccessPoints"));
}

// ---------------------------------------------------------------------------
// Device matches but exposes no access points.
// Branches: Interface matches -> GetAccessPoints -> reply list empty ->
// continue -> fallthrough false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_NoAccessPoints_ReturnsFalse)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.devicePaths = QStringList { "/org/freedesktop/NetworkManager/Devices/1" };
    m_dbus.deviceInterface = "wlan0";
    m_dbus.apPaths.clear();

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(w.is_wireless());
    EXPECT_TRUE(m_dbus.calledMethods.contains("GetAccessPoints"));
}

// ---------------------------------------------------------------------------
// GetAccessPoints answers with an error reply.
// Branch: activeAp.isValid() == false -> continue -> fallthrough false.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_GetAccessPointsFails_ReturnsFalse)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.devicePaths = QStringList { "/org/freedesktop/NetworkManager/Devices/1" };
    m_dbus.deviceInterface = "wlan0";
    m_dbus.nmGetApsOk = false;

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_FALSE(w.is_wireless());
}

// ---------------------------------------------------------------------------
// Full success path: device + AP found, Ssid/Strength parsed into members.
// Branches: all guards pass -> m_essid = Ssid; m_link_quality = m_signal_level
// = Strength; m_bwireless = true -> return true.
// NOTE: m_noise_level is never assigned by the implementation (recorded as a
// defect); it stays 0.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_ApInfoPresent_MembersUpdated)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.devicePaths = QStringList { "/org/freedesktop/NetworkManager/Devices/1" };
    m_dbus.deviceInterface = "wlan0";
    m_dbus.apPaths = QStringList { "/org/freedesktop/NetworkManager/AccessPoint/10" };
    m_dbus.apSsid = "Deepin-WiFi";
    m_dbus.apStrength = 77;

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(w.is_wireless());
    EXPECT_EQ(QByteArray("Deepin-WiFi"), w.essid());
    EXPECT_EQ(77u, w.link_quality());
    EXPECT_EQ(77u, w.signal_levle());
    EXPECT_EQ(0u, w.noise_level());   // never written by implementation
}

// ---------------------------------------------------------------------------
// Boundary: Strength of 0 propagates to quality/signal.
// Branch: property Strength -> toUInt() == 0.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Read_wireless_info_ZeroStrength_ZeroQualityAndSignal)
{
    // Arrange
    core::system::wireless w(QByteArray("wlan0"));
    m_dbus.devicePaths = QStringList { "/org/freedesktop/NetworkManager/Devices/1" };
    m_dbus.deviceInterface = "wlan0";
    m_dbus.apPaths = QStringList { "/org/freedesktop/NetworkManager/AccessPoint/10" };
    m_dbus.apStrength = 0;

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(0u, w.link_quality());
    EXPECT_EQ(0u, w.signal_levle());
    EXPECT_EQ(QByteArray("Deepin-WiFi"), w.essid());
}

// ---------------------------------------------------------------------------
// Getter equivalence classes after a failed read: getters keep returning the
// construction defaults.
// Branch: m_bwireless stays false; getters read untouched members.
// ---------------------------------------------------------------------------
TEST_F(UT_wireless, Getters_AfterFailedRead_KeepDefaults)
{
    // Arrange
    m_dbus.valid = false;
    core::system::wireless w(QByteArray("wlan0"));

    // Act
    const bool ok = w.read_wireless_info();

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(w.essid().isEmpty());
    EXPECT_EQ(0u, w.link_quality());
    EXPECT_EQ(0u, w.signal_levle());
    EXPECT_EQ(0u, w.noise_level());
}

} // namespace
