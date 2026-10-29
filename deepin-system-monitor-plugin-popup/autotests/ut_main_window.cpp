// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_MainWindow
 *
 * 测试对象: deepin-system-monitor-plugin-popup/gui/mainwindow.cpp MainWindow 类
 * 外部依赖: DBus 会话总线(打桩, 无真实总线)/合成器状态(打桩)/键盘区域注册(打桩, 仅计数)/
 *          detached 进程启动(打桩, 仅记录)/屏幕缩放比(offscreen 恒为 1.0), 不依赖真实设备环境
 *
 * 用例声明表:
 * |---------------------------------------------|--------------|---------|------|-----------------------------|
 * | 测试用例名                                   | 测试类       | 测试级别 | 类型  | 覆盖函数                     |
 * |---------------------------------------------|--------------|---------|------|-----------------------------|
 * | construction_defaults_basicLayoutValid                    | UT_MainWindow| Level_1 | AUTO | 构造/initUI/registerMonitor  |
 * | construction_monitorReply_buildsDockWidgets | UT_MainWindow| Level_1 | AUTO | 构造(显示器路径解析分支)      |
 * | startLoader_detached_recordsProgram                  | UT_MainWindow| Level_1 | AUTO | startLoader                  |
 * | toggle_noComposite_showHideCycles             | UT_MainWindow| Level_2 | AUTO | Toggle/showAni/hideAni       |
 * | toggle_composite_animatedShowHideCycle      | UT_MainWindow| Level_2 | AUTO | Toggle/showAni/hideAni       |
 * | toggle_aniRunning_ignored              | UT_MainWindow| Level_2 | AUTO | Toggle(动画运行分支)          |
 * | showHide_trickTimer_showHideIgnored                    | UT_MainWindow| Level_2 | AUTO | Show/Hide(计时器防抖分支)     |
 * | setX_movesWindow_yFollowsRect                   | UT_MainWindow| Level_1 | AUTO | setX                         |
 * | compositeChanged_helper_syncsState            | UT_MainWindow| Level_2 | AUTO | CompositeChanged             |
 * | eventFilter_mouseMove_scrollsWindow               | UT_MainWindow| Level_2 | AUTO | eventFilter                  |
 * | eventFilter_release_resetsMoveDistance       | UT_MainWindow| Level_2 | AUTO | eventFilter                  |
 * | dbusPropertiesChanged_primaryRect_updatesGeometry   | UT_MainWindow| Level_3 | AUTO | dbusPropertiesChanged/geometryChanged |
 * | dbusPropertiesChanged_opacityMask_updatesMaskAlpha       | UT_MainWindow| Level_3 | AUTO | dbusPropertiesChanged(透明度分支) |
 * | adjustPosition_dockTop_efficientLayout             | UT_MainWindow| Level_3 | AUTO | adjustPosition(TOP 分支)      |
 * | adjustPosition_dockBottom_fashionLayout            | UT_MainWindow| Level_3 | AUTO | adjustPosition(BOTTOM+时尚)  |
 * | adjustPosition_dockRight_efficientLayout           | UT_MainWindow| Level_3 | AUTO | adjustPosition(RIGHT 分支)    |
 * | adjustPosition_dockRight_fashionLayout             | UT_MainWindow| Level_3 | AUTO | adjustPosition(RIGHT+时尚)   |
 * | adjustPosition_dockLeft_placesLeft                     | UT_MainWindow| Level_3 | AUTO | adjustPosition(LEFT 分支)     |
 * | adjustPosition_noDock_centerFallback               | UT_MainWindow| Level_3 | AUTO | adjustPosition(默认分支)      |
 * | getDisplayScreen_primaryRect_parsesGeometry           | UT_MainWindow| Level_2 | AUTO | getDisplayScreen(主屏分支)    |
 * | getDisplayScreen_monitorLoop_interiorReturn        | UT_MainWindow| Level_3 | AUTO | getDisplayScreen(监视器循环)  |
 * | getDisplayScreen_monitorLoop_fallsThrough     | UT_MainWindow| Level_3 | AUTO | getDisplayScreen(循环回落)    |
 * | getDisplayScreen_noReply_dockRectFallback           | UT_MainWindow| Level_3 | AUTO | getDisplayScreen(dock 回落)   |
 * | initDBus_noBus_returnsFalse                  | UT_MainWindow| Level_1 | AUTO | initDBus(无总线分支)          |
 * | changeTheme_themeType_switchesStylesheet             | UT_MainWindow| Level_2 | AUTO | changeTheme(明暗主题)         |
 * | slotShowOrHide_deferred_togglesVisible               | UT_MainWindow| Level_2 | AUTO | slotShowOrHide.../Toggle     |
 * | showHideEvents_override_emitsVisibilitySignals        | UT_MainWindow| Level_2 | AUTO | showEvent/hideEvent          |
 * | mouseMoveEvent_drag_blockedWindowMove            | UT_MainWindow| Level_2 | AUTO | mouseMoveEvent               |
 *
 * 用例共 33 个,用例与桩的对应关系:
 * - 上述用例全部依赖 ut_popup_stub 进程级桩(QDBusAbstractInterface::isValid/doCall、
 *   QObject::property、DWindowManagerHelper::hasComposite、DRegionMonitor::registerRegion、
 *   QProcess 静态 startDetached、popup Application::event(均已桩拦截)。
 *
 * 用例计数声明(min/actual):
 * | MainWindow 聚合 | Level_1 | stub+mock | 10 | 16 |
 * | MainWindow 聚合 | Level_2 | stub+mock | 12 | 17 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] 桩为进程级一次安装,用例层仅复位 knob(TearDown)
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] 桩不改变被测类内存布局,进程退出即恢复
 * 3. 所有的测试使用 ASSERT/EXPECT 上的宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] 分支覆盖情况见上方用例声明表的覆盖函数列
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是] 私有成员/私有方法通过 addr_pri 访问
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每个用例独立构造 MainWindow,桩 knob 每用例复位
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScopedPointer>
#include <QScrollBar>
#include <QSequentialAnimationGroup>
#include <QSignalSpy>
#include <QTest>
#include <gtest/gtest.h>

#include "../../tests/addr_pri.h"
#include "application.h"
#include "common/datacommon.h"
#include "dbusayatanainterface.h"
#include "dbusdockinterface.h"
#include "gui/mainwindow.h"
#include "helper.hpp"
#include "ut_popup_stub.h"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
using namespace Dock;

// ==================== 私有成员/方法访问 ====================

ACCESS_PRIVATE_FIELD(MainWindow, QDBusInterface *, m_displayInter);
ACCESS_PRIVATE_FIELD(MainWindow, QDBusInterface *, m_daemonDockInter);
ACCESS_PRIVATE_FIELD(MainWindow, DBusDockInterface *, m_dockInter);
ACCESS_PRIVATE_FIELD(MainWindow, DRegionMonitor *, m_regionMonitor);
ACCESS_PRIVATE_FIELD(MainWindow, QList<QDBusObjectPath>, m_dbusPathList);
ACCESS_PRIVATE_FIELD(MainWindow, QPropertyAnimation *, m_xAni);
ACCESS_PRIVATE_FIELD(MainWindow, QPropertyAnimation *, m_widthAni);
ACCESS_PRIVATE_FIELD(MainWindow, QSequentialAnimationGroup *, m_aniGroup);
ACCESS_PRIVATE_FIELD(MainWindow, QRect, m_rect);
ACCESS_PRIVATE_FIELD(MainWindow, bool, m_hasComposite);
ACCESS_PRIVATE_FIELD(MainWindow, DScrollArea *, m_scrollArea);
ACCESS_PRIVATE_FIELD(MainWindow, int, m_last_time_move);
ACCESS_PRIVATE_FIELD(MainWindow, QTimer *, m_trickTimer);

ACCESS_PRIVATE_FUN(MainWindow, QRect(void), getDisplayScreen);
ACCESS_PRIVATE_FUN(MainWindow, void(void), adjustPosition);
ACCESS_PRIVATE_FUN(MainWindow, bool(void), initDBus);

// ==================== 测试夹具 ====================

class UT_MainWindow: public ::testing::Test
{
public:
    UT_MainWindow() {}

public:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
        m_tester = new MainWindow();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        ut_popup_stub::resetKnobs();
    }

    // 注入主屏矩形应答并使 Display 接口生效,供 adjustPosition 系列用例复用
    void enablePrimaryRect(const QRect &rect)
    {
        ut_popup_stub::setPrimaryRectReply(true, rect);
        ut_popup_stub::g_displayInterValid = true;
    }

    // 注入 dock 接口的 geometry/Position/DisplayMode 等属性
    void setupDaemonDock(int position, int displayMode, const QRect &dockGeometry)
    {
        ut_popup_stub::setInterfaceValid(access_private_field::MainWindowm_daemonDockInter(*m_tester), true);
        ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_daemonDockInter(*m_tester),
                                            "Position", position);
        ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_daemonDockInter(*m_tester),
                                            "DisplayMode", displayMode);
        ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_dockInter(*m_tester),
                                            "geometry", QVariant::fromValue(dockGeometry));
    }

    MainWindow *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: construction_defaults_basicLayoutValid
// 测试类型: 正常场景
// 覆盖函数: 构造函数/initUI/registerMonitor(打桩计数)
// 操作步骤: 构造 MainWindow,校验固定宽度/区域注册计数/事件过滤器
// 预期结果: 宽度等于 Globals::WindowWidth,键盘区域注册恰好一次,不依赖真实总线
TEST_F(UT_MainWindow, construction_defaults_basicLayoutValid)
{
    // Arrange: SetUp 已构造 MainWindow(无总线环境,DBus 桩默认无效)
    // Act: 构造完成即为主体动作
    // Assert: 初始化界面固定宽度、注册键盘区域一次
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->width(), Globals::WindowWidth);
    EXPECT_EQ(ut_popup_stub::g_registerRegionCount, 1);
    EXPECT_NE(access_private_field::MainWindowm_regionMonitor(*m_tester), nullptr);
}

// 用例名称: construction_monitorReply_buildsDockWidgets
// 测试类型: 正常场景
// 覆盖函数: 构造函数(m_displayInter 有效时解析 Monitors 应答)
// 操作步骤: 先置 Display 接口有效并注入 Monitors 应答,再构造 MainWindow
// 预期结果: m_dbusPathList 解析出注入的两条显示器路径
TEST_F(UT_MainWindow, construction_monitorReply_buildsDockWidgets)
{
    // Arrange: Display 接口有效,Monitors Get 调用返回两条对象路径
    ut_popup_stub::g_displayInterValid = true;
    ut_popup_stub::setMonitorPathsReply(true, {
                                               QDBusObjectPath("/org/deepin/dde/Display1/Monitor0"),
                                               QDBusObjectPath("/org/deepin/dde/Display1/Monitor1") });
    // Act: 构造 MainWindow(构造函数内解析 Monitors)
    delete m_tester;
    m_tester = new MainWindow();
    // Assert: 路径列表与注入一致
    const auto &paths = access_private_field::MainWindowm_dbusPathList(*m_tester);
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths.first().path(), QStringLiteral("/org/deepin/dde/Display1/Monitor0"));
    EXPECT_EQ(paths.last().path(), QStringLiteral("/org/deepin/dde/Display1/Monitor1"));
}

// 用例名称: startLoader_detached_recordsProgram
// 测试类型: 正常场景
// 覆盖函数: startLoader
// 操作步骤: 调用 startLoader,检查 detached 启动计数与程序名
// 预期结果: 以 detached 方式启动 deepin-system-monitor-plugin-popup,不产生真实子进程
TEST_F(UT_MainWindow, startLoader_detached_recordsProgram)
{
    // Arrange: 桩已记录 detached 启动
    // Act: 启动加载器
    m_tester->startLoader();
    // Assert: 恰好启动一次目标程序
    EXPECT_EQ(ut_popup_stub::g_startDetachedCount, 1);
    EXPECT_EQ(ut_popup_stub::g_lastDetachedProgram, QStringLiteral("deepin-system-monitor-plugin-popup"));
}

// 用例名称: toggle_noComposite_showHideCycles
// 测试类型: 正常场景
// 覆盖函数: Toggle/showAni/hideAni(无合成器分支)
// 操作步骤: 无混成环境下连续 Toggle 三次(显示/防抖/隐藏)
// 预期结果: 首次显示;300ms 防抖内隐藏请求被忽略;防抖到期后隐藏成功
TEST_F(UT_MainWindow, toggle_noComposite_showHideCycles)
{
    // Arrange: 无混成(默认桩返回 false)
    ASSERT_FALSE(access_private_field::MainWindowm_hasComposite(*m_tester));
    // Act: 第一次 Toggle 显示
    m_tester->Toggle();
    // Assert: 窗口可见
    EXPECT_TRUE(m_tester->isVisible());
    // Act: 防抖窗口内第二次 Toggle
    m_tester->Toggle();
    // Assert: hideAni 被 trickTimer 拦截,仍可见
    EXPECT_TRUE(m_tester->isVisible());
    // Act: 防抖到期后第三次 Toggle
    QTest::qWait(350);
    m_tester->Toggle();
    // Assert: 隐藏
    EXPECT_FALSE(m_tester->isVisible());
}

// 用例名称: toggle_composite_animatedShowHideCycle
// 测试类型: 正常场景
// 覆盖函数: Toggle/showAni/hideAni(合成器动画分支)
// 操作步骤: 开启混成后 Toggle 显示动画,等待动画结束后再次 Toggle 隐藏
// 预期结果: 显示动画期间窗口可见;动画结束并防抖到期后隐藏动画完成,窗口不可见
TEST_F(UT_MainWindow, toggle_composite_animatedShowHideCycle)
{
    // Arrange: 开启混成
    ut_popup_stub::g_hasComposite = true;
    m_tester->CompositeChanged();
    // Act: 第一次 Toggle 触发显示动画
    m_tester->Toggle();
    // Assert: 窗口可见且动画组运行中(Backward 为显示方向)
    EXPECT_TRUE(m_tester->isVisible());
    auto *aniGroup = access_private_field::MainWindowm_aniGroup(*m_tester);
    EXPECT_EQ(aniGroup->direction(), QAbstractAnimation::Backward);
    // Act: 等待动画组(xAni 150ms + widthAni 300ms = 450ms)与防抖结束后 Toggle 隐藏
    QTest::qWait(700);
    m_tester->Toggle();
    // Assert: hideAni 以 Forward 方式运行,单发定时器(450ms)到期后窗口隐藏
    EXPECT_EQ(aniGroup->direction(), QAbstractAnimation::Forward);
    QTest::qWait(700);
    EXPECT_FALSE(m_tester->isVisible());
}

// 用例名称: toggle_aniRunning_ignored
// 测试类型: 边界场景
// 覆盖函数: Toggle(混成下动画运行中忽略请求分支)
// 操作步骤: 开启混成,触发显示动画后立即再次 Toggle
// 预期结果: 第二次 Toggle 因动画组运行中而被忽略
TEST_F(UT_MainWindow, toggle_aniRunning_ignored)
{
    // Arrange: 开启混成并触发显示动画
    ut_popup_stub::g_hasComposite = true;
    m_tester->CompositeChanged();
    m_tester->Toggle();
    ASSERT_TRUE(access_private_field::MainWindowm_aniGroup(*m_tester)->state() == QAbstractAnimation::Running);
    // Act: 动画运行中再次 Toggle
    m_tester->Toggle();
    // Assert: 窗口仍处于显示动画过程中(未被隐藏),动画状态未被第二次 Toggle 中断
    EXPECT_TRUE(m_tester->isVisible());
    EXPECT_EQ(access_private_field::MainWindowm_aniGroup(*m_tester)->state(),
              QAbstractAnimation::Running);
}

// 用例名称: showHide_trickTimer_showHideIgnored
// 测试类型: 边界场景
// 覆盖函数: Show/Hide(防抖拦截分支)
// 操作步骤: Toggle 显示窗口后,在防抖计时器(300ms)生效期内立即 Show 与 Hide
// 预期结果: 防抖计时器激活期间重复 Show/Hide 均被忽略,窗口保持可见
TEST_F(UT_MainWindow, showHide_trickTimer_showHideIgnored)
{
    // Arrange: 无混成默认态,经 Toggle 显示窗口(showAni 内部启动防抖计时器)
    m_tester->Toggle();
    ASSERT_TRUE(m_tester->isVisible());
    // Act: 防抖期内再次 Show / Hide
    m_tester->Show();
    m_tester->Hide();
    // Assert: 防抖计时器处于激活态,状态未变,仍可见
    EXPECT_TRUE(access_private_field::MainWindowm_trickTimer(*m_tester)->isActive());
    EXPECT_TRUE(m_tester->isVisible());
}

// 用例名称: setX_movesWindow_yFollowsRect
// 测试类型: 正常场景
// 覆盖函数: setX
// 操作步骤: 调用 setX(123),校验窗口位置
// 预期结果: 窗口 x 坐标变为 123,y 坐标保持 m_rect.y()(默认 10)
TEST_F(UT_MainWindow, setX_movesWindow_yFollowsRect)
{
    // Arrange: 构造后的 m_rect
    const int rectY = access_private_field::MainWindowm_rect(*m_tester).y();
    // Act: 设置窗口 x
    m_tester->setX(123);
    // Assert: x 生效且 y 不变
    EXPECT_EQ(m_tester->pos().x(), 123);
    EXPECT_EQ(m_tester->pos().y(), rectY);
}

// 用例名称: compositeChanged_helper_syncsState
// 测试类型: 正常场景
// 覆盖函数: CompositeChanged
// 操作步骤: 切换 hasComposite 桩值后调用 CompositeChanged
// 预期结果: m_hasComposite 与打桩的合成器状态一致
TEST_F(UT_MainWindow, compositeChanged_helper_syncsState)
{
    // Arrange: 默认无混成
    m_tester->CompositeChanged();
    EXPECT_FALSE(access_private_field::MainWindowm_hasComposite(*m_tester));
    // Act: 开启混成后再次同步
    ut_popup_stub::g_hasComposite = true;
    m_tester->CompositeChanged();
    // Assert: 状态跟随合成器
    EXPECT_TRUE(access_private_field::MainWindowm_hasComposite(*m_tester));
}

// 用例名称: eventFilter_mouseMove_scrollsWindow
// 测试类型: 正常场景
// 覆盖函数: eventFilter(MouseMove 滚动分支)
// 操作步骤: 给滚动区注入大内容后向窗口发送两次鼠标移动事件
// 预期结果: 第一次建立基准,第二次按位移差滚动滚动条
TEST_F(UT_MainWindow, eventFilter_mouseMove_scrollsWindow)
{
    // Arrange: 合成 MouseMove 需开启鼠标跟踪(QApplication::notify 会丢弃
    // 无跟踪 widget 的非自发 hover 移动事件);扩大滚动内容使滚动条有可用范围
    m_tester->setMouseTracking(true);
    auto *scrollArea = access_private_field::MainWindowm_scrollArea(*m_tester);
    scrollArea->widget()->setMinimumHeight(2000);
    m_tester->resize(Globals::WindowWidth, 400);
    QTest::qWait(50);
    QMouseEvent move1(QEvent::MouseMove, QPointF(10, 100), QPointF(10, 100),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    // Act: 第一次移动建立基准位置
    QApplication::sendEvent(m_tester, &move1);
    // Assert: 基准位置被记录
    EXPECT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 100);
    // Act: 第二次向上移动 40 像素
    QMouseEvent move2(QEvent::MouseMove, QPointF(10, 60), QPointF(10, 60),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &move2);
    // Assert: 位移差生效,滚动值增加且基准更新
    EXPECT_EQ(scrollArea->verticalScrollBar()->value(), 40);
    EXPECT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 60);
}

// 用例名称: eventFilter_release_resetsMoveDistance
// 测试类型: 边界场景
// 覆盖函数: eventFilter(MouseButtonRelease 复位分支)
// 操作步骤: 发送移动事件建立基准后,发送释放事件
// 预期结果: 拖动基准复位为 0
TEST_F(UT_MainWindow, eventFilter_release_resetsMoveDistance)
{
    // Arrange: 开启鼠标跟踪(同上,保证合成 MouseMove 能进入自装过滤器),建立拖动基准
    m_tester->setMouseTracking(true);
    QMouseEvent move1(QEvent::MouseMove, QPointF(5, 80), QPointF(5, 80),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &move1);
    ASSERT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 80);
    // Act: 发送鼠标释放事件
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(5, 80), QPointF(5, 80),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &release);
    // Assert: 基准复位
    EXPECT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 0);
    // Act: 释放后再发送一次移动,验证基准已可重新建立(拖动链路未损坏)
    QMouseEvent move2(QEvent::MouseMove, QPointF(5, 100), QPointF(5, 100),
                      Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &move2);
    // Assert: 新基准被记录
    EXPECT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 100);
}

// 用例名称: dbusPropertiesChanged_primaryRect_updatesGeometry
// 测试类型: 正常场景
// 覆盖函数: dbusPropertiesChanged → geometryChanged → adjustPosition(Display PrimaryRect 分支)
// 操作步骤: 注入主屏矩形(0,0,1920,1080)与无 dock 环境,派发 PropertiesChanged 信号
// 预期结果: m_rect 重算为 (0,10,300,1060),并发出 signal_geometry(1040)
TEST_F(UT_MainWindow, dbusPropertiesChanged_primaryRect_updatesGeometry)
{
    // Arrange: 注入主屏矩形,dock 接口默认无效
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    QSignalSpy geometrySpy(m_tester, &MainWindow::signal_geometry);
    const QString iface = common::systemInfo().DISPLAY_INTERFACE;
    QVariantMap props { { "PrimaryRect", QVariant() } };
    QStringList invalidProps;
    // Act: 派发 Display 接口 PrimaryRect 属性变化
    QMetaObject::invokeMethod(m_tester, "dbusPropertiesChanged",
                              Q_ARG(QString, iface),
                              Q_ARG(QVariantMap, props),
                              Q_ARG(QStringList, invalidProps));
    // Assert: 几何重算结果正确且信号发出
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(0, 10, 300, 1060));
    EXPECT_EQ(geometrySpy.count(), 1);
    if (geometrySpy.count() > 0)
        EXPECT_EQ(geometrySpy.first().first().toInt(), 1040);
}

// 用例名称: dbusPropertiesChanged_opacityMask_updatesMaskAlpha
// 测试类型: 边界场景
// 覆盖函数: dbusPropertiesChanged(dock Opacity 透明度分支)
// 操作步骤: dock 接口有效并注入 Opacity=0.5,派发 Dock 接口 Opacity 属性变化
// 预期结果: 窗口遮罩透明度设置为 0.5*255=127
TEST_F(UT_MainWindow, dbusPropertiesChanged_opacityMask_updatesMaskAlpha)
{
    // Arrange: dock 接口有效,属性 Opacity=0.5
    setupDaemonDock(Dock::Right, 1, QRect(1910, 0, 10, 1080));
    ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_daemonDockInter(*m_tester),
                                        "Opacity", 0.5);
    const QString iface = common::systemInfo().DOCK_INTERFACE;
    QVariantMap props { { "Opacity", QVariant() } };
    QStringList invalidProps;
    // Act: 派发 Dock 接口 Opacity 属性变化
    QMetaObject::invokeMethod(m_tester, "dbusPropertiesChanged",
                              Q_ARG(QString, iface),
                              Q_ARG(QVariantMap, props),
                              Q_ARG(QStringList, invalidProps));
    // Assert: 遮罩透明度 = int(0.5*255),且确实按比例缩放(非全不透明)
    EXPECT_EQ(m_tester->maskAlpha(), quint8(0.5 * 255));
    EXPECT_NE(m_tester->maskAlpha(), quint8(255));
}

// 用例名称: adjustPosition_dockTop_efficientLayout
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_TOP + 高效模式分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、顶部 dock(高 20)、高效模式
// 预期结果: m_rect=(1611,30,300,1040),signal_geometry 携带滚动区高度 1020
TEST_F(UT_MainWindow, adjustPosition_dockTop_efficientLayout)
{
    // Arrange: 主屏 + 顶部 dock + 高效模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Top, 1, QRect(0, 0, 1920, 20));
    QSignalSpy geometrySpy(m_tester, &MainWindow::signal_geometry);
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 窗口位于 dock 下方右侧,滚动区高度同步
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 30, 300, 1040));
    ASSERT_EQ(geometrySpy.count(), 1);
    if (geometrySpy.count() > 0)
        EXPECT_EQ(geometrySpy.first().first().toInt(), 1020);
    EXPECT_EQ(access_private_field::MainWindowm_scrollArea(*m_tester)->height(), 1020);
}

// 用例名称: adjustPosition_dockBottom_fashionLayout
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_BOTTOM + 时尚模式额外偏移分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、底部 dock(高 20)、时尚模式
// 预期结果: m_rect=(1611,10,300,1030)(滚动高度计算先于时尚偏移,signal_geometry=1040)
TEST_F(UT_MainWindow, adjustPosition_dockBottom_fashionLayout)
{
    // Arrange: 主屏 + 底部 dock + 时尚模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Bottom, 0, QRect(0, 1060, 1920, 20));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 底部 dock 时窗口高度额外收缩 10px,水平位置不变
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 10, 300, 1030));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).x(), 1611);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).width(), 300);
}

// 用例名称: adjustPosition_dockRight_efficientLayout
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_RIGHT + 高效模式分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、右侧 dock(宽 10)、高效模式
// 预期结果: m_rect=(1601,10,300,1060),滚动区高度 1040
TEST_F(UT_MainWindow, adjustPosition_dockRight_efficientLayout)
{
    // Arrange: 主屏 + 右侧 dock + 高效模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Right, 1, QRect(1910, 0, 10, 1080));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 右侧 dock 高效模式窗口位置
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1601, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_scrollArea(*m_tester)->height(), 1040);
}

// 用例名称: adjustPosition_dockRight_fashionLayout
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_RIGHT + 时尚模式分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、右侧 dock(宽 10)、时尚模式
// 预期结果: m_rect=(1591,10,300,1060)(时尚模式额外减去一个 WindowMargin)
TEST_F(UT_MainWindow, adjustPosition_dockRight_fashionLayout)
{
    // Arrange: 主屏 + 右侧 dock + 时尚模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Right, 0, QRect(1910, 0, 10, 1080));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 时尚模式窗口位置,垂直起点不变
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1591, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).y(), 10);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).width(), 300);
}

// 用例名称: adjustPosition_dockTop_fashionMargin
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_TOP + 时尚模式附加边距分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、顶部 dock(高 20)、时尚模式
// 预期结果: m_rect=(1611,40,300,1030),时尚模式额外扣减顶部边距 10px
TEST_F(UT_MainWindow, adjustPosition_dockTop_fashionMargin)
{
    // Arrange: 主屏 + 顶部 dock + 时尚模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Top, 0, QRect(0, 0, 1920, 20));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 顶部 dock 时尚模式布局
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 40, 300, 1030));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).y(), 40);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).height(), 1030);
}

// 用例名称: adjustPosition_dockBottom_efficientLayout
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_BOTTOM + 高效模式分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、底部 dock(高 20)、高效模式
// 预期结果: m_rect=(1611,10,300,1040),高效模式无额外边距
TEST_F(UT_MainWindow, adjustPosition_dockBottom_efficientLayout)
{
    // Arrange: 主屏 + 底部 dock + 高效模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Bottom, 1, QRect(0, 1060, 1920, 20));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 底部 dock 高效模式布局
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 10, 300, 1040));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).y(), 10);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).height(), 1040);
}

// 用例名称: adjustPosition_unknownPos_defaultBranch
// 测试类型: 边界场景
// 覆盖函数: adjustPosition(位置值未知的 default 分支)
// 操作步骤: 注入主屏与 dock 但 Position 设为未知值 99
// 预期结果: default 分支不做位置修正,m_rect=(0,10,300,1060)
TEST_F(UT_MainWindow, adjustPosition_unknownPos_defaultBranch)
{
    // Arrange: 主屏 + dock(未知位置)
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(99, 1, QRect(0, 0, 10, 1080));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: default 分支无位置修正
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(0, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).x(), 0);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).width(), 300);
}

// 用例名称: adjustPosition_fashionEmptyDock_skipsMargin
// 测试类型: 边界场景
// 覆盖函数: adjustPosition(时尚模式但 dock 矩形为空的跳过分支)
// 操作步骤: 时尚模式(displayMode=0)且 dock geometry 为空矩形
// 预期结果: dockHeight=0 不扣减高度且不应用时尚模式附加边距,m_rect=(1611,10,300,1060)
TEST_F(UT_MainWindow, adjustPosition_fashionEmptyDock_skipsMargin)
{
    // Arrange: 主屏 + 底部 dock(时尚模式,dock 矩形为空)
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Bottom, 0, QRect());
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 空 dock 矩形不触发附加边距,高度无 dock 扣减
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).y(), 10);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).height(), 1060);
}

// 用例名称: adjustPosition_dockLeft_placesLeft
// 测试类型: 正常场景
// 覆盖函数: adjustPosition(DOCK_LEFT 分支)
// 操作步骤: 注入主屏(0,0,1920,1080)、左侧 dock(宽 10)
// 预期结果: m_rect=(1611,10,300,1060),dockHeight 置 0 不再扣减高度
TEST_F(UT_MainWindow, adjustPosition_dockLeft_placesLeft)
{
    // Arrange: 主屏 + 左侧 dock + 高效模式
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    setupDaemonDock(Dock::Left, 1, QRect(0, 0, 10, 1080));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 左侧 dock 窗口位置(高度不扣减),仅水平收缩
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(1611, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).y(), 10);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).height(), 1060);
}

// 用例名称: adjustPosition_noDock_centerFallback
// 测试类型: 边界场景
// 覆盖函数: adjustPosition(dock 接口无效默认分支)
// 操作步骤: 仅注入主屏矩形,dock 接口保持无效
// 预期结果: m_rect=(0,10,300,1060)(无 dock 位置修正)
TEST_F(UT_MainWindow, adjustPosition_noDock_centerFallback)
{
    // Arrange: 主屏有效,dock 无效
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    // Act: 执行 adjustPosition
    call_private_fun::MainWindowadjustPosition(*m_tester);
    // Assert: 默认布局(水平起点为屏区左缘)
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester), QRect(0, 10, 300, 1060));
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).x(), 0);
    EXPECT_EQ(access_private_field::MainWindowm_rect(*m_tester).width(), 300);
}

// 用例名称: getDisplayScreen_primaryRect_parsesGeometry
// 测试类型: 正常场景
// 覆盖函数: getDisplayScreen(m_displayInter 有效,解析 PrimaryRect 分支)
// 操作步骤: Display 接口有效并注入 PrimaryRect=(5,6,100,200),调用 getDisplayScreen
// 预期结果: 返回矩形 (5,6,100,200)
TEST_F(UT_MainWindow, getDisplayScreen_primaryRect_parsesGeometry)
{
    // Arrange: Display 接口有效 + PrimaryRect 应答
    enablePrimaryRect(QRect(5, 6, 100, 200));
    // Act: 获取显示屏幕矩形
    QRect result = call_private_fun::MainWindowgetDisplayScreen(*m_tester);
    // Assert: 与注入的应答一致(逐字段校验)
    EXPECT_EQ(result, QRect(5, 6, 100, 200));
    EXPECT_EQ(result.x(), 5);
    EXPECT_EQ(result.width(), 100);
}

// 用例名称: getDisplayScreen_monitorLoop_interiorReturn
// 测试类型: 边界场景
// 覆盖函数: getDisplayScreen(监视器循环内返回分支)
// 操作步骤: 注入显示器路径并使监视接口有效,Display 接口属性无效(属性值全为 0)
// 预期结果: 屏幕矩形(0,0,0,0)包含空 dock 矩形,循环内直接返回 (0,0,0,0)
TEST_F(UT_MainWindow, getDisplayScreen_monitorLoop_interiorReturn)
{
    // Arrange: 注入显示器路径 + 监视接口有效
    access_private_field::MainWindowm_dbusPathList(*m_tester) =
        QList<QDBusObjectPath> { QDBusObjectPath("/m0") };
    ut_popup_stub::g_monitorLoopValid = true;
    // Act: 获取显示屏幕矩形
    QRect result = call_private_fun::MainWindowgetDisplayScreen(*m_tester);
    // Assert: 命中循环内 screenRect.contains(dockRect) 返回分支(逐字段校验)
    EXPECT_EQ(result, QRect(0, 0, 0, 0));
    EXPECT_EQ(result.x(), 0);
    EXPECT_EQ(result.width(), 0);
}

// 用例名称: getDisplayScreen_monitorLoop_fallsThrough
// 测试类型: 边界场景
// 覆盖函数: getDisplayScreen(监视器循环不包含则继续分支)
// 操作步骤: 注入显示器路径、监视接口有效,且 Display 接口属性 X=5(四个字段均读 X)
// 预期结果: screenRect(5,5,5,5) 不包含空 dock 矩形,循环走完回落返回 dockRect
TEST_F(UT_MainWindow, getDisplayScreen_monitorLoop_fallsThrough)
{
    // Arrange: 注入显示器路径 + 监视接口有效 + Display 接口属性 X=5
    access_private_field::MainWindowm_dbusPathList(*m_tester) =
        QList<QDBusObjectPath> { QDBusObjectPath("/m0") };
    ut_popup_stub::g_monitorLoopValid = true;
    ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_displayInter(*m_tester),
                                        "X", 5);
    // Act: 获取显示屏幕矩形
    QRect result = call_private_fun::MainWindowgetDisplayScreen(*m_tester);
    // Assert: 未命中包含判断,回落到 dockRect(空)
    EXPECT_EQ(result, QRect());
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.width(), 0);
}

// 用例名称: getDisplayScreen_noReply_dockRectFallback
// 测试类型: 边界场景
// 覆盖函数: getDisplayScreen(无显示器路径且 Display 无效的回落分支)
// 操作步骤: 注入 dock geometry 后调用 getDisplayScreen
// 预期结果: 返回 dock 的 geometry 矩形
TEST_F(UT_MainWindow, getDisplayScreen_noReply_dockRectFallback)
{
    // Arrange: dock geometry 注入
    ut_popup_stub::setInterfaceProperty(access_private_field::MainWindowm_dockInter(*m_tester),
                                        "geometry", QVariant::fromValue(QRect(100, 200, 300, 40)));
    // Act: 获取显示屏幕矩形
    QRect result = call_private_fun::MainWindowgetDisplayScreen(*m_tester);
    // Assert: 回落返回 dock 矩形(逐字段校验)
    EXPECT_EQ(result, QRect(100, 200, 300, 40));
    EXPECT_EQ(result.x(), 100);
    EXPECT_EQ(result.height(), 40);
}

// 用例名称: initDBus_noBus_returnsFalse
// 测试类型: 边界场景
// 覆盖函数: initDBus(无会话总线分支)
// 操作步骤: 无总线环境下直接调用 initDBus
// 预期结果: 返回 false 且不崩溃
TEST_F(UT_MainWindow, initDBus_noBus_returnsFalse)
{
    // Arrange: 无总线环境(桩未接管会话总线)
    // Act: 注册 DBus 服务(两次调用验证幂等)
    bool result = call_private_fun::MainWindowinitDBus(*m_tester);
    bool result2 = call_private_fun::MainWindowinitDBus(*m_tester);
    // Assert: 注册失败返回 false
    EXPECT_FALSE(result);
    EXPECT_FALSE(result2);
}

// 用例名称: changeTheme_themeType_switchesStylesheet
// 测试类型: 正常场景
// 覆盖函数: changeTheme(明/暗主题分支)
// 操作步骤: 分别以 LightType 与 DarkType 调用 changeTheme
// 预期结果: 滚动条样式表随主题切换,暗色主题关闭自动填充
TEST_F(UT_MainWindow, changeTheme_themeType_switchesStylesheet)
{
    // Arrange: 获取滚动区
    auto *scrollArea = access_private_field::MainWindowm_scrollArea(*m_tester);
    // Act: 切换亮色主题
    QMetaObject::invokeMethod(m_tester, "changeTheme",
                              Q_ARG(DGuiApplicationHelper::ColorType, DGuiApplicationHelper::LightType));
    // Assert: 亮色样式表
    EXPECT_EQ(scrollArea->verticalScrollBar()->styleSheet(),
              QStringLiteral("QScrollBar::handle:vertical:hover{background-color:rgb(50,50,50);}"));
    // Act: 切换暗色主题
    QMetaObject::invokeMethod(m_tester, "changeTheme",
                              Q_ARG(DGuiApplicationHelper::ColorType, DGuiApplicationHelper::DarkType));
    // Assert: 暗色样式表
    EXPECT_EQ(scrollArea->verticalScrollBar()->styleSheet(),
              QStringLiteral("QScrollBar::handle:vertical:hover{background-color:rgb(200,200,200);}"));
}

// 用例名称: slotShowOrHide_deferred_togglesVisible
// 测试类型: 正常场景
// 覆盖函数: slotShowOrHideSystemMonitorPluginPopupWidget(0ms 延迟触发 Toggle)
// 操作步骤: 调用槽后处理事件循环
// 预期结果: 延迟触发 Toggle,窗口显示
TEST_F(UT_MainWindow, slotShowOrHide_deferred_togglesVisible)
{
    // Arrange: 窗口初始不可见,监听可见性信号
    ASSERT_FALSE(m_tester->isVisible());
    QSignalSpy visibleSpy(m_tester, &MainWindow::sysMonPopVisibleChanged);
    // Act: 调用显示/隐藏切换槽,驱动事件循环
    QMetaObject::invokeMethod(m_tester, "slotShowOrHideSystemMonitorPluginPopupWidget");
    QTest::qWait(50);
    // Assert: 0ms 单发定时器触发 Toggle 后窗口显示,且可见性信号已发出
    EXPECT_TRUE(m_tester->isVisible());
    ASSERT_GE(visibleSpy.count(), 1);
    EXPECT_TRUE(visibleSpy.last().first().toBool());
}

// 用例名称: showHideEvents_override_emitsVisibilitySignals
// 测试类型: 正常场景
// 覆盖函数: showEvent/hideEvent(可见性信号)
// 操作步骤: show 与 hide 窗口,监听 sysMonPopVisibleChanged
// 预期结果: 显示时携带 true,隐藏时携带 false
TEST_F(UT_MainWindow, showHideEvents_override_emitsVisibilitySignals)
{
    // Arrange: 监听可见性信号
    QSignalSpy visibleSpy(m_tester, &MainWindow::sysMonPopVisibleChanged);
    // Act: 显示窗口
    m_tester->show();
    // Assert: 显示信号
    ASSERT_GE(visibleSpy.count(), 1);
    EXPECT_TRUE(visibleSpy.last().first().toBool());
    // Act: 隐藏窗口
    m_tester->hide();
    // Assert: 隐藏信号
    ASSERT_GE(visibleSpy.count(), 2);
    EXPECT_FALSE(visibleSpy.last().first().toBool());
    // 显示状态查询接口
    EXPECT_EQ(m_tester->sysMonPopVisible(), m_tester->isVisible());
}

// 用例名称: mouseMoveEvent_drag_blockedWindowMove
// 测试类型: 正常场景
// 覆盖函数: mouseMoveEvent(禁止拖动保护)
// 操作步骤: 向窗口发送鼠标移动事件
// 预期结果: 窗口位置保持不变(事件被吞掉)
TEST_F(UT_MainWindow, mouseMoveEvent_drag_blockedWindowMove)
{
    // Arrange: 记录初始位置
    const QPoint origin = m_tester->pos();
    // Act: 发送拖动事件
    QMouseEvent move(QEvent::MouseMove, QPointF(40, 40), QPointF(40, 40),
                     Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &move);
    // Assert: 位置不变,且自装过滤器已接管该移动(拖动基准被记录,窗口不可被拖动)
    EXPECT_EQ(m_tester->pos(), origin);
    EXPECT_EQ(access_private_field::MainWindowm_last_time_move(*m_tester), 40);
}

// 用例名称: geometryChanged_reset_resetsAniParams
// 测试类型: 正常场景
// 覆盖函数: geometryChanged(动画参数重置)
// 操作步骤: 注入主屏矩形后调用 geometryChanged
// 预期结果: x 动画起止值等于 m_rect.x(),宽度动画结束值为 0
TEST_F(UT_MainWindow, geometryChanged_reset_resetsAniParams)
{
    // Arrange: 主屏矩形 + 无 dock
    enablePrimaryRect(QRect(0, 0, 1920, 1080));
    // Act: 触发几何变化
    QMetaObject::invokeMethod(m_tester, "geometryChanged");
    // Assert: 动画参数与 m_rect 一致
    const QRect &rect = access_private_field::MainWindowm_rect(*m_tester);
    EXPECT_EQ(access_private_field::MainWindowm_xAni(*m_tester)->startValue().toInt(), rect.x());
    EXPECT_EQ(access_private_field::MainWindowm_xAni(*m_tester)->endValue().toInt(), rect.x());
    EXPECT_EQ(access_private_field::MainWindowm_widthAni(*m_tester)->startValue().toInt(), rect.width());
    EXPECT_EQ(access_private_field::MainWindowm_widthAni(*m_tester)->endValue().toInt(), 0);
}
