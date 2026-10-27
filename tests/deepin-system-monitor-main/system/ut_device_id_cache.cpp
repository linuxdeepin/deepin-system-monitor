// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::DeviceIDCache (deepin-system-monitor-main/system/device_id_cache.*).
// The libudev hardware database is stubbed (udev_new / udev_hwdb_new /
// udev_hwdb_unref), so no real udev context is required.
//
// Known implementation defect (not exercised here): DeviceIDCache::deviceName()
// is declared in device_id_cache.h:24 but has no implementation in
// device_id_cache.cpp - calling it is a link error. Recorded in defects.json.

#include <gtest/gtest.h>

#include <QByteArray>

#include <libudev.h>

#include "system/device_id_cache.h"
#include "system/udev.h"
#include "ut_stub_helpers.h"

using stub_ext::StubExt;

namespace {

// Sentinel pointers that substitute for real libudev handles.
static struct udev *kFakeUdev = reinterpret_cast<struct udev *>(0x0BAD0001);
static struct udev_hwdb *kFakeHwdb = reinterpret_cast<struct udev_hwdb *>(0x0BAD0002);

class UT_DeviceIDCache : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_hwdbNewCalls = 0;
        m_hwdbUnrefCalls = 0;
        m_udevUnrefCalls = 0;

        // udev_new returns the sentinel; udev_unref records the call.
        using UdevNewFn = struct udev *(*)(void);
        m_stub.set_lamda(static_cast<UdevNewFn>(&udev_new),
                         [](void) -> struct udev * {
                             return kFakeUdev;
                         });
        using UdevUnrefFn = struct udev *(*)(struct udev *);
        auto *udevUnrefCalls = &m_udevUnrefCalls;
        m_stub.set_lamda(static_cast<UdevUnrefFn>(&udev_unref),
                         [udevUnrefCalls](struct udev *ctx) -> struct udev * {
                             ++(*udevUnrefCalls);
                             return ctx;
                         });

        // udev_hwdb_new returns the sentinel; udev_hwdb_unref records the call.
        using HwdbNewFn = struct udev_hwdb *(*)(struct udev *);
        auto *hwdbNewCalls = &m_hwdbNewCalls;
        m_stub.set_lamda(static_cast<HwdbNewFn>(&udev_hwdb_new),
                         [hwdbNewCalls](struct udev *) -> struct udev_hwdb * {
                             ++(*hwdbNewCalls);
                             return kFakeHwdb;
                         });
        using HwdbUnrefFn = struct udev_hwdb *(*)(struct udev_hwdb *);
        auto *hwdbUnrefCalls = &m_hwdbUnrefCalls;
        m_stub.set_lamda(static_cast<HwdbUnrefFn>(&udev_hwdb_unref),
                         [hwdbUnrefCalls](struct udev_hwdb *hwdb) -> struct udev_hwdb * {
                             ++(*hwdbUnrefCalls);
                             return hwdb;
                         });
    }

    void TearDown() override
    {
    }

    StubExt m_stub;
    int m_hwdbNewCalls = 0;
    int m_hwdbUnrefCalls = 0;
    int m_udevUnrefCalls = 0;
};

// ---------------------------------------------------------------------------
// Constructor initialises the hwdb handle from the udev context once.
// Branch: DeviceIDCache ctor -> udev_hwdb_new(udev->handle()).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Construction_StoresHwdbHandle)
{
    // Act
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);

    // Assert: the handle came from our stub and was requested exactly once
    EXPECT_EQ(1, m_hwdbNewCalls);
    EXPECT_EQ(kFakeHwdb, cache.m_hwdb);
}

// ---------------------------------------------------------------------------
// Destructor releases the hwdb handle exactly once.
// Branch: ~DeviceIDCache -> udev_hwdb_unref(m_hwdb).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Destruction_UnrefsHwdbOnce)
{
    // Arrange
    {
        core::system::UDev udev;
        core::system::DeviceIDCache cache(&udev);
        EXPECT_EQ(0, m_hwdbUnrefCalls);
    }

    // Assert
    EXPECT_EQ(1, m_hwdbUnrefCalls);
}

// ---------------------------------------------------------------------------
// Full lifecycle balance: udev context created once and released on UDev
// destruction; hwdb created once and released once.
// Branch: UDev ctor/dtor + DeviceIDCache ctor/dtor pairing.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Lifecycle_NewAndUnrefBalanced)
{
    // Act
    {
        core::system::UDev udev;
        core::system::DeviceIDCache cache(&udev);
    }

    // Assert
    EXPECT_EQ(1, m_hwdbNewCalls);
    EXPECT_EQ(1, m_hwdbUnrefCalls);
    EXPECT_EQ(1, m_udevUnrefCalls);
}

// ---------------------------------------------------------------------------
// udev_hwdb_new returning null is tolerated by construction/destruction
// (the implementation performs no null check - recorded as a low-severity
// defect; the stubbed unref makes the destructor a safe no-op).
// Branch: m_hwdb == nullptr -> dtor still calls udev_hwdb_unref(NULL).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Construction_HwdbNull_Tolerated)
{
    // Arrange
    using HwdbNewFn = struct udev_hwdb *(*)(struct udev *);
    m_stub.set_lamda(static_cast<HwdbNewFn>(&udev_hwdb_new),
                     [](struct udev *) -> struct udev_hwdb * {
                         return nullptr;
                     });

    // Act
    {
        core::system::UDev udev;
        core::system::DeviceIDCache cache(&udev);
        EXPECT_EQ(nullptr, cache.m_hwdb);
    }

    // Assert: destructor ran without crashing
    EXPECT_EQ(1, m_hwdbUnrefCalls);
}

// ---------------------------------------------------------------------------
// deviceName() is declared but not implemented (link-time defect); the test
// suite therefore compiles a reference comment only. See defects.json entry
// for device_id_cache.cpp.
// ---------------------------------------------------------------------------

} // namespace
