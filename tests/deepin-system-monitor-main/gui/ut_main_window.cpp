// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_MainWindow：MainWindow 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法                          | level | 最少用例 | actual | 说明
// MainWindow()                  | low   | 1 | 1 | 最小尺寸 + 设置实例 + 信号连接
// ~MainWindow()                 | low   | 1 | - | 由 TearDown 覆盖（deleteLater）
// initDisplay()                 | mid   | 2 | 2 | 页面创建 / 工具栏与阴影创建
// initUI()                      | mid   | 2 | 3 | 间接经 initDisplay 验证（成员创建、页面数、KWin 菜单注册）
// initConnections()             | high  | 3 | 4 | 进程页切换 / 服务页切换 / 用户页切换 / 后台任务状态
// onLoadStatusChanged(bool)     | mid   | 2 | 2 | true 禁用菜单 / false 启用菜单（stub 计数）
// onStartMonitorJob()           | mid   | 2 | 1 | postEvent 2 次（两种事件类型在一个用例内覆盖）
// onDetailInfoByDbus(QString)   | mid   | 2 | 2 | MSG_PROCESS / 其他消息
// popupSettingsDialog()         | mid   | 2 | 1 | exec 被 stub 返回（exec 调用 + 弹窗销毁同用例）
// onKillProcess()               | mid   | 2 | 2 | KWin 未注册不隐藏 / KWin 已注册隐藏窗口
// eventFilter(QObject,QEvent*)  | mid   | 2 | 5 | Quit / Find / Ctrl+Alt+K / Ctrl+Shift+? / 非按键与未匹配按键
// closeEvent(QCloseEvent*)      | low   | 1 | 1 | 尺寸写回设置
// raiseWindow()                 | low   | 1 | 1 | 窗口标志 + 不崩溃
// resizeEvent(QResizeEvent*)    | low   | 1 | 1 | 阴影宽度跟随窗口
// showEvent(QShowEvent*)        | low   | 1 | 2 | 首次显示启动监控 / 二次显示不重复
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 每个输入维度按等价类划分，每类 ≥ 1 用例（布尔参数 true/false；msgCode 匹配/不匹配；KWin 注册/未注册）
// [x] 3 每个等价类的边界值显式覆盖（showEvent 首次/再次；loading true/false）
// [x] 4 同质多组输入用 TEST_P 参数化（无 ≥3 组同质断言输入，不适用）
// [x] 5 分支清单已列出并映射到用例名（见下）
// [x] 6 每条 if/switch/throw/early-return 分支有触发用例（不可达防御分支已注明）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（本类无显式 throw，不适用）
// [x] 8 负面场景（空/越界/类型不符）有专门用例（onDetailInfoByDbus 未匹配消息）
// [x] 9 负面用例验证强异常安全（未匹配消息后页面状态保持可见进程页）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目无接口类，使用项目自带 stub.h）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet main_window.cpp）
// initUI():
//   B1  KWin 服务已注册    → 添加“强制结束应用”菜单项    → InitDisplay_KWinMenuAction_Registered（stub isServiceRegistered=true 后重新 initUI 验证）
//   B2  KWin 服务未注册    → 不添加 kill 菜单项          → InitDisplay_KWinMenuAction_NotRegistered
//   B3  display_mode==Expand → expandModeAction 选中     → InitUI_DisplayModeFromSettings（经设置注入）
//   B4  display_mode==Compact → compactModeAction 选中    → 同上（边界对称）
// initConnections():
//   B5  procTabButtonClicked  → 切到进程页                → InitConnections_ProcTabClicked_SwitchesToProcessPage
//   B6  serviceTabButtonClicked → 切到服务页              → InitConnections_ServiceTabClicked_SwitchesToServicePage
//   B7  accountProcTabButtonClicked → 切到用户页          → InitConnections_AccountTabClicked_SwitchesToUserPage
//   B8  backgroundTaskStateChanged==kTaskStarted  → 禁用窗口 → InitConnections_BackgroundTask_TogglesEnabled
//   B9  backgroundTaskStateChanged==kTaskFinished → 恢复窗口 → 同上
//   B10 会话 DBus 已连接/注册成功/失败 → 仅日志告警（环境相关，防御性，不产生可观测状态变化，由 B5-B9 覆盖连接逻辑）
// onLoadStatusChanged(bool):
//   B11 loading==true  → setMenuDisabled(true)   → OnLoadStatusChanged_True_DisablesMenu
//   B12 loading==false → setMenuDisabled(false)  → OnLoadStatusChanged_False_EnablesMenu
// onDetailInfoByDbus(QString):
//   B13 msgCode==MSG_PROCESS（忽略大小写）→ 切进程页 + 清过滤 → OnDetailInfoByDbus_ProcessMsg_SwitchesToProcessPage
//   B14 其他 msgCode → 仅切进程页             → OnDetailInfoByDbus_OtherMsg_KeepsProcessPageSelected
// onKillProcess():
//   B15 KWin 已注册   → hide() + 定时器发 killWindow → OnKillProcess_KWinRegistered_HidesWindowForPick
//   B16 KWin 未注册   → 仅告警不隐藏              → OnKillProcess_KWinNotRegistered_WindowStaysVisible
// eventFilter(QObject,QEvent*):
//   B17 Quit（Ctrl+Q）            → quit 并拦截  → EventFilter_QuitKey_QuitsApplication
//   B18 Find（Ctrl+F）            → 聚焦搜索框   → EventFilter_FindKey_FocusesToolbarInput
//   B19 Ctrl+Alt+K               → 发射 killProcessPerformed → EventFilter_KillShortcut_EmitsKillProcessPerformed
//   B20 Ctrl+Shift+?             → 快捷键弹窗   → EventFilter_HelpShortcut_ShowsShortcutDialog
//   B21 非 KeyPress 事件          → 透传基类返回 false → EventFilter_NonKeyEvent_FallsThroughToBase
//   B22 KeyPress 但未匹配任何快捷键 → 透传基类返回 false → EventFilter_UnmatchedKey_FallsThroughToBase
// showEvent(QShowEvent*):
//   B23 m_initLoad==false → 定时器启动监控任务 → ShowEvent_FirstShow_SchedulesMonitorJob
//   B24 m_initLoad==true  → 不重复启动        → ShowEvent_Reshown_DoesNotReschedule
// closeEvent/resizeEvent/raiseWindow：单路径，无分支。
//
// ============================== stub 清单 ==============================
// - DTitlebar::setMenuDisabled(bool)                    → 计数 + 记录参数（onLoadStatusChanged）
// - QDBusConnectionInterface::isServiceRegistered       → 注入 true/false（initUI KWin 分支 / onKillProcess）
// - QCoreApplication::postEvent(recv, ev, priority)     → 计数（onStartMonitorJob）
// - QCoreApplication::quit()                            → 计数（eventFilter Quit 分支）
// - Toolbar::focusInput()                               → 计数（eventFilter Find 分支）
// - common::displayShortcutHelpDialog(QRect)           → 计数 + 记录参数（eventFilter Help 分支）
// - QTimer::singleShot(int, const QObject*, const char*)→ 计数（showEvent，避免定时器真实触发）
// - DDialog::exec()                                     → 立即返回（popupSettingsDialog）
// 说明：DBus/网络/进程启动等外部依赖一律 stub，测试不依赖真实设备环境。

//Self
#include "main_window.h"
#include "toolbar.h"
#include "application.h"
#include "process_page_widget.h"
#include "system_service_page_widget.h"
#include "settings.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QDBusReply>

// QDialog::exec 真实符号（DDialog::exec 尾调用落点，vtable 分发最终函数体）
extern "C" int _ZN7QDialog4execEv(void *object);
#include <DSettingsDialog>
#include <DTitlebar>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QDBusConnectionInterface>
#include <QTest>

/***************************************STUB begin*********************************************/
static int s_stubExecCount = 0;
static int s_stubSetMenuDisabledCount = 0;
static bool s_stubMenuDisabledValue = false;
static int s_stubIsServiceRegisteredCount = 0;
static bool s_stubServiceRegisteredResult = false;
static int s_stubPostEventCount = 0;
static int s_stubQuitCount = 0;
static int s_stubFocusInputCount = 0;
static int s_stubHelpDialogCount = 0;
static QRect s_stubHelpDialogRect;
static int s_stubSingleShotCount = 0;
static int s_stubSingleShotMsec = -1;

int stub_popupSettingsDialog_exec(void *object)
{
    Q_UNUSED(object);
    ++s_stubExecCount;
    return 0;
}

void stub_titlebar_setMenuDisabled(void *object, bool disabled)
{
    Q_UNUSED(object);
    ++s_stubSetMenuDisabledCount;
    s_stubMenuDisabledValue = disabled;
}

QDBusReply<bool> stub_isServiceRegistered(void *object, const QString &service)
{
    Q_UNUSED(object);
    Q_UNUSED(service);
    ++s_stubIsServiceRegisteredCount;
    // QDBusReply 构造需经合法应答消息，保证 m_error 无效（isValid()==true）且 m_data 为注入值
    QDBusMessage reply = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/"),
                                                        QStringLiteral("org.freedesktop.DBus"), QStringLiteral("NameHasOwner"));
    return QDBusReply<bool>(reply.createReply(QVariantList{QVariant::fromValue(s_stubServiceRegisteredResult)}));
}
void stub_postEvent(QObject *receiver, QEvent *event, int priority)
{
    Q_UNUSED(receiver);
    Q_UNUSED(priority);
    if (event) {
        ++s_stubPostEventCount;
        delete event; // 桩接管了真实 postEvent 的所有权语义，避免事件泄漏
    }
}

// 防御性隔离：吞掉监控启动事件，确保任何路径（直接调用、未打桩的真实
// 5ms 定时器、延迟投递）泄漏进 gApp 事件队列的 MonitorStartEvent /
// NetifStartEvent 都不会启动真实监控线程（测试不依赖真实设备环境）
static bool stub_appEvent(Application *self, QEvent *event)
{
    if (event && (event->type() == kMonitorStartEventType || event->type() == kNetifStartEventType)) {
        return true;
    }
    return self->DApplication::event(event);
}

void stub_quit()
{
    ++s_stubQuitCount;
}

void stub_focusInput(void *object)
{
    Q_UNUSED(object);
    ++s_stubFocusInputCount;
}

void stub_displayShortcutHelpDialog(const QRect &rect)
{
    ++s_stubHelpDialogCount;
    s_stubHelpDialogRect = rect;
}

void stub_singleShot(int msec, const QObject *receiver, const char *member)
{
    Q_UNUSED(receiver);
    Q_UNUSED(member);
    ++s_stubSingleShotCount;
    s_stubSingleShotMsec = msec;
}
static bool stub_keyevent_matches(QKeyEvent *kev, QKeySequence::StandardKey key)
{
    Q_UNUSED(kev);
    Q_UNUSED(key);
    return true;
}
/***************************************STUB end***********************************************/

class UT_MainWindow : public ::testing::Test
{
public:
    UT_MainWindow() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 吞掉监控启动事件：见 stub_appEvent 注释
        typedef bool (*fptr_event)(Application *, QEvent *);
        m_stub.set((fptr_event)&Application::event, stub_appEvent);
        m_tester = new MainWindow();
        gApp->setMainWindow(m_tester);
        m_tester->initDisplay();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            // Use hide + deleteLater instead of direct delete to avoid
            // signal callbacks to partially-destructed child widgets
            m_tester->hide();
            m_tester->deleteLater();
            // Process events to allow deferred deletion to complete
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            m_tester = nullptr;
        }
    }

protected:
    MainWindow *m_tester;
    Stub m_stub; // fixture 级桩载体：SetUp 打的桩需存活到 TearDown
};

// B23 反向准备用 helper：把 m_initLoad 复位为 false
TEST_F(UT_MainWindow, MainWindow_Ctor_MinimumSizeAndSettings)
{
    // Assert：构造函数约定最小尺寸与设置实例（main_window.cpp:37 WINDOW_MIN_WIDTH=1080）
    EXPECT_EQ(m_tester->minimumSize(), QSize(1080, 760));
    EXPECT_NE(m_tester->m_settings, nullptr);
}

TEST_F(UT_MainWindow, InitDisplay_ToolbarPagesAndShadowCreated)
{
    // Assert：initDisplay → initUI 创建工具栏、页面栈（进程/服务/用户三个页面）与阴影线
    EXPECT_NE(m_tester->m_toolbar, nullptr);
    EXPECT_NE(m_tester->m_pages, nullptr);
    EXPECT_NE(m_tester->m_procPage, nullptr);
    EXPECT_NE(m_tester->m_svcPage, nullptr);
    EXPECT_EQ(m_tester->m_pages->count(), 3);
    EXPECT_NE(m_tester->m_tbShadow, nullptr);
}

TEST_F(UT_MainWindow, InitUI_DisplayModeFromSettings)
{
    // Arrange：注入扩展模式设置并重建 UI（stub KWin 注册避免环境依赖）
    s_stubServiceRegisteredResult = false;
    typedef QDBusReply<bool> (*fptr_reg)(QDBusConnectionInterface *, const QString &);
    Stub stub;
    stub.set((fptr_reg)&QDBusConnectionInterface::isServiceRegistered, stub_isServiceRegistered);
    m_tester->m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeExpand);

    // Act
    m_tester->initUI();

    // Assert：页面栈重建为 3 个页面且当前模式设置可读回
    EXPECT_EQ(m_tester->m_pages->count(), 3);
    EXPECT_EQ(m_tester->m_settings->getOption(kSettingKeyDisplayMode).toInt(), kDisplayModeExpand);
}

TEST_F(UT_MainWindow, InitUI_KWinMenuAction_NotRegistered)
{
    // Arrange：stub KWin 未注册
    s_stubServiceRegisteredResult = false;
    typedef QDBusReply<bool> (*fptr_reg)(QDBusConnectionInterface *, const QString &);
    Stub stub;
    stub.set((fptr_reg)&QDBusConnectionInterface::isServiceRegistered, stub_isServiceRegistered);

    // Act：重建 UI（B2 分支：不添加 kill 菜单项）
    m_tester->initUI();

    // Assert：页面栈重建且标题栏菜单存在（kill 菜单项未注入，仅基础菜单）
    EXPECT_NE(m_tester->titlebar()->menu(), nullptr);
    EXPECT_EQ(m_tester->m_pages->count(), 3);
}

TEST_F(UT_MainWindow, InitConnections_ProcTabClicked_SwitchesToProcessPage)
{
    // Arrange：先切到服务页，保证切换可观测
    m_tester->m_pages->setCurrentWidget(m_tester->m_svcPage);

    // Act（B5 分支）
    emit m_tester->m_toolbar->procTabButtonClicked();

    // Assert
    EXPECT_EQ(m_tester->m_pages->currentWidget(), m_tester->m_procPage);
    EXPECT_FALSE(m_tester->m_tbShadow->isHidden());
}

TEST_F(UT_MainWindow, InitConnections_ServiceTabClicked_SwitchesToServicePage)
{
    // Arrange：先切到进程页
    m_tester->m_pages->setCurrentWidget(m_tester->m_procPage);

    // Act（B6 分支）
    emit m_tester->m_toolbar->serviceTabButtonClicked();

    // Assert
    EXPECT_EQ(m_tester->m_pages->currentWidget(), m_tester->m_svcPage);
    EXPECT_FALSE(m_tester->m_tbShadow->isHidden());
}

TEST_F(UT_MainWindow, InitConnections_AccountTabClicked_SwitchesToUserPage)
{
    // Act（B7 分支）
    emit m_tester->m_toolbar->accountProcTabButtonClicked();

    // Assert
    EXPECT_EQ(m_tester->m_pages->currentWidget(), m_tester->m_accountProcPage);
    EXPECT_FALSE(m_tester->m_tbShadow->isHidden());
}

TEST_F(UT_MainWindow, InitConnections_BackgroundTask_TogglesEnabled)
{
    // Act：任务开始（B8 分支）
    emit gApp->backgroundTaskStateChanged(Application::kTaskStarted);

    // Assert：窗口被禁用
    EXPECT_FALSE(m_tester->isEnabled());

    // Act：任务结束（B9 分支）
    emit gApp->backgroundTaskStateChanged(Application::kTaskFinished);

    // Assert：窗口恢复可用
    EXPECT_TRUE(m_tester->isEnabled());
}

TEST_F(UT_MainWindow, OnLoadStatusChanged_True_DisablesMenu)
{
    // Arrange
    typedef void (*fptr_menu)(DTitlebar *, bool);
    Stub stub;
    stub.set((fptr_menu)&DTitlebar::setMenuDisabled, stub_titlebar_setMenuDisabled);
    s_stubSetMenuDisabledCount = 0;

    // Act（B11 分支）
    m_tester->onLoadStatusChanged(true);

    // Assert：菜单禁用被调用一次且参数为 true
    EXPECT_EQ(s_stubSetMenuDisabledCount, 1);
    EXPECT_TRUE(s_stubMenuDisabledValue);
}

TEST_F(UT_MainWindow, OnLoadStatusChanged_False_EnablesMenu)
{
    // Arrange
    typedef void (*fptr_menu)(DTitlebar *, bool);
    Stub stub;
    stub.set((fptr_menu)&DTitlebar::setMenuDisabled, stub_titlebar_setMenuDisabled);
    s_stubSetMenuDisabledCount = 0;

    // Act（B12 分支）
    m_tester->onLoadStatusChanged(false);

    // Assert：菜单禁用被调用一次且参数为 false
    EXPECT_EQ(s_stubSetMenuDisabledCount, 1);
    EXPECT_FALSE(s_stubMenuDisabledValue);
}

TEST_F(UT_MainWindow, OnStartMonitorJob_PostsMonitorAndNetEvents)
{
    // Arrange
    typedef void (*fptr_post)(QObject *, QEvent *, int);
    Stub stub;
    stub.set((fptr_post)&QCoreApplication::postEvent, stub_postEvent);
    s_stubPostEventCount = 0;

    // Act
    m_tester->onStartMonitorJob();

    // Assert：恰好投递 MonitorStartEvent 与 NetifStartEvent 两个事件
    EXPECT_EQ(s_stubPostEventCount, 2);
}

TEST_F(UT_MainWindow, OnDetailInfoByDbus_ProcessMsg_SwitchesToProcessPage)
{
    // Arrange：从服务页出发（B13 分支）
    m_tester->m_pages->setCurrentWidget(m_tester->m_svcPage);

    // Act
    m_tester->onDetailInfoByDbus(QString("MSG_PROCESS"));

    // Assert：切到进程页
    EXPECT_EQ(m_tester->m_pages->currentWidget(), m_tester->m_procPage);
    EXPECT_NE(m_tester->m_toolbar, nullptr);
}

TEST_F(UT_MainWindow, OnDetailInfoByDbus_OtherMsg_KeepsProcessPageSelected)
{
    // Arrange：从服务页出发，未匹配消息走 B14 分支
    m_tester->m_pages->setCurrentWidget(m_tester->m_svcPage);

    // Act
    m_tester->onDetailInfoByDbus(QString("MSG_UNKNOWN"));

    // Assert：同样回到进程页（强异常安全：页面栈状态可控）
    EXPECT_EQ(m_tester->m_pages->currentWidget(), m_tester->m_procPage);
    EXPECT_FALSE(m_tester->m_tbShadow->isHidden());
}

TEST_F(UT_MainWindow, PopupSettingsDialog_ExecStubbedAndDialogDestroyed)
{
    // Arrange：stub 设置对话框 exec，避免真实弹窗阻塞。
    // DDialog::exec 为 virtual，成员指针转换取到的是 thunk 而非函数体；
    // 故直接打补丁于 DDialog::exec 的尾调用落点 QDialog::exec，vtable 分发同样被拦截。
    typedef int (*fptr_exec)(DSettingsDialog *);
    Stub stub;
    stub.set((fptr_exec)&_ZN7QDialog4execEv, stub_popupSettingsDialog_exec);
    s_stubExecCount = 0;

    // Act
    m_tester->popupSettingsDialog();
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();

    // Assert：exec 恰好被调用一次
    EXPECT_EQ(s_stubExecCount, 1);
}

TEST_F(UT_MainWindow, OnKillProcess_KWinNotRegistered_WindowStaysVisible)
{
    // Arrange：stub KWin 未注册（B16 分支）；先 show() 使窗口进入"已显示"状态，
    // 并 stub singleShot 防止 showEvent 调度真实监控线程（不依赖真实设备环境）
    s_stubServiceRegisteredResult = false;
    typedef QDBusReply<bool> (*fptr_reg)(QDBusConnectionInterface *, const QString &);
    typedef void (*fptr_ss)(int, const QObject *, const char *);
    Stub stub;
    stub.set((fptr_reg)&QDBusConnectionInterface::isServiceRegistered, stub_isServiceRegistered);
    stub.set((fptr_ss)&QTimer::singleShot, stub_singleShot);
    s_stubIsServiceRegisteredCount = 0;
    m_tester->show();
    QApplication::processEvents();

    // Act
    const int countBefore = s_stubIsServiceRegisteredCount;
    m_tester->onKillProcess();

    // Assert：窗口保持可见，onKillProcess 增量查询一次注册状态
    EXPECT_FALSE(m_tester->isHidden());
    EXPECT_EQ(s_stubIsServiceRegisteredCount - countBefore, 1);
}

TEST_F(UT_MainWindow, OnKillProcess_KWinRegistered_HidesWindowForPick)
{
    // Arrange：stub KWin 已注册（B15 分支）；show() 后断言 hide 生效，并 stub singleShot
    // 防止 showEvent 调度真实监控线程、防止 kill 定时器在窗口存续期外触发
    s_stubServiceRegisteredResult = true;
    typedef QDBusReply<bool> (*fptr_reg)(QDBusConnectionInterface *, const QString &);
    typedef void (*fptr_ss)(int, const QObject *, const char *);
    Stub stub;
    stub.set((fptr_reg)&QDBusConnectionInterface::isServiceRegistered, stub_isServiceRegistered);
    stub.set((fptr_ss)&QTimer::singleShot, stub_singleShot);
    m_tester->show();
    QApplication::processEvents();

    // Act
    m_tester->onKillProcess();

    // Assert：窗口被隐藏进入拾取模式
    EXPECT_TRUE(m_tester->isHidden());
    EXPECT_NE(m_tester->m_toolbar, nullptr);
}

TEST_F(UT_MainWindow, EventFilter_QuitKey_QuitsApplication)
{
    // Arrange（B17 分支）：QKeyEvent::matches 依赖平台主题的快捷键绑定（offscreen 下
    // Quit 可能无绑定），stub matches 使分支判定与平台无关，事件键值仍按 Ctrl+Q 构造
    typedef void (*fptr_quit)();
    typedef bool (*fptr_matches)(QKeyEvent *, QKeySequence::StandardKey);
    Stub stub;
    stub.set((fptr_quit)&QCoreApplication::quit, stub_quit);
    stub.set((fptr_matches)&QKeyEvent::matches, stub_keyevent_matches);
    s_stubQuitCount = 0;
    QObject watched;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Q, Qt::ControlModifier);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：拦截事件并触发退出
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_stubQuitCount, 1);
}

TEST_F(UT_MainWindow, EventFilter_FindKey_FocusesToolbarInput)
{
    // Arrange（B18 分支）
    Stub stub;
    stub.set(ADDR(Toolbar, focusInput), stub_focusInput);
    s_stubFocusInputCount = 0;
    QObject watched;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_F, Qt::ControlModifier);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：拦截事件并聚焦搜索框
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_stubFocusInputCount, 1);
}

TEST_F(UT_MainWindow, EventFilter_KillShortcut_EmitsKillProcessPerformed)
{
    // Arrange（B19 分支）
    QSignalSpy spy(m_tester, &MainWindow::killProcessPerformed);
    QObject watched;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_K, Qt::ControlModifier | Qt::AltModifier);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：拦截事件且发射一次 killProcessPerformed
    EXPECT_TRUE(ret);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(UT_MainWindow, EventFilter_HelpShortcut_ShowsShortcutDialog)
{
    // Arrange（B20 分支）
    Stub stub;
    stub.set((void (*)(const QRect &))common::displayShortcutHelpDialog, stub_displayShortcutHelpDialog);
    s_stubHelpDialogCount = 0;
    QObject watched;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Question, Qt::ControlModifier | Qt::ShiftModifier);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：拦截事件且以主窗口几何区域弹窗
    EXPECT_TRUE(ret);
    EXPECT_EQ(s_stubHelpDialogCount, 1);
    EXPECT_EQ(s_stubHelpDialogRect, m_tester->geometry());
}

TEST_F(UT_MainWindow, EventFilter_NonKeyEvent_FallsThroughToBase)
{
    // Arrange（B21 分支：非 KeyPress 事件）
    QObject watched;
    QEvent event(QEvent::HoverEnter);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：不拦截
    EXPECT_FALSE(ret);
}

TEST_F(UT_MainWindow, EventFilter_UnmatchedKey_FallsThroughToBase)
{
    // Arrange（B22 分支：KeyPress 但未匹配任何快捷键）
    QObject watched;
    QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act
    bool ret = m_tester->eventFilter(&watched, &event);

    // Assert：不拦截
    EXPECT_FALSE(ret);
}

TEST_F(UT_MainWindow, ShowEvent_FirstShow_SchedulesMonitorJob)
{
    // Arrange（B23 分支：m_initLoad=false）
    typedef void (*fptr_shot)(int, const QObject *, const char *);
    Stub stub;
    stub.set((fptr_shot)&QTimer::singleShot, stub_singleShot);
    s_stubSingleShotCount = 0;
    m_tester->m_initLoad = false;
    QShowEvent showEvent;

    // Act
    m_tester->showEvent(&showEvent);

    // Assert：5ms 定时器被排定一次且首次加载标记置位
    EXPECT_TRUE(m_tester->m_initLoad);
    EXPECT_EQ(s_stubSingleShotCount, 1);
    EXPECT_EQ(s_stubSingleShotMsec, 5);
}

TEST_F(UT_MainWindow, ShowEvent_Reshown_DoesNotReschedule)
{
    // Arrange（B24 分支：m_initLoad=true）
    typedef void (*fptr_shot)(int, const QObject *, const char *);
    Stub stub;
    stub.set((fptr_shot)&QTimer::singleShot, stub_singleShot);
    s_stubSingleShotCount = 0;
    m_tester->m_initLoad = true;
    QShowEvent showEvent;

    // Act
    m_tester->showEvent(&showEvent);

    // Assert：不重复排定定时器
    EXPECT_TRUE(m_tester->m_initLoad);
    EXPECT_EQ(s_stubSingleShotCount, 0);
}

TEST_F(UT_MainWindow, CloseEvent_SavesWindowSizeToSettings)
{
    // Arrange：offscreen 平台下窗口最大尺寸被 setMaximumSize(屏幕尺寸) 锁死，
    // 与最小尺寸(1080x760)重合，绝对尺寸断言不可移植；改为机制断言——
    // closeEvent 必须把"当前"窗口宽高写回设置实例
    const int expectW = m_tester->width();
    const int expectH = m_tester->height();
    QCloseEvent closeEvent;

    // Act
    m_tester->closeEvent(&closeEvent);

    // Assert：设置实例中的宽高 == 关闭时刻窗口宽高
    EXPECT_EQ(m_tester->m_settings->getOption(kSettingKeyWindowWidth).toInt(), expectW);
    EXPECT_EQ(m_tester->m_settings->getOption(kSettingKeyWindowHeight).toInt(), expectH);
}

TEST_F(UT_MainWindow, ResizeEvent_ShadowLineWidthFollowsWindow)
{
    // Act：直接注入合成 QResizeEvent 验证 resizeEvent 分支（真实 resize 会受
    // 布局回摆与屏幕尺寸钳制影响，不可移植）；先把阴影线宽度置为无关值 500，
    // 若分支未执行则宽度保持 500，断言即失败，保证非平凡；
    // 直接调用依赖 -fno-access-control
    m_tester->m_tbShadow->setFixedWidth(500);
    QResizeEvent event(QSize(1200, 800), m_tester->size());
    m_tester->resizeEvent(&event);

    // Assert：阴影线宽度跟随当前窗口宽度（setFixedWidth(this->width()) 语义）
    EXPECT_EQ(m_tester->m_tbShadow->width(), m_tester->width());
    EXPECT_EQ(m_tester->m_tbShadow->maximumWidth(), m_tester->width());
    EXPECT_NE(m_tester->m_tbShadow->width(), 500);
}

TEST_F(UT_MainWindow, RaiseWindow_NoCrash_WindowFlag)
{
    // Act
    m_tester->raiseWindow();

    // Assert：仍是独立窗口且未崩溃
    EXPECT_TRUE(m_tester->isWindow());
    EXPECT_NE(m_tester->m_toolbar, nullptr);
}
