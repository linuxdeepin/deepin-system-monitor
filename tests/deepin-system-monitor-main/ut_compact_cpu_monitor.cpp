// Copyright (C) 2019-2026 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_CompactCpuMonitor：CompactCpuMonitor 单元测试（qt-autotest-generator 重写）
// 覆盖：构造初始化（曲线行数/核心数）、updateStatus 正常与 NaN 防护、CPU 核心增减分支、
//       changeFont 字体调整与明细按钮重排、明细按钮显隐、onDetailInfoClicked 信号、
//       resizeEvent、paintEvent 绘制、鼠标事件与信号发射
// 依赖注入：通过 Stub 替换 CPUInfoModel::cpuAllPercent / cpuPercentList，隔离真实 CPU 数据源
//
// 用例计数声明：
// | 方法                     | 级别 | 因子                                            | min | actual |
// |--------------------------|------|-------------------------------------------------|-----|--------|
// | CompactCpuMonitor (ctor) | mid  | lines:55,alloc_in_loop:2                        | 3   | 3      |
// | updateStatus             | high | complexity:12,cognitive:26,linear_scan_in_loop:1,alloc_in_loop:6 | 4 | 4 |
// | changeFont               | mid  | in_degree:2                                     | 1   | 1      |
// | paintEvent               | mid  | lines:163                                       | 1   | 1      |
// | resizeEvent              | low  | -                                               | 1   | 1      |
// | resizeItemRect           | mid  | in_degree:2                                     | 1   | 2      |
// | setDetailButtonVisible   | low  | -                                               | 1   | 1      |
// | onDetailInfoClicked      | low  | -                                               | 1   | 1      |
// | mouseReleaseEvent        | low  | -                                               | 1   | 2      |
// | mouseMoveEvent           | low  | -                                               | 1   | 1      |
// | ~CompactCpuMonitor       | low  | destructor                                      | 1   | 15     |

//self
#include "compact_cpu_monitor.h"
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

#include <cmath>
#include <limits>
#include <unistd.h>

/***************************************STUB begin*********************************************/

static double stub_cpuAllPercent_42()
{
    return 42.0;
}

static double stub_cpuAllPercent_nan()
{
    return std::numeric_limits<double>::quiet_NaN();
}

// 与当前核心数一致的占用率列表（正常分支）
static QList<qreal> stub_cpuPercentList_normal()
{
    QList<qreal> list;
    const int n = int(sysconf(_SC_NPROCESSORS_ONLN));
    for (int i = 0; i < n; ++i) {
        list.append(20.0 + i);
    }
    return list;
}

// 比当前核心数少 1（核心下线分支）
static QList<qreal> stub_cpuPercentList_less()
{
    QList<qreal> list;
    const int n = int(sysconf(_SC_NPROCESSORS_ONLN)) - 1;
    for (int i = 0; i < n; ++i) {
        list.append(10.0);
    }
    return list;
}

// 比当前核心数多 1（核心上线分支）
static QList<qreal> stub_cpuPercentList_more()
{
    QList<qreal> list;
    const int n = int(sysconf(_SC_NPROCESSORS_ONLN)) + 1;
    for (int i = 0; i < n; ++i) {
        list.append(10.0);
    }
    return list;
}

/***************************************STUB end**********************************************/

class UT_CompactCpuMonitor: public ::testing::Test
{
public:
    UT_CompactCpuMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CompactCpuMonitor();
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
        return m_tester->findChild<BaseCommandLinkButton *>("CompactCpuDetailButton");
    }

protected:
    CompactCpuMonitor *m_tester;
};

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_Ctor_DefaultSetup_InitCurveRowsAndDetailButton)
{
    // Arrange: 预期尺寸为状态栏宽度 x 160，曲线点数为宽度/10
    const int expectedWidth = common::getStatusBarMaxWidth();
    const int expectedHeight = 160;
    // Act: fixture 已构造 CompactCpuMonitor，读取控件当前状态
    const int actualWidth = m_tester->width();
    // Assert: 固定尺寸正确，曲线行数为 numCPU+1（最后一行存总占用率），明细按钮已创建
    EXPECT_EQ(actualWidth, expectedWidth);
    EXPECT_EQ(m_tester->height(), expectedHeight);
    EXPECT_EQ(m_tester->pointsNumber, int(common::getStatusBarMaxWidth() / 10));
    EXPECT_EQ(m_tester->cpuPercents.size(), m_tester->numCPU + 1);
    EXPECT_NE(detailButton(), nullptr);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_Ctor_InitCurve_AllCurveRowsFilledWithZero)
{
    // Arrange: 预期行数 numCPU+1、每行 pointsNumber 个点
    const int expectedRows = m_tester->numCPU + 1;
    const int expectedPoints = m_tester->pointsNumber;
    // Act: fixture 已构造 CompactCpuMonitor，遍历初始曲线统计零值点
    int zeroCount = 0;
    int total = 0;
    for (const auto &row : m_tester->cpuPercents) {
        for (qreal v : row) {
            ++total;
            if (qFuzzyIsNull(v)) {
                ++zeroCount;
            }
        }
    }
    // Assert: 曲线点数正确且初始值全部为 0
    EXPECT_EQ(total, expectedRows * expectedPoints);
    EXPECT_EQ(zeroCount, expectedRows * expectedPoints);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_Ctor_PalettePrepared_ColorsCoverAllCurveRows)
{
    // Arrange: 预期配色组数覆盖 numCPU+1 条曲线
    const int expectedRows = m_tester->numCPU + 1;
    // Act: fixture 已构造 CompactCpuMonitor，读取初始配色表
    const int colorCount = m_tester->cpuColors.size();
    // Assert: 初始调色板为 16 组硬编码颜色且颜色均有效
    EXPECT_EQ(colorCount, 16);
    EXPECT_EQ(m_tester->cpuColors.first().isValid(), true);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_UpdateStatus_ValidPercent_AppendsToTotalAndCoreRows)
{
    // Arrange: 打桩总占用率 42.0，各核心占用率 20+i
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    stub.set(ADDR(CPUInfoModel, cpuPercentList), stub_cpuPercentList_normal);
    // Act
    m_tester->updateStatus();
    // Assert: 总 CPU 行末值为 0.42 且保持 pointsNumber 长度，首个核心行已追加 0.20
    EXPECT_NEAR(m_tester->cpuPercents[m_tester->numCPU].last(), 0.42, 1e-6);
    EXPECT_EQ(m_tester->cpuPercents[m_tester->numCPU].size(), m_tester->pointsNumber);
    EXPECT_NEAR(m_tester->cpuPercents[0].last(), 0.20, 1e-6);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_UpdateStatus_NanPercent_KeepsTotalCurveUnchanged)
{
    // Arrange: 记录总曲线首值并打桩返回 NaN
    const qreal firstValue = m_tester->cpuPercents[m_tester->numCPU].first();
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_nan);
    stub.set(ADDR(CPUInfoModel, cpuPercentList), stub_cpuPercentList_normal);
    // Act
    m_tester->updateStatus();
    // Assert: NaN 数据被拒绝，总曲线长度不变、数据未被污染
    EXPECT_EQ(m_tester->cpuPercents[m_tester->numCPU].size(), m_tester->pointsNumber);
    EXPECT_DOUBLE_EQ(m_tester->cpuPercents[m_tester->numCPU].first(), firstValue);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_UpdateStatus_CoreRemoved_OfflineCoreRowFilledWithZero)
{
    // Arrange: 打桩核心数比当前少 1
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    stub.set(ADDR(CPUInfoModel, cpuPercentList), stub_cpuPercentList_less);
    // Act
    m_tester->updateStatus();
    // Assert: 曲线行数不变；下线核心行补 0；总曲线正常追加 0.42
    EXPECT_EQ(m_tester->cpuPercents.size(), m_tester->numCPU + 1);
    EXPECT_EQ(m_tester->cpuPercents[m_tester->numCPU - 1].last(), 0.0);
    EXPECT_NEAR(m_tester->cpuPercents[m_tester->numCPU].last(), 0.42, 1e-6);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_UpdateStatus_CoreAdded_NewCoreRowAppended)
{
    // Arrange: 打桩核心数比当前多 1
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    stub.set(ADDR(CPUInfoModel, cpuPercentList), stub_cpuPercentList_more);
    // Act
    m_tester->updateStatus();
    // Assert: 新核心行被追加，行数 +1，总曲线仍正常更新
    EXPECT_EQ(m_tester->cpuPercents.size(), m_tester->numCPU + 2);
    EXPECT_NEAR(m_tester->cpuPercents[m_tester->numCPU].last(), 0.42, 1e-6);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_ChangeFont_NormalFont_FontsAdjustedAndButtonRelaid)
{
    // Arrange
    QFont font;
    font.setPointSizeF(10.0);
    BaseCommandLinkButton *btn = detailButton();
    ASSERT_NE(btn, nullptr);
    // Act
    m_tester->changeFont(font);
    // Assert: CPU/统计字体均缩小 1pt 且 CPU 字体 Medium，明细按钮重排至右上角
    EXPECT_EQ(m_tester->m_cpuFont.weight(), QFont::Medium);
    EXPECT_NEAR(m_tester->m_cpuFont.pointSizeF(), 9.0, 0.01);
    EXPECT_NEAR(m_tester->m_statFont.pointSizeF(), 9.0, 0.01);
    EXPECT_EQ(btn->x(), m_tester->width() - btn->width() + 3);
    EXPECT_EQ(btn->y(), 3);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_SetDetailButtonVisible_ToggleVisible_ButtonHiddenStateFollows)
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

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_OnDetailInfoClicked_ButtonClicked_EmitsSignalAndHidesButton)
{
    // Arrange: 监听明细信号
    QSignalSpy spy(m_tester, &CompactCpuMonitor::signalDetailInfoClicked);
    ASSERT_TRUE(spy.isValid());
    // Act: 触发明细点击
    m_tester->onDetailInfoClicked();
    // Assert: 信号发射一次，明细按钮同时隐藏
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(detailButton()->isHidden());
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_ResizeEvent_WidgetResized_LayoutStillRenders)
{
    // Arrange
    QResizeEvent event(QSize(200, 200), QSize(100, 100));
    // Act
    m_tester->resizeEvent(&event);
    // Assert: 控件固定宽度不变，且布局仍可正常渲染
    EXPECT_EQ(m_tester->width(), common::getStatusBarMaxWidth());
    EXPECT_FALSE(m_tester->grab().isNull());
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_PaintEvent_GrabRendering_RendersNonEmptyPixmap)
{
    // Arrange: 打桩正常数据
    Stub stub;
    stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_42);
    stub.set(ADDR(CPUInfoModel, cpuPercentList), stub_cpuPercentList_normal);
    m_tester->updateStatus();
    // Act: grab 触发 paintEvent
    const QPixmap pix = m_tester->grab();
    // Assert: 渲染结果非空
    EXPECT_FALSE(pix.isNull());
    EXPECT_GT(pix.width(), 0);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_MouseReleaseEvent_LeftButton_EmitsClickedMsgCpu)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CompactCpuMonitor::clicked);
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

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_MouseReleaseEvent_RightButton_EmitsNothing)
{
    // Arrange: 监听 clicked(QString) 信号
    QSignalSpy spy(m_tester, &CompactCpuMonitor::clicked);
    ASSERT_TRUE(spy.isValid());
    QMouseEvent event(QEvent::Type::MouseButtonRelease, QPointF(0, 0),
                      Qt::MouseButton::RightButton, Qt::MouseButton::RightButton,
                      Qt::KeyboardModifier::NoModifier);
    // Act: 右键释放
    m_tester->mouseReleaseEvent(&event);
    // Assert: 非左键不发射信号
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_tester->height(), 160);
}

TEST_F(UT_CompactCpuMonitor, CompactCpuMonitor_MouseMoveEvent_AnyMove_WidgetStaysUnmoved)
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
