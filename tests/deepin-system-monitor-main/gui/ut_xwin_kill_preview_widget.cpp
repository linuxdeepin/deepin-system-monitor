// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_XWinKillPreviewWidget：XWinKillPreviewWidget 单元测试（qt-autotest-generator 重写）
//
// ============================ 用例计数声明 ============================
// 方法                          | level | 最少用例 | actual | 说明
// XWinKillPreviewWidget(QWidget) | low   | 1 | 1 | ctor 全链路：WMInfo 桩 + 帧窗属性 + 1x1 隐藏位置
// ~XWinKillPreviewWidget()       | low   | 1 | - | TearDown delete 覆盖（mainWindow 已设置，修复旧套件空指针崩溃）
// initUI()                       | mid   | 2 | 2 | ctor 间接覆盖 + 重复调用（鼠标跟踪/窗口标志保持）
// initConnections()              | mid   | 1 | 1 | WaylandCentered=false 分支（空实现不崩溃）
// mousePressEvent(QMouseEvent*)  | high  | 3 | 5 | 非左键 / 悬停dock早退 / 无窗口 / 命中窗口发信号并自关 / 自身pid跳过
// mouseMoveEvent(QMouseEvent*)   | high  | 3 | 4 | 悬停dock恢复默认光标 / 无窗口恢复默认光标 / 命中窗口(空背景列表不发信号) / 自身pid未命中
// keyPressEvent(QKeyEvent*)      | high  | 2 | 2 | ESC 还原主窗状态并关闭 / 非 ESC 无动作
//
// ============================== 分支清单 ==============================
// mousePressEvent:
//   B1 event->button() != LeftButton            → 早退                    → test_mousePressEvent_01
//   B2 WaylandCentered 分支（编译宏内，运行时 false）→ 不可达（X11 环境）
//   B3 isCursorHoveringDocks==true              → 早退                    → test_mousePressEvent_03
//   B4 getpid()==select->pid                    → continue                → test_mousePressEvent_05
//   B5 select->rect.contains(windowPos)         → emit windowClicked+close → test_mousePressEvent_04
//   B6 无命中窗口                               → 无信号                  → test_mousePressEvent_02
// mouseMoveEvent:
//   B1 isCursorHoveringDocks==true              → 清选区+默认光标早退      → test_mouseMoveEvent_01
//   B2 getpid()==select->pid                    → continue                → test_mouseMoveEvent_04
//   B3 selRect.contains(pos)                    → found=true（背景列表空无裁剪信号）→ test_mouseMoveEvent_03
//   B4 !found                                   → 默认光标信号            → test_mouseMoveEvent_02
// keyPressEvent:
//   B1 Key_Escape                               → 还原主窗+close          → test_keyPressEvent_01
//   B2 其他按键                                 → 无动作                  → test_keyPressEvent_02
// 背景 500ms 定时器 lambda：测试全程不运行事件循环，不触发（grabWindow 依赖真实 X11 屏幕截屏）

//Self
#include "xwin_kill_preview_widget.h"
#include "wm/wm_info.h"
#include "application.h"
#include "main_window.h"
#include "common/common.h"

using namespace common::init;

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

using namespace core::wm;
/***************************************STUB begin*********************************************/
ACCESS_PRIVATE_FUN(WMInfo, void(void), buildWindowTreeSchema);
ACCESS_PRIVATE_FUN(WMInfo, void(void), findDockWindows);

void stub_xwk_wm_buildWindowTreeSchema()
{
    return;
}

void stub_xwk_wm_findDockWindows()
{
    return;
}

std::list<WMWindowArea> stub_xwk_wm_selectWindow(const QPoint &)
{
    return std::list<WMWindowArea>();
}

std::list<WMWindowArea> stub_xwk_wm_selectWindow_selfpid(const QPoint &)
{
    wm_window_area_t *area = new wm_window_area_t;
    area->pid = getpid();
    area->wid = 111111;
    area->rect = QRect(0, 0, 100, 100);
    std::list<WMWindowArea> list;
    list.emplace_back(area);
    return list;
}

std::list<WMWindowArea> stub_xwk_wm_selectWindow_hit(const QPoint &)
{
    wm_window_area_t *area = new wm_window_area_t;
    area->pid = 100000;   // 与测试进程自身 pid 不同，避免被跳过
    area->wid = 111111;
    area->rect = QRect(0, 0, 100, 100);
    std::list<WMWindowArea> list;
    list.emplace_back(area);
    return list;
}

bool stub_xwk_wm_isCursorHoveringDocks_true(const QPoint &)
{
    return true;
}

bool stub_xwk_wm_isCursorHoveringDocks_false(const QPoint &)
{
    return false;
}

std::list<WMWindowArea> stub_xwk_wm_getHoveredByWindowList(WMWId, QRect &)
{
    return std::list<WMWindowArea>();
}

QPoint stub_xwk_cursorPos_50_50()
{
    return QPoint(50, 50);
}

// initUI 会 grabKeyboard()；离屏套件中若键盘抓取者状态陈旧（前序夹具累积），
// Qt 内部 releaseKeyboard 会解引用失效窗口句柄导致段错误，这里直接禁用抓取
void stub_xwk_noGrabKeyboard(QWidget *)
{
    return;
}

void stub_xwk_noGrabMouse(QWidget *)
{
    return;
}

// 吞掉监控线程启动事件，其余事件回落 QApplication 默认处理
bool stub_xwk_appEvent_swallow(Application *self, QEvent *event)
{
    if (event && (event->type() == kMonitorStartEventType || event->type() == kNetifStartEventType)) {
        return true;
    }
    return self->QApplication::event(event);
}
/***************************************STUB end**********************************************/

class UT_XWinKillPreviewWidget : public ::testing::Test
{
public:
    UT_XWinKillPreviewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 修复旧套件崩溃根因：析构时 gApp->mainWindow() 为空指针。
        // 参照 ut_process_table_view 的成熟模式，预先设置静态 MainWindow。
        static QWidget wid;
        static MainWindow wm;
        gApp->setMainWindow(&wm);

        // 进程级一次性拦截监控线程启动事件：MainWindow 构造/显示会 postEvent
        // 启动 SystemMonitor/NetifMonitor 工作线程，线程与静态对象在进程退出
        // 时析构竞态会段错误并丢失 gcda，测试不依赖真实监控数据
        static bool s_appEventStubbed = false;
        if (!s_appEventStubbed) {
            auto *appEventStub = new Stub();
            typedef bool (*appEventFptr)(Application *, QEvent *);
            appEventFptr fptr_appEvent = (appEventFptr)&Application::event;
            appEventStub->set(fptr_appEvent, stub_xwk_appEvent_swallow);
            s_appEventStubbed = true;
        }

        // 进程级一次性禁用键盘抓取（Stub 对象有意保留，打桩存续至进程结束，
        // 避免跨夹具累积的陈旧 keyboard_grabber 在 Qt releaseKeyboard 中段错误）
        static bool s_keyboardStubbed = false;
        if (!s_keyboardStubbed) {
            auto *keyboardStub = new Stub();
            typedef void (*grabFptr)(QWidget *);
            grabFptr fptr_grabKeyboard = (grabFptr)&QWidget::grabKeyboard;
            keyboardStub->set(fptr_grabKeyboard, stub_xwk_noGrabKeyboard);
            typedef void (QWidget::*grabMouseMemFptr)();
            grabFptr fptr_grabMouse = (grabFptr)static_cast<grabMouseMemFptr>(&QWidget::grabMouse);
            keyboardStub->set(fptr_grabMouse, stub_xwk_noGrabMouse);
            s_keyboardStubbed = true;
        }

        // WMInfo 构造期的 X11 窗口树构建在离屏环境不可用，统一打桩
        auto buildWindowTreeSchema = get_private_fun::WMInfobuildWindowTreeSchema();
        auto findDockWindows = get_private_fun::WMInfofindDockWindows();
        Stub stub;
        stub.set(buildWindowTreeSchema, stub_xwk_wm_buildWindowTreeSchema);
        stub.set(findDockWindows, stub_xwk_wm_findDockWindows);

        m_tester = new XWinKillPreviewWidget(&wid);
    }

    virtual void TearDown()
    {
        // 若用例触发了 close()（WA_DeleteOnClose），先冲刷延迟删除再清指针
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // 触发过 close() 的用例调用：对象已被延迟删除，防止 TearDown 二次 delete
    void closeAndForget()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        m_tester = nullptr;
    }

protected:
    XWinKillPreviewWidget *m_tester;
};

TEST_F(UT_XWinKillPreviewWidget, initTest)
{
    ASSERT_NE(m_tester, nullptr);
    // 帧窗属性
    EXPECT_TRUE(m_tester->windowFlags() & Qt::FramelessWindowHint);
    EXPECT_TRUE(m_tester->windowFlags() & Qt::WindowStaysOnTopHint);
    EXPECT_TRUE(m_tester->windowFlags() & Qt::Window);
    // 透明背景 + 关闭即自删
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_TranslucentBackground));
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_DeleteOnClose));
    // 鼠标跟踪（grabMouse 期间用于悬停高亮）
    EXPECT_TRUE(m_tester->hasMouseTracking());
    // 1x1 尺寸且被移出屏幕外
    EXPECT_EQ(m_tester->size(), QSize(1, 1));
    EXPECT_EQ(m_tester->pos(), QPoint(-65536, -65536));
    // 窗口管理器实例已创建
    EXPECT_NE(m_tester->m_wminfo, nullptr);
}

TEST_F(UT_XWinKillPreviewWidget, test_initUI_02)
{
    // 重复调用 initUI：鼠标跟踪与帧窗标志保持不变（定时器 lambda 不触发，见文件头说明）
    m_tester->initUI();

    EXPECT_TRUE(m_tester->hasMouseTracking());
    EXPECT_TRUE(m_tester->windowFlags() & Qt::FramelessWindowHint);
    EXPECT_TRUE(m_tester->testAttribute(Qt::WA_DeleteOnClose));
}

TEST_F(UT_XWinKillPreviewWidget, test_initConnections_01)
{
    // WaylandCentered=false 分支：Wayland 连接块被编译宏+运行时开关双重隔离，调用不应崩溃
    m_tester->initConnections();

    EXPECT_FALSE(WaylandCentered);
}

TEST_F(UT_XWinKillPreviewWidget, test_mousePressEvent_01)
{
    // 非左键点击：直接早退，不查询窗口、不发信号、不关闭
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_hit);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::windowClicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPoint(0, 0), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    m_tester->mousePressEvent(&me);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_XWinKillPreviewWidget, test_mousePressEvent_02)
{
    // 左键但无命中窗口：不发信号
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::windowClicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPoint(50, 50), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mousePressEvent(&me);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_XWinKillPreviewWidget, test_mousePressEvent_03)
{
    // 左键且光标悬停在任务栏区域：早退不发信号
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_hit);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_true);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::windowClicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPoint(50, 50), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mousePressEvent(&me);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_XWinKillPreviewWidget, test_mousePressEvent_04)
{
    // 左键命中普通窗口：emit windowClicked(pid) 后自关（WA_DeleteOnClose 延迟删除）
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_hit);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::windowClicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPoint(50, 50), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mousePressEvent(&me);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 100000);
    EXPECT_FALSE(m_tester->isVisible());
    closeAndForget();
}

TEST_F(UT_XWinKillPreviewWidget, test_mousePressEvent_05)
{
    // 左键命中但窗口属于自身进程：跳过（getpid continue 分支），不发信号不关闭
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_selfpid);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::windowClicked);
    QMouseEvent me(QEvent::MouseButtonPress, QPoint(50, 50), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mousePressEvent(&me);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_XWinKillPreviewWidget, test_mouseMoveEvent_01)
{
    // 光标悬停任务栏：清空背景选区并恢复默认(禁止)光标
    // （selectWindow 在悬停检查之前被调用，同样需要打桩避免真实 X11 查询）
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_true);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::cursorUpdated);
    QMouseEvent me(QEvent::MouseMove, QPoint(50, 50), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    m_tester->mouseMoveEvent(&me);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<QCursor>().shape(), Qt::ForbiddenCursor);
}

TEST_F(UT_XWinKillPreviewWidget, test_mouseMoveEvent_02)
{
    // 无命中窗口：恢复默认(禁止)光标
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::cursorUpdated);
    QMouseEvent me(QEvent::MouseMove, QPoint(50, 50), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    m_tester->mouseMoveEvent(&me);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<QCursor>().shape(), Qt::ForbiddenCursor);
}

TEST_F(UT_XWinKillPreviewWidget, test_mouseMoveEvent_03)
{
    // 命中窗口但背景列表为空（500ms 定时器未触发，m_backgroundList 为空）：
    // 不做选区更新也不发光标信号
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_hit);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);
    stub.set(ADDR(WMInfo, getHoveredByWindowList), stub_xwk_wm_getHoveredByWindowList);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::cursorUpdated);
    QMouseEvent me(QEvent::MouseMove, QPoint(50, 50), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    m_tester->mouseMoveEvent(&me);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(UT_XWinKillPreviewWidget, test_mouseMoveEvent_04)
{
    // 唯一窗口属于自身进程：continue 跳过，found=false → 默认光标
    Stub stub;
    stub.set((QPoint (*)())&QCursor::pos, stub_xwk_cursorPos_50_50);
    stub.set(ADDR(WMInfo, selectWindow), stub_xwk_wm_selectWindow_selfpid);
    stub.set(ADDR(WMInfo, isCursorHoveringDocks), stub_xwk_wm_isCursorHoveringDocks_false);

    QSignalSpy spy(m_tester, &XWinKillPreviewWidget::cursorUpdated);
    QMouseEvent me(QEvent::MouseMove, QPoint(50, 50), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    m_tester->mouseMoveEvent(&me);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).value<QCursor>().shape(), Qt::ForbiddenCursor);
}

TEST_F(UT_XWinKillPreviewWidget, test_keyPressEvent_02)
{
    // 非 ESC 按键：无动作，窗口保持存活
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    m_tester->keyPressEvent(&keyEvent);

    // ctor 已 show()，非 ESC 按键不应关闭窗口（对比 ESC 用例 close 后不可见）
    EXPECT_TRUE(m_tester->isVisible());
    EXPECT_EQ(gApp->mainWindow()->windowState() & Qt::WindowMinimized, Qt::WindowNoState);
}

TEST_F(UT_XWinKillPreviewWidget, test_keyPressEvent_01)
{
    // ESC：还原主窗状态（清除最小化标志）并 close（WA_DeleteOnClose 延迟删除）
    gApp->mainWindow()->setWindowState(Qt::WindowMinimized);

    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    m_tester->keyPressEvent(&keyEvent);

    EXPECT_EQ(gApp->mainWindow()->windowState() & Qt::WindowMinimized, Qt::WindowNoState);
    EXPECT_FALSE(m_tester->isVisible());
    closeAndForget();
}
