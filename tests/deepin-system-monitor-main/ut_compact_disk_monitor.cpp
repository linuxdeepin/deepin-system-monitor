// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "compact_disk_monitor.h"

//dependence
#include "system/diskio_info.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//qt
#include <QPainterPath>
#include <QMouseEvent>
#include <QSignalSpy>

using namespace core::system;


// 私有成员/受保护函数经 addr_pri.h 访问（不修改生产头文件）
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QList<qreal> *, readSpeeds)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QList<qreal> *, writeSpeeds)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, qreal, m_readBps)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, qreal, m_writeBps)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QPainterPath, readPath)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QPainterPath, writePath)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QFont, m_tagFont)
ACCESS_PRIVATE_FIELD(CompactDiskMonitor, QFont, m_statFont)
ACCESS_PRIVATE_FUN(CompactDiskMonitor, void(const QFont &), changeFont)
using UtPathBuildFn = void(QList<double> *, QPainterPath &, qreal);
ACCESS_PRIVATE_FUN(CompactDiskMonitor, UtPathBuildFn, getPainterPathByData)
ACCESS_PRIVATE_FUN(CompactDiskMonitor, void(QMouseEvent *), mouseReleaseEvent)
ACCESS_PRIVATE_FUN(CompactDiskMonitor, void(QMouseEvent *), mouseMoveEvent)

/***************************************STUB begin*********************************************/
namespace {
// CompactDiskMonitor::updateStatus() 依赖 DeviceDB::instance()->diskIoInfo() 的
// diskIoReadBps()/diskIoWriteBps()，打桩隔离真实磁盘 IO 数据源（/proc/diskstats）
static qreal s_readBps = 0.0;
static qreal s_writeBps = 0.0;

static SystemMonitor *stub_SystemMonitor_instance()
{
    return nullptr;  // 阻止构造重型 SystemMonitor 单例
}

static DeviceDB *stub_DeviceDB_instance()
{
    return nullptr;  // diskIoInfo() 已打桩，不会解引用 this
}

static DiskIOInfo *stub_DeviceDB_diskIoInfo(DeviceDB *)
{
    static DiskIOInfo fakeDiskIoInfo;
    return &fakeDiskIoInfo;
}

static qreal stub_DiskIOInfo_readBps(DiskIOInfo *)
{
    return s_readBps;
}

static qreal stub_DiskIOInfo_writeBps(DiskIOInfo *)
{
    return s_writeBps;
}
}  // namespace
/***************************************STUB end***********************************************/

static const int kPointsNumber = 30;  // compact_disk_monitor.cpp 内 const int pointsNumber = 30

// 分支清单（源自 compact_disk_monitor.cpp）:
// 1. ctor: 固定宽 280、固定高 160、读写速率列表各 pointsNumber+1 项
// 2. changeFont: tagFont=-1ptF+Medium、statFont=-1ptF
// 3. updateStatus: 追加速率 -> size>pointsNumber+1 时 pop_front -> 构建 read/writePath -> update()
// 4. getPainterPathByData: for i<dataCount-1 -> y=renderMaxHeight*v/maxVlaue
// 5. mouseReleaseEvent: LeftButton -> emit clicked("MSG_DISK") | 其它按键不发射
// 6. mouseMoveEvent: 直接返回（不发射、不崩溃）
// 7. paintEvent: 完整绘制路径（grab 触发）

class CompactDiskMonitorTest : public ::testing::Test
{
public:
    CompactDiskMonitorTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        // Arrange: 全量打桩，隔离 SystemMonitor/DeviceDB 真实数据源
        m_stub.set(ADDR(SystemMonitor, instance), stub_SystemMonitor_instance);
        m_stub.set(ADDR(DeviceDB, instance), stub_DeviceDB_instance);
        m_stub.set(ADDR(DeviceDB, diskIoInfo), stub_DeviceDB_diskIoInfo);
        m_stub.set(ADDR(DiskIOInfo, diskIoReadBps), stub_DiskIOInfo_readBps);
        m_stub.set(ADDR(DiskIOInfo, diskIoWriteBps), stub_DiskIOInfo_writeBps);

        s_readBps = 0.0;
        s_writeBps = 0.0;

        // Act
        m_tester = new CompactDiskMonitor();
        // Assert: 构造不崩溃
        ASSERT_NE(m_tester, nullptr);
    }

    virtual void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    CompactDiskMonitor *m_tester;
    Stub m_stub;
};

// ───────────────────────── ctor ─────────────────────────
TEST_F(CompactDiskMonitorTest, Construction_DefaultInit_FixedSizeAndSpeedsListInitialized)
{
    // Arrange: SetUp 中已完成打桩（隔离重型单例）并构造控件
    // Act: 读取构造后的固定尺寸与速率列表状态
    const int fixedWidth = m_tester->width();
    const int fixedHeight = m_tester->height();
    const int rdSize = access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->size();
    const int wrSize = access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->size();
    const double rdFirst = access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->first();
    const double wrFirst = access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->first();
    // Assert: 宽度 = 280，高度 = 160
    EXPECT_EQ(fixedWidth, 280);
    EXPECT_EQ(fixedHeight, 160);
    // Assert: 读写速率列表均初始化为 pointsNumber+1 个 0 值
    EXPECT_NE(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester), nullptr);
    EXPECT_NE(access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester), nullptr);
    EXPECT_EQ(rdSize, kPointsNumber + 1);
    EXPECT_EQ(wrSize, kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(rdFirst, 0.0);
    EXPECT_DOUBLE_EQ(wrFirst, 0.0);
}

// ───────────────────────── updateStatus ─────────────────────────
TEST_F(CompactDiskMonitorTest, UpdateStatus_WhenInvoked_ReadsDiskIoInfoAndAppendsSpeeds)
{
    // Arrange: 打桩返回受控磁盘 IO 数据
    s_readBps = 4096.0;
    s_writeBps = 8192.0;

    // Act
    m_tester->updateStatus();

    // Assert: 私有状态被刷新为打桩值
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorm_readBps(*m_tester), 4096.0);
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorm_writeBps(*m_tester), 8192.0);
    // Assert: 列表从初始 31 项 -> 追加 1 项(32) 超界 -> pop 1 项 = 31 项
    EXPECT_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->size(), kPointsNumber + 1);
    EXPECT_EQ(access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->size(), kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->last(), 4096.0);
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->last(), 8192.0);
}

TEST_F(CompactDiskMonitorTest, UpdateStatus_WhenListAtBoundary_PopsOldestEntry)
{
    // Arrange: 清空列表后填满 pointsNumber+1 个可辨识旧值（31 项，处于阈值边界）
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->clear();
    for (int i = 0; i <= kPointsNumber; ++i)
        access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(i * 2.0);
    const double oldestExpectedAfterPop = access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->at(1);
    s_readBps = 1024.0;

    // Act: 追加后变为 32 > pointsNumber+1，应 pop 掉最老一项
    m_tester->updateStatus();

    // Assert: 窗口长度维持 pointsNumber+1，且最老一项（值为 0）已被移除
    EXPECT_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->size(), kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->first(), oldestExpectedAfterPop);
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->last(), 1024.0);
}

TEST_F(CompactDiskMonitorTest, UpdateStatus_WhenListBelowBoundary_KeepsAllEntries)
{
    // Arrange: 列表长度(5)小于 pointsNumber+1，不触发 pop 分支
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->clear();
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(1.0);
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(2.0);
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(3.0);
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(4.0);
    access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->append(5.0);
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->clear();
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->append(1.0);
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->append(2.0);
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->append(3.0);
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->append(4.0);
    access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->append(5.0);

    // Act
    m_tester->updateStatus();

    // Assert: 全部保留，仅各自追加 1 项
    EXPECT_EQ(access_private_field::CompactDiskMonitorreadSpeeds(*m_tester)->size(), 6);
    EXPECT_EQ(access_private_field::CompactDiskMonitorwriteSpeeds(*m_tester)->size(), 6);
}

TEST_F(CompactDiskMonitorTest, UpdateStatus_WhenInvoked_BuildsReadAndWritePaths)
{
    // Arrange
    s_readBps = 2048.0;
    s_writeBps = 1024.0;

    // Act
    m_tester->updateStatus();

    // Assert: 路径由 31 个数据点生成：隐式 moveTo + 30 段 cubicTo（每段 3 元素）= 91
    EXPECT_EQ(access_private_field::CompactDiskMonitorreadPath(*m_tester).elementCount(), 1 + 3 * kPointsNumber);
    EXPECT_EQ(access_private_field::CompactDiskMonitorwritePath(*m_tester).elementCount(), 1 + 3 * kPointsNumber);
}

// ───────────────────────── getPainterPathByData ─────────────────────────
TEST_F(CompactDiskMonitorTest, GetPainterPathByData_WithEmptyList_ReturnsEmptyPath)
{
    // Arrange: 空数据列表
    QList<qreal> emptyList;

    // Act
    QPainterPath path;
    call_private_fun::CompactDiskMonitorgetPainterPathByData(*m_tester, &emptyList, path, 30.0);

    // Assert: 循环体不执行，路径为空
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(CompactDiskMonitorTest, GetPainterPathByData_WithSinglePoint_ReturnsEmptyPath)
{
    // Arrange: 单点数据（dataCount-1 = 0，不进入循环）
    QList<qreal> single {42.0};

    // Act
    QPainterPath path;
    call_private_fun::CompactDiskMonitorgetPainterPathByData(*m_tester, &single, path, 30.0);

    // Assert
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(CompactDiskMonitorTest, GetPainterPathByData_WithThreePoints_GeneratesSevenElements)
{
    // Arrange: 3 个点 -> 隐式 moveTo + 2 段 cubicTo
    QList<qreal> data {0.0, 0.5, 1.0};

    // Act
    QPainterPath path;
    call_private_fun::CompactDiskMonitorgetPainterPathByData(*m_tester, &data, path, 1.0);

    // Assert: 元素数 = 1 + 3*2 = 7，隐式 moveTo 落在原点
    EXPECT_EQ(path.elementCount(), 7);
    EXPECT_DOUBLE_EQ(path.elementAt(0).x, 0.0);
}

// ───────────────────────── changeFont ─────────────────────────
TEST_F(CompactDiskMonitorTest, ChangeFont_WithCustomFont_UpdatesTagAndStatFonts)
{
    // Arrange
    QFont font;
    font.setPointSizeF(9.0);

    // Act
    call_private_fun::CompactDiskMonitorchangeFont(*m_tester, font);

    // Assert: tag 字体 -1ptF 且 Medium 字重
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorm_tagFont(*m_tester).pointSizeF(), 8.0);
    EXPECT_EQ(access_private_field::CompactDiskMonitorm_tagFont(*m_tester).weight(), QFont::Medium);
    // Assert: stat 字体 -1ptF
    EXPECT_DOUBLE_EQ(access_private_field::CompactDiskMonitorm_statFont(*m_tester).pointSizeF(), 8.0);
}

// ───────────────────────── mouseReleaseEvent ─────────────────────────
TEST_F(CompactDiskMonitorTest, MouseReleaseEvent_LeftButton_EmitsClickedSignal)
{
    // Arrange
    QSignalSpy spy(m_tester, &CompactDiskMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::CompactDiskMonitormouseReleaseEvent(*m_tester, &ev);

    // Assert: 左键 -> 发射 clicked("MSG_DISK")
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().size(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_DISK"));
}

TEST_F(CompactDiskMonitorTest, MouseReleaseEvent_RightButton_DoesNotEmitSignal)
{
    // Arrange: 先按右键释放，再按左键释放，验证按键过滤逻辑有效
    QSignalSpy spy(m_tester, &CompactDiskMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    // Act: 右键释放
    call_private_fun::CompactDiskMonitormouseReleaseEvent(*m_tester, &ev);

    // Assert: 非左键 -> 不发射
    EXPECT_EQ(spy.count(), 0);

    // Act: 随后左键释放，验证信号连接本身有效
    QMouseEvent leftEv(QEvent::MouseButtonRelease, QPointF(10, 10),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    call_private_fun::CompactDiskMonitormouseReleaseEvent(*m_tester, &leftEv);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_DISK"));
}

// ───────────────────────── mouseMoveEvent ─────────────────────────
TEST_F(CompactDiskMonitorTest, MouseMoveEvent_AnyMovement_IsIgnoredAndSilent)
{
    // Arrange: mouseMoveEvent 按设计屏蔽鼠标移动响应；随后以左键释放验证事件管线完好
    QSignalSpy spy(m_tester, &CompactDiskMonitor::clicked);
    QMouseEvent ev(QEvent::MouseMove, QPointF(5, 5),
                   Qt::NoButton, Qt::LeftButton, Qt::NoModifier);

    // Act: 两次移动不应产生任何点击
    call_private_fun::CompactDiskMonitormouseMoveEvent(*m_tester, &ev);
    call_private_fun::CompactDiskMonitormouseMoveEvent(*m_tester, &ev);

    // Assert: 不崩溃且不产生任何点击信号
    EXPECT_EQ(spy.count(), 0);

    // Act: 左键释放
    QMouseEvent relEv(QEvent::MouseButtonRelease, QPointF(5, 5),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    call_private_fun::CompactDiskMonitormouseReleaseEvent(*m_tester, &relEv);

    // Assert: 点击事件管线未被移动事件破坏
    EXPECT_EQ(spy.count(), 1);
}

// ───────────────────────── paintEvent ─────────────────────────
TEST_F(CompactDiskMonitorTest, PaintEvent_AfterGrab_RendersPixmapWithWidgetSize)
{
    // Arrange: 注入受控数据后触发绘制
    s_readBps = 4096.0;
    s_writeBps = 8192.0;
    m_tester->updateStatus();

    // Act: 第一次 grab 触发 paintEvent，稳定尺寸后再次 grab
    const QPixmap firstPix = m_tester->grab();
    const QPixmap pix = m_tester->grab();

    // Assert: 渲染非空且尺寸与控件一致
    EXPECT_FALSE(firstPix.isNull());
    EXPECT_FALSE(pix.isNull());
    EXPECT_EQ(pix.size(), m_tester->size());
}
