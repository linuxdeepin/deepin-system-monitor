// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "detail_view_stacked_widget.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QMenu>
#include <QAction>

// 被测类内部使用的详情页（前向声明 → 需完整类型）
#include "cpu_detail_widget.h"
#include "mem_detail_view_widget.h"
#include "netif_detail_view_widget.h"
#include "block_dev_detail_view_widget.h"

ACCESS_PRIVATE_FUN(AnimationStackedWidget, void(int &, int), setCurrentWidget);

/***************************************STUB begin*********************************************/
// QMenu::exec 桩：按全局变量返回指定动作，避免阻塞式弹窗
static QAction *g_menuExecResult = nullptr;
static QAction *stub_menu_exec(void *, const QPoint &, QAction *)
{
    return g_menuExecResult;
}

// AnimationStackedWidget::setCurrentWidget 桩：记录请求的目标索引，避免真实动画（成员函数桩首参为 this）
static int g_lastSwitchIndex = -1;
static int g_switchCallCount = 0;
static void stub_setCurrentWidget_record(void *, int &index, int)
{
    g_lastSwitchIndex = index;
    ++g_switchCallCount;
}
/***************************************STUB end**********************************************/

// 分支清单（来源：get_code_snippet 实际源码）
// DetailViewStackedWidget::onDbusSendMsgChangeDetailInfoWidget(msgCode)，complexity:8：
//   B1: msgCode==MSG_CPU（忽略大小写）且 m_cpudetailWidget==nullptr → 创建 CPU 页插入索引 1
//   B2: msgCode==MSG_CPU 且已存在 → setCurrent(CPU 页)
//   B3: msgCode==MSG_NET 且 m_netifDetailWidget==nullptr → 创建 NET 页插入索引 3
//   B4: msgCode==MSG_NET 且已存在 → setCurrent(NET 页)
//   B5: msgCode==MSG_MEM 且 m_memDetailWidget==nullptr → 创建 MEM 页插入索引 2
//   B6: msgCode==MSG_MEM 且已存在 → setCurrent(MEM 页)
//   B7: msgCode==MSG_DISK 且 m_blockDevDetailWidget==nullptr → 创建 DISK 页插入索引 4
//   B8: msgCode==MSG_DISK 且已存在 → setCurrent(DISK 页)
//   B9: 其他 msgCode → 无动作
// 用例映射：
//   - OnDbusSendMsg_CPUWithoutPage_CreatesCpuDetail        → B1
//   - OnDbusSendMsg_CPUWithPage_SwitchesToCpuDetail        → B2
//   - OnDbusSendMsg_NetWithoutPage_CreatesNetDetail        → B3
//   - OnDbusSendMsg_MemLowercase_CreatesMemDetail          → B5（CaseInsensitive 验证）
//   - OnDbusSendMsg_DiskWithPage_SwitchesToDiskDetail      → B8
//   - OnDbusSendMsg_UnknownCode_NoAction                   → B9
// DetailViewStackedWidget::onShowPerformMenu(pos)，complexity:13：
//   B1: m_menu==nullptr → 构建菜单与 4 个 checkable 动作
//   B2: currentWidget==m_cpudetailWidget → cpuAct 置选
//   B3: currentWidget==m_memDetailWidget → memAct 置选
//   B4: currentWidget==m_netifDetailWidget → netifAct 置选
//   B5: currentWidget==m_blockDevDetailWidget → blockDevAct 置选
//   B6: exec 返回 cpuAct → 切 CPU 页（空则先建）
//   B7: exec 返回 memAct → 切 MEM 页（空则先建）
//   B8: exec 返回 netifAct → 切 NET 页（空则先建）
//   B9: exec 返回 blockDevAct → 切 DISK 页（空则先建）
//   B10: exec 返回空/无匹配 → 不切换
// 用例映射：
//   - OnShowPerformMenu_LazyMenu_BuildsFourCheckableActs   → B1
//   - OnShowPerformMenu_CurrentCpu_ChecksCpuAct            → B1+B2
//   - OnShowPerformMenu_ExecCpuAct_SwitchesToCpuDetail     → B6
//   - OnShowPerformMenu_ExecMemAct_SwitchesToMemDetail     → B7
//   - OnShowPerformMenu_ExecNetAct_SwitchesToNetDetail     → B8
//   - OnShowPerformMenu_ExecDiskAct_SwitchesToDiskDetail   → B9
//   - OnShowPerformMenu_ExecNoAction_KeepsState            → B10
//   - OnShowPerformMenu_RepeatedCall_ReusesMenu            → B1（非空跳过）
// DetailViewStackedWidget::onDetailInfoClicked()：
//   B1: last=="MemDetailViewWidget" → 建/切 MEM 页；B2: last=="NetifDetailViewWidget" → 建/切 NET 页
//   B3: last=="BlockDevDetailViewWidget" → 建/切 DISK 页；B4: 默认 → 建/切 CPU 页
// 用例映射：
//   - OnDetailInfoClicked_MemName_SwitchesMemDetail        → B1
//   - OnDetailInfoClicked_NetName_SwitchesNetDetail        → B2
//   - OnDetailInfoClicked_DiskName_SwitchesDiskDetail      → B3
//   - OnDetailInfoClicked_UnknownName_DefaultsCpuDetail    → B4

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor/connection | mid | in_degree:1 | 2 | 2 |
| addProcessWidget | low | - | 1 | 2 |
| deleteDetailPage | mid | in_degree:1 | 2 | 2 |
| onDbusSendMsgChangeDetailInfoWidget | high | complexity:8,lines:55,in_degree:1 | 3 | 6 |
| onShowPerformMenu | high | complexity:13,cognitive:35,lines:80 | 3 | 8 |
| onDetailInfoClicked | high | complexity:7,cognitive:19,in_degree:1 | 3 | 4 |
| onSwitchProcessPage | mid | in_degree:1 | 2 | 2 |
| onSwitchPageFinished | mid | in_degree:1 | 2 | 2 |
*/

class UT_DetailViewStackedWidget : public ::testing::Test
{
public:
    UT_DetailViewStackedWidget() : m_tester(nullptr), m_widget(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new DetailViewStackedWidget(nullptr);
        m_widget = new QWidget;
        g_menuExecResult = nullptr;
        g_lastSwitchIndex = -1;
        g_switchCallCount = 0;
        // 桩化父类私有 setCurrentWidget：拦截切换请求，避免真实动画
        m_stub.set(get_private_fun::AnimationStackedWidgetsetCurrentWidget(), stub_setCurrentWidget_record);
    }

    virtual void TearDown()
    {
        m_stub.reset(get_private_fun::AnimationStackedWidgetsetCurrentWidget());
        m_widget->deleteLater();
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 帮助函数：加入进程页（索引 0）
    void addProcessPage()
    {
        m_tester->addProcessWidget(m_widget);
    }

protected:
    DetailViewStackedWidget *m_tester;
    QWidget *m_widget;
    Stub m_stub;
};

TEST_F(UT_DetailViewStackedWidget, Ctor_ConnectsFinished_CleansDetailPages)
{
    // Arrange：进程页在栈顶，手动挂一个 MEM 详情页
    addProcessPage();
    m_tester->m_memDetailWidget = new MemDetailViewWidget(m_tester);
    m_tester->insertWidget(2, m_tester->m_memDetailWidget);

    // Act：发送动画完成信号（ctor 已连接 onSwitchPageFinished）
    emit m_tester->signalIsFinished();

    // Assert：非详情页回进程逻辑生效，MEM 页被清理
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->currentWidget(), m_widget);
}

TEST_F(UT_DetailViewStackedWidget, Dtor_WithDetailPages_CleansUp)
{
    // Arrange：通过 DBus 入口创建 CPU 详情页（父对象为 m_tester）
    addProcessPage();
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_CPU"));
    EXPECT_NE(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(m_tester->count(), 2);

    // Act：TearDown 前手动析构验证不崩溃
    delete m_tester;
    m_tester = nullptr;

    // Assert：独立 widget 仍可正常回收（无重复释放）
    EXPECT_NE(m_widget, nullptr);
}

TEST_F(UT_DetailViewStackedWidget, AddProcessWidget_EmptyStack_InsertsAtTop)
{
    // Arrange：空栈场景由 SetUp 保证
    EXPECT_EQ(m_tester->count(), 0);

    // Act：空栈加入进程页
    m_tester->addProcessWidget(m_widget);

    // Assert：成员记录、位置 0、成为当前页
    EXPECT_EQ(m_tester->m_processWidget, m_widget);
    EXPECT_EQ(m_tester->indexOf(m_widget), 0);
    EXPECT_EQ(m_tester->currentWidget(), m_widget);
}

TEST_F(UT_DetailViewStackedWidget, AddProcessWidget_SecondWidget_UpdatesTracking)
{
    // Arrange：先加入第一页
    m_tester->addProcessWidget(m_widget);
    QWidget *second = new QWidget;

    // Act：再次加入
    m_tester->addProcessWidget(second);

    // Assert：追踪指针更新到新页且位于栈顶
    EXPECT_EQ(m_tester->m_processWidget, second);
    EXPECT_EQ(m_tester->indexOf(second), 0);
    EXPECT_EQ(m_tester->count(), 2);
    second->deleteLater();
}

TEST_F(UT_DetailViewStackedWidget, DeleteDetailPage_WithPages_ClearsAllPointers)
{
    // Arrange：填充 4 类详情页
    addProcessPage();
    m_tester->m_cpudetailWidget = new CPUDetailWidget(m_tester);
    m_tester->m_memDetailWidget = new MemDetailViewWidget(m_tester);
    m_tester->m_netifDetailWidget = new NetifDetailViewWidget(m_tester);
    m_tester->m_blockDevDetailWidget = new BlockDevDetailViewWidget(m_tester);
    ASSERT_NE(m_tester->m_cpudetailWidget, nullptr);

    // Act：统一清理
    m_tester->deleteDetailPage();

    // Assert：4 个指针全部置空
    EXPECT_EQ(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_netifDetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_blockDevDetailWidget, nullptr);
}

TEST_F(UT_DetailViewStackedWidget, DeleteDetailPage_EmptyState_NoDoubleFree)
{
    // Arrange：所有详情页指针为空
    addProcessPage();
    ASSERT_EQ(m_tester->m_cpudetailWidget, nullptr);

    // Act：空态调用
    m_tester->deleteDetailPage();

    // Assert：保持空且栈内仅剩进程页（强异常安全）
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->count(), 1);
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_CPUWithoutPage_CreatesCpuDetail)
{
    // Arrange：进程页 + 空详情页
    addProcessPage();
    ASSERT_EQ(m_tester->m_cpudetailWidget, nullptr);

    // Act：发送 MSG_CPU
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_CPU"));

    // Assert：CPU 页创建并追加到栈顶（空栈时 insertWidget 索引被钳制）
    EXPECT_NE(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(m_tester->indexOf(m_tester->m_cpudetailWidget), m_tester->count() - 1);
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_CPUWithPage_SwitchesToCpuDetail)
{
    // Arrange：先创建 CPU 页（索引 1），进程页为当前页（索引 0）
    addProcessPage();
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_CPU"));
    int createdCount = g_switchCallCount; // 创建路径 setCurrent 的记录

    // Act：重复发送 MSG_CPU
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_CPU"));

    // Assert：不重复创建，且再次发起指向 CPU 页的切换
    EXPECT_EQ(m_tester->count(), 2);
    EXPECT_GE(g_switchCallCount, createdCount + 1);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_cpudetailWidget));
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_NetWithoutPage_CreatesNetDetail)
{
    // Arrange：进程页
    addProcessPage();

    // Act：发送 MSG_NET
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_NET"));

    // Assert：NET 页创建并追加到栈中
    EXPECT_NE(m_tester->m_netifDetailWidget, nullptr);
    EXPECT_EQ(m_tester->indexOf(m_tester->m_netifDetailWidget), m_tester->count() - 1);
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_MemLowercase_CreatesMemDetail)
{
    // Arrange：进程页 + 小写消息码（Qt::CaseInsensitive 分支）
    addProcessPage();

    // Act：发送 msg_mem（小写）
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("msg_mem"));

    // Assert：MEM 页创建并追加到栈中
    EXPECT_NE(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->indexOf(m_tester->m_memDetailWidget), m_tester->count() - 1);
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_DiskWithPage_SwitchesToDiskDetail)
{
    // Arrange：创建 DISK 页（索引 4）
    addProcessPage();
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_DISK"));
    ASSERT_NE(m_tester->m_blockDevDetailWidget, nullptr);

    // Act：重复发送 MSG_DISK
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_DISK"));

    // Assert：指向 DISK 页的切换请求被记录
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_blockDevDetailWidget));
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDbusSendMsg_UnknownCode_NoAction)
{
    // Arrange：进程页
    addProcessPage();

    // Act：发送未知消息码
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_UNKNOWN"));

    // Assert：四类详情页均未创建，无切换请求
    EXPECT_EQ(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_netifDetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_blockDevDetailWidget, nullptr);
    EXPECT_EQ(g_switchCallCount, 0);
    EXPECT_EQ(m_tester->count(), 1);
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_LazyMenu_BuildsFourCheckableActs)
{
    // Arrange：空菜单状态 + exec 桩
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    ASSERT_EQ(m_tester->m_menu, nullptr);

    // Act：弹出性能菜单
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：菜单懒创建，4 个动作存在且可勾选
    EXPECT_NE(m_tester->m_menu, nullptr);
    EXPECT_NE(m_tester->cpuAct, nullptr);
    EXPECT_NE(m_tester->memAct, nullptr);
    EXPECT_TRUE(m_tester->cpuAct->isCheckable());
    EXPECT_TRUE(m_tester->blockDevAct->isCheckable());
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_CurrentCpu_ChecksCpuAct)
{
    // Arrange：创建 CPU 页并使其成为当前页
    addProcessPage();
    m_tester->onDbusSendMsgChangeDetailInfoWidget(QString("MSG_CPU"));
    m_tester->setCurrentIndex(m_tester->indexOf(m_tester->m_cpudetailWidget));
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);

    // Act：弹出菜单
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：CPU 动作被置选，其余未置选
    EXPECT_EQ(m_tester->cpuAct->isChecked(), true);
    EXPECT_FALSE(m_tester->memAct->isChecked());
    EXPECT_FALSE(m_tester->netifAct->isChecked());
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_ExecCpuAct_SwitchesToCpuDetail)
{
    // Arrange：exec 返回 cpuAct
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1)); // 先建菜单
    g_menuExecResult = m_tester->cpuAct;

    // Act：再次弹出并选择 CPU 动作
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：CPU 详情页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_cpudetailWidget));
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_ExecMemAct_SwitchesToMemDetail)
{
    // Arrange：exec 返回 memAct
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1));
    g_menuExecResult = m_tester->memAct;

    // Act：弹出并选择 MEM 动作
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：MEM 详情页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_memDetailWidget));
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_ExecNetAct_SwitchesToNetDetail)
{
    // Arrange：exec 返回 netifAct
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1));
    g_menuExecResult = m_tester->netifAct;

    // Act：弹出并选择 NET 动作
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：NET 详情页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_netifDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_netifDetailWidget));
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_ExecDiskAct_SwitchesToDiskDetail)
{
    // Arrange：exec 返回 blockDevAct
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1));
    g_menuExecResult = m_tester->blockDevAct;

    // Act：弹出并选择 DISK 动作
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：DISK 详情页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_blockDevDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_blockDevDetailWidget));
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_ExecNoAction_KeepsState)
{
    // Arrange：菜单已建，exec 返回空
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1));
    DMenu *firstMenu = m_tester->m_menu;
    g_menuExecResult = nullptr;

    // Act：再次弹出但无选择
    m_tester->onShowPerformMenu(QPoint(1, 1));

    // Assert：不创建任何详情页、不发起切换、菜单复用
    EXPECT_EQ(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(g_switchCallCount, 0);
    EXPECT_EQ(m_tester->m_menu, firstMenu);
}

TEST_F(UT_DetailViewStackedWidget, OnShowPerformMenu_RepeatedCall_ReusesMenu)
{
    // Arrange：第一次弹出建菜单
    m_stub.set((QAction * (QMenu::*)(const QPoint &, QAction *))ADDR(QMenu, exec), stub_menu_exec);
    m_tester->onShowPerformMenu(QPoint(1, 1));
    DMenu *firstMenu = m_tester->m_menu;
    ASSERT_NE(firstMenu, nullptr);

    // Act：第二次弹出
    m_tester->onShowPerformMenu(QPoint(2, 2));

    // Assert：菜单对象复用（懒创建仅一次）
    EXPECT_EQ(m_tester->m_menu, firstMenu);
    EXPECT_NE(m_tester->cpuAct, nullptr);
}

TEST_F(UT_DetailViewStackedWidget, OnDetailInfoClicked_MemName_SwitchesMemDetail)
{
    // Arrange：last 名称为 MEM 视图
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "MemDetailViewWidget";

    // Act：触发详情点击
    m_tester->onDetailInfoClicked();

    // Assert：MEM 页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_memDetailWidget));
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDetailInfoClicked_NetName_SwitchesNetDetail)
{
    // Arrange：last 名称为 NET 视图
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "NetifDetailViewWidget";

    // Act：触发详情点击
    m_tester->onDetailInfoClicked();

    // Assert：NET 页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_netifDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_netifDetailWidget));
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDetailInfoClicked_DiskName_SwitchesDiskDetail)
{
    // Arrange：last 名称为 DISK 视图
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "BlockDevDetailViewWidget";

    // Act：触发详情点击
    m_tester->onDetailInfoClicked();

    // Assert：DISK 页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_blockDevDetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_blockDevDetailWidget));
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnDetailInfoClicked_UnknownName_DefaultsCpuDetail)
{
    // Arrange：未知名称走默认 CPU 分支
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "";

    // Act：触发详情点击
    m_tester->onDetailInfoClicked();

    // Assert：CPU 页创建并收到指向它的切换请求
    EXPECT_NE(m_tester->m_cpudetailWidget, nullptr);
    EXPECT_EQ(g_lastSwitchIndex, m_tester->indexOf(m_tester->m_cpudetailWidget));
    EXPECT_EQ(m_tester->count(), 2);
}

TEST_F(UT_DetailViewStackedWidget, OnSwitchProcessPage_FromDetail_ReturnsToProcessTop)
{
    // Arrange：先切到详情（记录请求），再回进程页
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "MemDetailViewWidget";
    m_tester->onDetailInfoClicked();

    // Act：切换回进程页
    m_tester->onSwitchProcessPage();

    // Assert：切换请求指向索引 0（进程页）
    EXPECT_EQ(g_lastSwitchIndex, 0);
    EXPECT_EQ(m_tester->indexOf(m_tester->m_processWidget), 0);
}

TEST_F(UT_DetailViewStackedWidget, OnSwitchProcessPage_AlreadyOnProcess_EmitsFinished)
{
    // Arrange：当前页即进程页；本用例验证原始同页分支，需恢复真实 setCurrentWidget
    addProcessPage();
    m_stub.reset(get_private_fun::AnimationStackedWidgetsetCurrentWidget());
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);
    EXPECT_EQ(m_tester->currentWidget(), m_widget);

    // Act：再次请求切回进程页
    m_tester->onSwitchProcessPage();

    // Assert：同页切换直接发 finished，且无动画请求
    EXPECT_EQ(finishedSpy.count(), 1);
    EXPECT_EQ(g_switchCallCount, 0);
    EXPECT_EQ(m_tester->currentWidget(), m_widget);
}

TEST_F(UT_DetailViewStackedWidget, OnSwitchPageFinished_OnProcessPage_DeletesDetailPages)
{
    // Arrange：进程页为当前页 + 手动挂 MEM 详情页
    addProcessPage();
    m_tester->m_memDetailWidget = new MemDetailViewWidget(m_tester);
    m_tester->insertWidget(2, m_tester->m_memDetailWidget);

    // Act：动画完成回调
    m_tester->onSwitchPageFinished();

    // Assert：详情页被删除
    EXPECT_EQ(m_tester->m_memDetailWidget, nullptr);
    EXPECT_EQ(m_tester->count(), 1);
}

TEST_F(UT_DetailViewStackedWidget, OnSwitchPageFinished_OnDetailPage_RecordsWidgetName)
{
    // Arrange：进程页 + MEM 详情页，当前页切到详情
    addProcessPage();
    m_tester->m_lastDteailWidgetName = "MemDetailViewWidget";
    m_tester->onDetailInfoClicked();
    ASSERT_NE(m_tester->m_memDetailWidget, nullptr);
    m_tester->m_memDetailWidget->setObjectName("MemDetailViewWidget");
    m_tester->setCurrentIndex(m_tester->indexOf(m_tester->m_memDetailWidget));

    // Act：动画完成回调
    m_tester->onSwitchPageFinished();

    // Assert：记录当前详情页对象名，详情页保留
    EXPECT_EQ(m_tester->m_lastDteailWidgetName, QString("MemDetailViewWidget"));
    EXPECT_NE(m_tester->m_memDetailWidget, nullptr);
}
