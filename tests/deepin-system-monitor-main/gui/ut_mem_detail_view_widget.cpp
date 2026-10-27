// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
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

protected:
    MemDetailViewWidget *m_tester;
    Stub m_memInfoStub;
};

// 构造：标题、子控件创建与初始数据填充均完成
TEST_F(UT_MemDetailViewWidget, initTest)
{
    // Act: SetUp 已构造
    // Assert
    ASSERT_NE(m_tester, nullptr);
    ASSERT_NE(m_tester->m_memstatWIdget, nullptr);
    ASSERT_NE(m_tester->m_memsummaryWidget, nullptr);
    EXPECT_EQ(m_tester->title(),
              QApplication::translate("Process.Graph.Title", "Memory"));
}

// 构造后父级 detail 属性已携带内存总量文本
TEST_F(UT_MemDetailViewWidget, test_onModelUpdate_initial)
{
    // Act: ctor 已调用 onModelUpdate
    // Assert
    QVariant detail = m_tester->property("detail");
    EXPECT_TRUE(detail.isValid());
    EXPECT_TRUE(detail.toString().contains(formatUnit_memory_disk(STUB_MEM_TOTAL << 10, B, 1)));
}

// onModelUpdate 应向两个子控件分发数据更新
TEST_F(UT_MemDetailViewWidget, test_onModelUpdate_01)
{
    // Act
    m_tester->onModelUpdate();

    // Assert: 子控件调用不崩溃，detail 属性保持有效
    EXPECT_TRUE(m_tester->property("detail").isValid());
    SUCCEED();
}

// detailFontChanged 应将字体传播到两个子控件
TEST_F(UT_MemDetailViewWidget, test_detailFontChanged_01)
{
    // Arrange
    QFont font;
    font.setBold(true);

    // Act
    m_tester->detailFontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_memstatWIdget->m_font, font);
    EXPECT_EQ(m_tester->m_memsummaryWidget->m_font, font);
}

// detailFontChanged：非加粗字体同样传播
TEST_F(UT_MemDetailViewWidget, test_detailFontChanged_02)
{
    // Arrange
    QFont font;
    font.setItalic(true);

    // Act
    m_tester->detailFontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_memstatWIdget->m_font, font);
    EXPECT_EQ(m_tester->m_memsummaryWidget->m_font, font);
}

// paintEvent：grab 渲染产出非空位图
TEST_F(UT_MemDetailViewWidget, test_paintEvent_01)
{
    // Act & Assert
    EXPECT_FALSE(m_tester->grab().isNull());
}

// paintEvent：字体变化后渲染仍正常
TEST_F(UT_MemDetailViewWidget, test_paintEvent_02)
{
    // Arrange
    QFont font;
    font.setPointSizeF(12.0);
    m_tester->detailFontChanged(font);

    // Act & Assert
    EXPECT_FALSE(m_tester->grab().isNull());
}

// resizeEvent 应触发布局更新（不崩溃）
TEST_F(UT_MemDetailViewWidget, test_resizeEvent_01)
{
    // Act
    m_tester->resize(500, 300);

    // Assert
    EXPECT_EQ(m_tester->width(), 500);
    EXPECT_FALSE(m_tester->grab().isNull());
}
