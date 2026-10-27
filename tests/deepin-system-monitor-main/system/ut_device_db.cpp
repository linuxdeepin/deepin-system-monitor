// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/device_db.h"
#include "system/sys_info.h"
#include "system/netif_info_db.h"
#include "system/block_device_info_db.h"
#include "system/mem.h"
#include "system/cpu_set.h"
#include "system/diskio_info.h"
#include "system/net_info.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

// 六个子对象的刷新入口计数器 —— 验证 DeviceDB::update 的委托关系
static int g_cpuUpdateCalls = 0;
static int g_memReadCalls = 0;
static int g_netifUpdateCalls = 0;
static int g_blkUpdateCalls = 0;
static int g_dioUpdateCalls = 0;
static int g_netReadCalls = 0;

static void stub_cpuUpdate() { ++g_cpuUpdateCalls; }
static void stub_memReadMemInfo() { ++g_memReadCalls; }
static void stub_netifUpdate() { ++g_netifUpdateCalls; }
static void stub_blkUpdate() { ++g_blkUpdateCalls; }
static void stub_dioUpdate() { ++g_dioUpdateCalls; }
static void stub_netResdNetInfo() { ++g_netReadCalls; }

// SysInfo::readSysInfoStatic —— 截断 DeviceDB::instance() 链路中的 /proc 静态读取
static int g_sysStaticCalls = 0;
static void stub_readSysInfoStatic()
{
    ++g_sysStaticCalls;
}

/***************************************STUB end***********************************************/

class UT_DeviceDB : public ::testing::Test
{
public:
    UT_DeviceDB() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        g_cpuUpdateCalls = 0;
        g_memReadCalls = 0;
        g_netifUpdateCalls = 0;
        g_blkUpdateCalls = 0;
        g_dioUpdateCalls = 0;
        g_netReadCalls = 0;
        g_sysStaticCalls = 0;
        m_tester = new DeviceDB();
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    DeviceDB *m_tester;
};

/**
 * @brief 构造时创建全部六类设备信息对象
 * 覆盖分支: DeviceDB 构造 → new CPUSet/MemInfo/NetifInfoDB/BlockDeviceInfoDB/DiskIOInfo/NetInfo
 */
TEST_F(UT_DeviceDB, initTest_createsAllSubObjects)
{
    // Arrange: SetUp 已创建 DeviceDB
    // Act & Assert: 各访问器均返回非空指针
    EXPECT_NE(m_tester->cpuSet(), nullptr);
    EXPECT_NE(m_tester->memInfo(), nullptr);
    EXPECT_NE(m_tester->netifInfoDB(), nullptr);
    EXPECT_NE(m_tester->blockDeviceInfoDB(), nullptr);
    EXPECT_NE(m_tester->diskIoInfo(), nullptr);
    EXPECT_NE(m_tester->netInfo(), nullptr);
}

/**
 * @brief 各访问器返回稳定指针（幂等）
 * 覆盖分支: blockDeviceInfoDB/memInfo/netifInfoDB/cpuSet/diskIoInfo/netInfo → 返回成员指针
 */
TEST_F(UT_DeviceDB, test_accessors_returnStablePointers)
{
    // Arrange: SetUp 已创建 DeviceDB
    // Act & Assert: 两次调用返回同一对象
    EXPECT_EQ(m_tester->cpuSet(), m_tester->cpuSet());
    EXPECT_EQ(m_tester->memInfo(), m_tester->memInfo());
    EXPECT_EQ(m_tester->netifInfoDB(), m_tester->netifInfoDB());
    EXPECT_EQ(m_tester->blockDeviceInfoDB(), m_tester->blockDeviceInfoDB());
    EXPECT_EQ(m_tester->diskIoInfo(), m_tester->diskIoInfo());
    EXPECT_EQ(m_tester->netInfo(), m_tester->netInfo());
}

/**
 * @brief update() 逐个委托六个子对象刷新
 * 覆盖分支: update: [cpuSet->update] [memInfo->readMemInfo] [netifInfoDB->update]
 *           [blkDevInfoDB->update] [diskIoInfo->update] [netInfo->resdNetInfo]
 */
TEST_F(UT_DeviceDB, test_update_delegatesToAllChildren)
{
    Stub stub;
    // Arrange
    stub.set(ADDR(CPUSet, update), stub_cpuUpdate);
    stub.set(ADDR(MemInfo, readMemInfo), stub_memReadMemInfo);
    stub.set(ADDR(NetifInfoDB, update), stub_netifUpdate);
    stub.set(ADDR(BlockDeviceInfoDB, update), stub_blkUpdate);
    stub.set(ADDR(DiskIOInfo, update), stub_dioUpdate);
    stub.set(ADDR(NetInfo, resdNetInfo), stub_netResdNetInfo);
    // Act
    m_tester->update();
    // Assert: 六个子对象各被调用一次
    EXPECT_EQ(g_cpuUpdateCalls, 1);
    EXPECT_EQ(g_memReadCalls, 1);
    EXPECT_EQ(g_netifUpdateCalls, 1);
    EXPECT_EQ(g_blkUpdateCalls, 1);
    EXPECT_EQ(g_dioUpdateCalls, 1);
    EXPECT_EQ(g_netReadCalls, 1);
    // Act: 再次刷新
    m_tester->update();
    // Assert: 计数累加
    EXPECT_EQ(g_cpuUpdateCalls, 2);
    EXPECT_EQ(g_dioUpdateCalls, 2);
}

/**
 * @brief 析构安全释放全部子对象（含空指针保护）
 * 覆盖分支: ~DeviceDB: [if (m_xxx) { delete; = nullptr; } × 6]
 */
TEST_F(UT_DeviceDB, test_destructor_releasesAllSubObjects)
{
    // Arrange
    DeviceDB *db = new DeviceDB();
    // Act & Assert: 正常释放不崩溃
    delete db;
    SUCCEED();
}

/**
 * @brief instance() 经 SystemMonitorThread 链路返回监控实例的 DeviceDB
 * 覆盖分支: instance: [ThreadManager::instance → thread<SystemMonitorThread>
 *           → systemMonitorInstance → deviceDB]
 * 注: readSysInfoStatic 已 stub，避免构造链路读取真实 /proc。
 */
TEST_F(UT_DeviceDB, test_instance_returnsMonitorDeviceDB)
{
    Stub stub;
    // Arrange
    stub.set(ADDR(SysInfo, readSysInfoStatic), stub_readSysInfoStatic);
    // Act
    DeviceDB *db = DeviceDB::instance();
    // Assert: 非空且两次获取为同一实例
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(DeviceDB::instance(), db);
    EXPECT_NE(db->blockDeviceInfoDB(), nullptr);
}
