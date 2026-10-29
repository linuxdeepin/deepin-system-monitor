// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ut_device_db.cpp
 * @brief popup 插件 DeviceDB 设备数据库单测(任务文件范围:
 *        deepin-system-monitor-plugin-popup/system/device_db.cpp,99 行)
 *
 * 测试对象: core::system::DeviceDB(构造/析构/instance/update/cpuSet/memInfo/
 * diskIoInfo/blockDeviceInfoDB/netInfo,共 9 个可测方法)。
 *
 * 外部依赖打桩(外部依赖一律 stub,不触碰真实设备环境):
 *  - update() 内部 5 个子系统刷新调用(CPUSet::update / MemInfo::readMemInfo /
 *    DiskIOInfo::update / BlockDeviceInfoDB::update / NetInfo::resdNetInfo)以
 *    Stub 计数打桩:真实实现读取 /proc 等系统文件,单测不允许依赖;计数断言
 *    使用调用前后差值,规避后台 SystemMonitor 线程并发刷新的干扰。
 *  - instance() 依赖 Application 构造的 SystemMonitorThread(main-ut.cpp 已
 *    构造 Application),不额外打桩,验证真实接线。
 *
 * 注意: popup 的 MemInfo/NetInfo/DiskIOInfo/BlockDeviceInfoDB 实现来自主程序
 * (deepin-system-monitor-main),net_info.h 的 include guard 存在缺陷
 * (误写为 DISKIO_INFO_H,见缺陷清单 #4),故本文件沿用生产 include 顺序:
 * diskio_info.h 必须先于 net_info.h。
 *
 * 用例共 6 个,声明如下(文件:用例计数声明,格式:| 分组 | 级别 | 圈复杂度因子 | min | actual |):
 * | 分组      | 级别 | 圈复杂度因子 | min | actual |
 * |-----------|------|--------------|-----|--------|
 * | DeviceDB  | low  | 1-5          | 6   | 6      |
 *
 * 测试类型: 行为与接线性测试(非实现细节验证——访问器只断言所有权接线
 * 非空与稳定身份,update 只断言子系统刷新被触发)。
 */

#include "system/device_db.h"

#include "cpu_set.h"
#include "system/mem.h"
// 缺陷 #4 规避: net_info.h include guard 误写 DISKIO_INFO_H,diskio_info.h 必须在前
#include "system/diskio_info.h"
#include "system/net_info.h"
#include "system/block_device_info_db.h"

#include "../../tests/stub.h"
#include <gtest/gtest.h>

#include <QByteArrayList>

namespace {

int g_cpuSetUpdateCalls = 0;
int g_memReadCalls = 0;
int g_diskIoUpdateCalls = 0;
int g_blkDevUpdateCalls = 0;
int g_netResdCalls = 0;

void stub_cpuSetUpdate() { ++g_cpuSetUpdateCalls; }
void stub_memReadMemInfo() { ++g_memReadCalls; }
void stub_diskIoUpdate() { ++g_diskIoUpdateCalls; }
void stub_blkDevUpdate() { ++g_blkDevUpdateCalls; }
void stub_netResdNetInfo() { ++g_netResdCalls; }

}  // namespace

class UT_DeviceDB : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        // 打桩 5 个子系统刷新入口,隔离 /proc 等真实系统读取
        s_stub.set((void (core::system::CPUSet::*)())ADDR(core::system::CPUSet, update),
                   stub_cpuSetUpdate);
        s_stub.set((void (core::system::MemInfo::*)())ADDR(core::system::MemInfo, readMemInfo),
                   stub_memReadMemInfo);
        s_stub.set((void (core::system::DiskIOInfo::*)())ADDR(core::system::DiskIOInfo, update),
                   stub_diskIoUpdate);
        s_stub.set((void (core::system::BlockDeviceInfoDB::*)())ADDR(core::system::BlockDeviceInfoDB, update),
                   stub_blkDevUpdate);
        s_stub.set((void (core::system::NetInfo::*)())ADDR(core::system::NetInfo, resdNetInfo),
                   stub_netResdNetInfo);
    }

    static void TearDownTestSuite()
    {
        // Stub 析构自动还原符号,显式 reset 以明确生命周期
        s_stub.reset((void (core::system::CPUSet::*)())ADDR(core::system::CPUSet, update));
        s_stub.reset((void (core::system::MemInfo::*)())ADDR(core::system::MemInfo, readMemInfo));
        s_stub.reset((void (core::system::DiskIOInfo::*)())ADDR(core::system::DiskIOInfo, update));
        s_stub.reset((void (core::system::BlockDeviceInfoDB::*)())ADDR(core::system::BlockDeviceInfoDB, update));
        s_stub.reset((void (core::system::NetInfo::*)())ADDR(core::system::NetInfo, resdNetInfo));
    }

    void SetUp() override
    {
        // Arrange: 计数器清零
        g_cpuSetUpdateCalls = 0;
        g_memReadCalls = 0;
        g_diskIoUpdateCalls = 0;
        g_blkDevUpdateCalls = 0;
        g_netResdCalls = 0;
    }

    void TearDown() override
    {
        // 无状态需清理,保留空实现以满足测试类生命周期约定
    }

    static Stub s_stub;
};

Stub UT_DeviceDB::s_stub;

// 单例 instance() 在 Application 构造的监控线程上可用,重复调用返回同一实例
TEST_F(UT_DeviceDB, Instance_RepeatedCalls_ReturnsStablePointer)
{
    // Arrange: Application 已在 main-ut 构造,监控线程就绪
    core::system::DeviceDB *first = nullptr;

    // Act: 两次获取单例
    first = core::system::DeviceDB::instance();
    core::system::DeviceDB *second = core::system::DeviceDB::instance();

    // Assert: 非空且身份稳定(单例接线正确)
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// 构造后 5 个子系统访问器均返回非空所有权对象
TEST_F(UT_DeviceDB, Ctor_DefaultConstruct_AccessorsNonNull)
{
    // Arrange: 独立构造 DeviceDB,不与单例共享状态
    core::system::DeviceDB db;

    // Act: 逐一访问子系统
    core::system::CPUSet *cpuSet = db.cpuSet();
    core::system::MemInfo *memInfo = db.memInfo();
    core::system::DiskIOInfo *diskIoInfo = db.diskIoInfo();
    core::system::BlockDeviceInfoDB *blkDevDB = db.blockDeviceInfoDB();
    core::system::NetInfo *netInfo = db.netInfo();

    // Assert: 全部子系统已构造
    EXPECT_NE(cpuSet, nullptr);
    EXPECT_NE(memInfo, nullptr);
    EXPECT_NE(diskIoInfo, nullptr);
    EXPECT_NE(blkDevDB, nullptr);
    EXPECT_NE(netInfo, nullptr);
}

// 访问器重复调用返回同一指针(所有权在 DeviceDB,不做懒构造)
TEST_F(UT_DeviceDB, Accessors_RepeatedCalls_ReturnSamePointer)
{
    // Arrange: 独立构造 DeviceDB
    core::system::DeviceDB db;

    // Act: 每个访问器取两次指针
    core::system::CPUSet *cpu1 = db.cpuSet();
    core::system::CPUSet *cpu2 = db.cpuSet();

    // Assert: 两次返回同一指针(无悬空/重建),其余子系统同理
    EXPECT_EQ(cpu1, cpu2);
    EXPECT_EQ(db.memInfo(), db.memInfo());
    EXPECT_EQ(db.diskIoInfo(), db.diskIoInfo());
    EXPECT_EQ(db.blockDeviceInfoDB(), db.blockDeviceInfoDB());
    EXPECT_EQ(db.netInfo(), db.netInfo());
}

// update() 依次触发 5 个子系统刷新(计数差值断言,容忍后台线程并发调用)
TEST_F(UT_DeviceDB, Update_SingleCall_TriggersFiveUpdaters)
{
    // Arrange: 使用单例对象并记录调用前计数
    core::system::DeviceDB *db = core::system::DeviceDB::instance();
    ASSERT_NE(db, nullptr);
    const int cpu0 = g_cpuSetUpdateCalls;
    const int mem0 = g_memReadCalls;
    const int disk0 = g_diskIoUpdateCalls;
    const int blk0 = g_blkDevUpdateCalls;
    const int net0 = g_netResdCalls;

    // Act: 执行一次整体刷新
    db->update();

    // Assert: 5 个子系统刷新入口各至少被触发一次
    EXPECT_GE(g_cpuSetUpdateCalls, cpu0 + 1);
    EXPECT_GE(g_memReadCalls, mem0 + 1);
    EXPECT_GE(g_diskIoUpdateCalls, disk0 + 1);
    EXPECT_GE(g_blkDevUpdateCalls, blk0 + 1);
    EXPECT_GE(g_netResdCalls, net0 + 1);
}

// update() 可重复调用,每次刷新均触发子系统(幂等刷新)
TEST_F(UT_DeviceDB, Update_SecondCall_TriggersAgain)
{
    // Arrange: 首次刷新后记录计数
    core::system::DeviceDB *db = core::system::DeviceDB::instance();
    ASSERT_NE(db, nullptr);
    db->update();
    const int cpu0 = g_cpuSetUpdateCalls;
    const int mem0 = g_memReadCalls;

    // Act: 第二次刷新
    db->update();

    // Assert: 计数继续增长
    EXPECT_GE(g_cpuSetUpdateCalls, cpu0 + 1);
    EXPECT_GE(g_memReadCalls, mem0 + 1);
}

// 析构安全删除 5 个子系统指针(独立对象构造-析构一遍)
TEST_F(UT_DeviceDB, Dtor_ScopeExit_DestroysOwnedSubsystems)
{
    // Arrange: 块作用域内构造子系统完整的 DeviceDB
    {
        // Act: 作用域结束自动析构
        core::system::DeviceDB db;
        ASSERT_NE(db.cpuSet(), nullptr);
        ASSERT_NE(db.netInfo(), nullptr);
    }

    // Assert: 析构无崩溃,单例接线仍可用
    EXPECT_NE(core::system::DeviceDB::instance(), nullptr);
    EXPECT_NE(core::system::DeviceDB::instance()->cpuSet(), nullptr);
}
