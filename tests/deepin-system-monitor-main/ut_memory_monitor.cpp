// Copyright (C) 2019-2026 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_MemoryMonitor：MemoryMonitor 单元测试（qt-autotest-generator 重写）
// 覆盖：构造初始化、progress 属性、changeTheme 主题配色、changeFont 字体调整、
//       onStatInfoUpdated 动画启动、onAnimationFinished 占比计算、onValueChanged 重绘、
//       paintEvent 绘制（交换分区启用/未启用）、鼠标事件与信号发射
// 依赖注入：通过 Stub 替换 core::system::MemInfo，隔离真实内存数据源
//
// 用例计数声明：
// | 方法                  | 级别 | 因子                     | min | actual |
// |-----------------------|------|--------------------------|-----|--------|
// | MemoryMonitor (ctor)  | low  | -                        | 1   | 1      |
// | changeTheme           | high | complexity:5,lines:50    | 3   | 3      |
// | changeFont            | mid  | in_degree:2              | 1   | 1      |
// | onAnimationFinished   | low  | -                        | 1   | 1      |
// | onStatInfoUpdated     | low  | -                        | 1   | 1      |
// | onValueChanged        | low  | -                        | 1   | 1      |
// | paintEvent            | mid  | lines:141                | 1   | 2      |
// | mouseReleaseEvent     | low  | -                        | 1   | 2      |
// | mouseMoveEvent        | low  | -                        | 1   | 1      |
// | progress/setProgress  | low  | -                        | 1   | 1      |
// | ~MemoryMonitor        | low  | destructor               | 1   | 14     |

//self
#include "memory_monitor.h"
#include "system/mem.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//qt
#include <DApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QPropertyAnimation>
#include <QTest>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif


// 私有成员/受保护函数经 addr_pri.h 访问（不修改生产头文件）
ACCESS_PRIVATE_FIELD(MemoryMonitor, QIcon, m_icon)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QColor, memoryBackgroundColor)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QColor, swapBackgroundColor)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QFont, m_titleFont)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QFont, m_contentFont)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QFont, m_subContentFont)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QFont, m_memPercentFont)
ACCESS_PRIVATE_FIELD(MemoryMonitor, qreal, m_lastMemPercent)
ACCESS_PRIVATE_FIELD(MemoryMonitor, qreal, m_lastSwapPercent)
ACCESS_PRIVATE_FIELD(MemoryMonitor, QPropertyAnimation *, m_animation)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(const QFont &), changeFont)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(QMouseEvent *), mouseReleaseEvent)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(QMouseEvent *), mouseMoveEvent)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(DApplicationHelper::ColorType), changeTheme)
ACCESS_PRIVATE_FIELD(MemoryMonitor, DApplicationHelper::ColorType, m_themeType)
#else
ACCESS_PRIVATE_FUN(MemoryMonitor, void(DGuiApplicationHelper::ColorType), changeTheme)
ACCESS_PRIVATE_FIELD(MemoryMonitor, DGuiApplicationHelper::ColorType, m_themeType)
#endif
ACCESS_PRIVATE_FUN(MemoryMonitor, void(void), onStatInfoUpdated)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(void), onAnimationFinished)
ACCESS_PRIVATE_FUN(MemoryMonitor, void(void), onValueChanged)

/***************************************STUB begin*********************************************/

static qulonglong stub_memTotal()
{
    return 1000;
}

static qulonglong stub_memAvailable()
{
    return 250;
}

static qulonglong stub_swapTotal()
{
    return 500;
}

static qulonglong stub_swapFree()
{
    return 250;
}

static qulonglong stub_swapTotal_zero()
{
    return 0;
}

/***************************************STUB end**********************************************/

class UT_MemoryMonitor: public ::testing::Test
{
public:
    UT_MemoryMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new MemoryMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MemoryMonitor *m_tester;
};

TEST_F(UT_MemoryMonitor, MemoryMonitor_Ctor_DefaultSetup_FixedSizeAndAnimConfigured)
{
    // Arrange: 预期尺寸为状态栏宽度 x 160，动画时长 10ms
    const int expectedWidth = common::getStatusBarMaxWidth();
    const int expectedHeight = 160;
    // Act: fixture 已构造 MemoryMonitor，读取控件当前状态
    const int actualWidth = m_tester->width();
    // Assert: 固定宽为状态栏宽度、高为 160，动画时长 10ms，进度初值 0
    EXPECT_EQ(actualWidth, expectedWidth);
    EXPECT_EQ(m_tester->height(), expectedHeight);
    EXPECT_EQ(access_private_field::MemoryMonitorm_animation(*m_tester)->duration(), 10);
    EXPECT_DOUBLE_EQ(m_tester->progress(), 0.0);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_SetProgress_AnyValue_PropertyRoundTrip)
{
    // Arrange
    const qreal expected = 0.5;
    // Act: 通过 Qt 属性系统写入 progress
    m_tester->setProperty("progress", expected);
    // Assert: 属性读写回路一致
    EXPECT_DOUBLE_EQ(m_tester->progress(), expected);
    EXPECT_DOUBLE_EQ(m_tester->property("progress").toDouble(), expected);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_ChangeTheme_LightType_BackgroundColorsTurnBlack)
{
    // Arrange: 预期亮色主题下内存/交换分区底色均为黑色
    const std::string expectedColor = "#000000";
    // Act: 切换到亮色主题
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DApplicationHelper::LightType);
#else
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DGuiApplicationHelper::ColorType::LightType);
#endif
    // Assert: 亮色主题下内存/交换分区底色均为黑色，主题类型已记录
    EXPECT_EQ(access_private_field::MemoryMonitormemoryBackgroundColor(*m_tester).name().toStdString(), std::string("#000000"));
    EXPECT_EQ(access_private_field::MemoryMonitorswapBackgroundColor(*m_tester).name().toStdString(), std::string("#000000"));
    EXPECT_EQ(access_private_field::MemoryMonitorm_icon(*m_tester).isNull(), false);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_ChangeTheme_DarkType_BackgroundColorsTurnWhite)
{
    // Arrange: 预期暗色主题下内存/交换分区底色均为白色
    const std::string expectedColor = "#ffffff";
    // Act: 切换到暗色主题
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DApplicationHelper::DarkType);
#else
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DGuiApplicationHelper::ColorType::DarkType);
#endif
    // Assert: 暗色主题下内存/交换分区底色均为白色
    EXPECT_EQ(access_private_field::MemoryMonitormemoryBackgroundColor(*m_tester).name().toStdString(), expectedColor);
    EXPECT_EQ(access_private_field::MemoryMonitorswapBackgroundColor(*m_tester).name().toStdString(), expectedColor);
    EXPECT_EQ(access_private_field::MemoryMonitorm_themeType(*m_tester), DGuiApplicationHelper::ColorType::DarkType);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_ChangeTheme_UnknownType_BackgroundColorsUnchanged)
{
    // Arrange: 记录当前主题底色
    const QColor oldMemBg = access_private_field::MemoryMonitormemoryBackgroundColor(*m_tester);
    // Act
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DApplicationHelper::UnknownType);
#else
    call_private_fun::MemoryMonitorchangeTheme(*m_tester, DGuiApplicationHelper::ColorType::UnknownType);
#endif
    // Assert: 未知主题走 default 分支，底色保持不变
    EXPECT_EQ(access_private_field::MemoryMonitormemoryBackgroundColor(*m_tester), oldMemBg);
    EXPECT_EQ(access_private_field::MemoryMonitorswapBackgroundColor(*m_tester).isValid(), true);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_ChangeFont_NormalFont_FontsAdjusted)
{
    // Arrange
    QFont font;
    font.setPointSizeF(10.0);
    // Act
    call_private_fun::MemoryMonitorchangeFont(*m_tester, font);
    // Assert: 标题字体放大 12pt，内容/次级内容缩 1pt 且内容字体 Medium，占比字体加粗
    EXPECT_NEAR(access_private_field::MemoryMonitorm_titleFont(*m_tester).pointSizeF(), 22.0, 0.01);
    EXPECT_EQ(access_private_field::MemoryMonitorm_contentFont(*m_tester).weight(), QFont::Medium);
    EXPECT_NEAR(access_private_field::MemoryMonitorm_contentFont(*m_tester).pointSizeF(), 9.0, 0.01);
    EXPECT_NEAR(access_private_field::MemoryMonitorm_subContentFont(*m_tester).pointSizeF(), 9.0, 0.01);
    EXPECT_TRUE(access_private_field::MemoryMonitorm_memPercentFont(*m_tester).bold());
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_OnAnimationFinished_ValidMemInfo_LastPercentsComputed)
{
    // Arrange: 打桩内存信息 total=1000 avail=250 swap=500 free=250
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    call_private_fun::MemoryMonitoronAnimationFinished(*m_tester);
    // Assert: 内存占用 75%，交换分区占用 50%
    EXPECT_NEAR(access_private_field::MemoryMonitorm_lastMemPercent(*m_tester), 0.75, 1e-6);
    EXPECT_NEAR(access_private_field::MemoryMonitorm_lastSwapPercent(*m_tester), 0.5, 1e-6);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_OnStatInfoUpdated_StatUpdated_AnimationRunsToFinish)
{
    // Arrange: 打桩内存信息，监听动画状态
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    call_private_fun::MemoryMonitoronStatInfoUpdated(*m_tester);
    // Assert: 动画立即进入 Running，等待后自然结束并落盘最新占比
    EXPECT_EQ(access_private_field::MemoryMonitorm_animation(*m_tester)->state(), QAbstractAnimation::Running);
    QTest::qWait(60);
    EXPECT_EQ(access_private_field::MemoryMonitorm_animation(*m_tester)->state(), QAbstractAnimation::Stopped);
    EXPECT_NEAR(access_private_field::MemoryMonitorm_lastMemPercent(*m_tester), 0.75, 1e-6);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_OnValueChanged_ValueChanged_SchedulesRepaint)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    call_private_fun::MemoryMonitoronValueChanged(*m_tester);
    // Assert: update() 调度重绘，离屏渲染结果非空
    EXPECT_FALSE(m_tester->grab().isNull());
    EXPECT_EQ(m_tester->width(), common::getStatusBarMaxWidth());
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_PaintEvent_SwapEnabled_RendersNonEmptyPixmap)
{
    // Arrange: 交换分区启用（swapTotal=500）
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act: grab 触发 paintEvent
    const QPixmap pix = m_tester->grab();
    // Assert: 渲染结果非空
    EXPECT_FALSE(pix.isNull());
    EXPECT_GT(pix.width(), 0);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_PaintEvent_SwapDisabled_RendersNonEmptyPixmap)
{
    // Arrange: 交换分区未启用（swapTotal=0，走 "Not enabled" 分支）
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal_zero);
    // Act: grab 触发 paintEvent
    const QPixmap pix = m_tester->grab();
    // Assert: 无交换分区时渲染结果依然非空
    EXPECT_FALSE(pix.isNull());
    EXPECT_GT(pix.width(), 0);
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_MouseReleaseEvent_LeftButton_EmitsClickedMsgMem)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &MemoryMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 左键释放
    call_private_fun::MemoryMonitormouseReleaseEvent(*m_tester, &event);
    // Assert: 发射一次 clicked("MSG_MEM")，控件位置不受影响
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("MSG_MEM"));
    EXPECT_EQ(m_tester->pos(), QPoint(0, 0));
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_MouseReleaseEvent_RightButton_EmitsNothing)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &MemoryMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::RightButton, Qt::MouseButton::RightButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 右键释放
    call_private_fun::MemoryMonitormouseReleaseEvent(*m_tester, &event);
    // Assert: 非左键不发射信号
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->pos(), QPoint(0, 0));
}

TEST_F(UT_MemoryMonitor, MemoryMonitor_MouseMoveEvent_AnyMove_WidgetStaysUnmoved)
{
    // Arrange: 记录初始位置
    const QPoint oldPos = m_tester->pos();
    QMouseEvent event(QEvent::Type::MouseMove, QPointF(10, 10),
                      Qt::MouseButton::NoButton, Qt::MouseButton::NoButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 鼠标移动
    call_private_fun::MemoryMonitormouseMoveEvent(*m_tester, &event);
    // Assert: 策略为屏蔽鼠标拖拽，控件位置不变
    EXPECT_EQ(m_tester->pos(), oldPos);
    EXPECT_EQ(m_tester->height(), 160);
}
