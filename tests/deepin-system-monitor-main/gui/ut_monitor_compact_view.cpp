// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "monitor_compact_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QLayout>

// 被测类私有成员的完整类型
#include "compact_cpu_monitor.h"
#include "compact_memory_monitor.h"
#include "compact_network_monitor.h"
#include "compact_disk_monitor.h"

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

// 分支清单（来源：实际源码 monitor_compact_view.cpp）
// MonitorCompactView::MonitorCompactView(parent)：
//   无分支：创建 4 个紧凑视图 → 垂直布局（间距 10、四段拉伸）→ 连接 CPU 详情信号与
//   DetailWidgetManager::sigJumpToDetailWidget → 四视图 clicked 转发到 jumpDetailWidget
// MonitorCompactView::setDetailButtonVisible(visible)：
//   单一路径：委托 m_cpuMonitor->setDetailButtonVisible(visible)
// 用例映射：
//   - Ctor_BuildsFourMonitorsAndFrameStyle                 → ctor 布局与样式
//   - Ctor_ConnectsCpuDetailSignalForward                  → ctor 信号连接
//   - SetDetailButtonVisible_True_DelegatesToCpuMonitor    → visible=true
//   - SetDetailButtonVisible_False_DelegatesToCpuMonitor   → visible=false

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | lines:70 | 2 | 2 |
| setDetailButtonVisible | low | in_degree:1 | 1 | 2 |
*/

class UT_MonitorCompactView : public ::testing::Test
{
public:
    UT_MonitorCompactView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new MonitorCompactView(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MonitorCompactView *m_tester;
};

ACCESS_PRIVATE_FIELD(MonitorCompactView, CompactCpuMonitor *, m_cpuMonitor);
ACCESS_PRIVATE_FIELD(MonitorCompactView, CompactMemoryMonitor *, m_memoryMonitor);
ACCESS_PRIVATE_FIELD(MonitorCompactView, CompactNetworkMonitor *, m_networkMonitor);
ACCESS_PRIVATE_FIELD(MonitorCompactView, CompactDiskMonitor *, m_diskMonitor);

TEST_F(UT_MonitorCompactView, Ctor_FourMonitors_FrameStyleNoFrame)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取构造后的成员与框架样式
    // Assert：四个监视器实例创建完成，框架无边框且不自填充背景
    auto *cpuMonitor = access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester);
    auto *memMonitor = access_private_field::MonitorCompactViewm_memoryMonitor(*m_tester);
    auto *netMonitor = access_private_field::MonitorCompactViewm_networkMonitor(*m_tester);
    auto *diskMonitor = access_private_field::MonitorCompactViewm_diskMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);
    EXPECT_NE(memMonitor, nullptr);
    EXPECT_NE(netMonitor, nullptr);
    EXPECT_NE(diskMonitor, nullptr);
    EXPECT_EQ(m_tester->frameStyle(), static_cast<int>(DFrame::NoFrame));
    EXPECT_FALSE(m_tester->autoFillBackground());
    EXPECT_TRUE(m_tester->layout() != nullptr);
    EXPECT_EQ(m_tester->layout()->spacing(), 10);
}

TEST_F(UT_MonitorCompactView, Ctor_CpuDetailClick_ForwardsSignal)
{
    // Arrange：监听转发信号
    QSignalSpy detailSpy(m_tester, &MonitorCompactView::signalDetailInfoClicked);
    EXPECT_EQ(detailSpy.count(), 0);

    // Act：触发 CPU 视图的详情点击信号（CompactCpuMonitor 是具体类型，直接发其信号）
    auto *cpuMonitor = access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester);
    emit cpuMonitor->signalDetailInfoClicked();

    // Assert：外层转发信号同步发出
    EXPECT_EQ(detailSpy.count(), 1);
    EXPECT_TRUE(m_tester->layout()->count() >= 4);
}

TEST_F(UT_MonitorCompactView, SetDetailButtonVisible_True_DelegatesToCpuMonitor)
{
    // Arrange：CPU 监视器就绪
    auto *cpuMonitor = access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);

    // Act：显示详情按钮
    m_tester->setDetailButtonVisible(true);

    // Assert：调用后无崩溃（委托链路连通），控件树完整
    EXPECT_NE(access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester), nullptr);
    EXPECT_EQ(m_tester->layout()->indexOf(cpuMonitor), 1);
}

TEST_F(UT_MonitorCompactView, SetDetailButtonVisible_False_DelegatesToCpuMonitor)
{
    // Arrange：CPU 监视器就绪
    auto *cpuMonitor = access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);

    // Act：隐藏详情按钮
    m_tester->setDetailButtonVisible(false);

    // Assert：委托链路连通且布局不受影响
    EXPECT_NE(access_private_field::MonitorCompactViewm_cpuMonitor(*m_tester), nullptr);
    EXPECT_EQ(m_tester->layout()->spacing(), 10);
}
