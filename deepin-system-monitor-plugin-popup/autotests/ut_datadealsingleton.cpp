// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ut_datadealsingleton.cpp
 * @brief popup 插件 DataDealSingleton 数据处理单测(任务文件范围:
 *        deepin-system-monitor-plugin-popup/gui/datadealsingleton.cpp,162 行)
 *
 * 测试对象: DataDealSingleton(getInstance / readCpuPer / readMemInfo /
 * readNetInfo / readDiskInfo / sendJumpWidgetMessage / launchMainProcessByAM /
 * 构造与析构)。
 *
 * 外部依赖打桩(外部依赖一律 stub,不触碰真实设备环境):
 *  - DataDealSingleton 数据源桩组(ut_popup_stub::uninstallDataDealStubs):
 *    进程启动时 installAll 常驻打桩 readNetInfo/readDiskInfo/sendJumpWidgetMessage,
 *    本文件在 SetUp 临时卸载以覆盖真实实现,TearDown 恢复,GUI 用例不受影响。
 *  - MemInfo/NetInfo/DiskIOInfo 数据 getter:打桩注入固定读数(真实实现仅从
 *    /proc 等系统文件读取,单测不允许依赖)。
 *  - BlockDeviceInfoDB::deviceList:打桩注入固定设备列表(BlockDevice 默认
 *    构造,capacity=0,deviceName 经真实 setter 设置)。
 *  - QDBusConnection::call:注入 ReplyMessage / ErrorMessage 应答,不发起真实
 *    DBus 调用(AM 服务在本机不存在)。
 *  - detached 进程启动:沿用 ut_popup_stub 常驻桩(计数 + 固定返回 true)。
 *  - common::systemInfo() 版本态:经 SystemState::updateVersion 公有接口注入,
 *    TearDown 复位。
 *
 * 用例共 12 个,声明如下(文件:用例计数声明,格式:| 分组 | 级别 | 圈复杂度因子 | min | actual |):
 * | 分组                  | 级别 | 圈复杂度因子 | min | actual |
 * |-----------------------|------|--------------|-----|--------|
 * | GetInstance 单例      | mid  | 1            | 2   | 2      |
 * | ReadCpuPer 读 CPU     | mid  | 1            | 1   | 1      |
 * | ReadMemInfo 读内存    | mid  | 1            | 2   | 2      |
 * | ReadNetInfo 读网络    | mid  | 1            | 1   | 1      |
 * | ReadDiskInfo 读磁盘   | mid  | 2            | 2   | 2      |
 * | SendJumpWidgetMessage 跳转 | mid | 4         | 4   | 4      |
 * | 合计                  |      |              | 12  | 12     |
 *
 * 分支映射(datadealsingleton.cpp 行号为工作区源码实际行号):
 * | 方法                    | 分支                                  | 用例                                    |
 * |-------------------------|---------------------------------------|-----------------------------------------|
 * | getInstance             | 首次创建 / 已存在                     | RecreatesAfterExplicitDestroy / Identity |
 * | readCpuPer              | 单一路径                              | ReturnsTrueWithZeroInitialPercent        |
 * | readMemInfo             | 正常值 / 全 0(0/0)                  | FormatsMemAndSwapUnits / YieldsNanPercent|
 * | readNetInfo             | 单一路径                              | FormatsFromNetInfoGetters                |
 * | readDiskInfo            | 列表非空循环 / diskAvailable 未写入   | FormatsDiskIoAndBlockDbTotals ×2         |
 * | sendJumpWidgetMessage   | 计时器活跃返回 false                  | RepeatWithinWindow_ReturnsFalse          |
 * | sendJumpWidgetMessage   | isOldVersion=true 走 startDetached    | OldVersion_LaunchesDetachedProcess       |
 * | sendJumpWidgetMessage   | AM 启动失败 rt=false                  | NewVersion_AmLaunchFails_ReturnsFalse    |
 * | sendJumpWidgetMessage   | AM 启动成功 rt=true                   | NewVersion_AmLaunchSucceeds_ReturnsTrue  |
 * | launchMainProcessByAM   | ReplyMessage / ErrorMessage 两分支    | AmLaunchFails / AmLaunchSucceeds         |
 *
 * 已知源码行为(仅标红记录,不修改生产代码):
 *  - readDiskInfo 的输出参数 diskAvailable 从未被赋值(源码 83-103 行),
 *    用例 ReadDiskInfo_AvailableParamNotWritten 记录该行为;
 *  - readMemInfo 在 memTotal==0 时 memPercent 计算 0/0 得 NaN("nan");
 *  - getInstance 在锁内 testAndSetOrdered 失败的罕见竞态下会泄漏新分配实例。
 */

#include "ut_popup_stub.h"

#include "dbus/dbuscallmaininterface.h"
#include "gui/datadealsingleton.h"
#include "common/datacommon.h"
// 注意: net_info.h 的 include guard 误写为 DISKIO_INFO_H(源码缺陷,已标红),
// 本文件必须先包含 diskio_info.h 再包含 net_info.h,否则 DiskIOInfo 声明被吞。
#include "system/diskio_info.h"
#include "system/mem.h"
#include "system/net_info.h"
#include "system/block_device_info_db.h"
#include "system/block_device.h"
#include "helper.hpp"
#include "ddlog.h"

#include <gtest/gtest.h>

#include "../../tests/stub.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QEventLoop>
#include <QTimer>

using namespace core::system;

namespace {

/**
 * @brief AM DBus 调用观测计数(桩注入,非真实外部调用)
 */
int g_amDbusCallCount = 0;

/**
 * @brief 停止单例的防抖计时器,消除用例间 1000ms 窗口的时序耦合
 * (m_popupTrickTimer 为私有成员,依赖编译期 -fno-access-control 白盒访问)
 */
void stopTrickTimer()
{
    auto *timer = DataDealSingleton::getInstance().m_popupTrickTimer;
    if (timer)
        timer->stop();
}

// ---- QDBusConnection::call 注入应答(AM 启动调用) ----
static QDBusMessage stub_dbusCall_reply(void *, const QDBusMessage &msg, QDBus::CallMode)
{
    ++g_amDbusCallCount;
    return msg.createReply();
}

static QDBusMessage stub_dbusCall_error(void *, const QDBusMessage &msg, QDBus::CallMode)
{
    ++g_amDbusCallCount;
    return msg.createErrorReply(QDBusError::ServiceUnknown,
                                QStringLiteral("org.desktopspec.ApplicationManager1 not provided"));
}

// ---- 数据源 getter 注入 ----
static qulonglong stub_mem_total(void *) { return 8000; }
static qulonglong stub_mem_available(void *) { return 3000; }
static qulonglong stub_mem_swapTotal(void *) { return 2000; }
static qulonglong stub_mem_swapFree(void *) { return 500; }

static qreal stub_net_recvBps(void *) { return 1024.0; }
static qreal stub_net_sentBps(void *) { return 2048.0; }
static qulonglong stub_net_totalRecv(void *) { return 1048576ULL; }
static qulonglong stub_net_totalSent(void *) { return 512ULL; }

static qreal stub_disk_readBps(void *) { return 1048576.0; }
static qreal stub_disk_writeBps(void *) { return 2097152.0; }

static QList<BlockDevice> stub_bdb_deviceList(BlockDeviceInfoDB *)
{
    QList<BlockDevice> list;
    // 设备名故意用不存在的伪设备,避免 setDeviceName 内部 readDeviceInfo
    // 从真实 /proc/partitions 读到真实容量(容量保持 0,结果确定)
    BlockDevice pseudoA;
    pseudoA.setDeviceName(QByteArrayLiteral("zzuta"));
    BlockDevice pseudoB;
    pseudoB.setDeviceName(QByteArrayLiteral("zzutb"));
    list << pseudoA << pseudoB;
    return list;
}

/**
 * @brief 注入 common::SystemState::isOldVersion 的返回值
 * 说明: systemInfo() 为 static(内部链接),函数内静态状态每 TU 独立,不能跨 TU 注入;
 * isOldVersion 是外部链接的 inline 成员,打在符号上全进程生效。
 */
static bool stub_isOldVersion_true(common::SystemState *) { return true; }
static bool stub_isOldVersion_false(common::SystemState *) { return false; }

// ---- DbusCallMainInterface::jumpWidget 打桩(阻止 lambda 内的真实 DBus 调用) ----
static int g_jumpCallCount = 0;
static void stub_jumpWidget(void *, QString) { ++g_jumpCallCount; }

}  // namespace

class UT_DataDealSingleton : public ::testing::Test
{
public:
    void SetUp() override
    {
        // 临时卸载常驻的 DataDealSingleton 数据源桩,覆盖真实实现
        ut_popup_stub::uninstallDataDealStubs();
        ut_popup_stub::resetKnobs();
    }

    void TearDown() override
    {
        // 删除单例: 取消挂起的 singleShot 跳转 lambda(上下文销毁即取消),
        // 防止其在后续用例的事件循环中触发真实 DBus 调用
        if (DataDealSingleton *stale = DataDealSingleton::instance.loadRelaxed())
            delete stale;
        DataDealSingleton::instance.storeRelaxed(nullptr);
        // 恢复 GUI 用例依赖的数据源桩与全局状态
        ut_popup_stub::installDataDealStubs();
        ut_popup_stub::resetKnobs();
    }
};

/**
 * @brief 用例: getInstance 连续两次调用返回同一实例(非首次创建路径)
 * 前置条件: 单例已存在(前序用例或本次首次调用创建)
 * 操作步骤: 连续两次 getInstance
 * 预期结果: 两次地址一致且非空
 */
TEST_F(UT_DataDealSingleton, GetInstance_TwiceCall_ReturnsSameIdentity)
{
    // Arrange
    DataDealSingleton &first = DataDealSingleton::getInstance();
    // Act
    DataDealSingleton &second = DataDealSingleton::getInstance();
    // Assert: 同一实例且内部状态完好
    EXPECT_NE(&first, nullptr);
    EXPECT_EQ(&first, &second);
    EXPECT_NE(first.m_popupTrickTimer, nullptr);
}

/**
 * @brief 用例: 白盒销毁单例后 getInstance 重建实例
 * 前置条件: 单例已存在,静态原子指针指向它
 * 操作步骤: delete 当前实例并将静态 instance 原子指针置空后再次 getInstance
 * 预期结果: 置空后原子指针为空,再次 getInstance 重新创建实例并回填指针
 *           (同时覆盖私有析构函数;新旧指针不做不等断言,堆内存可能复用)
 */
TEST_F(UT_DataDealSingleton, GetInstance_AfterDestroy_RecreatesNewInstance)
{
    // Arrange
    DataDealSingleton &old = DataDealSingleton::getInstance();
    EXPECT_EQ(DataDealSingleton::instance.loadRelaxed(), &old);
    // Act: 销毁并复位静态原子指针(白盒,-fno-access-control)
    delete &old;
    DataDealSingleton::instance.storeRelaxed(nullptr);
    // Assert: 复位后指针为空,重新 getInstance 后重建并回填
    EXPECT_EQ(DataDealSingleton::instance.loadRelaxed(), nullptr);
    DataDealSingleton &fresh = DataDealSingleton::getInstance();
    EXPECT_NE(&fresh, nullptr);
    EXPECT_EQ(DataDealSingleton::instance.loadRelaxed(), &fresh);
    EXPECT_NE(fresh.m_popupTrickTimer, nullptr);
}

/**
 * @brief 用例: readCpuPer 空样本库时返回 true 且初始 CPU 占用为 0
 * 前置条件: CPUInfoModel 为空样本库(recentSamplePair 返回空对,cpupc 返回 0)
 * 操作步骤: 传入 -1 初始值调用 readCpuPer
 * 预期结果: 返回 true,cpuPer 被写为 0.0
 */
TEST_F(UT_DataDealSingleton, ReadCpuPer_EmptySamples_ReturnsTrueAndZero)
{
    // Arrange
    qreal cpuPer = -1.0;
    // Act
    bool rt = DataDealSingleton::getInstance().readCpuPer(cpuPer);
    // Assert
    EXPECT_TRUE(rt);
    EXPECT_DOUBLE_EQ(cpuPer, 0.0);
}

/**
 * @brief 用例: readMemInfo 按注入内存读数格式化内存/交换分区单位与百分比
 * 前置条件: 打桩 MemInfo 四个 getter(memTotal=8000KB available=3000KB
 *           swapTotal=2000KB swapFree=500KB)
 * 操作步骤: 调用 readMemInfo
 * 预期结果: 各输出符合 formatUnit_memory_disk 换算:
 *           usage=5000KB→"4.9 MB" total=8000KB→"7.8 MB"
 *           percent=(5000/8000)*100→"62.5"
 *           swapUsage=1500KB→"1.5 MB" swapTotal=2000KB→"2.0 MB" swapPercent→"75.0"
 */
TEST_F(UT_DataDealSingleton, ReadMemInfo_InjectedValues_FormatsUnitsPercent)
{
    // Arrange: 注入内存读数
    Stub stub;
    stub.set((qulonglong (MemInfo::*)() const)ADDR(MemInfo, memTotal), stub_mem_total);
    stub.set((qulonglong (MemInfo::*)() const)ADDR(MemInfo, memAvailable), stub_mem_available);
    stub.set((qulonglong (MemInfo::*)() const)ADDR(MemInfo, swapTotal), stub_mem_swapTotal);
    stub.set((qulonglong (MemInfo::*)() const)ADDR(MemInfo, swapFree), stub_mem_swapFree);
    QString memUsage, memTotal, memPercent, swapUsage, swapTotal, swapPercent;
    // Act
    bool rt = DataDealSingleton::getInstance().readMemInfo(
        memUsage, memTotal, memPercent, swapUsage, swapTotal, swapPercent);
    // Assert
    EXPECT_TRUE(rt);
    EXPECT_EQ(memUsage, QStringLiteral("4.9 MB"));
    EXPECT_EQ(memTotal, QStringLiteral("7.8 MB"));
    EXPECT_EQ(memPercent, QStringLiteral("62.5"));
    EXPECT_EQ(swapUsage, QStringLiteral("1.5 MB"));
    EXPECT_EQ(swapTotal, QStringLiteral("2.0 MB"));
    EXPECT_EQ(swapPercent, QStringLiteral("75.0"));
}

/**
 * @brief 用例: 全 0 内存读数时 memPercent 计算 0/0 产生 "nan"(标红记录的现有行为)
 * 前置条件: 不打桩,MemInfo 为默认零值
 * 操作步骤: 调用 readMemInfo
 * 预期结果: 单位串为 "0.0 B",百分比串为 "nan"(memTotal==0 时除零)
 */
TEST_F(UT_DataDealSingleton, ReadMemInfo_ZeroTotal_YieldsNanPercent)
{
    // Arrange
    QString memUsage, memTotal, memPercent, swapUsage, swapTotal, swapPercent;
    // Act
    bool rt = DataDealSingleton::getInstance().readMemInfo(
        memUsage, memTotal, memPercent, swapUsage, swapTotal, swapPercent);
    // Assert: 记录现有除零行为(疑似缺陷,不修改生产代码)
    EXPECT_TRUE(rt);
    EXPECT_EQ(memUsage, QStringLiteral("0.0 B"));
    EXPECT_EQ(memPercent, QStringLiteral("nan"));
}

/**
 * @brief 用例: readNetInfo 按注入网卡读数格式化速率与累计流量
 * 前置条件: 打桩 NetInfo 四个 getter(recv=1024B/s sent=2048B/s
 *           totalRecv=1MiB totalSent=512B)
 * 操作步骤: 调用 readNetInfo
 * 预期结果: recv*8=8192b→"8.0 Kb/s" sent*8=16384b→"16.0 Kb/s"
 *           totalRecv*8=8388608b→"8.0 Mb" totalSent*8=4096b→"4.0 Kb"
 */
TEST_F(UT_DataDealSingleton, ReadNetInfo_InjectedGetters_FormatsTrafficStrings)
{
    // Arrange: 注入网络读数
    Stub stub;
    stub.set((qreal (NetInfo::*)())ADDR(NetInfo, recvBps), stub_net_recvBps);
    stub.set((qreal (NetInfo::*)())ADDR(NetInfo, sentBps), stub_net_sentBps);
    stub.set((qulonglong (NetInfo::*)())ADDR(NetInfo, totalRecvBytes), stub_net_totalRecv);
    stub.set((qulonglong (NetInfo::*)())ADDR(NetInfo, totalSentBytes), stub_net_totalSent);
    QString netReceive, netTotalReceive, netSend, totalSend;
    // Act
    bool rt = DataDealSingleton::getInstance().readNetInfo(netReceive, netTotalReceive, netSend, totalSend);
    // Assert
    EXPECT_TRUE(rt);
    EXPECT_EQ(netReceive, QStringLiteral("8.0 Kb/s"));
    EXPECT_EQ(netSend, QStringLiteral("16.0 Kb/s"));
    EXPECT_EQ(netTotalReceive, QStringLiteral("8.0 Mb"));
    EXPECT_EQ(totalSend, QStringLiteral("4.0 Kb"));
}

/**
 * @brief 用例: readDiskInfo 按注入 IO 读数与设备列表汇总格式化
 * 前置条件: 打桩 DiskIOInfo 读/写速率与 BlockDeviceInfoDB::deviceList
 *           (两台默认设备,capacity=0)
 * 操作步骤: 调用 readDiskInfo
 * 预期结果: diskRead="1024.0 KB/s"(1048576B 恰在 KB 边界,while 条件为严格大于)\n *           diskWrite="2.0 MB/s"
 *           totalDiskAva=0→diskTotalSize="0.0 B"
 */
TEST_F(UT_DataDealSingleton, ReadDiskInfo_InjectedGetters_FormatsIoAndTotal)
{
    // Arrange: 注入磁盘 IO 与块设备列表
    Stub stub;
    stub.set((qreal (DiskIOInfo::*)())ADDR(DiskIOInfo, diskIoReadBps), stub_disk_readBps);
    stub.set((qreal (DiskIOInfo::*)())ADDR(DiskIOInfo, diskIoWriteBps), stub_disk_writeBps);
    stub.set((QList<BlockDevice> (BlockDeviceInfoDB::*)())ADDR(BlockDeviceInfoDB, deviceList),
             stub_bdb_deviceList);
    QString diskRead, diskTotalSize, diskWrite, diskAvailable;
    // Act
    bool rt = DataDealSingleton::getInstance().readDiskInfo(diskRead, diskTotalSize, diskWrite, diskAvailable);
    // Assert
    EXPECT_TRUE(rt);
    EXPECT_EQ(diskRead, QStringLiteral("1024.0 KB/s"));
    EXPECT_EQ(diskWrite, QStringLiteral("2.0 MB/s"));
    EXPECT_EQ(diskTotalSize, QStringLiteral("0.0 B"));
}

/**
 * @brief 用例: readDiskInfo 的输出参数 diskAvailable 从未被赋值(标红记录的现有行为)
 * 前置条件: 传入带哨兵值的 diskAvailable
 * 操作步骤: 调用 readDiskInfo
 * 预期结果: diskAvailable 保持哨兵值不变(疑似缺陷,不修改生产代码)
 */
TEST_F(UT_DataDealSingleton, ReadDiskInfo_SentinelParam_AvailableNotWritten)
{
    // Arrange
    QString diskRead, diskTotalSize, diskWrite;
    QString diskAvailable = QStringLiteral("SENTINEL");
    // Act
    bool rt = DataDealSingleton::getInstance().readDiskInfo(diskRead, diskTotalSize, diskWrite, diskAvailable);
    // Assert: 输出参数未被写入
    EXPECT_TRUE(rt);
    EXPECT_EQ(diskAvailable, QStringLiteral("SENTINEL"));
}

/**
 * @brief 用例: 防抖计时器活跃期间重复点击,第二次调用直接返回 false
 * 前置条件: 打桩 isOldVersion=true(经常驻 detached 启动桩,快速成功启动计时器)
 * 操作步骤: 窗口期内连续两次 sendJumpWidgetMessage
 * 预期结果: 第一次 true,第二次 false;detached 启动仅发生一次
 *           (源码 108-112 行分支)
 */
TEST_F(UT_DataDealSingleton, SendJump_RepeatWithinWindow_ReturnsFalse)
{
    // Arrange: 停止遗留计时器,注入旧版本态
    stopTrickTimer();
    Stub stub;
    stub.set((bool (common::SystemState::*)() const)ADDR(common::SystemState, isOldVersion),
             stub_isOldVersion_true);
    ut_popup_stub::g_startDetachedCount = 0;
    // Act
    bool first = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_CPU"));
    bool second = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_CPU"));
    // Assert: 第二次被防抖拦截,且未重复启动进程
    EXPECT_TRUE(first);
    EXPECT_FALSE(second);
    EXPECT_EQ(ut_popup_stub::g_startDetachedCount, 1);
}

/**
 * @brief 用例: 旧版本系统经 detached 启动唤醒主进程
 * 前置条件: isOldVersion=true;detached 启动为 ut_popup_stub 常驻桩(返回 true)
 * 操作步骤: 调用 sendJumpWidgetMessage
 * 预期结果: 返回 true,常驻桩记录到 deepin-system-monitor 的 detached 启动一次
 */
TEST_F(UT_DataDealSingleton, SendJump_OldVersion_LaunchesDetachedProcess)
{
    // Arrange
    stopTrickTimer();
    Stub stub;
    stub.set((bool (common::SystemState::*)() const)ADDR(common::SystemState, isOldVersion),
             stub_isOldVersion_true);
    ut_popup_stub::g_startDetachedCount = 0;
    // Act
    bool rt = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_DISK"));
    // Assert
    EXPECT_TRUE(rt);
    EXPECT_EQ(ut_popup_stub::g_startDetachedCount, 1);
    EXPECT_EQ(ut_popup_stub::g_lastDetachedProgram, Globals::DEEPIN_SYSTEM_MONITOR_PATH);
}

/**
 * @brief 用例: 新版本系统 AM 启动失败(DBus 错误应答)时返回 false
 * 前置条件: isOldVersion=false;QDBusConnection::call 注入 ErrorMessage
 * 操作步骤: 调用 sendJumpWidgetMessage
 * 预期结果: 返回 false,且未触发 detached 启动
 */
TEST_F(UT_DataDealSingleton, SendJump_NewVersion_AmLaunchFails_ReturnsFalse)
{
    // Arrange
    stopTrickTimer();
    Stub stub;
    stub.set((bool (common::SystemState::*)() const)ADDR(common::SystemState, isOldVersion),
             stub_isOldVersion_false);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode))ADDR(QDBusConnection, call),
             stub_dbusCall_error);
    ut_popup_stub::g_startDetachedCount = 0;
    g_amDbusCallCount = 0;
    // Act
    bool rt = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_NET"));
    // Assert: AM 调用发出但应答为错误,未触发 detached 启动
    EXPECT_FALSE(rt);
    EXPECT_EQ(g_amDbusCallCount, 1);
    EXPECT_EQ(ut_popup_stub::g_startDetachedCount, 0);
}

/**
 * @brief 用例: 启动成功后延时跳转 lambda 在事件循环中执行
 * 前置条件: 打桩 isOldVersion=false;AM 启动应答成功;jumpWidget 打桩计数
 * 操作步骤: sendJumpWidgetMessage 成功后驱动事件循环超过 POPUP_WAITING_TIME
 * 预期结果: 延时跳转恰执行一次且未触发真实 DBus(jumpWidget 已打桩)
 */
TEST_F(UT_DataDealSingleton, SendJump_SuccessfulLaunch_DelayedJumpRunsInEventLoop)
{
    // Arrange
    stopTrickTimer();
    Stub stub;
    stub.set((bool (common::SystemState::*)() const)ADDR(common::SystemState, isOldVersion),
             stub_isOldVersion_false);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode))ADDR(QDBusConnection, call),
             stub_dbusCall_reply);
    stub.set((void (DbusCallMainInterface::*)(QString))ADDR(DbusCallMainInterface, jumpWidget),
             stub_jumpWidget);
    g_amDbusCallCount = 0;
    g_jumpCallCount = 0;
    // Act: 成功发起跳转后,驱动事件循环越过 POPUP_WAITING_TIME
    bool rt = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_CPU"));
    QEventLoop loop;
    QTimer::singleShot(700, &loop, &QEventLoop::quit);
    loop.exec();
    // Assert: 延时跳转 lambda 执行一次,AM 启动一次
    EXPECT_TRUE(rt);
    EXPECT_EQ(g_amDbusCallCount, 1);
    EXPECT_EQ(g_jumpCallCount, 1);
}

/**
 * @brief 用例: 新版本系统 AM 启动成功(DBus 正常应答)时返回 true
 * 前置条件: isOldVersion=false;QDBusConnection::call 注入 ReplyMessage
 * 操作步骤: 调用 sendJumpWidgetMessage(其内部安排延时 DBus 跳转,不在本用例驱动事件循环)
 * 预期结果: 返回 true
 */
TEST_F(UT_DataDealSingleton, SendJump_NewVersion_AmLaunchSucceeds_ReturnsTrue)
{
    // Arrange
    stopTrickTimer();
    Stub stub;
    stub.set((bool (common::SystemState::*)() const)ADDR(common::SystemState, isOldVersion),
             stub_isOldVersion_false);
    stub.set((QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode))ADDR(QDBusConnection, call),
             stub_dbusCall_reply);
    g_amDbusCallCount = 0;
    // Act
    bool rt = DataDealSingleton::getInstance().sendJumpWidgetMessage(QStringLiteral("MSG_PROCESS"));
    // Assert: AM 启动应答成功
    EXPECT_TRUE(rt);
    EXPECT_EQ(g_amDbusCallCount, 1);
}
