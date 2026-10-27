// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/udev_device.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//c
#include <libudev.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

// 伪造句柄（stub 掉 libudev 后永不解引用；使用伪造句柄的对象必须在 Stub 作用域内释放）
static const uintptr_t kFakeDevice = 0x1000;
static const uintptr_t kFakeParent = 0x2000;

// libudev 属性读取打桩：返回受控字符串
static const char *stub_get_devpath(udev_device *) { return "/devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda"; }
static const char *stub_get_subsystem(udev_device *) { return "block"; }
static const char *stub_get_syspath(udev_device *) { return "/sys/devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda"; }
static const char *stub_get_sysname(udev_device *) { return "sda"; }
static const char *stub_get_devtype(udev_device *) { return "disk"; }
static const char *stub_get_sysattr_value(udev_device *, const char *) { return "512"; }
static const char *stub_get_property_value(udev_device *, const char *) { return "/dev/sda"; }

// 父设备查找打桩
static int g_parentLookups = 0;
static bool g_parentFound = true;
static udev_device *stub_get_parent(udev_device *, const char *, const char *)
{
    ++g_parentLookups;
    return g_parentFound ? reinterpret_cast<udev_device *>(kFakeParent) : nullptr;
}

// 引用计数打桩
static int g_devRefs = 0;
static udev_device *stub_device_ref(udev_device *dev)
{
    ++g_devRefs;
    return dev;
}

static int g_devUnrefs = 0;
static void stub_device_unref(udev_device *)
{
    ++g_devUnrefs;
}

/***************************************STUB end***********************************************/

class UT_UDevDevice : public ::testing::Test
{
public:
    UT_UDevDevice() {}

    virtual void SetUp()
    {
        g_parentLookups = 0;
        g_parentFound = true;
        g_devRefs = 0;
        g_devUnrefs = 0;
    }

    virtual void TearDown()
    {
    }
};

/**
 * @brief 空句柄构造：各属性读取走空值保护分支
 * 覆盖分支: UDevDevice(nullptr) 构造;
 *   devPath/subSystem/sysPath/sysName/devType/attrValue/propValue: [m_device == nullptr → 返回空 QByteArray];
 *   ~UDevDevice: [m_device == nullptr → 不调用 udev_device_unref]
 */
TEST_F(UT_UDevDevice, initTest_nullHandle)
{
    Stub stub;
    // Arrange
    stub.set(udev_device_unref, stub_device_unref);
    // Act & Assert: 空句柄下全部返回空值，析构不触发 unref
    {
        UDevDevice dev(nullptr);
        EXPECT_EQ(dev.handle(), nullptr);
        EXPECT_TRUE(dev.devPath().isEmpty());
        EXPECT_TRUE(dev.subSystem().isEmpty());
        EXPECT_TRUE(dev.sysPath().isEmpty());
        EXPECT_TRUE(dev.sysName().isEmpty());
        EXPECT_TRUE(dev.devType().isEmpty());
        EXPECT_TRUE(dev.attrValue("size").isEmpty());
        EXPECT_TRUE(dev.propValue("DEVNAME").isEmpty());
    }
    EXPECT_EQ(g_devUnrefs, 0);
}

/**
 * @brief 非空句柄 + libudev 打桩：各属性读取包装为 QByteArray
 * 覆盖分支: devPath/subSystem/sysPath/sysName/devType/attrValue/propValue:
 *   [m_device != nullptr → udev_device_get_xxx 包装返回]
 */
TEST_F(UT_UDevDevice, test_getters_withFakeHandle)
{
    Stub stub;
    // Arrange
    stub.set(udev_device_unref, stub_device_unref);
    stub.set(udev_device_get_devpath, stub_get_devpath);
    stub.set(udev_device_get_subsystem, stub_get_subsystem);
    stub.set(udev_device_get_syspath, stub_get_syspath);
    stub.set(udev_device_get_sysname, stub_get_sysname);
    stub.set(udev_device_get_devtype, stub_get_devtype);
    stub.set(udev_device_get_sysattr_value, stub_get_sysattr_value);
    stub.set(udev_device_get_property_value, stub_get_property_value);
    // Act & Assert
    {
        UDevDevice dev(reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
        EXPECT_EQ(dev.handle(), reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
        EXPECT_EQ(dev.devPath(), QByteArray("/devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda"));
        EXPECT_EQ(dev.subSystem(), QByteArray("block"));
        EXPECT_EQ(dev.sysPath(), QByteArray("/sys/devices/pci0000:00/0000:00:17.0/ata1/host0/target0:0:0/0:0:0:0/block/sda"));
        EXPECT_EQ(dev.sysName(), QByteArray("sda"));
        EXPECT_EQ(dev.devType(), QByteArray("disk"));
        EXPECT_EQ(dev.attrValue("size"), QByteArray("512"));
        EXPECT_EQ(dev.propValue("DEVNAME"), QByteArray("/dev/sda"));
    }
}

/**
 * @brief parent() 找到父设备：ref 后包装为 unique_ptr
 * 覆盖分支: parent: [get_parent_with_subsystem_devtype 非空 → udev_device_ref → new UDevDevice(p)]
 */
TEST_F(UT_UDevDevice, test_parent_found)
{
    Stub stub;
    // Arrange
    stub.set(udev_device_unref, stub_device_unref);
    stub.set(udev_device_get_parent_with_subsystem_devtype, stub_get_parent);
    stub.set(udev_device_ref, stub_device_ref);
    // Act & Assert
    {
        UDevDevice dev(reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
        std::unique_ptr<UDevDevice> p = dev.parent("scsi", "disk");
        ASSERT_NE(p.get(), nullptr);
        EXPECT_EQ(p->handle(), reinterpret_cast<UDevDevice::HANDLE>(kFakeParent));
        EXPECT_EQ(g_parentLookups, 1);
        EXPECT_EQ(g_devRefs, 1);
    }
}

/**
 * @brief parent() 无父设备：返回空指针且不触发 ref
 * 覆盖分支: parent: [get_parent_with_subsystem_devtype 为空 → return nullptr]
 */
TEST_F(UT_UDevDevice, test_parent_absent)
{
    Stub stub;
    // Arrange
    stub.set(udev_device_unref, stub_device_unref);
    stub.set(udev_device_get_parent_with_subsystem_devtype, stub_get_parent);
    stub.set(udev_device_ref, stub_device_ref);
    g_parentFound = false;
    // Act & Assert
    {
        UDevDevice dev(reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
        std::unique_ptr<UDevDevice> p = dev.parent("nvme", "disk");
        EXPECT_EQ(p.get(), nullptr);
        EXPECT_EQ(g_parentLookups, 1);
        EXPECT_EQ(g_devRefs, 0);
    }
}

/**
 * @brief 非空句柄析构：恰好调用一次 udev_device_unref
 * 覆盖分支: ~UDevDevice: [m_device != nullptr → udev_device_unref(m_device)]
 */
TEST_F(UT_UDevDevice, test_destructor_unrefsHandle)
{
    Stub stub;
    // Arrange
    stub.set(udev_device_unref, stub_device_unref);
    // Act & Assert
    {
        UDevDevice dev(reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
        EXPECT_EQ(dev.handle(), reinterpret_cast<UDevDevice::HANDLE>(kFakeDevice));
    }
    EXPECT_EQ(g_devUnrefs, 1);
}
