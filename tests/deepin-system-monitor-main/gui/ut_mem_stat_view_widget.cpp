// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "mem_stat_view_widget.h"
#include "system/mem.h"
#include "system/device_db.h"
#include "chart_view_widget.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QResizeEvent>
#include <QImage>

using namespace common::format;
using namespace core::system;

/***************************************STUB begin*********************************************/
// 固定的内存数值（KiB），隔离真实 /proc/meminfo 读取
static const quint64 STUB_MEM_TOTAL = 8000;
static const quint64 STUB_MEM_AVAILABLE = 3000;
static const quint64 STUB_MEM_SWAP_TOTAL = 900;
static const quint64 STUB_MEM_SWAP_FREE = 90;

static quint64 stub_mem_memTotal() { return STUB_MEM_TOTAL; }
static quint64 stub_mem_memAvailable() { return STUB_MEM_AVAILABLE; }
static quint64 stub_mem_swapTotal_swapOn() { return STUB_MEM_SWAP_TOTAL; }
static quint64 stub_mem_swapFree_swapOn() { return STUB_MEM_SWAP_FREE; }
static quint64 stub_mem_swapTotal_swapOff() { return 0; }
static quint64 stub_mem_swapFree_swapOff() { return 0; }
/***************************************STUB end**********************************************/

namespace {
// 统计非纯白像素数：paint 类用例用它证明确实绘制了内容
int nonWhitePixels(const QImage &image)
{
    int drawn = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixelColor(x, y) != Qt::white)
                ++drawn;
    return drawn;
}
}  // namespace

class UT_MemStatViewWidget : public ::testing::Test
{
public:
    UT_MemStatViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 对 MemInfo 关键取值打桩；默认开启 swap 便于覆盖双图布局
        m_memInfoStub.set(ADDR(MemInfo, memTotal), stub_mem_memTotal);
        m_memInfoStub.set(ADDR(MemInfo, memAvailable), stub_mem_memAvailable);
        m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOn);
        m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOn);

        m_parent = new QWidget();
        m_parent->resize(600, 400);
        m_tester = new MemStatViewWidget(m_parent);
        m_tester->resize(400, 200);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        if (m_parent) {
            delete m_parent;
            m_parent = nullptr;
        }
    }

    // 离屏渲染当前控件到白色画布
    QImage renderWidget()
    {
        QImage image(m_tester->size(), QImage::Format_ARGB32);
        image.fill(Qt::white);
        m_tester->render(&image);
        return image;
    }

protected:
    MemStatViewWidget *m_tester;
    QWidget *m_parent = nullptr;
    Stub m_memInfoStub;
};

// 构造：内存/交换两张图表创建完成
TEST_F(UT_MemStatViewWidget, Ctor_DefaultState_TwoChartsCreated)
{
    // Arrange：默认构造（SetUp 完成）
    QList<ChartViewWidget *> charts = m_tester->findChildren<ChartViewWidget *>();

    // Act：构造已完成（SetUp 完成）
    ASSERT_NE(m_tester, nullptr);

    // Assert：内存/交换两张图表创建完成
    EXPECT_EQ(charts.size(), 2);
    EXPECT_NE(m_tester->m_memChartWidget, nullptr);
    EXPECT_NE(m_tester->m_swapChartWidget, nullptr);
}

// onModelUpdate：父级 detail 属性携带内存总量文本，双图各收到一份数据
TEST_F(UT_MemStatViewWidget, test_onModelUpdate_01)
{
    // Arrange：默认构造（SetUp 完成）

    // Act：模型更新回调
    m_tester->onModelUpdate();

    // Assert: parent()->setProperty("detail", ...) 携带内存总量文本
    QVariant detail = m_parent->property("detail");
    EXPECT_TRUE(detail.isValid());
    EXPECT_EQ(detail.toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)), true);
}

// onModelUpdate：重复调用应持续追加数据（不崩溃）
TEST_F(UT_MemStatViewWidget, test_onModelUpdate_02_repeat)
{
    // Arrange：默认构造（SetUp 完成）

    // Act：连续两次模型更新回调
    m_tester->onModelUpdate();
    m_tester->onModelUpdate();

    // Assert：detail 属性持续有效
    EXPECT_TRUE(m_parent->property("detail").isValid());
    EXPECT_EQ(m_parent->property("detail").toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)), true);
}

// updateWidgetGeometry：swap 开启 -> 双图布局（宽度对半减 10）
TEST_F(UT_MemStatViewWidget, test_updateWidgetGeometry_01_swapOn)
{
    // Arrange：默认 swap 开启（SetUp 完成）
    m_tester->resize(400, 200);

    // Act：重算几何
    m_tester->updateWidgetGeometry();

    // Assert：双图布局（宽度对半减 10）
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), false);
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400 / 2 - 10);
    EXPECT_EQ(m_tester->m_memChartWidget->height(), 200 - QFontMetrics(m_tester->m_font).height() / 2);
}

// updateWidgetGeometry：swap 关闭 -> 隐藏交换图表，内存图占满宽度
TEST_F(UT_MemStatViewWidget, test_updateWidgetGeometry_02_swapOff)
{
    // Arrange：swap 关闭打桩
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);
    m_tester->resize(400, 200);

    // Act：重算几何
    m_tester->updateWidgetGeometry();

    // Assert：交换图表隐藏，内存图占满宽度
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), true);
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400);
    EXPECT_EQ(m_tester->m_memChartWidget->height(), 200 - QFontMetrics(m_tester->m_font).height() / 2);
}

// resizeEvent 应联动 updateWidgetGeometry（swap 开启布局）
TEST_F(UT_MemStatViewWidget, test_resizeEvent_01_swapOn)
{
    // Arrange：尺寸从 100x100 变为 400x200
    QResizeEvent event(QSize(400, 200), QSize(100, 100));

    // Act：触发尺寸变化回调
    m_tester->resizeEvent(&event);

    // Assert: 双图布局生效
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400 / 2 - 10);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), false);
}

// resizeEvent：swap 关闭时单图布局
TEST_F(UT_MemStatViewWidget, test_resizeEvent_02_swapOff)
{
    // Arrange：swap 关闭打桩 + 尺寸变化事件
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);
    QResizeEvent event(QSize(400, 200), QSize(100, 100));

    // Act：触发尺寸变化回调
    m_tester->resizeEvent(&event);

    // Assert：单图布局生效
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), true);
}

// fontChanged 应更新 m_font
TEST_F(UT_MemStatViewWidget, test_fontChanged_01)
{
    // Arrange：目标字体
    QFont font;
    font.setItalic(true);

    // Act：字体变化回调
    m_tester->fontChanged(font);

    // Assert：字体已更新
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->m_font.italic(), true);
}

// paintEvent：swap 开启时绘制交换标题与图例
TEST_F(UT_MemStatViewWidget, test_paintEvent_01_swapOn)
{
    // Arrange：注入图表数据（onModelUpdate）后离屏渲染
    m_tester->onModelUpdate();

    // Act：离屏渲染（paintEvent 经完整绘制链执行）
    QImage image = renderWidget();

    // Assert：标题与图例已落在画布上
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), false);
}

// paintEvent：swap 关闭时跳过交换绘制分支
TEST_F(UT_MemStatViewWidget, test_paintEvent_02_swapOff)
{
    // Arrange：swap 开启基线渲染
    m_tester->onModelUpdate();
    const QImage imageOn = renderWidget();

    // Act：关闭 swap 后重算布局并渲染
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);
    m_tester->updateWidgetGeometry();
    const QImage imageOff = renderWidget();

    // Assert：单图布局产物与双图基线不同
    EXPECT_GT(nonWhitePixels(imageOff), 0);
    EXPECT_FALSE(imageOff == imageOn);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), true);
}

// paintEvent：字体变化后渲染正常
TEST_F(UT_MemStatViewWidget, test_paintEvent_03_afterFontChange)
{
    // Arrange：目标字体
    QFont font;
    font.setPointSizeF(11.0);

    // Act：字体变化后离屏渲染
    m_tester->fontChanged(font);
    QImage image = renderWidget();

    // Assert：字体已应用且绘制产物非空
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_GT(nonWhitePixels(image), 0);
}
