// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/udev.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//c
#include <libudev.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

// 伪造句柄（stub 掉 libudev 后永不解引用；对象必须在 Stub 作用域内释放，
// 保证伪造句柄不会流入恢复后的真实 udev_unref）
static const uintptr_t kFakeUdev = 0x3000;

// udev_new —— 返回受控句柄
static bool g_udevNewFail = false;
static struct udev *stub_udev_new()
{
    return g_udevNewFail ? nullptr : reinterpret_cast<struct udev *>(kFakeUdev);
}

// udev_unref —— 计数释放调用
static int g_udevUnrefs = 0;
static void stub_udev_unref(struct udev *)
{
    ++g_udevUnrefs;
}

/***************************************STUB end***********************************************/

class UT_UDev : public ::testing::Test
{
public:
    UT_UDev() {}

    virtual void SetUp()
    {
        g_udevNewFail = false;
        g_udevUnrefs = 0;
    }

    virtual void TearDown()
    {
    }
};

/**
 * @brief udev_new 成功：句柄透传
 * 覆盖分支: UDev 构造: [udev_new() 非空 → m_udev = udev]; handle(): [返回 m_udev]
 */
TEST_F(UT_UDev, initTest_newOk)
{
    Stub stub;
    // Arrange
    stub.set(udev_new, stub_udev_new);
    stub.set(udev_unref, stub_udev_unref);
    // Act & Assert: 对象在 Stub 作用域内创建并释放
    {
        UDev udev;
        ASSERT_NE(udev.handle(), nullptr);
        EXPECT_EQ(udev.handle(), reinterpret_cast<UDev::HANDLE>(kFakeUdev));
    }
    EXPECT_EQ(g_udevUnrefs, 1);
}

/**
 * @brief udev_new 失败：句柄为空，析构空保护不崩溃
 * 覆盖分支: UDev 构造: [udev_new() == nullptr → m_udev = nullptr]; ~UDev: [m_udev == nullptr → 不释放]
 */
TEST_F(UT_UDev, test_udevNewFail)
{
    Stub stub;
    // Arrange
    stub.set(udev_new, stub_udev_new);
    stub.set(udev_unref, stub_udev_unref);
    g_udevNewFail = true;
    // Act & Assert
    {
        UDev udev;
        EXPECT_EQ(udev.handle(), nullptr);
    }
    // 空句柄下析构不触发 unref
    EXPECT_EQ(g_udevUnrefs, 0);
}

/**
 * @brief 非空句柄析构：恰好调用一次 udev_unref
 * 覆盖分支: ~UDev: [m_udev != nullptr → udev_unref(m_udev)]
 */
TEST_F(UT_UDev, test_destructor_unrefsHandle)
{
    Stub stub;
    // Arrange
    stub.set(udev_new, stub_udev_new);
    stub.set(udev_unref, stub_udev_unref);
    // Act & Assert
    {
        UDev udev;
        EXPECT_EQ(udev.handle(), reinterpret_cast<UDev::HANDLE>(kFakeUdev));
    }
    EXPECT_EQ(g_udevUnrefs, 1);
}
