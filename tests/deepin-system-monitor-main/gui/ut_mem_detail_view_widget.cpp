// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "mem_detail_view_widget.h"
#include "mem_stat_view_widget.h"
#include "mem_summary_view_widget.h"
#include "system/mem.h"
#include "system/device_db.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QApplication>
#include <QImage>
#include <DApplication>

using namespace common::format;
using namespace core::system;

/***************************************STUB begin*********************************************/
// 固定的内存数值（KiB），隔离真实 /proc/meminfo 读取
static const quint64 STUB_MEM_TOTAL = 8000;
static const quint64 STUB_MEM_AVAILABLE = 3000;
static const quint64 STUB_MEM_SHMEM = 100;
static const quint64 STUB_MEM_CACHED = 200;
static const quint64 STUB_MEM_BUFFERS = 300;
static const quint64 STUB_MEM_SWAP_CACHED = 400;
static const quint64 STUB_MEM_ACTIVE = 500;
static const quint64 STUB_MEM_INACTIVE = 600;
static const quint64 STUB_MEM_DIRTY = 700;
static const quint64 STUB_MEM_MAPPED = 800;
static const quint64 STUB_MEM_SWAP_TOTAL = 900;
static const quint64 STUB_MEM_SWAP_FREE = 90;
static const quint64 STUB_MEM_SLAB = 1100;

static quint64 stub_mem_memTotal() { return STUB_MEM_TOTAL; }
static quint64 stub_mem_memAvailable() { return STUB_MEM_AVAILABLE; }
static quint64 stub_mem_shmem() { return STUB_MEM_SHMEM; }
static quint64 stub_mem_cached() { return STUB_MEM_CACHED; }
static quint64 stub_mem_buffers() { return STUB_MEM_BUFFERS; }
static quint64 stub_mem_swapCached() { return STUB_MEM_SWAP_CACHED; }
static quint64 stub_mem_active() { return STUB_MEM_ACTIVE; }
static quint64 stub_mem_inactive() { return STUB_MEM_INACTIVE; }
static quint64 stub_mem_dirty() { return STUB_MEM_DIRTY; }
static quint64 stub_mem_mapped() { return STUB_MEM_MAPPED; }
static quint64 stub_mem_swapTotal() { return STUB_MEM_SWAP_TOTAL; }
static quint64 stub_mem_swapFree() { return STUB_MEM_SWAP_FREE; }
static quint64 stub_mem_slab() { return STUB_MEM_SLAB; }
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

class UT_MemDetailViewWidget : public ::testing::Test
{
public:
    UT_MemDetailViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 对 MemInfo 的全部取值接口打桩，隔离真实 /proc/meminfo 读取
        m_memInfoStub.set(ADDR(MemInfo, memTotal), stub_mem_memTotal);
        m_memInfoStub.set(ADDR(MemInfo, memAvailable), stub_mem_memAvailable);
        m_memInfoStub.set(ADDR(MemInfo, shmem), stub_mem_shmem);
        m_memInfoStub.set(ADDR(MemInfo, cached), stub_mem_cached);
        m_memInfoStub.set(ADDR(MemInfo, buffers), stub_mem_buffers);
        m_memInfoStub.set(ADDR(MemInfo, swapCached), stub_mem_swapCached);
        m_memInfoStub.set(ADDR(MemInfo, active), stub_mem_active);
        m_memInfoStub.set(ADDR(MemInfo, inactive), stub_mem_inactive);
        m_memInfoStub.set(ADDR(MemInfo, dirty), stub_mem_dirty);
        m_memInfoStub.set(ADDR(MemInfo, mapped), stub_mem_mapped);
        m_memInfoStub.set(ADDR(MemInfo, swapTotal), stub_mem_swapTotal);
        m_memInfoStub.set(ADDR(MemInfo, swapFree), stub_mem_swapFree);
        m_memInfoStub.set(ADDR(MemInfo, slab), stub_mem_slab);

        m_tester = new MemDetailViewWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
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
    MemDetailViewWidget *m_tester;
    Stub m_memInfoStub;
};

// 构造：标题、子控件创建与初始数据填充均完成
TEST_F(UT_MemDetailViewWidget, Ctor_DefaultState_TitleAndSubViewsReady)
{
    // Arrange：默认构造（SetUp 完成）

    // Act：构造已完成（SetUp 完成）
    ASSERT_NE(m_tester, nullptr);

    // Assert：标题翻译 + 两个子控件创建完成
    EXPECT_NE(m_tester->m_memstatWIdget, nullptr);
    EXPECT_NE(m_tester->m_memsummaryWidget, nullptr);
    EXPECT_EQ(m_tester->title(),
              QApplication::translate("Process.Graph.Title", "Memory"));
    EXPECT_EQ(m_tester->m_memstatWIdget->parent(), m_tester);
}

// 构造后父级 detail 属性已携带内存总量文本
TEST_F(UT_MemDetailViewWidget, test_onModelUpdate_initial)
{
    // Arrange：默认构造（ctor 已调用 onModelUpdate）

    // Act：读取 detail 属性
    QVariant detail = m_tester->property("detail");

    // Assert：携带内存总量文本
    EXPECT_TRUE(detail.isValid());
    EXPECT_EQ(detail.toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)), true);
}

// onModelUpdate 应向两个子控件分发数据更新
TEST_F(UT_MemDetailViewWidget, test_onModelUpdate_01)
{
    // Arrange：默认构造（SetUp 完成）

    // Act：模型更新回调
    m_tester->onModelUpdate();

    // Assert: 子控件调用不崩溃，detail 属性保持有效
    EXPECT_TRUE(m_tester->property("detail").isValid());
    EXPECT_EQ(m_tester->property("detail").toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)), true);
}

// detailFontChanged 应将字体传播到两个子控件
TEST_F(UT_MemDetailViewWidget, test_detailFontChanged_01)
{
    // Arrange：加粗目标字体
    QFont font;
    font.setBold(true);

    // Act：字体变化转发
    m_tester->detailFontChanged(font);

    // Assert：字体传播到两个子控件
    EXPECT_EQ(m_tester->m_memstatWIdget->m_font, font);
    EXPECT_EQ(m_tester->m_memsummaryWidget->m_font, font);
}

// detailFontChanged：非加粗字体同样传播
TEST_F(UT_MemDetailViewWidget, test_detailFontChanged_02)
{
    // Arrange：斜体目标字体
    QFont font;
    font.setItalic(true);

    // Act：字体变化转发
    m_tester->detailFontChanged(font);

    // Assert：字体传播到两个子控件
    EXPECT_EQ(m_tester->m_memstatWIdget->m_font, font);
    EXPECT_EQ(m_tester->m_memsummaryWidget->m_font, font);
}

// paintEvent：渲染产出非空位图
TEST_F(UT_MemDetailViewWidget, test_paintEvent_01)
{
    // Arrange：记录渲染前尺寸（render 可能触发布局重算改尺寸）
    const QSize sizeBefore = m_tester->size();

    // Act：离屏渲染（paintEvent 经完整绘制链执行）
    QImage image = renderWidget();

    // Assert：标题与子控件内容已落在画布上，画布尺寸为渲染前尺寸
    EXPECT_GT(nonWhitePixels(image), 0);
    EXPECT_EQ(image.size(), sizeBefore);
}

// paintEvent：字体变化后渲染仍正常
TEST_F(UT_MemDetailViewWidget, test_paintEvent_02)
{
    // Arrange：字体变化
    QFont font;
    font.setPointSizeF(12.0);
    m_tester->detailFontChanged(font);

    // Act：离屏渲染
    QImage image = renderWidget();

    // Assert：字体已应用且绘制产物非空
    EXPECT_EQ(m_tester->m_memstatWIdget->m_font, font);
    EXPECT_GT(nonWhitePixels(image), 0);
}

// resizeEvent 应触发布局更新（不崩溃）
TEST_F(UT_MemDetailViewWidget, test_resizeEvent_01)
{
    // Arrange：尺寸变化
    const QSize kNewSize(500, 300);

    // Act：调整尺寸
    m_tester->resize(kNewSize);
    QImage image = renderWidget();

    // Assert：宽度生效（高度受布局最小高度约束不小于请求值）且渲染产物非空
    EXPECT_EQ(m_tester->width(), 500);
    EXPECT_GE(m_tester->height(), 300);
    EXPECT_GT(nonWhitePixels(image), 0);
}
