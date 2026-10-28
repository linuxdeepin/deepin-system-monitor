// Copyright (C) 2019-2026 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_CpuMonitor：CpuMonitor 单元测试（qt-autotest-generator 重写）
// 覆盖：构造初始化、progress 属性、明细按钮显隐、onDetailInfoClicked 信号、
//       changeTheme 主题配色、updateStatus 数据更新与 NaN 防护、changeFont 字体调整、
//       resizeEvent 重排、paintEvent 绘制、鼠标事件与信号发射
// 依赖注入：通过 Stub 替换 CPUInfoModel::cpuAllPercent，隔离真实 CPU 数据源
//
// 用例计数声明：
// | 方法                  | 级别 | 因子                                 | min | actual |
// |-----------------------|------|--------------------------------------|-----|--------|
// | CpuMonitor (ctor)     | low  | -                                    | 1   | 1      |
// | updateStatus          | high | complexity:6,transitive_loop_depth:4,alloc_in_loop:2 | 4 | 4 |
// | changeTheme           | mid  | complexity:6,in_degree:3             | 2   | 3      |
// | changeFont            | mid  | in_degree:2                          | 1   | 1      |
// | paintEvent            | mid  | lines:56                             | 1   | 1      |
// | resizeEvent           | low  | -                                    | 1   | 1      |
// | resizeItemWidgetRect  | mid  | in_degree:2                          | 1   | 2      |
// | setDetailButtonVisible| low  | -                                    | 1   | 1      |
// | onDetailInfoClicked   | low  | -                                    | 1   | 1      |
// | mouseReleaseEvent     | low  | -                                    | 1   | 2      |
// | mouseMoveEvent        | low  | -                                    | 1   | 1      |
// | progress/setProgress  | low  | -                                    | 1   | 1      |
// | ~CpuMonitor           | low  | destructor                           | 1   | 17     |

//self
#include "cpu_monitor.h"
#include "model/cpu_info_model.h"
#include "gui/base/base_commandlink_button.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <DApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QPropertyAnimation>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

#include <cmath>
#include <limits>

/***************************************STUB begin*********************************************/

double stub_cpuAllPercent_42()
{
    return 42.0;
}

double stub_cpuAllPercent_nan()
{
    return std::numeric_limits<double>::quiet_NaN();
}

static double g_stubCpuAllPercent = 0.0;
double stub_cpuAllPercent_dynamic()
{
    return g_stubCpuAllPercent;
}

/***************************************STUB end**********************************************/

class UT_CpuMonitor: public ::testing::Test
{
public:
    UT_CpuMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CpuMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    BaseCommandLinkButton *detailButton() const
    {
        return m_tester->findChild<BaseCommandLinkButton *>("CpuDetailButton");
    }

protected:
    CpuMonitor *m_tester;
};

TEST_F(UT_CpuMonitor, CpuMonitor_Ctor_DefaultSetup_FixedSizeAndInitCurve)
{
    // Arrange: 预期尺寸为状态栏宽度 x 240
    const int expectedWidth = common::getStatusBarMaxWidth();
    const int expectedHeight = 240;
    // Act: fixture 已构造 CpuMonitor，读取控件当前状态
    const int actualWidth = m_tester->width();
    // Assert: 固定尺寸为状态栏宽度 x 240，初始曲线已填充 pointsNumber 个 0，明细按钮已创建
    EXPECT_EQ(actualWidth, expectedWidth);
    EXPECT_EQ(m_tester->height(), expectedHeight);
    EXPECT_EQ(m_tester->cpuPercents->size(), m_tester->pointsNumber);
    EXPECT_DOUBLE_EQ(m_tester->progress(), 0.0);
    EXPECT_NE(detailButton(), nullptr);
}

TEST_F(UT_CpuMonitor, CpuMonitor_SetProgress_AnyValue_PropertyRoundTrip)
{
    // Arrange
    const qreal expected = 0.75;
    // Act: 通过 Qt 属性系统写入 progress
    m_tester->setProperty("progress", expected);
    // Assert: 属性读写回路一致
    EXPECT_DOUBLE_EQ(m_tester->progress(), expected);
    EXPECT_DOUBLE_EQ(m_tester->property("progress").toDouble(), expected);
}

TEST_F(UT_CpuMonitor, CpuMonitor_SetDetailButtonVisible_ToggleVisible_ButtonHiddenStateFollows)
{
    // Arrange
    BaseCommandLinkButton *btn = detailButton();
    ASSERT_NE(btn, nullptr);
    // Act: 依次隐藏、恢复显示
    m_tester->setDetailButtonVisible(false);
    EXPECT_EQ(btn->isHidden(), true);
    m_tester->setDetailButtonVisible(true);
    // Assert: 显隐状态跟随设置
    EXPECT_EQ(btn->isHidden(), false);
    EXPECT_EQ(btn->text().isEmpty(), false);
}

TEST_F(UT_CpuMonitor, CpuMonitor_OnDetailInfoClicked_ButtonClicked_EmitsSignalAndHidesButton)
{
    // Arrange: 监听明细信号
    QSignalSpy spy(m_tester, &CpuMonitor::signalDetailInfoClicked);
    ASSERT_TRUE(spy.isValid());
    // Act: 触发明细点击
    m_tester->onDetailInfoClicked();
    // Assert: 信号发射一次，明细按钮同时隐藏
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(detailButton()->isHidden());
}

TEST_F(UT_CpuMonitor, CpuMonitor_ChangeTheme_LightType_RingBackgroundTurnsBlack)
{
    // Arrange: 预期亮色主题环底色为黑色
    const std::string expectedColor = "#000000";
    // Act: 切换到亮色主题
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_tester->changeTheme(DApplicationHelper::LightType);
#else
    m_tester->changeTheme(DGuiApplicationHelper::ColorType::LightType);
#endif
    // Assert: 亮色主题下环底色为黑色
    EXPECT_EQ(m_tester->ringBackgroundColor.name().toStdString(), std::string("#000000"));
    EXPECT_EQ(m_tester->m_icon.isNull(), false);
}

TEST_F(UT_CpuMonitor, CpuMonitor_ChangeTheme_DarkType_RingBackgroundTurnsWhite)
{
    // Arrange: 预期暗色主题环底色为白色
    const std::string expectedColor = "#ffffff";
    // Act: 切换到暗色主题
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_tester->changeTheme(DApplicationHelper::DarkType);
#else
    m_tester->changeTheme(DGuiApplicationHelper::ColorType::DarkType);
#endif
    // Assert: 暗色主题下环底色为白色
    EXPECT_EQ(m_tester->ringBackgroundColor.name().toStdString(), expectedColor);
    EXPECT_EQ(m_tester->m_icon.isNull(), false);
}

TEST_F(UT_CpuMonitor, CpuMonitor_ChangeTheme_UnknownType_ThemeColorsUnchanged)
{
    // Arrange: 记录当前主题配色
    const QColor oldRingBg = m_tester->ringBackgroundColor;
    // Act
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_tester->changeTheme(DApplicationHelper::UnknownType);
#else
    m_tester->changeTheme(DGuiApplicationHelper::ColorType::UnknownType);
#endif
    // Assert: 未知主题走 default 分支，环底色保持不变
    EXPECT_EQ(m_tester->ringBackgroundColor, oldRingBg);
    EXPECT_EQ(m_tester->ringBackgroundColor.isValid(), true);
}

TEST_F(UT_CpuMonitor, CpuMonitor_UpdateStatus_NanPercent_KeepsCurveUnchanged)
{
    // Arrange: 记录当前曲线并打桩返回 NaN
    const int oldSize = m_tester->cpuPercents->size();
    const double oldValue = m_tester->cpuPercents->last();
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_nan);
    // Act
    m_tester->updateStatus();
    // Assert: NaN 数据被拒绝，曲线长度与末值均不变
    EXPECT_EQ(m_tester->cpuPercents->size(), oldSize);
    EXPECT_DOUBLE_EQ(m_tester->cpuPercents->last(), oldValue);
}

TEST_F(UT_CpuMonitor, CpuMonitor_UpdateStatus_ValidPercent_AppendsAndGeneratesSmoothCurve)
{
    // Arrange: 打桩返回 42.0
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    // Act: 连续更新两次
    m_tester->updateStatus();
    m_tester->updateStatus();
    // Assert: 最新采样已追加到曲线末尾，长度不超过 pointsNumber，平滑曲线路径已生成
    EXPECT_EQ(m_tester->cpuPercents->last(), 42.0);
    EXPECT_LE(m_tester->cpuPercents->size(), m_tester->pointsNumber);
    EXPECT_GT(m_tester->cpuPath.elementCount(), 0);
}

TEST_F(UT_CpuMonitor, CpuMonitor_UpdateStatus_RepeatedCalls_CurveSizeStaysAtPointsNumber)
{
    // Arrange: 打桩返回 42.0
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    // Act: 连续执行 pointsNumber + 5 次更新
    for (int i = 0; i < m_tester->pointsNumber + 5; ++i) {
        m_tester->updateStatus();
    }
    // Assert: 曲线长度始终收敛在 pointsNumber（append 后立即裁剪）
    EXPECT_EQ(m_tester->cpuPercents->size(), m_tester->pointsNumber);
    EXPECT_GE(m_tester->cpuPercents->size(), 1);
}

TEST_F(UT_CpuMonitor, CpuMonitor_UpdateStatus_NanThenValid_RecoversCurveGrowth)
{
    // Arrange: 打桩动态数据源，先注入 NaN 再切换为有效值
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_dynamic);
    g_stubCpuAllPercent = std::numeric_limits<double>::quiet_NaN();
    m_tester->updateStatus();
    const int sizeAfterNan = m_tester->cpuPercents->size();
    // Act: 切换回有效采样 42.0
    g_stubCpuAllPercent = 42.0;
    m_tester->updateStatus();
    // Assert: 曲线恢复采样（append 后按 pointsNumber 裁剪），末值为最新有效采样
    EXPECT_EQ(m_tester->cpuPercents->size(), sizeAfterNan);
    EXPECT_EQ(m_tester->cpuPercents->last(), 42.0);
}

TEST_F(UT_CpuMonitor, CpuMonitor_ChangeFont_MediumFont_FontsAdjusted)
{
    // Arrange
    QFont font;
    font.setPointSizeF(10.0);
    // Act
    m_tester->changeFont(font);
    // Assert: 使用字体加粗放大 3pt，展示字体放大 12pt，明细字体 Medium 且缩小 1pt
    EXPECT_TRUE(m_tester->m_cpuUsageFont.bold());
    EXPECT_NEAR(m_tester->m_cpuUsageFont.pointSizeF(), 13.0, 0.01);
    EXPECT_NEAR(m_tester->m_cpuDisplayFont.pointSizeF(), 22.0, 0.01);
    EXPECT_EQ(m_tester->m_detailFont.weight(), QFont::Medium);
    EXPECT_NEAR(m_tester->m_detailFont.pointSizeF(), 9.0, 0.01);
}

TEST_F(UT_CpuMonitor, CpuMonitor_ResizeEvent_WidgetResized_DetailButtonRecentered)
{
    // Arrange
    BaseCommandLinkButton *btn = detailButton();
    ASSERT_NE(btn, nullptr);
    QResizeEvent event(QSize(400, 300), QSize(100, 100));
    // Act
    m_tester->resizeEvent(&event);
    // Assert: 明细按钮保持水平居中（中心点位于 width/2 - 4）
    EXPECT_EQ(btn->geometry().center().x(), m_tester->width() / 2 - 4);
    EXPECT_GT(btn->width(), 0);
}

TEST_F(UT_CpuMonitor, CpuMonitor_PaintEvent_GrabRendering_RendersNonEmptyPixmap)
{
    // Arrange: 记录控件当前宽度作为渲染基准（paintEvent 会按内容重设高度，不新告宽高恒定）
    const int expectedWidth = m_tester->width();
    // Act: grab 触发 paintEvent 完成离屏渲染
    const QPixmap pix = m_tester->grab();
    // Assert: 渲染结果非空且宽度与控件一致
    EXPECT_FALSE(pix.isNull());
    EXPECT_EQ(pix.width(), expectedWidth);
    EXPECT_GT(m_tester->height(), 0);
}

TEST_F(UT_CpuMonitor, CpuMonitor_MouseReleaseEvent_LeftButton_EmitsClickedMsgCpu)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CpuMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::LeftButton, Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 左键释放
    m_tester->mouseReleaseEvent(&event);
    // Assert: 发射一次 clicked("MSG_CPU")，控件位置不受影响
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString("MSG_CPU"));
    EXPECT_EQ(m_tester->pos(), QPoint(0, 0));
}

TEST_F(UT_CpuMonitor, CpuMonitor_MouseReleaseEvent_RightButton_EmitsNothing)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CpuMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::RightButton, Qt::MouseButton::RightButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 右键释放
    m_tester->mouseReleaseEvent(&event);
    // Assert: 非左键不发射信号
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->pos(), QPoint(0, 0));
}

TEST_F(UT_CpuMonitor, CpuMonitor_MouseMoveEvent_AnyMove_WidgetStaysUnmoved)
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
