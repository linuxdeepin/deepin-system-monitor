// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
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

protected:
    MemStatViewWidget *m_tester;
    QWidget *m_parent = nullptr;
    Stub m_memInfoStub;
};

// 构造：内存/交换两张图表创建完成
TEST_F(UT_MemStatViewWidget, initTest)
{
    // Act: SetUp 已构造
    // Assert
    ASSERT_NE(m_tester, nullptr);
    QList<ChartViewWidget *> charts = m_tester->findChildren<ChartViewWidget *>();
    EXPECT_EQ(charts.size(), 2);
    ASSERT_NE(m_tester->m_memChartWidget, nullptr);
    ASSERT_NE(m_tester->m_swapChartWidget, nullptr);
}

// onModelUpdate：父级 detail 属性携带内存总量文本，双图各收到一份数据
TEST_F(UT_MemStatViewWidget, test_onModelUpdate_01)
{
    // Act
    m_tester->onModelUpdate();

    // Assert: parent()->setProperty("detail", ...) 生效
    QVariant detail = m_parent->property("detail");
    EXPECT_TRUE(detail.isValid());
    EXPECT_TRUE(detail.toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)));
}

// onModelUpdate：重复调用应持续追加数据（不崩溃）
TEST_F(UT_MemStatViewWidget, test_onModelUpdate_02)
{
    // Act
    m_tester->onModelUpdate();
    m_tester->onModelUpdate();

    // Assert
    EXPECT_TRUE(m_parent->property("detail").isValid());
}

// updateWidgetGeometry：swap 开启 -> 双图布局（宽度对半减 10）
TEST_F(UT_MemStatViewWidget, test_updateWidgetGeometry_01)
{
    // Arrange
    m_tester->resize(400, 200);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), false);
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400 / 2 - 10);
    EXPECT_EQ(m_tester->m_memChartWidget->height(), 200 - QFontMetrics(m_tester->m_font).height() / 2);
}

// updateWidgetGeometry：swap 关闭 -> 隐藏交换图表，内存图占满宽度
TEST_F(UT_MemStatViewWidget, test_updateWidgetGeometry_02)
{
    // Arrange
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);
    m_tester->resize(400, 200);

    // Act
    m_tester->updateWidgetGeometry();

    // Assert
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), true);
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400);
    EXPECT_EQ(m_tester->m_memChartWidget->height(), 200 - QFontMetrics(m_tester->m_font).height() / 2);
}

// resizeEvent 应联动 updateWidgetGeometry（swap 开启布局）
TEST_F(UT_MemStatViewWidget, test_resizeEvent_01)
{
    // Arrange
    QResizeEvent event(QSize(400, 200), QSize(100, 100));

    // Act
    m_tester->resizeEvent(&event);

    // Assert: 双图布局生效
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400 / 2 - 10);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), false);
}

// resizeEvent：swap 关闭时单图布局
TEST_F(UT_MemStatViewWidget, test_resizeEvent_02)
{
    // Arrange
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);
    QResizeEvent event(QSize(400, 200), QSize(100, 100));

    // Act
    m_tester->resizeEvent(&event);

    // Assert
    EXPECT_EQ(m_tester->m_memChartWidget->width(), 400);
    EXPECT_EQ(m_tester->m_swapChartWidget->isHidden(), true);
}

// fontChanged 应更新 m_font
TEST_F(UT_MemStatViewWidget, test_fontChanged_01)
{
    // Arrange
    QFont font;
    font.setItalic(true);

    // Act
    m_tester->fontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_font, font);
}

// paintEvent：swap 开启时绘制交换标题与图例
TEST_F(UT_MemStatViewWidget, test_paintEvent_01)
{
    // Act & Assert
    EXPECT_FALSE(m_tester->grab().isNull());
}

// paintEvent：swap 关闭时跳过交换绘制分支
TEST_F(UT_MemStatViewWidget, test_paintEvent_02)
{
    // Arrange
    m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal_swapOff);
    m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree_swapOff);

    // Act & Assert
    EXPECT_FALSE(m_tester->grab().isNull());
}

// paintEvent：字体变化后渲染正常
TEST_F(UT_MemStatViewWidget, test_paintEvent_03)
{
    // Arrange
    QFont font;
    font.setPointSizeF(11.0);
    m_tester->fontChanged(font);

    // Act & Assert
    EXPECT_FALSE(m_tester->grab().isNull());
}
