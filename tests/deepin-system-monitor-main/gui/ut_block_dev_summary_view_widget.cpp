// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "block_dev_summary_view_widget.h"
#include "system/block_device.h"
#include "system/block_device_info_db.h"
#include "common/common.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QStandardItemModel>
#include <DApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
#else
#include <DGuiApplicationHelper>
#endif

#include <memory>

using namespace core::system;
using namespace common::format;

/***************************************STUB begin*********************************************/
static void stub_readDeviceInfo_noop()
{
}

QList<BlockDevice> stub_deviceList_twoDevices()
{
    // 两台"虚拟"设备：名称合法但不存在于 /proc/diskstats，速率等保持初始值 0
    QList<BlockDevice> listDB;
    {
        Stub stub;
        stub.set(ADDR(BlockDevice, readDeviceInfo), stub_readDeviceInfo_noop);
        BlockDevice dev1;
        dev1.setDeviceName(QByteArray("ut-sm-disk-a"));
        listDB.append(dev1);

        BlockDevice dev2;
        dev2.setDeviceName(QByteArray("ut-sm-disk-b"));
        listDB.append(dev2);
    }
    return listDB;
}

QList<BlockDevice> stub_deviceList_empty()
{
    return QList<BlockDevice>();
}
/***************************************STUB end**********************************************/

class UT_BlockDevSummaryViewWidget : public ::testing::Test
{
public:
    UT_BlockDevSummaryViewWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 全程对 BlockDeviceInfoDB::deviceList 打桩，隔离真实 /proc/diskstats 读取
        m_deviceListStub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_twoDevices);
        m_tester = new BlockDevSummaryViewWidget();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDevSummaryViewWidget *m_tester;
    Stub m_deviceListStub;
};

// Arrange-Act-Assert：构造后模型挂接与基础尺寸应就绪
TEST_F(UT_BlockDevSummaryViewWidget, Ctor_DefaultState_Model7x2)
{
    // Arrange：SetUp 已完成打桩与构造
    ASSERT_NE(m_tester, nullptr);

    // Act：读取构造后的模型指针与尺寸
    QAbstractItemModel *model = m_tester->model();

    // Assert：模型挂接且固定 7 行 2 列
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->rowCount(), 7);
    EXPECT_EQ(model->columnCount(), 2);
}

// fontChanged 应更新 m_font、控件字体并固定高度 260
TEST_F(UT_BlockDevSummaryViewWidget, test_fontChanged_01)
{
    // Arrange
    QFont font;
    font.setBold(true);

    // Act
    m_tester->fontChanged(font);

    // Assert
    EXPECT_EQ(m_tester->m_font, font);
    EXPECT_EQ(m_tester->font().bold(), font.bold());
    EXPECT_EQ(m_tester->height(), 260);
}

// chageSummaryInfo 应设置内部模型当前设备名（非空名称走 setCurrentName 分支）
TEST_F(UT_BlockDevSummaryViewWidget, test_chageSummaryInfo_01)
{
    // Arrange
    const QString deviceName = "ut-sm-disk-a";

    // Act
    m_tester->chageSummaryInfo(deviceName);

    // Assert: 有效索引下数据为源码标题文本，说明当前设备名已生效
    QModelIndex index = m_tester->model()->index(0, 0);
    QVariant value = m_tester->model()->data(index, Qt::DisplayRole);
    EXPECT_TRUE(value.isValid());
    EXPECT_EQ(value.toString(), QApplication::translate("DeailTableModelBlock", "Read speed"));
}

// chageSummaryInfo 空名称触发 updateModel 的"自动选中首台设备"分支，数据恢复有效
TEST_F(UT_BlockDevSummaryViewWidget, test_chageSummaryInfo_02)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    QModelIndex index = m_tester->model()->index(0, 0);

    // Act
    m_tester->chageSummaryInfo(QString(""));

    // Assert: updateModel 在 currDeciveName 为空时自动选中首台设备，数据恢复为标题文本
    EXPECT_TRUE(m_tester->model()->data(index, Qt::DisplayRole).isValid());
    EXPECT_TRUE(m_tester->model()->data(index, Qt::UserRole).isValid());
    EXPECT_EQ(m_tester->model()->data(index, Qt::DisplayRole).toString(),
              QApplication::translate("DeailTableModelBlock", "Read speed"));
}

// paintEvent：grab 渲染不崩溃且产出非空位图
TEST_F(UT_BlockDevSummaryViewWidget, test_paintEvent_01)
{
    // Arrange：构造后的控件
    ASSERT_NE(m_tester, nullptr);

    // Act
    QPixmap pixmap = m_tester->grab();

    // Assert：渲染产出非空且与控件同尺寸的位图
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_EQ(pixmap.size(), m_tester->size());
}

// paintEvent：设置字体后渲染仍正常（覆盖字体相关绘制路径）
TEST_F(UT_BlockDevSummaryViewWidget, test_paintEvent_02)
{
    // Arrange：切换字体后渲染
    QFont font;
    font.setPointSizeF(12.0);
    m_tester->fontChanged(font);

    // Act
    QPixmap pixmap = m_tester->grab();

    // Assert：渲染产出非空且与控件同尺寸的位图
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_EQ(pixmap.size(), m_tester->size());
}

// 模型固定 7 行 2 列
TEST_F(UT_BlockDevSummaryViewWidget, test_rowCount_01)
{
    // Arrange：构造后的模型
    ASSERT_NE(m_tester->model(), nullptr);

    // Act：读取行数与末尾单元格有效性
    int rows = m_tester->model()->rowCount();

    // Assert：固定 7 行，末行末列索引有效
    EXPECT_EQ(rows, 7);
    EXPECT_EQ(m_tester->model()->index(rows - 1, 1).isValid(), true);
}

TEST_F(UT_BlockDevSummaryViewWidget, test_columnCount_01)
{
    // Arrange：构造后的模型
    ASSERT_NE(m_tester->model(), nullptr);

    // Act：读取列数与首行末列有效性
    int cols = m_tester->model()->columnCount();

    // Assert：固定 2 列，首行末列索引有效
    EXPECT_EQ(cols, 2);
    EXPECT_EQ(m_tester->model()->index(0, cols - 1).isValid(), true);
}

// DisplayRole：7x2 全矩阵标题文本与源码一致（经真实模型调用，非测试副本）
TEST_F(UT_BlockDevSummaryViewWidget, test_data_displayRole_matrix)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    struct Cell { int row; int col; const char *text; };
    const Cell cells[] = {
        {0, 0, "Read speed"},         {0, 1, "Write speed"},
        {1, 0, "Capacity"},           {1, 1, "Sectors read"},
        {2, 0, "Reads/s"},            {2, 1, "Sectors read/s"},
        {3, 0, "Reads merged/s"},     {3, 1, "Sectors written"},
        {4, 0, "Writes completed "},  {4, 1, "Writes merged"},
        {5, 0, "Writes/s"},           {5, 1, "Sectors written/s"},
        {6, 0, "Writes merged/s"},    {6, 1, ""},
    };

    // Act：逐单元格读取 DisplayRole 数据
    // Assert（前置守卫）：模型维度固定 7x2
    EXPECT_EQ(m_tester->model()->rowCount(), 7);
    EXPECT_EQ(m_tester->model()->columnCount(), 2);
    for (const Cell &c : cells) {
        QModelIndex index = m_tester->model()->index(c.row, c.col);
        QVariant value = m_tester->model()->data(index, Qt::DisplayRole);
        QString expected = QApplication::translate("DeailTableModelBlock", c.text);
        // Assert: 与源码翻译文本一致
        EXPECT_EQ(value.toString(), expected)
                << "cell(" << c.row << "," << c.col << ")";
    }
}

// DisplayRole：越界行应返回无效 QVariant
TEST_F(UT_BlockDevSummaryViewWidget, test_data_displayRole_outOfRange)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(7, 0), Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(value.toString(), QString());
}

// UserRole：formatUnit 单元格（速度/容量）在零值设备下可稳定求值
TEST_F(UT_BlockDevSummaryViewWidget, test_data_userRole_unitCells)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    struct Cell { int row; int col; };
    const Cell cells[] = { {0, 0}, {0, 1}, {1, 0} };

    // Act & Assert: 速率/容量均为 0，格式化结果一致且非空
    for (const Cell &c : cells) {
        QModelIndex index = m_tester->model()->index(c.row, c.col);
        QVariant value = m_tester->model()->data(index, Qt::UserRole);
        // Assert: UserRole 数据有效且等于零值 formatUnit 结果
        EXPECT_TRUE(value.isValid()) << "cell(" << c.row << "," << c.col << ")";
        EXPECT_EQ(value.toString(), formatUnit_memory_disk(static_cast<quint64>(0), B, 1))
                << "cell(" << c.row << "," << c.col << ")";
    }
}

// UserRole：计数值单元格在零值设备下均为 0
TEST_F(UT_BlockDevSummaryViewWidget, test_data_userRole_countCells)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    struct Cell { int row; int col; bool isReal; };
    const Cell cells[] = {
        {1, 1, false}, {2, 0, true}, {2, 1, true}, {3, 0, true},
        {3, 1, false}, {4, 0, false}, {4, 1, false}, {5, 0, true},
        {5, 1, true}, {6, 0, true},
    };

    // Act & Assert
    for (const Cell &c : cells) {
        QModelIndex index = m_tester->model()->index(c.row, c.col);
        QVariant value = m_tester->model()->data(index, Qt::UserRole);
        // Assert: 数据有效且数值单元格均为零值设备下的 0
        EXPECT_TRUE(value.isValid()) << "cell(" << c.row << "," << c.col << ")";
        if (c.isReal) {
            EXPECT_DOUBLE_EQ(value.toDouble(), 0.0) << "cell(" << c.row << "," << c.col << ")";
        } else {
            EXPECT_EQ(value.toULongLong(), 0ULL) << "cell(" << c.row << "," << c.col << ")";
        }
    }
}

// UserRole：row6 col1 返回空字符串分支
TEST_F(UT_BlockDevSummaryViewWidget, test_data_userRole_row6col1)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(6, 1), Qt::UserRole);

    // Assert
    EXPECT_TRUE(value.isValid());
    EXPECT_EQ(value.toString(), QString(""));
}

// 前景色分支返回调色板文本色
TEST_F(UT_BlockDevSummaryViewWidget, test_data_foregroundRole)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(0, 0),
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                             Qt::TextColorRole);
#else
                                             Qt::ForegroundRole);
#endif

    // Assert
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DPalette palette = DApplicationHelper::instance()->applicationPalette();
#else
    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
#endif
    EXPECT_TRUE(value.canConvert<QColor>());
    EXPECT_EQ(value.value<QColor>(), palette.color(DPalette::Text));
}

// 未处理角色返回无效 QVariant
TEST_F(UT_BlockDevSummaryViewWidget, test_data_unhandledRole)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));

    // Act
    QVariant value = m_tester->model()->data(m_tester->model()->index(0, 0), Qt::ToolTipRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(value.toString(), QString());
}

// 无效索引返回无效 QVariant（isValid 分支）
TEST_F(UT_BlockDevSummaryViewWidget, test_data_invalidIndex)
{
    // Arrange
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    QModelIndex invalidIndex;

    // Act：以无效索引读取两个角色
    QVariant displayValue = m_tester->model()->data(invalidIndex, Qt::DisplayRole);
    QVariant userValue = m_tester->model()->data(invalidIndex, Qt::UserRole);

    // Assert: 均返回无效 QVariant，与空 QVariant 相等
    EXPECT_EQ(displayValue, QVariant());
    EXPECT_EQ(userValue, QVariant());
}

// 设备列表为空时模型仍可用（updateModel 空表分支，不崩溃）
TEST_F(UT_BlockDevSummaryViewWidget, test_updateModel_emptyDeviceList)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(BlockDeviceInfoDB, deviceList), stub_deviceList_empty);

    // Act: 触发 updateModel —— 空 id 列表时 currDeciveName 保持原值
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));

    // Assert: 模型尺寸恒定，数据可安全访问
    EXPECT_EQ(m_tester->model()->rowCount(), 7);
    EXPECT_EQ(m_tester->model()->columnCount(), 2);
    EXPECT_FALSE(m_tester->grab().isNull());
}

// flags：有效索引恒为 NoItemFlags（只读表格）
TEST_F(UT_BlockDevSummaryViewWidget, test_flags_01)
{
    // Arrange
    QModelIndex index = m_tester->model()->index(0, 0);
    QModelIndex otherIndex = m_tester->model()->index(3, 1);

    // Act：读取两个不同索引的 flags
    Qt::ItemFlags flags00 = m_tester->model()->flags(index);
    Qt::ItemFlags flags31 = m_tester->model()->flags(otherIndex);

    // Assert：均为 NoItemFlags（只读表格）
    EXPECT_EQ(flags00, Qt::NoItemFlags);
    EXPECT_EQ(flags31, Qt::NoItemFlags);
}

// 合成越界列索引（外部模型索引）：switch 落空走 break 分支后返回无效 QVariant
TEST_F(UT_BlockDevSummaryViewWidget, test_data_outOfRangeColumn)
{
    // Arrange: data() 仅校验 isValid，不校验索引归属，可用外部模型构造越界列索引
    m_tester->chageSummaryInfo(QString("ut-sm-disk-a"));
    QStandardItemModel fakeModel;
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 3; ++col) {
            fakeModel.setItem(row, col, new QStandardItem());
        }
    }

    // Act & Assert: column==2 时 DisplayRole/UserRole 均落入 break 分支返回无效
    for (int row = 0; row < 7; ++row) {
        QModelIndex idx = fakeModel.index(row, 2);
        QVariant displayValue = m_tester->model()->data(idx, Qt::DisplayRole);
        QVariant userValue = m_tester->model()->data(idx, Qt::UserRole);
        // Assert: 两角色均无效且与空 QVariant 相等
        EXPECT_EQ(displayValue, QVariant()) << row;
        EXPECT_EQ(userValue, QVariant()) << row;
    }
}
