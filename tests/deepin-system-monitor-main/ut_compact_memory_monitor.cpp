// Copyright (C) 2019-2026 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_CompactMemoryMonitor：CompactMemoryMonitor 单元测试（qt-autotest-generator 重写）
// 覆盖：构造初始化、progress 属性、changeTheme 主题配色、changeFont 字体调整、
//       animationFinshed 占比计算、onStatInfoUpdated 动画启动、onValueChanged 重绘、
//       paintEvent 绘制（交换分区启用/未启用）、鼠标事件与信号发射
// 依赖注入：通过 Stub 替换 core::system::MemInfo，隔离真实内存数据源
//
// 用例计数声明：
// | 方法                         | 级别 | 因子                  | min | actual |
// |------------------------------|------|-----------------------|-----|--------|
// | CompactMemoryMonitor (ctor)  | mid  | in_degree:2           | 1   | 1      |
// | changeTheme                  | mid  | complexity:6          | 2   | 3      |
// | changeFont                   | mid  | in_degree:2           | 1   | 1      |
// | animationFinshed             | low  | -                     | 1   | 1      |
// | onStatInfoUpdated            | low  | -                     | 1   | 1      |
// | onValueChanged               | low  | -                     | 1   | 1      |
// | paintEvent                   | mid  | lines:124             | 1   | 2      |
// | progress/setProgress         | low  | -                     | 1   | 1      |
// | mouseReleaseEvent            | low  | -                     | 1   | 2      |
// | mouseMoveEvent               | low  | -                     | 1   | 1      |
// | ~CompactMemoryMonitor        | low  | destructor            | 1   | 14     |

//self
#include "compact_memory_monitor.h"
#include "system/mem.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

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

class UT_CompactMemoryMonitor: public ::testing::Test
{
public:
    UT_CompactMemoryMonitor() : m_tester(nullptr) {}
    QWidget *m_widget = nullptr;

public:
    virtual void SetUp()
    {
        m_widget = new QWidget();
        m_tester = new CompactMemoryMonitor(m_widget);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_widget) {
            delete m_widget;
            m_widget = nullptr;
        }
    }

protected:
    CompactMemoryMonitor *m_tester;
};

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_Ctor_DefaultSetup_FixedSizeAndAnimConfigured)
{
    // Arrange: 预期尺寸为状态栏宽度 x 150，动画时长 10ms
    const int expectedWidth = common::getStatusBarMaxWidth();
    const int expectedHeight = 150;
    // Act: fixture 已构造 CompactMemoryMonitor，读取控件当前状态
    const int actualWidth = m_tester->width();
    // Assert: 固定宽为状态栏宽度、高为 150，动画时长 10ms，进度初值 0
    EXPECT_EQ(actualWidth, expectedWidth);
    EXPECT_EQ(m_tester->height(), expectedHeight);
    EXPECT_EQ(m_tester->m_animation->duration(), 10);
    EXPECT_DOUBLE_EQ(m_tester->progress(), 0.0);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_SetProgress_AnyValue_PropertyRoundTrip)
{
    // Arrange
    const qreal expected = 0.5;
    // Act: 通过 Qt 属性系统写入 progress
    m_tester->setProperty("progress", expected);
    // Assert: 属性读写回路一致
    EXPECT_DOUBLE_EQ(m_tester->progress(), expected);
    EXPECT_DOUBLE_EQ(m_tester->property("progress").toDouble(), expected);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_ChangeTheme_LightType_BackgroundColorsTurnBlack)
{
    // Arrange: 预期亮色主题下内存/交换分区底色均为黑色
    const std::string expectedColor = "#000000";
    // Act: 亮色主题（int 形参，LightType=1）
    m_tester->changeTheme(1);
    // Assert: 亮色主题下内存/交换分区底色均为黑色
    EXPECT_EQ(m_tester->memoryBackgroundColor.name().toStdString(), std::string("#000000"));
    EXPECT_EQ(m_tester->swapBackgroundColor.name().toStdString(), std::string("#000000"));
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_ChangeTheme_DarkType_BackgroundColorsTurnWhite)
{
    // Arrange: 预期暗色主题下内存/交换分区底色均为白色
    const std::string expectedColor = "#ffffff";
    // Act: 暗色主题（int 形参，DarkType=2）
    m_tester->changeTheme(2);
    // Assert: 暗色主题下内存/交换分区底色均为白色
    EXPECT_EQ(m_tester->memoryBackgroundColor.name().toStdString(), expectedColor);
    EXPECT_EQ(m_tester->swapBackgroundColor.name().toStdString(), expectedColor);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_ChangeTheme_UnknownType_ThemeColorsUnchanged)
{
    // Arrange: 先置亮色，记录当前底色
    m_tester->changeTheme(1);
    const QColor oldMemBg = m_tester->memoryBackgroundColor;
    // Act: 未知主题（0 走 default 分支）
    m_tester->changeTheme(0);
    // Assert: 未知主题不改变底色
    EXPECT_EQ(m_tester->memoryBackgroundColor, oldMemBg);
    EXPECT_EQ(m_tester->memoryBackgroundColor.isValid(), true);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_ChangeFont_NormalFont_FontsAdjusted)
{
    // Arrange
    QFont font;
    font.setPointSizeF(10.0);
    // Act
    m_tester->changeFont(font);
    // Assert: 内容/次级内容字体缩 1pt 且内容字体 Medium，占比字体加粗
    EXPECT_EQ(m_tester->m_contentFont.weight(), QFont::Medium);
    EXPECT_NEAR(m_tester->m_contentFont.pointSizeF(), 9.0, 0.01);
    EXPECT_NEAR(m_tester->m_subContentFont.pointSizeF(), 9.0, 0.01);
    EXPECT_TRUE(m_tester->m_memPercentFont.bold());
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_AnimationFinshed_ValidMemInfo_LastPercentsComputed)
{
    // Arrange: 打桩内存信息 total=1000 avail=250 swap=500 free=250
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    m_tester->animationFinshed();
    // Assert: 内存占用 75%，交换分区占用 50%
    EXPECT_NEAR(m_tester->m_lastMemPercent, 0.75, 1e-6);
    EXPECT_NEAR(m_tester->m_lastSwapPercent, 0.5, 1e-6);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_OnStatInfoUpdated_StatUpdated_AnimationRunsToFinish)
{
    // Arrange: 打桩内存信息，监听动画状态
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    m_tester->onStatInfoUpdated();
    // Assert: 动画立即进入 Running，等待后自然结束并落盘最新占比
    EXPECT_EQ(m_tester->m_animation->state(), QAbstractAnimation::Running);
    QTest::qWait(60);
    EXPECT_EQ(m_tester->m_animation->state(), QAbstractAnimation::Stopped);
    EXPECT_NEAR(m_tester->m_lastMemPercent, 0.75, 1e-6);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_OnValueChanged_ValueChanged_SchedulesRepaint)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(core::system::MemInfo, memTotal), stub_memTotal);
    stub.set(ADDR(core::system::MemInfo, memAvailable), stub_memAvailable);
    stub.set(ADDR(core::system::MemInfo, swapTotal), stub_swapTotal);
    stub.set(ADDR(core::system::MemInfo, swapFree), stub_swapFree);
    // Act
    m_tester->onValueChanged();
    // Assert: update() 调度重绘，离屏渲染结果非空
    EXPECT_FALSE(m_tester->grab().isNull());
    EXPECT_EQ(m_tester->width(), common::getStatusBarMaxWidth());
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_PaintEvent_SwapEnabled_RendersNonEmptyPixmap)
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

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_PaintEvent_SwapDisabled_RendersNonEmptyPixmap)
{
    // Arrange: 交换分区未启用（swapTotal=0，走 "Not enabled" 分支且不画交换环）
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

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_MouseReleaseEvent_LeftButton_EmitsClickedMsgMem)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CompactMemoryMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 左键释放
    m_tester->mouseReleaseEvent(&event);
    // Assert: 发射一次 clicked("MSG_MEM")，控件位置不受影响
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("MSG_MEM"));
    EXPECT_EQ(m_tester->pos(), QPoint(0, 0));
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_MouseReleaseEvent_RightButton_EmitsNothing)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CompactMemoryMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::RightButton, Qt::MouseButton::RightButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 右键释放
    m_tester->mouseReleaseEvent(&event);
    // Assert: 非左键不发射信号
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->height(), 150);
}

TEST_F(UT_CompactMemoryMonitor, CompactMemoryMonitor_MouseMoveEvent_AnyMove_WidgetStaysUnmoved)
{
    // Arrange: 记录初始位置
    const QPoint oldPos = m_tester->pos();
    QMouseEvent event(QEvent::Type::MouseMove, QPointF(10, 10),
                      Qt::MouseButton::NoButton, Qt::MouseButton::NoButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 鼠标移动
    m_tester->mouseMoveEvent(&event);
    // Assert: 策略为屏蔽鼠标拖拽，控件位置不变
    EXPECT_EQ(m_tester->pos(), oldPos);
    EXPECT_EQ(m_tester->width(), common::getStatusBarMaxWidth());
}
