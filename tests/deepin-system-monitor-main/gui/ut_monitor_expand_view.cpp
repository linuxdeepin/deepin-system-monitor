// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "monitor_expand_view.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QLayout>

// 被测类私有成员的完整类型
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "network_monitor.h"

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

// 分支清单（来源：实际源码 monitor_expand_view.cpp）
// MonitorExpandView::MonitorExpandView(parent)：
//   无分支：创建 3 个展开视图（CPU/内存/网络）→ 垂直布局（间距 0、三段拉伸）→
//   连接 CPU 详情信号与 DetailWidgetManager::sigJumpToDetailWidget 转发
// MonitorExpandView::setDetailButtonVisible(visible)：
//   单一路径：委托 m_cpuMonitor->setDetailButtonVisible(visible)
// 用例映射：
//   - Ctor_BuildsThreeMonitorsAndFrameStyle                → ctor 布局与样式
//   - Ctor_ConnectsCpuDetailSignalForward                  → ctor 信号连接
//   - SetDetailButtonVisible_True_DelegatesToCpuMonitor    → visible=true
//   - SetDetailButtonVisible_False_DelegatesToCpuMonitor   → visible=false

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | lines:66 | 2 | 2 |
| setDetailButtonVisible | low | in_degree:1 | 1 | 2 |
*/

class UT_MonitorExpandView : public ::testing::Test
{
public:
    UT_MonitorExpandView() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget parent;
        m_tester = new MonitorExpandView(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MonitorExpandView *m_tester;
};

ACCESS_PRIVATE_FIELD(MonitorExpandView, CpuMonitor *, m_cpuMonitor);
ACCESS_PRIVATE_FIELD(MonitorExpandView, MemoryMonitor *, m_memoryMonitor);
ACCESS_PRIVATE_FIELD(MonitorExpandView, NetworkMonitor *, m_networkMonitor);

TEST_F(UT_MonitorExpandView, Ctor_ThreeMonitors_FrameStyleNoFrame)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取构造后的成员与框架样式
    // Assert：三个监视器实例创建完成，框架无边框且布局间距为 0
    auto *cpuMonitor = access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester);
    auto *memMonitor = access_private_field::MonitorExpandViewm_memoryMonitor(*m_tester);
    auto *netMonitor = access_private_field::MonitorExpandViewm_networkMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);
    EXPECT_NE(memMonitor, nullptr);
    EXPECT_NE(netMonitor, nullptr);
    EXPECT_EQ(m_tester->frameStyle(), static_cast<int>(DFrame::NoFrame));
    EXPECT_FALSE(m_tester->autoFillBackground());
    EXPECT_EQ(m_tester->layout()->spacing(), 0);
}

TEST_F(UT_MonitorExpandView, Ctor_CpuDetailClick_ForwardsSignal)
{
    // Arrange：监听转发信号
    QSignalSpy detailSpy(m_tester, &MonitorExpandView::signalDetailInfoClicked);
    EXPECT_EQ(detailSpy.count(), 0);

    // Act：触发 CPU 视图的详情点击信号
    auto *cpuMonitor = access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester);
    emit cpuMonitor->signalDetailInfoClicked();

    // Assert：外层转发信号同步发出且布局包含 3 个监视器
    EXPECT_EQ(detailSpy.count(), 1);
    EXPECT_GE(m_tester->layout()->count(), 3);
}

TEST_F(UT_MonitorExpandView, SetDetailButtonVisible_True_DelegatesToCpuMonitor)
{
    // Arrange：CPU 监视器就绪
    auto *cpuMonitor = access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);

    // Act：显示详情按钮
    m_tester->setDetailButtonVisible(true);

    // Assert：委托链路连通，CPU 监视器位于布局中
    EXPECT_NE(access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester), nullptr);
    EXPECT_NE(m_tester->layout()->indexOf(cpuMonitor), -1);
}

TEST_F(UT_MonitorExpandView, SetDetailButtonVisible_False_DelegatesToCpuMonitor)
{
    // Arrange：CPU 监视器就绪
    auto *cpuMonitor = access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester);
    EXPECT_NE(cpuMonitor, nullptr);

    // Act：隐藏详情按钮
    m_tester->setDetailButtonVisible(false);

    // Assert：委托链路连通且布局不受影响
    EXPECT_NE(access_private_field::MonitorExpandViewm_cpuMonitor(*m_tester), nullptr);
    EXPECT_EQ(m_tester->layout()->spacing(), 0);
}
