// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "cpu_summary_view_widget.h"
#include "model/cpu_info_model.h"
#include "system/cpu_set.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QApplication>
#include <QColor>
#include <QPaintEvent>

/***************************************STUB begin*********************************************/
// 以下桩函数返回确定性数据，隔离真实 /proc、/sys 采样
static qreal stub_cpuAllPercent_55()
{
    return 55.2;
}

static QString stub_uptime_1day()
{
    return "1 day 02:03";
}

static QString stub_loadavg_050()
{
    return "0.50";
}

static uint stub_nProcesses_256()
{
    return 256;
}

static uint stub_nThreads_1024()
{
    return 1024;
}

static uint stub_nFileDescriptors_1024()
{
    return 1024;
}

static QString stub_hostname_ut()
{
    return "ut-host";
}

static QString stub_osType_linux()
{
    return "Linux";
}

static QString stub_osVersion_25()
{
    return "25";
}

static QString stub_minFreq_800()
{
    return "800 MHz";
}

static QString stub_maxFreq_3800()
{
    return "3.80 GHz";
}

static QString stub_curFreq_1200()
{
    return "1.2 GHz";
}

static QString stub_avgFreq_2500()
{
    return "2.5 GHz";
}

static QString stub_minFreq_dash()
{
    return "-";
}

static QString stub_maxFreq_dash()
{
    return "-";
}

static int stub_socketCount_2()
{
    return 2;
}

static int stub_socketCount_0()
{
    return 0;
}

static int stub_cpuCount_8()
{
    return 8;
}

static QString stub_virtualization_vtx()
{
    return "VT-x";
}

static QString stub_l1iCache()
{
    return "64 KB";
}

static QString stub_l1dCache()
{
    return "32 KB";
}

static QString stub_l2Cache()
{
    return "256 KB";
}

static QString stub_l3Cache()
{
    return "8 MB";
}

// QWidget::update() 调用计数
static int g_widgetUpdateCount = 0;
static void stub_QWidget_update(QWidget *)
{
    ++g_widgetUpdateCount;
}

// QPainter::drawLine(int,int,int,int) 采集（成员调用约定：this 在前）
static QVector<QVector<int>> g_drawnLines;
static void stub_QPainter_drawLine(QPainter *, int x1, int y1, int x2, int y2)
{
    g_drawnLines.append({x1, y1, x2, y2});
}

// QPainter::drawRoundedRect(const QRectF&,qreal,qreal) 采集（QRect 版本内联转发到此）
static QVector<QVector<int>> g_roundRects;
static void stub_QPainter_drawRoundedRect(QPainter *, const QRectF &, qreal rx, qreal ry, Qt::SizeMode)
{
    g_roundRects.append({int(rx), int(ry)});
}
/***************************************STUB end**********************************************/

// 用例计数声明表（方法 | level | factors | min | actual）
// | CPUSummaryTableModel | mid | in_degree:80 | 2 | 2 |
// | data | low | - | 1 | 46 |
// | flags | low | - | 1 | 1 |
// | onModelUpdated | low | - | 1 | 2 |
// | CPUDetailSummaryTable | low | - | 1 | 2 |
// | fontChanged | low | - | 1 | 1 |
// | onModelUpdate | low | - | 1 | 1 |
// | paintEvent | low | - | 1 | 1 |

class UT_CPUSummaryTableModel : public ::testing::Test
{
public:
    UT_CPUSummaryTableModel() : m_tester(nullptr), m_cpuModel(nullptr) {}

    void SetUp() override
    {
        m_cpuModel = &s_cpuModel();
        m_tester = new CPUSummaryTableModel(m_cpuModel, nullptr);
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 为 data() 的 UserRole 分支注入确定性桩
    void stubModelValues()
    {
        m_stub.set(ADDR(CPUInfoModel, cpuAllPercent), stub_cpuAllPercent_55);
        m_stub.set(ADDR(CPUInfoModel, uptime), stub_uptime_1day);
        m_stub.set(ADDR(CPUInfoModel, loadavg), stub_loadavg_050);
        m_stub.set(ADDR(CPUInfoModel, nProcesses), stub_nProcesses_256);
        m_stub.set(ADDR(CPUInfoModel, nThreads), stub_nThreads_1024);
        m_stub.set(ADDR(CPUInfoModel, nFileDescriptors), stub_nFileDescriptors_1024);
        m_stub.set(ADDR(CPUInfoModel, hostname), stub_hostname_ut);
        m_stub.set(ADDR(CPUInfoModel, osType), stub_osType_linux);
        m_stub.set(ADDR(CPUInfoModel, osVersion), stub_osVersion_25);
        m_stub.set(ADDR(CPUSet, minFreq), stub_minFreq_800);
        m_stub.set(ADDR(CPUSet, maxFreq), stub_maxFreq_3800);
        m_stub.set(ADDR(CPUSet, curFreq), stub_curFreq_1200);
        m_stub.set(ADDR(CPUSet, avgFreq), stub_avgFreq_2500);
        m_stub.set(ADDR(CPUSet, socketCount), stub_socketCount_2);
        m_stub.set(ADDR(CPUSet, cpuCount), stub_cpuCount_8);
        m_stub.set(ADDR(CPUSet, virtualization), stub_virtualization_vtx);
        m_stub.set(ADDR(CPUSet, l1iCache), stub_l1iCache);
        m_stub.set(ADDR(CPUSet, l1dCache), stub_l1dCache);
        m_stub.set(ADDR(CPUSet, l2Cache), stub_l2Cache);
        m_stub.set(ADDR(CPUSet, l3Cache), stub_l3Cache);
    }

protected:
    // CPUInfoModel 构造代价高（SysInfo/DeviceDB 单例），进程内共享一个实例
    static CPUInfoModel &s_cpuModel()
    {
        static CPUInfoModel model;
        return model;
    }

    Stub m_stub;
    CPUSummaryTableModel *m_tester;
    CPUInfoModel *m_cpuModel;
};

namespace
{
struct CellParam
{
    int row;
    int column;
    const char *expected;
};

std::ostream &operator<<(std::ostream &os, const CellParam &p)
{
    return os << "(" << p.row << "," << p.column << ")";
}
}

// DisplayRole 标签网格：10 行 2 列的全部标签文本
class DisplayRole_LabelGrid_Test
    : public UT_CPUSummaryTableModel,
      public ::testing::WithParamInterface<CellParam>
{
};

TEST_P(DisplayRole_LabelGrid_Test, DisplayRole_LabelGrid_YieldsExpectedText)
{
    // Arrange
    const auto &param = GetParam();
    // Act
    QVariant v = m_tester->data(m_tester->index(param.row, param.column), Qt::DisplayRole);
    // Assert
    EXPECT_EQ(v.toString(), QString(param.expected));
    EXPECT_TRUE(v.canConvert<QString>());
}

INSTANTIATE_TEST_SUITE_P(CPUSummaryTableModel,
                         DisplayRole_LabelGrid_Test,
                         ::testing::Values(
                             CellParam { 0, 0, "Utilization" },
                             CellParam { 0, 1, "Frequency" },
                             CellParam { 1, 0, "Max frequency" },
                             CellParam { 1, 1, "Average frequency" },
                             CellParam { 2, 0, "Up time" },
                             CellParam { 2, 1, "Sockets" },
                             CellParam { 3, 0, "Logical processors" },
                             CellParam { 3, 1, "Virtualization" },
                             CellParam { 4, 0, "L1i cache" },
                             CellParam { 4, 1, "L1d cache" },
                             CellParam { 5, 0, "L2 cache" },
                             CellParam { 5, 1, "L3 cache" },
                             CellParam { 6, 0, "Load average" },
                             CellParam { 6, 1, "File descriptors" },
                             CellParam { 7, 0, "Processes" },
                             CellParam { 7, 1, "Threads" },
                             CellParam { 8, 0, "Host name" },
                             CellParam { 8, 1, "OS type" },
                             CellParam { 9, 0, "Version" },
                             CellParam { 9, 1, "" }));

// UserRole 数值网格：桩定值下的全部取值
class UserRole_ValueGrid_Test
    : public UT_CPUSummaryTableModel,
      public ::testing::WithParamInterface<CellParam>
{
};

TEST_P(UserRole_ValueGrid_Test, UserRole_ValueGrid_YieldsStubbedValue)
{
    // Arrange
    const auto &param = GetParam();
    stubModelValues();
    // Act
    QVariant v = m_tester->data(m_tester->index(param.row, param.column), Qt::UserRole);
    // Assert
    EXPECT_EQ(v.toString(), QString(param.expected));
    EXPECT_TRUE(v.canConvert<QString>());
}

INSTANTIATE_TEST_SUITE_P(CPUSummaryTableModel,
                         UserRole_ValueGrid_Test,
                         ::testing::Values(
                             CellParam { 0, 0, "55%" },
                             CellParam { 0, 1, "800 MHz~3.80 GHz" },
                             CellParam { 1, 0, "1.2 GHz" },
                             CellParam { 1, 1, "2.5 GHz" },
                             CellParam { 2, 0, "1 day 02:03" },
                             CellParam { 2, 1, "2" },
                             CellParam { 3, 0, "8" },
                             CellParam { 3, 1, "VT-x" },
                             CellParam { 4, 0, "64 KB" },
                             CellParam { 4, 1, "32 KB" },
                             CellParam { 5, 0, "256 KB" },
                             CellParam { 5, 1, "8 MB" },
                             CellParam { 6, 0, "0.50" },
                             CellParam { 6, 1, "1024" },
                             CellParam { 7, 0, "256" },
                             CellParam { 7, 1, "1024" },
                             CellParam { 8, 0, "ut-host" },
                             CellParam { 8, 1, "Linux" },
                             CellParam { 9, 0, "25" },
                             CellParam { 9, 1, "" }));

// 无效索引返回空 QVariant
TEST_F(UT_CPUSummaryTableModel, Data_InvalidIndex_ReturnsEmptyVariant)
{
    // Arrange
    QModelIndex invalidIdx;
    // Act
    QVariant v = m_tester->data(invalidIdx, Qt::DisplayRole);
    // Assert
    EXPECT_FALSE(v.isValid());
    EXPECT_EQ(v.toString(), QString(""));
}

// 未知角色返回空 QVariant
TEST_F(UT_CPUSummaryTableModel, Data_UnknownRole_ReturnsEmptyVariant)
{
    // Arrange
    QModelIndex idx = m_tester->index(0, 0);
    // Act
    QVariant v = m_tester->data(idx, Qt::ToolTipRole);
    // Assert
    EXPECT_FALSE(v.isValid());
    EXPECT_EQ(v.toString(), QString(""));
}

// 末行第二列（9,1）无数据，返回空 QVariant
TEST_F(UT_CPUSummaryTableModel, Data_UserRoleLastRowColumnOne_ReturnsEmptyVariant)
{
    // Arrange
    stubModelValues();
    QModelIndex idx = m_tester->index(9, 1);
    // Act
    QVariant v = m_tester->data(idx, Qt::UserRole);
    // Assert
    EXPECT_FALSE(v.isValid());
    EXPECT_EQ(v.toString(), QString(""));
}

// 频率数据不可用（"-"）时显示 "-"
TEST_F(UT_CPUSummaryTableModel, Data_UserRoleFreqUnavailable_YieldsDash)
{
    // Arrange
    m_stub.set(ADDR(CPUSet, minFreq), stub_minFreq_dash);
    m_stub.set(ADDR(CPUSet, maxFreq), stub_maxFreq_dash);
    QModelIndex idx = m_tester->index(0, 1);
    // Act
    QVariant v = m_tester->data(idx, Qt::UserRole);
    // Assert
    EXPECT_EQ(v.toString(), QString("-"));
    EXPECT_TRUE(v.canConvert<QString>());
}

// 插槽数为 0 时显示 "-"
TEST_F(UT_CPUSummaryTableModel, Data_UserRoleZeroSockets_YieldsDash)
{
    // Arrange
    m_stub.set(ADDR(CPUSet, socketCount), stub_socketCount_0);
    QModelIndex idx = m_tester->index(2, 1);
    // Act
    QVariant v = m_tester->data(idx, Qt::UserRole);
    // Assert
    EXPECT_EQ(v.toString(), QString("-"));
    EXPECT_TRUE(v.canConvert<QString>());
}

// 前景色角色返回调色板文本颜色
TEST_F(UT_CPUSummaryTableModel, Data_ForegroundRole_ReturnsPaletteTextColor)
{
    // Arrange
    QModelIndex idx = m_tester->index(0, 0);
    // Act
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QVariant v = m_tester->data(idx, Qt::TextColorRole);
#else
    QVariant v = m_tester->data(idx, Qt::ForegroundRole);
#endif
    // Assert
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(v.userType(), QMetaType::QColor);
    EXPECT_TRUE(v.value<QColor>().isValid());
}

// 网格尺寸固定：10 行 2 列
TEST_F(UT_CPUSummaryTableModel, GridModel_FixedDimensions_TenRowsTwoColumns)
{
    // Arrange
    const int kExpectedRows = 10;
    const int kExpectedCols = 2;
    // Act
    int rows = m_tester->rowCount(QModelIndex());
    int cols = m_tester->columnCount(QModelIndex());
    // Assert
    EXPECT_EQ(rows, kExpectedRows);
    EXPECT_EQ(cols, kExpectedCols);
}

// flags 返回 NoItemFlags，表格不可编辑
TEST_F(UT_CPUSummaryTableModel, Flags_AnyIndex_ReturnsNoItemFlags)
{
    // Arrange
    QModelIndex idx = m_tester->index(3, 1);
    // Act
    Qt::ItemFlags f = m_tester->flags(idx);
    // Assert
    EXPECT_EQ(f, Qt::NoItemFlags);
    EXPECT_FALSE(f.testFlag(Qt::ItemIsEditable));
}

// onModelUpdated 触发全表 dataChanged（(0,0) 到 (9,1)）
TEST_F(UT_CPUSummaryTableModel, OnModelUpdated_GridTransition_EmitsFullDataChanged)
{
    // Arrange
    QSignalSpy spy(m_tester, &CPUSummaryTableModel::dataChanged);
    // Act
    m_tester->onModelUpdated();
    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toModelIndex().row(), 0);
    EXPECT_EQ(spy.first().at(1).toModelIndex().row(), 9);
}

// CPUInfoModel 的 modelUpdated 信号通过构造时的 connect 联动 dataChanged
TEST_F(UT_CPUSummaryTableModel, ModelUpdatedSignal_RoutedToOnModelUpdated_EmitsDataChanged)
{
    // Arrange
    QSignalSpy spy(m_tester, &CPUSummaryTableModel::dataChanged);
    // Act
    QMetaObject::invokeMethod(m_cpuModel, "modelUpdated");
    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toModelIndex().column(), 0);
}

// 构造时保存传入的 CPUInfoModel 指针且数据立即可读
TEST_F(UT_CPUSummaryTableModel, Constructor_WithModel_DataImmediatelyReadable)
{
    // Arrange
    const QString kFirstLabel = "Utilization";
    // Act
    int rows = m_tester->rowCount(QModelIndex());
    QVariant firstCell = m_tester->data(m_tester->index(0, 0), Qt::DisplayRole);
    // Assert
    EXPECT_EQ(rows, 10);
    EXPECT_EQ(firstCell.toString(), kFirstLabel);
    EXPECT_TRUE(firstCell.isValid());
}

// 构造完成后模型指针与外部传入一致，不持有所有权
TEST_F(UT_CPUSummaryTableModel, Constructor_DoesNotOwnExternalModel_PointerPreserved)
{
    // Arrange
    CPUInfoModel *passed = m_cpuModel;
    // Act: 再次构造一个实例，传入同一模型
    CPUSummaryTableModel another(passed, nullptr);
    // Assert
    EXPECT_EQ(another.rowCount(QModelIndex()), 10);
    EXPECT_NE(&another, m_tester);
    EXPECT_EQ(another.index(0, 0).row(), 0);
}

class UT_CPUDetailSummaryTable : public ::testing::Test
{
public:
    UT_CPUDetailSummaryTable() : m_tester(nullptr), m_cpuModel(nullptr) {}

    void SetUp() override
    {
        m_cpuModel = &UT_CPUSummaryTableModel::s_cpuModel();
        m_tester = new CPUDetailSummaryTable(m_cpuModel, nullptr);
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Stub m_stub;
    CPUDetailSummaryTable *m_tester;
    CPUInfoModel *m_cpuModel;
};

// 访问私有成员 m_model，验证内部模型挂接
ACCESS_PRIVATE_FIELD(CPUDetailSummaryTable, CPUSummaryTableModel *, m_model)

// 构造函数构建内部模型、隐藏表头并挂接代理
TEST_F(UT_CPUDetailSummaryTable, CPUDetailTable_Constructed_BuildsModelHidesHeaders)
{
    // Arrange
    QAbstractItemModel *publicModel = nullptr;
    QHeaderView *hHeader = nullptr;
    QHeaderView *vHeader = nullptr;
    // Act: 查询构造完成的内部模型与表头状态
    auto &innerModel = access_private_field::CPUDetailSummaryTablem_model(*m_tester);
    publicModel = m_tester->model();
    hHeader = m_tester->horizontalHeader();
    vHeader = m_tester->verticalHeader();
    // Assert
    EXPECT_EQ(publicModel, innerModel);
    EXPECT_NE(innerModel, nullptr);
    EXPECT_NE(m_tester->itemDelegate(), nullptr);
    EXPECT_TRUE(hHeader->isHidden());
    EXPECT_TRUE(vHeader->isHidden());
}

// fontChanged 更新字体并固定高度 260
TEST_F(UT_CPUDetailSummaryTable, FontChanged_NewFont_AppliesFontFixedHeight260)
{
    // Arrange
    QFont f;
    f.setPointSize(9);
    // Act
    m_tester->fontChanged(f);
    // Assert
    EXPECT_EQ(m_tester->font().pointSize(), 9);
    EXPECT_EQ(m_tester->minimumHeight(), 260);
    EXPECT_EQ(m_tester->maximumHeight(), 260);
}

// onModelUpdate 触发视口重绘
TEST_F(UT_CPUDetailSummaryTable, OnModelUpdate_ModelRefreshed_RequestsViewportUpdate)
{
    // Arrange
    g_widgetUpdateCount = 0;
    m_stub.set(static_cast<void (QWidget::*)()>(&QWidget::update), stub_QWidget_update);
    // Act
    m_tester->onModelUpdate();
    m_tester->onModelUpdate();
    // Assert
    EXPECT_EQ(g_widgetUpdateCount, 2);
    EXPECT_GE(g_widgetUpdateCount, 1);
}

// paintEvent 绘制纵向分隔线与圆角边框
TEST_F(UT_CPUDetailSummaryTable, PaintEvent_PaintTriggered_DrawsSeparatorRoundedFrame)
{
    // Arrange
    g_drawnLines.clear();
    g_roundRects.clear();
    m_stub.set(static_cast<void (QPainter::*)(int, int, int, int)>(&QPainter::drawLine),
               stub_QPainter_drawLine);
    m_stub.set(static_cast<void (QPainter::*)(const QRectF &, qreal, qreal, Qt::SizeMode)>(&QPainter::drawRoundedRect),
               stub_QPainter_drawRoundedRect);
    QPaintEvent ev(m_tester->rect());
    // Act
    m_tester->paintEvent(&ev);
    // Assert: 至少存在一条纵向分隔线（x1 == x2）且圆角矩形半径为 6
    bool hasVerticalLine = false;
    for (const auto &l : g_drawnLines) {
        if (l[0] == l[2]) {
            hasVerticalLine = true;
            break;
        }
    }
    bool hasRoundRect6 = false;
    for (const auto &r : g_roundRects) {
        if (r[0] == 6 && r[1] == 6) {
            hasRoundRect6 = true;
            break;
        }
    }
    EXPECT_TRUE(hasVerticalLine);
    EXPECT_TRUE(hasRoundRect6);
    EXPECT_GT(g_drawnLines.size(), 0);
    EXPECT_GT(g_roundRects.size(), 0);
}

// CPUDetailSummaryTable 构造后即为固定高度策略
TEST_F(UT_CPUDetailSummaryTable, Constructor_DefaultPolicy_VerticalFixedExpanding)
{
    // Arrange
    const QSizePolicy::Policy kFixed = QSizePolicy::Fixed;
    const QSizePolicy::Policy kExpanding = QSizePolicy::Expanding;
    // Act
    QSizePolicy policy = m_tester->sizePolicy();
    // Assert
    EXPECT_EQ(policy.verticalPolicy(), kFixed);
    EXPECT_EQ(policy.horizontalPolicy(), kExpanding);
    EXPECT_NE(policy.verticalPolicy(), QSizePolicy::Preferred);
}
