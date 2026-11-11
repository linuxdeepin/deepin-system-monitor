// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
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
#include <QStandardItemModel>
#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif

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

class UT_MemSummaryViewWidget : public ::testing::Test
{
public:
    UT_MemSummaryViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 对 MemInfo 的全部取值接口打桩，模型数据不再依赖真实 /proc/meminfo
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

        static QWidget parent;
        m_tester = new MemSummaryViewWidget(&parent);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MemSummaryViewWidget *m_tester;
    Stub m_memInfoStub;
};

// 构造：模型挂接、固定 7 行 2 列、高度固定 260
TEST_F(UT_MemSummaryViewWidget, Ctor_DefaultState_7Rows2ColsHeight260)
{
    // Arrange：SetUp 已完成构造与模型数据打桩
    ASSERT_NE(m_tester, nullptr);

    // Act: 读取构造后的模型与几何状态
    QAbstractItemModel *model = m_tester->model();

    // Assert
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->rowCount(), 7);
    EXPECT_EQ(model->columnCount(), 2);
    EXPECT_EQ(m_tester->height(), 260);
}

// fontChanged 应更新 m_font 与控件字体
TEST_F(UT_MemSummaryViewWidget, test_fontChanged_01)
{
    // Arrange
    QFont font;
    font.setItalic(true);

    // Act
    m_tester->fontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_TRUE(m_tester->font().italic());
}

// onModelUpdate 触发视口重绘：模型保持有效，渲染不崩溃
TEST_F(UT_MemSummaryViewWidget, test_onModelUpdate_01)
{
    // Arrange：模型有效且可正常渲染
    ASSERT_NE(m_tester->model(), nullptr);

    // Act：触发视口重绘
    m_tester->onModelUpdate();
    QPixmap grabbed = m_tester->grab();

    // Assert：重绘后仍可正常渲染，模型保持有效
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
    EXPECT_NE(m_tester->model(), nullptr);
}

// paintEvent：grab 渲染产出非空位图
TEST_F(UT_MemSummaryViewWidget, test_paintEvent_01)
{
    // Arrange：构造后未做额外配置
    ASSERT_NE(m_tester, nullptr);

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且尺寸与控件一致
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

// paintEvent：设置字体后渲染仍正常
TEST_F(UT_MemSummaryViewWidget, test_paintEvent_02)
{
    // Arrange：自定义控件字体
    QFont font;
    font.setPointSizeF(12.0);
    m_tester->fontChanged(font);

    // Act：离屏渲染整幅控件
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且字体已生效
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
    EXPECT_EQ(m_tester->font().pointSizeF(), 12.0);
}

// DisplayRole：7x2 全矩阵标题文本与源码一致（经真实内部模型）
TEST_F(UT_MemSummaryViewWidget, test_data_displayRole_matrix)
{
    // Arrange
    struct Cell { int row; int col; const char *text; bool hasCell; };
    const Cell cells[] = {
        {0, 0, "Used", true},          {0, 1, "Available", true},
        {1, 0, "Shared", true},        {1, 1, "Cached", true},
        {2, 0, "Buffers", true},       {2, 1, "Cached swap", true},
        {3, 0, "Active", true},        {3, 1, "Inactive", true},
        {4, 0, "Dirty", true},         {4, 1, "Mapped", true},
        {5, 0, "Total swap", true},    {5, 1, "Free swap", true},
        {6, 0, "Slab", true},          {6, 1, "", false},
    };

    // Act：在循环中逐格读取 DisplayRole 数据
    for (const Cell &c : cells) {
        QModelIndex index = m_tester->model()->index(c.row, c.col);
        QVariant value = m_tester->model()->data(index, Qt::DisplayRole);

        // Assert：文本与源码定义一致，无单元格位置返回无效数据
        if (c.hasCell) {
            EXPECT_EQ(value.toString(), QApplication::translate("DeailTableModel", c.text))
                    << "cell(" << c.row << "," << c.col << ")";
        } else {
            EXPECT_FALSE(value.isValid()) << "cell(" << c.row << "," << c.col << ")";
        }
    }
}

// DisplayRole：越界行返回无效 QVariant
TEST_F(UT_MemSummaryViewWidget, test_data_displayRole_outOfRange)
{
    // Arrange：行 7 超出模型的 7 行范围
    const int overRow = 7;

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(overRow, 0), Qt::DisplayRole);

    // Assert：返回无效 QVariant 且类型为 UnknownType
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(value.typeId(), int(QMetaType::UnknownType));
}

// UserRole：Used/Available 按桩值计算（(total-available)<<10 走 formatUnit）
TEST_F(UT_MemSummaryViewWidget, test_data_userRole_usedAvailable)
{
    // Arrange：取 Used 与 Available 两个单元格索引
    QModelIndex used = m_tester->model()->index(0, 0);
    QModelIndex avail = m_tester->model()->index(0, 1);

    // Act：读取 UserRole 格式化文本
    QString usedText = m_tester->model()->data(used, Qt::UserRole).toString();
    QString availText = m_tester->model()->data(avail, Qt::UserRole).toString();

    // Assert：与桩值推算的 formatUnit 结果一致
    EXPECT_EQ(usedText,
              formatUnit_memory_disk((STUB_MEM_TOTAL - STUB_MEM_AVAILABLE) << 10, B, 1));
    EXPECT_EQ(availText,
              formatUnit_memory_disk(STUB_MEM_AVAILABLE << 10, B, 1));
}

// UserRole：其余单元格与对应桩值一致
TEST_F(UT_MemSummaryViewWidget, test_data_userRole_otherCells)
{
    // Arrange：先确认矩阵规格，再逐格构造索引
    EXPECT_EQ(m_tester->model()->rowCount(), 7);

    // Act：在循环中逐格读取 UserRole 数据
    struct Cell { int row; int col; quint64 value; };
    const Cell cells[] = {
        {1, 0, STUB_MEM_SHMEM << 10},        {1, 1, STUB_MEM_CACHED << 10},
        {2, 0, STUB_MEM_BUFFERS << 10},      {2, 1, STUB_MEM_SWAP_CACHED << 10},
        {3, 0, STUB_MEM_ACTIVE << 10},       {3, 1, STUB_MEM_INACTIVE << 10},
        {4, 0, STUB_MEM_DIRTY << 10},        {4, 1, STUB_MEM_MAPPED << 10},
        {5, 0, STUB_MEM_SWAP_TOTAL << 10},   {5, 1, STUB_MEM_SWAP_FREE << 10},
        {6, 0, STUB_MEM_SLAB << 10},
    };
    for (const Cell &c : cells) {
        QModelIndex index = m_tester->model()->index(c.row, c.col);
        QVariant value = m_tester->model()->data(index, Qt::UserRole);

        // Assert：格式化文本与桩值推算一致
        EXPECT_EQ(value.toString(), formatUnit_memory_disk(c.value, B, 1))
                << "cell(" << c.row << "," << c.col << ")";
    }
}

// UserRole：row6 col1 无单元格（default 分支返回无效 QVariant）
TEST_F(UT_MemSummaryViewWidget, test_data_userRole_row6col1)
{
    // Arrange：行 6 列 1 在源码 switch 中无对应 case
    const int noCellRow = 6;
    const int noCellCol = 1;

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(noCellRow, noCellCol), Qt::UserRole);

    // Assert：返回无效 QVariant 且类型为 UnknownType
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(value.typeId(), int(QMetaType::UnknownType));
}

// 前景色分支返回调色板文本色
TEST_F(UT_MemSummaryViewWidget, test_data_foregroundRole)
{
    // Arrange：取源码使用的调色板文本色作为期望值
    const QColor expected =
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            DApplicationHelper::instance()->applicationPalette().color(DPalette::Text);
#else
            DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Text);
#endif

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(0, 0),
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                             Qt::TextColorRole);
#else
                                             Qt::ForegroundRole);
#endif

    // Assert：可转为颜色且与调色板文本色一致
    EXPECT_TRUE(value.canConvert<QColor>());
    EXPECT_EQ(value.value<QColor>(), expected);
}

// 无效索引返回无效 QVariant（isValid 分支）
TEST_F(UT_MemSummaryViewWidget, test_data_invalidIndex)
{
    // Arrange：构造无效索引
    QModelIndex invalidIndex;
    ASSERT_FALSE(invalidIndex.isValid());

    // Act：分别读取 DisplayRole 与 UserRole
    QVariant display = m_tester->model()->data(invalidIndex, Qt::DisplayRole);
    QVariant userRole = m_tester->model()->data(invalidIndex, Qt::UserRole);

    // Assert：两种角色均返回无效 QVariant
    EXPECT_FALSE(display.isValid());
    EXPECT_FALSE(userRole.isValid());
    EXPECT_EQ(display.typeId(), int(QMetaType::UnknownType));
}

// flags：有效索引恒为 NoItemFlags
TEST_F(UT_MemSummaryViewWidget, test_flags_01)
{
    // Arrange：取两个不同位置的索引
    QModelIndex idx00 = m_tester->model()->index(0, 0);
    QModelIndex idx11 = m_tester->model()->index(1, 1);

    // Act：读取两项 flags
    Qt::ItemFlags flags00 = m_tester->model()->flags(idx00);
    Qt::ItemFlags flags11 = m_tester->model()->flags(idx11);

    // Assert：任意有效索引均为 NoItemFlags（模型不可交互）
    EXPECT_EQ(flags00, Qt::NoItemFlags);
    EXPECT_EQ(flags11, Qt::NoItemFlags);
}

// 合成越界索引（外部模型索引）：列越界与行越界分别命中 break/default 分支
TEST_F(UT_MemSummaryViewWidget, test_data_outOfRangeIndex)
{
    // Arrange: data() 仅校验 isValid，不校验索引归属，可用外部模型构造越界索引
    QStandardItemModel fakeModel;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 3; ++col) {
            fakeModel.setItem(row, col, new QStandardItem());
        }
    }

    // Act：用外部模型索引逐格读取（column==2 命中各 case 的 break 分支）
    for (int row = 0; row < 7; ++row) {
        QModelIndex idx = fakeModel.index(row, 2);
        EXPECT_FALSE(m_tester->model()->data(idx, Qt::DisplayRole).isValid()) << row;
        EXPECT_FALSE(m_tester->model()->data(idx, Qt::UserRole).isValid()) << row;
    }

    // Act：row==7 命中 default 分支
    QModelIndex overRow = fakeModel.index(7, 0);

    // Assert：行列越界均返回无效 QVariant
    EXPECT_FALSE(m_tester->model()->data(overRow, Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->model()->data(overRow, Qt::UserRole).isValid());
}
