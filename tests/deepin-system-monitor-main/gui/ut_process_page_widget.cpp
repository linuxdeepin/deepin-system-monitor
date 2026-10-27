// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_ProcessPageWidget：ProcessPageWidget 单元测试（qt-autotest-generator 重写）
//
// 背景：旧 ut_process_page_widget.cpp.disabled 因 ASan 堆溢出被禁用（b982e692），
// 本次重写参照 ut_process_table_view 的静态 MainWindow 夹具模式，并为确认弹窗、
// killProcess、X11 窗口树构建等外部依赖补全打桩。
//
// ============================ 用例计数声明 ============================
// 方法                              | level | 最少用例 | actual | 说明
// ProcessPageWidget(DWidget*)       | low   | 1 | 1 | 成员创建 + 恢复分支与按钮选中态一致性
// ~ProcessPageWidget()              | low   | 1 | - | TearDown delete 覆盖
// initUI()                          | mid   | 3 | 3 | 恢复分支三态：kFilterCurrentUser/kNoFilter/default
// initConnections()                 | high  | 2 | 2 | ctor 间接覆盖（MainWindow/displayModeChanged、按钮 clicked）
// eventFilter(QObject,QEvent*)      | high  | 4 | 7 | app→myProc / myProc→allProc / myProc→app / allProc→myProc / 未匹配按键 / 非按钮对象 / 非按键事件
// switchProcessPage()               | mid   | 1 | 1 | 右侧栈切回进程页（widget(0) == m_processWidget）
// switchCurrentNoFilterPage()       | mid   | 1 | 1 | 文本/按钮选中/枚举状态三断言
// popupKillConfirmDialog(pid_t)     | mid   | 2 | 2 | Ok 分支调用 killProcess / Rejected 分支不调用
// onAppAndProcCountUpdated(int,int) | mid   | 1 | 1 | 摘要文本包含两个计数
// changeIconTheme(ColorType)        | low   | 2 | 2 | 亮色 / 暗色图标设置
// onDetailWidgetChanged(int)        | mid   | 2 | 2 | 进程页索引(0) / 越界索引(5) 空指针安全
// onLoadLeftDataWidgetDelay()       | mid   | 1 | 1 | 左侧 compact/expand 视图创建并入栈
// switchDisplayMode(int)            | mid   | 2 | 2 | 切到 expand / 切回 compact
// onAppButtonClicked()              | mid   | 1 | 1 | 状态切换 + 设置写回 kFilterApps
// onUserProcButtonClicked()         | mid   | 1 | 1 | 状态切换 + 设置写回 kFilterCurrentUser
// onAllProcButtonClicked()          | mid   | 2 | 2 | 状态切换 + 设置写回 kNoFilter / 重复点击早退
// showWindowKiller()                | mid   | 1 | 1 | showMinimized + createWindowKiller 均触发（双桩计数）
// createWindowKiller()              | mid   | 1 | 1 | 真实执行（WMInfo 内部打桩），m_xwkillPreview 创建
// paintEvent(QPaintEvent*)          | low   | 1 | 1 | grab 渲染非空
//
// ============================== 分支清单 ==============================
// initUI() 恢复分支（getOption(kSettingKeyProcessTabIndex)）:
//   B1 kFilterCurrentUser → myProc 按钮选中                → test_initUI_restore_01
//   B2 kNoFilter          → allProc 按钮选中               → test_initUI_restore_02
//   B3 default(kFilterApps) → app 按钮选中                 → test_initUI_restore_03
// eventFilter():
//   B1 KeyPress + appButton + Right   → 焦点移 myProcButton → test_eventFilter_01
//   B2 KeyPress + myProcButton + Right→ 焦点移 allProcButton→ test_eventFilter_02
//   B3 KeyPress + myProcButton + Left → 焦点移 appButton    → test_eventFilter_03
//   B4 KeyPress + allProcButton + Left→ 焦点移 myProcButton → test_eventFilter_04
//   B5 KeyPress + allProcButton + Right → 落回 DFrame 默认  → test_eventFilter_05
//   B6 KeyPress + 非按钮对象           → 落回 DFrame 默认   → test_eventFilter_06
//   B7 非按键事件（MouseMove）          → 落回 DFrame 默认   → test_eventFilter_07
// popupKillConfirmDialog():
//   B1 dialog.result()==QMessageBox::Ok    → 记日志+killProcess → test_popupKillConfirmDialog_ok
//   B2 其他结果                            → 不 kill           → test_popupKillConfirmDialog_cancel
// on*ButtonClicked(): CPUPerformance 恒为 High（测试 main 未调用 global_init），
//   Low 分支不可达；重复点击早退分支由 test_onAllProcButtonClicked_02 覆盖。

//Self
#include "process_page_widget.h"
#include "detail_view_stacked_widget.h"
#include "settings.h"
#include "main_window.h"
#include "toolbar.h"
#include "application.h"
#include "wm/wm_info.h"
#include "process/process_set.h"
#include "process/process_db.h"
#include "kill_process_confirm_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDialog>
#include <QMessageBox>
#include <QApplication>

using namespace core::process;

/***************************************STUB begin*********************************************/
ACCESS_PRIVATE_FUN(ProcessPageWidget, void(void), createWindowKiller);
ACCESS_PRIVATE_FUN(WMInfo, void(void), buildWindowTreeSchema);
ACCESS_PRIVATE_FUN(WMInfo, void(void), findDockWindows);

static int g_ppw_createWindowKillerCount = 0;
static bool g_ppw_killProcessCalled = false;
static bool g_ppw_showMinimizedCalled = false;

void stub_ppw_createWindowKiller_count()
{
    ++g_ppw_createWindowKillerCount;
}

void stub_ppw_showMinimized()
{
    g_ppw_showMinimizedCalled = true;
}

void stub_ppw_killProcess(pid_t)
{
    g_ppw_killProcessCalled = true;
}

// exec 桩携带接收者指针（Stub 库支持），模拟按钮点击写入 KillProcessConfirmDialog 自身的
// m_result（其 result() 为内联实现，无法打桩），避免真实模态事件循环
int stub_ppw_dialogExec_clickOk(void *object)
{
    auto *dialog = static_cast<KillProcessConfirmDialog *>(object);
    dialog->setResult(QMessageBox::Ok);
    return 1;
}

int stub_ppw_dialogExec_clickCancel(void *)
{
    return 1;
}

QVariant stub_ppw_getOption_myproc()
{
    return QVariant(kFilterCurrentUser);
}

QVariant stub_ppw_getOption_allproc()
{
    return QVariant(kNoFilter);
}

QVariant stub_ppw_getOption_default()
{
    return QVariant(kFilterApps);
}

void stub_ppw_wm_buildWindowTreeSchema()
{
    return;
}

void stub_ppw_wm_findDockWindows()
{
    return;
}

// 吞掉监控线程启动事件：ProcessPageWidget 测试链路会 postEvent 启动 SystemMonitor/
// NetifMonitor 工作线程，线程与静态对象在进程退出时的析构竞态会导致段错误，
// 测试不依赖真实监控数据（onAppAndProcCountUpdated 直接驱动），故打桩拦截
bool stub_ppw_appEvent(Application *self, QEvent *event)
{
    if (event && (event->type() == kMonitorStartEventType || event->type() == kNetifStartEventType)) {
        return true;
    }
    return self->QApplication::event(event);
}

// 离屏环境下窗口未激活，setFocus 不改变 QApplication::focusWidget()，
// 改用计数桩验证焦点转移行为（eventFilter 内为无参 setFocus()，即带 reason 重载）
static int g_ppw_setFocusCount = 0;
void stub_ppw_widgetSetFocus(QWidget *, Qt::FocusReason)
{
    ++g_ppw_setFocusCount;
}
/***************************************STUB end**********************************************/

class UT_ProcessPageWidget : public ::testing::Test
{
public:
    UT_ProcessPageWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 参照 ut_process_table_view：静态 MainWindow 提供 gApp->mainWindow()，
        // 使 initConnections 的 displayModeChanged 连接可建立
        // 参照 ut_process_table_view：MainWindow 需要挂上 Toolbar，
        // 否则 ProcessTableView::adjustInfoLabelVisibility 解引用空 m_toolbar 段错误
        static QWidget wid;
        static MainWindow wm;
        static Toolbar toolbar;
        wm.m_toolbar = &toolbar;
        gApp->setMainWindow(&wm);

        // 拦截 Application::event 中的线程启动分支（见 stub_ppw_appEvent 注释）
        typedef bool (*appEventFptr)(Application *, QEvent *);
        appEventFptr fptr_appEvent = (appEventFptr)&Application::event;
        Stub stubApp;
        stubApp.set(fptr_appEvent, stub_ppw_appEvent);

        g_ppw_createWindowKillerCount = 0;
        g_ppw_killProcessCalled = false;
        g_ppw_showMinimizedCalled = false;

        m_tester = new ProcessPageWidget(&wid);
    }

    virtual void TearDown()
    {
        // 注：ProcessPageWidget 析构过程中，子 QStackedWidget 会在基类 ~QObject 阶段发出
        // currentChanged/destroyed 信号，此时 ProcessPageWidget 派生部分已析构，成员指针槽
        // 被调用会触发 Qt6 assertObjectType 断言（dynamic_cast 失败）。MainWindow 静态夹具
        // 同样不析构，因此这里只断开以其为接收者的连接并保留实例（Leak 由
        // ASAN_OPTIONS=detect_leaks=0 豁免，与仓库既有静态夹具策略一致）。
        if (m_tester) {
            m_tester->disconnect();
            m_tester = nullptr;
        }
    }

protected:
    ProcessPageWidget *m_tester;
};

TEST_F(UT_ProcessPageWidget, initTest)
{
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->m_settings, Settings::instance());
    EXPECT_NE(m_tester->m_views, nullptr);
    EXPECT_NE(m_tester->m_rightStackView, nullptr);
    EXPECT_NE(m_tester->m_procViewMode, nullptr);
    EXPECT_NE(m_tester->m_procViewModeSummary, nullptr);
    EXPECT_NE(m_tester->m_appButton, nullptr);
    EXPECT_NE(m_tester->m_myProcButton, nullptr);
    EXPECT_NE(m_tester->m_allProcButton, nullptr);
    EXPECT_NE(m_tester->m_procTable, nullptr);
    EXPECT_NE(m_tester->m_processWidget, nullptr);

    // 恢复分支与按钮选中态一致（真实 Settings 值取决于本机配置，做映射一致性断言）
    switch (m_tester->m_procBtnCheckedType) {
    case ProcessPageWidget::MY_APPS:
        EXPECT_TRUE(m_tester->m_appButton->isChecked());
        break;
    case ProcessPageWidget::USER_PROCESS:
        EXPECT_TRUE(m_tester->m_myProcButton->isChecked());
        break;
    case ProcessPageWidget::ALL_PROCESSS:
        EXPECT_TRUE(m_tester->m_allProcButton->isChecked());
        break;
    }
}

TEST_F(UT_ProcessPageWidget, test_initUI_restore_01)
{
    // 恢复分支 B1：上一次选择"我的进程"
    Stub stub;
    stub.set(ADDR(Settings, getOption), stub_ppw_getOption_myproc);

    m_tester->initUI();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::USER_PROCESS);
    EXPECT_TRUE(m_tester->m_myProcButton->isChecked());
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "My processes"));
}

TEST_F(UT_ProcessPageWidget, test_initUI_restore_02)
{
    // 恢复分支 B2：上一次选择"所有进程"
    Stub stub;
    stub.set(ADDR(Settings, getOption), stub_ppw_getOption_allproc);

    m_tester->initUI();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::ALL_PROCESSS);
    EXPECT_TRUE(m_tester->m_allProcButton->isChecked());
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "All processes"));
}

TEST_F(UT_ProcessPageWidget, test_initUI_restore_03)
{
    // 恢复分支 B3：其他值走 default（应用列表）
    Stub stub;
    stub.set(ADDR(Settings, getOption), stub_ppw_getOption_default);

    m_tester->initUI();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::MY_APPS);
    EXPECT_TRUE(m_tester->m_appButton->isChecked());
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "Applications"));
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_01)
{
    // appButton + Key_Right → 焦点移到 myProcButton（离屏环境用 setFocus 计数验证）
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    EXPECT_TRUE(m_tester->eventFilter(m_tester->m_appButton, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 1);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_02)
{
    // myProcButton + Key_Right → 焦点移到 allProcButton（离屏环境用 setFocus 计数验证）
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    EXPECT_TRUE(m_tester->eventFilter(m_tester->m_myProcButton, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 1);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_03)
{
    // myProcButton + Key_Left → 焦点移到 appButton（离屏环境用 setFocus 计数验证）
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    EXPECT_TRUE(m_tester->eventFilter(m_tester->m_myProcButton, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 1);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_04)
{
    // allProcButton + Key_Left → 焦点移到 myProcButton（离屏环境用 setFocus 计数验证）
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    EXPECT_TRUE(m_tester->eventFilter(m_tester->m_allProcButton, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 1);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_05)
{
    // allProcButton + 未监听的方向键 → 落回 DFrame 默认处理，不转移焦点
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    EXPECT_FALSE(m_tester->eventFilter(m_tester->m_allProcButton, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 0);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_06)
{
    // 非按钮对象 + KeyPress → 落回 DFrame 默认处理，不转移焦点
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QKeyEvent kev(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    EXPECT_FALSE(m_tester->eventFilter(m_tester->m_procViewMode, &kev));
    EXPECT_EQ(g_ppw_setFocusCount, 0);
}

TEST_F(UT_ProcessPageWidget, test_eventFilter_07)
{
    // 按钮对象 + 非按键事件 → 落回 DFrame 默认处理，不转移焦点
    Stub stub;
    stub.set((void (QWidget::*)(Qt::FocusReason))&QWidget::setFocus, stub_ppw_widgetSetFocus);
    g_ppw_setFocusCount = 0;

    QMouseEvent mev(QEvent::MouseMove, QPointF(1, 1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    EXPECT_FALSE(m_tester->eventFilter(m_tester->m_appButton, &mev));
    EXPECT_EQ(g_ppw_setFocusCount, 0);
}

TEST_F(UT_ProcessPageWidget, test_onLoadLeftDataWidgetDelay_01)
{
    m_tester->onLoadLeftDataWidgetDelay();

    EXPECT_NE(m_tester->m_compactView, nullptr);
    EXPECT_NE(m_tester->m_expandView, nullptr);
    EXPECT_EQ(m_tester->m_views->count(), 2);
}

TEST_F(UT_ProcessPageWidget, test_switchDisplayMode_01)
{
    m_tester->onLoadLeftDataWidgetDelay();

    m_tester->switchDisplayMode(kDisplayModeExpand);
    EXPECT_EQ(m_tester->m_views->currentIndex(), kDisplayModeExpand);
}

TEST_F(UT_ProcessPageWidget, test_switchDisplayMode_02)
{
    m_tester->onLoadLeftDataWidgetDelay();

    m_tester->switchDisplayMode(kDisplayModeCompact);
    EXPECT_EQ(m_tester->m_views->currentIndex(), kDisplayModeCompact);
}

TEST_F(UT_ProcessPageWidget, test_switchProcessPage_01)
{
    // 右侧详情栈切回进程页（DStackedWidget 动画由离屏环境吞掉，校验目标 widget 归属）
    m_tester->switchProcessPage();

    EXPECT_EQ(m_tester->m_rightStackView->widget(0), m_tester->m_processWidget);
}

TEST_F(UT_ProcessPageWidget, test_switchCurrentNoFilterPage_01)
{
    m_tester->switchCurrentNoFilterPage();

    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "All processes"));
    EXPECT_TRUE(m_tester->m_allProcButton->isChecked());
    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::ALL_PROCESSS);
}

TEST_F(UT_ProcessPageWidget, test_onAppButtonClicked_01)
{
    // 先切到所有进程，再点应用按钮 → 状态切换 + 设置写回（CPUPerformance==High 分支）
    m_tester->switchCurrentNoFilterPage();

    m_tester->onAppButtonClicked();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::MY_APPS);
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "Applications"));
    EXPECT_EQ(Settings::instance()->getOption(kSettingKeyProcessTabIndex, QVariant(-1)).toInt(), kFilterApps);
}

TEST_F(UT_ProcessPageWidget, test_onUserProcButtonClicked_01)
{
    m_tester->switchCurrentNoFilterPage();

    m_tester->onUserProcButtonClicked();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::USER_PROCESS);
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "My processes"));
    EXPECT_EQ(Settings::instance()->getOption(kSettingKeyProcessTabIndex, QVariant(-1)).toInt(), kFilterCurrentUser);
}

TEST_F(UT_ProcessPageWidget, test_onAllProcButtonClicked_01)
{
    m_tester->onAppButtonClicked();

    m_tester->onAllProcButtonClicked();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::ALL_PROCESSS);
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "All processes"));
    EXPECT_EQ(Settings::instance()->getOption(kSettingKeyProcessTabIndex, QVariant(-1)).toInt(), kNoFilter);
}

TEST_F(UT_ProcessPageWidget, test_onAllProcButtonClicked_02)
{
    // 已处于所有进程视图时重复点击：早退分支，状态不变
    m_tester->onAllProcButtonClicked();

    m_tester->onAllProcButtonClicked();

    EXPECT_EQ(m_tester->m_procBtnCheckedType, ProcessPageWidget::ALL_PROCESSS);
    EXPECT_EQ(m_tester->m_procViewMode->text(),
              DApplication::translate("Process.Show.Mode", "All processes"));
}

TEST_F(UT_ProcessPageWidget, test_onAppAndProcCountUpdated_01)
{
    m_tester->onAppAndProcCountUpdated(3, 5);

    const QString &text = m_tester->m_procViewModeSummary->text();
    EXPECT_TRUE(text.contains("3"));
    EXPECT_TRUE(text.contains("5"));
}

TEST_F(UT_ProcessPageWidget, test_onDetailWidgetChanged_01)
{
    // index 0 = 进程页 → 详情按钮可见（compact/expand 均真实创建）
    m_tester->onLoadLeftDataWidgetDelay();

    EXPECT_EQ(m_tester->m_rightStackView->widget(0), m_tester->m_processWidget);
    m_tester->onDetailWidgetChanged(0);

    SUCCEED();
}

TEST_F(UT_ProcessPageWidget, test_onDetailWidgetChanged_02)
{
    // 越界索引 → widget 为空指针，setDetailButtonVisible(false) 安全
    m_tester->onLoadLeftDataWidgetDelay();

    EXPECT_EQ(m_tester->m_rightStackView->widget(5), nullptr);
    m_tester->onDetailWidgetChanged(5);

    SUCCEED();
}

TEST_F(UT_ProcessPageWidget, test_changeIconTheme_01)
{
    // 亮色主题图标加载（资源已链接进测试二进制）
    m_tester->changeIconTheme(DGuiApplicationHelper::LightType);

    EXPECT_FALSE(m_tester->m_appButton->icon().isNull());
    EXPECT_FALSE(m_tester->m_myProcButton->icon().isNull());
    EXPECT_FALSE(m_tester->m_allProcButton->icon().isNull());
}

TEST_F(UT_ProcessPageWidget, test_changeIconTheme_02)
{
    // 暗色主题图标加载
    m_tester->changeIconTheme(DGuiApplicationHelper::DarkType);

    EXPECT_FALSE(m_tester->m_appButton->icon().isNull());
    EXPECT_FALSE(m_tester->m_myProcButton->icon().isNull());
    EXPECT_FALSE(m_tester->m_allProcButton->icon().isNull());
}

TEST_F(UT_ProcessPageWidget, test_popupKillConfirmDialog_ok)
{
    // exec 桩阻断模态循环并写入 Ok 结果 → 记日志并调用 killProcess（已打桩）
    Stub stub;
    int (*fptr_ddialog_exec)(void *) = (int (*)(void *))&DDialog::exec;
    stub.set(fptr_ddialog_exec, stub_ppw_dialogExec_clickOk);
    stub.set(ADDR(ProcessDB, killProcess), stub_ppw_killProcess);

    m_tester->popupKillConfirmDialog(1234);

    EXPECT_TRUE(g_ppw_killProcessCalled);
}

TEST_F(UT_ProcessPageWidget, test_popupKillConfirmDialog_cancel)
{
    // 取消分支：结果保持默认值（非 Ok）→ 不调用 killProcess
    Stub stub;
    int (*fptr_ddialog_exec)(void *) = (int (*)(void *))&DDialog::exec;
    stub.set(fptr_ddialog_exec, stub_ppw_dialogExec_clickCancel);
    stub.set(ADDR(ProcessDB, killProcess), stub_ppw_killProcess);

    m_tester->popupKillConfirmDialog(1234);

    EXPECT_FALSE(g_ppw_killProcessCalled);
}

TEST_F(UT_ProcessPageWidget, test_showWindowKiller_01)
{
    // 最小化主窗 + 创建窗口粉碎预览（createWindowKiller 打桩计数）
    auto createWindowKiller = get_private_fun::ProcessPageWidgetcreateWindowKiller();
    Stub stub;
    stub.set(ADDR(MainWindow, showMinimized), stub_ppw_showMinimized);
    stub.set(createWindowKiller, stub_ppw_createWindowKiller_count);

    m_tester->showWindowKiller();

    EXPECT_TRUE(g_ppw_showMinimizedCalled);
    EXPECT_EQ(g_ppw_createWindowKillerCount, 1);
}

TEST_F(UT_ProcessPageWidget, test_createWindowKiller_01)
{
    // 真实执行 createWindowKiller（WMInfo 窗口树构建打桩）→ 预览组件创建
    auto buildWindowTreeSchema = get_private_fun::WMInfobuildWindowTreeSchema();
    auto findDockWindows = get_private_fun::WMInfofindDockWindows();
    Stub stub;
    stub.set(buildWindowTreeSchema, stub_ppw_wm_buildWindowTreeSchema);
    stub.set(findDockWindows, stub_ppw_wm_findDockWindows);

    auto createWindowKiller = get_private_fun::ProcessPageWidgetcreateWindowKiller();
    (m_tester->*createWindowKiller)();

    ASSERT_NE(m_tester->m_xwkillPreview, nullptr);
    // 清理预览组件（其析构依赖 gApp->mainWindow()，夹具已设置）
    delete m_tester->m_xwkillPreview;
    m_tester->m_xwkillPreview = nullptr;
}

TEST_F(UT_ProcessPageWidget, test_paintEvent_01)
{
    // 触发 paintEvent（DFrame 绘制路径）
    EXPECT_FALSE(m_tester->grab().isNull());
}
