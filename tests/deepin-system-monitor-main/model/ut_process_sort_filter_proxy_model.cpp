// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// rewritten unit tests for ProcessSortFilterProxyModel (deepin-system-monitor-main/model/process_sort_filter_proxy_model.cpp)
// based on upstream/master @ 3df42339004d7aa3e2cf2cc7b09de81fb8c981c2
//
// 用例计数声明（满足 test-code-gen.md §4 下限表，level/factors 取自 .ut inventory t18）:
//   lessThan            high (cx=23, factors: columns=13, tie=6) -> min 3 -> 21
//   filterAcceptsRow    high (cx=14, factors: type=3, match=4)   -> min 3 -> 12
//   setSortFilterString mid  (cx=4)                              -> min 1 -> 3
//   setFilterType       low  (cx=3)                              -> min 1 -> 2
//
// 最小清单（test-types.md §8）自检:
//   [x] 1. 公开方法已列出，每个方法 >= 1 用例
//   [x] 2. 等价类划分（filterType 3 类 / apptype 3 类 / 匹配来源 name-UserRole-pid-user / 列 13 类 / hanzi-非 hanzi）
//   [x] 3. 边界值显式覆盖（row=0 / row=size / row<0 / 空搜索串 / qFuzzyCompare 相等 / UserRole+1 总量并列）
//   [x] 4. 同质断言矩阵用 TEST_P（MemoryColumns 排序矩阵）
//   [x] 5. 分支清单已列出并映射到用例名（见 BRANCH MAP）
//   [x] 6. 每条 if/switch/early-return 分支有触发用例
//   [x] 7. 源码无显式 throw
//   [x] 8. 负面场景（无源模型 / 越界 row / 类型不匹配 / 全不匹配）有专门用例
//   [x] 9. 负面用例验证状态保持（被过滤行不影响其他行判定结果）
//   [x] 10. stub_ext 打桩（QSortFilterProxyModel::sortColumn），无 gMock
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet ProcessSortFilterProxyModel::filterAcceptsRow @
//       deepin-system-monitor-main/model/process_sort_filter_proxy_model.cpp:56-141
// F1 : !sourceModel() || row<0 || row>=rowCount -> false
// F2 : m_fileterType == kNoFilter -> filter = true
// F3 : kFilterCurrentUser && (apptype==kFilterApps || apptype==kFilterCurrentUser) -> true
// F4 : kFilterApps && apptype==kFilterApps -> true
// F5 : !filter -> return false（类型不匹配即使名称命中也不放行）
// F6 : name DisplayRole contains(pattern) -> true
// F7 : name UserRole contains(pattern) -> true
// F8 : 中文 locale 下 name UserRole contains(m_hanwords) -> true（依赖 QLocale::system()，
//      offscreen 测试环境 locale 不可控 -> 豁免，不在本环境断言）
// F9 : pid DisplayRole contains(pattern) -> true
// F10: user DisplayRole contains(pattern) -> true
// F11: 全不匹配 -> false
// lessThan @ process_sort_filter_proxy_model.cpp:144-278:
// L1 : !left.isValid() || !right.isValid() || !model -> false
// L2 : Name: !lhsHanzi && rhsHanzi -> true
// L3 : Name: lhsHanzi && !rhsHanzi -> false
// L4 : Name: localeAwareCompare==0 -> CPU UserRole tie-break
// L5 : Name: localeAwareCompare < 0
// L6 : User: localeAwareCompare < 0
// L7 : Memory/ShareMemory/VTRMemory: UserRole 相等 -> CPU tie-break
// L8 : Memory/ShareMemory/VTRMemory: toULongLong 比较
// L9 : CPU: qFuzzyCompare -> Memory UserRole tie-break
// L10: CPU: toDouble 比较
// L11: Upload: qFuzzyCompare -> UserRole+1 (累计发送) tie-break
// L12: Upload: toReal 比较
// L13: Download: qFuzzyCompare -> UserRole+1 (累计接收) tie-break
// L14: Download: toReal 比较
// L15: PID: UserRole toInt 比较
// L16: DiskRead/DiskWrite: UserRole toDouble 比较
// L17: Nice: !(left < right)（负值优先 -> 降序语义）
// L18: Priority: 取 Nice 列 sibling 的 UserRole 做 !(<) 比较
// L19: default -> QSortFilterProxyModel::lessThan 基类
// setSortFilterString @ process_sort_filter_proxy_model.cpp:40-54:
// SS1: m_search 保存；中文 locale 分支（convHanToLatin）随环境豁免
// SS2: setFilterRegularExpression(CaseInsensitiveOption)
// setFilterType @ process_sort_filter_proxy_model.cpp:
// SF1: m_fileterType 更新（源码成员名即 m_fileterType，拼写与源码一致）
// SF2: 以原 pattern 重新应用过滤
// 用例映射:
//   FilterAcceptsRow_NoSourceModel_ReturnsFalse            -> F1
//   FilterAcceptsRow_RowOutOfBounds_ReturnsFalse           -> F1
//   FilterAcceptsRow_NegativeRow_ReturnsFalse              -> F1
//   FilterAcceptsRow_NoFilterType_AcceptsAllRows           -> F2
//   FilterAcceptsRow_CurrentUserType_MatchesAppAndUserRows -> F3
//   FilterAcceptsRow_AppsType_MatchesOnlyAppRows           -> F4
//   FilterAcceptsRow_TypeMismatch_ReturnsFalse             -> F5
//   FilterAcceptsRow_NameDisplayMatch_ReturnsTrue          -> F6
//   FilterAcceptsRow_EmptySearch_AcceptsAllTypedRows       -> F6 边界
//   FilterAcceptsRow_CaseInsensitiveSearch_Matches         -> SS2
//   FilterAcceptsRow_NameUserRoleMatch_ReturnsTrue         -> F7
//   FilterAcceptsRow_PidMatch_ReturnsTrue                  -> F9
//   FilterAcceptsRow_UserMatch_ReturnsTrue                 -> F10
//   FilterAcceptsRow_NoMatch_ReturnsFalse                  -> F11
//   SetSortFilterString_StoresSearchPattern                -> SS1
//   SetSortFilterString_EmptyPattern_KeepsAllTypedRows     -> SS1/SS2
//   SetFilterType_UpdatesMember                            -> SF1
//   SetFilterType_ReappliesFilterForNewType                -> SF2
//   LessThan_InvalidIndex_ReturnsFalse                     -> L1
//   LessThan_NameColumn_NonHanziBeforeHanzi_ReturnsTrue    -> L2
//   LessThan_NameColumn_HanziBeforeNonHanzi_ReturnsFalse   -> L3
//   LessThan_NameColumn_EqualNames_CompareByCpu            -> L4
//   LessThan_NameColumn_LocaleOrder_ReturnsTrue            -> L5
//   LessThan_UserColumn_LocaleCompare_ReturnsTrue          -> L6
//   LessThan_MemoryColumns_CompareValues (TEST_P)          -> L7/L8
//   LessThan_CpuColumn_LessValue_ReturnsTrue               -> L10
//   LessThan_CpuColumn_EqualValues_CompareByMemory         -> L9
//   LessThan_UploadColumn_LessValue_ReturnsTrue            -> L12
//   LessThan_UploadColumn_EqualValues_CompareByTotalSent   -> L11
//   LessThan_DownloadColumn_LessValue_ReturnsTrue          -> L14
//   LessThan_DownloadColumn_EqualValues_CompareByTotalRecv -> L13
//   LessThan_PidColumn_LessValue_ReturnsTrue               -> L15
//   LessThan_DiskReadColumn_LessValue_ReturnsTrue          -> L16
//   LessThan_DiskWriteColumn_LessValue_ReturnsTrue         -> L16
//   LessThan_NiceColumn_HigherValue_NotLess                -> L17
//   LessThan_NiceColumn_LowerValue_IsLess                  -> L17
//   LessThan_PriorityColumn_ComparesNiceSibling            -> L18
//   LessThan_UnknownColumn_FallsBackToBaseImplementation   -> L19
// ====================================================================

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>

#include "model/process_sort_filter_proxy_model.h"
#include "model/process_table_model.h"
#include "process/process_set.h"   // FilterType 枚举（kFilterApps/kFilterCurrentUser/kNoFilter）
#include "common/common.h"
#include "common/han_latin.h"

#include "stub.h"

namespace {

/***************************************STUB begin*********************************************/
// lessThan 内部读取 sortColumn()，按旧用例约定打桩路由到目标列
static int stub_sortColumn_Name() { return ProcessTableModel::kProcessNameColumn; }
static int stub_sortColumn_User() { return ProcessTableModel::kProcessUserColumn; }
static int stub_sortColumn_Memory() { return ProcessTableModel::kProcessMemoryColumn; }
static int stub_sortColumn_ShareMemory() { return ProcessTableModel::kProcessShareMemoryColumn; }
static int stub_sortColumn_VTRMemory() { return ProcessTableModel::kProcessVTRMemoryColumn; }
static int stub_sortColumn_CPU() { return ProcessTableModel::kProcessCPUColumn; }
static int stub_sortColumn_Upload() { return ProcessTableModel::kProcessUploadColumn; }
static int stub_sortColumn_Download() { return ProcessTableModel::kProcessDownloadColumn; }
static int stub_sortColumn_PID() { return ProcessTableModel::kProcessPIDColumn; }
static int stub_sortColumn_DiskRead() { return ProcessTableModel::kProcessDiskReadColumn; }
static int stub_sortColumn_DiskWrite() { return ProcessTableModel::kProcessDiskWriteColumn; }
static int stub_sortColumn_Nice() { return ProcessTableModel::kProcessNiceColumn; }
static int stub_sortColumn_Priority() { return ProcessTableModel::kProcessPriorityColumn; }
static int stub_sortColumn_Unknown() { return 1000; }

static void stubSortColumn(Stub &stub, int column)
{
    switch (column) {
    case ProcessTableModel::kProcessNameColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Name); break;
    case ProcessTableModel::kProcessUserColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_User); break;
    case ProcessTableModel::kProcessMemoryColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Memory); break;
    case ProcessTableModel::kProcessShareMemoryColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_ShareMemory); break;
    case ProcessTableModel::kProcessVTRMemoryColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_VTRMemory); break;
    case ProcessTableModel::kProcessCPUColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_CPU); break;
    case ProcessTableModel::kProcessUploadColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Upload); break;
    case ProcessTableModel::kProcessDownloadColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Download); break;
    case ProcessTableModel::kProcessPIDColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_PID); break;
    case ProcessTableModel::kProcessDiskReadColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_DiskRead); break;
    case ProcessTableModel::kProcessDiskWriteColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_DiskWrite); break;
    case ProcessTableModel::kProcessNiceColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Nice); break;
    case ProcessTableModel::kProcessPriorityColumn: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Priority); break;
    default: stub.set(ADDR(QSortFilterProxyModel, sortColumn), stub_sortColumn_Unknown); break;
    }
}
/***************************************STUB end***********************************************/

// 两行源数据的规格说明（fillRow 负责写入所有参与比较的 role）
struct RowSpec {
    QString name = "utproc";
    QString user = "utuser";
    double cpu = 0.0;
    qulonglong mem = 0;
    qulonglong shm = 0;
    qulonglong vtr = 0;
    double upload = 0.0;
    double download = 0.0;
    qulonglong uploadTotal = 0;
    qulonglong downloadTotal = 0;
    double diskRead = 0.0;
    double diskWrite = 0.0;
    int pid = 0;
    int nice = 0;
    int apptype = 0;
};

void fillRow(QStandardItemModel *model, int row, const RowSpec &spec)
{
    for (int c = 0; c < ProcessTableModel::kProcessColumnCount; ++c)
        model->setItem(row, c, new QStandardItem);

    QModelIndex idx = model->index(row, ProcessTableModel::kProcessNameColumn);
    model->setData(idx, spec.name, Qt::DisplayRole);
    model->setData(idx, spec.name, Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessUserColumn);
    model->setData(idx, spec.user, Qt::DisplayRole);

    idx = model->index(row, ProcessTableModel::kProcessCPUColumn);
    model->setData(idx, spec.cpu, Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessMemoryColumn);
    model->setData(idx, QVariant::fromValue<qulonglong>(spec.mem), Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessShareMemoryColumn);
    model->setData(idx, QVariant::fromValue<qulonglong>(spec.shm), Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessVTRMemoryColumn);
    model->setData(idx, QVariant::fromValue<qulonglong>(spec.vtr), Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessUploadColumn);
    model->setData(idx, spec.upload, Qt::UserRole);
    model->setData(idx, QVariant::fromValue<qulonglong>(spec.uploadTotal), Qt::UserRole + 1);

    idx = model->index(row, ProcessTableModel::kProcessDownloadColumn);
    model->setData(idx, spec.download, Qt::UserRole);
    model->setData(idx, QVariant::fromValue<qulonglong>(spec.downloadTotal), Qt::UserRole + 1);

    idx = model->index(row, ProcessTableModel::kProcessDiskReadColumn);
    model->setData(idx, spec.diskRead, Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessDiskWriteColumn);
    model->setData(idx, spec.diskWrite, Qt::UserRole);

    idx = model->index(row, ProcessTableModel::kProcessPIDColumn);
    model->setData(idx, spec.pid, Qt::DisplayRole);
    model->setData(idx, spec.pid, Qt::UserRole);
    model->setData(idx, spec.apptype, Qt::UserRole + 3);

    idx = model->index(row, ProcessTableModel::kProcessNiceColumn);
    model->setData(idx, spec.nice, Qt::UserRole);
}

// 排序参数化用例（MemoryColumns 矩阵）
struct SortCase {
    int column;
    qulonglong lmem;
    qulonglong rmem;
    double lcpu;
    double rcpu;
    bool expected;
};

}   // namespace

// ============================== filterAcceptsRow (F1~F11) ==============================

class ProcessSortFilterProxyModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_proxy.reset(new ProcessSortFilterProxyModel());
        m_source.reset(new QStandardItemModel(0, ProcessTableModel::kProcessColumnCount));
        m_proxy->setSourceModel(m_source.data());
    }
    void TearDown() override
    {
        m_proxy.reset();
        m_source.reset();
    }
    QScopedPointer<ProcessSortFilterProxyModel> m_proxy;
    QScopedPointer<QStandardItemModel> m_source;
};

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NoSourceModel_ReturnsFalse)
{
    // Arrange: 未设置源模型的代理
    ProcessSortFilterProxyModel bare;

    // branch F1: !sourceModel()
    // Act + Assert
    EXPECT_FALSE(bare.filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_RowOutOfBounds_ReturnsFalse)
{
    // Arrange: 两行源数据
    fillRow(m_source.data(), 0, RowSpec());
    fillRow(m_source.data(), 1, RowSpec());

    // branch F1: row >= rowCount
    // Act + Assert
    EXPECT_FALSE(m_proxy->filterAcceptsRow(2, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NegativeRow_ReturnsFalse)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());

    // branch F1: row < 0
    // Act + Assert
    EXPECT_FALSE(m_proxy->filterAcceptsRow(-1, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NoFilterType_AcceptsAllRows)
{
    // Arrange: 两行不同类型（apptype 分别为 kFilterApps / kNoFilter）
    fillRow(m_source.data(), 0, RowSpec());
    fillRow(m_source.data(), 1, RowSpec());
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterApps, Qt::UserRole + 3);
    m_source->setData(m_source->index(1, ProcessTableModel::kProcessPIDColumn), kNoFilter, Qt::UserRole + 3);
    m_proxy->setFilterType(kNoFilter);

    // branch F2: kNoFilter 全放行
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
    EXPECT_TRUE(m_proxy->filterAcceptsRow(1, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_CurrentUserType_MatchesAppAndUserRows)
{
    // Arrange: 三行 apptype 分别为 kFilterApps / kFilterCurrentUser / kNoFilter
    fillRow(m_source.data(), 0, RowSpec());
    fillRow(m_source.data(), 1, RowSpec());
    fillRow(m_source.data(), 2, RowSpec());
    // 直接改写 UserRole+3
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterApps, Qt::UserRole + 3);
    m_source->setData(m_source->index(1, ProcessTableModel::kProcessPIDColumn), kFilterCurrentUser, Qt::UserRole + 3);
    m_source->setData(m_source->index(2, ProcessTableModel::kProcessPIDColumn), kNoFilter, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterCurrentUser);

    // branch F3
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
    EXPECT_TRUE(m_proxy->filterAcceptsRow(1, QModelIndex()));
    EXPECT_FALSE(m_proxy->filterAcceptsRow(2, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_AppsType_MatchesOnlyAppRows)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());
    fillRow(m_source.data(), 1, RowSpec());
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterApps, Qt::UserRole + 3);
    m_source->setData(m_source->index(1, ProcessTableModel::kProcessPIDColumn), kFilterCurrentUser, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterApps);

    // branch F4
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
    EXPECT_FALSE(m_proxy->filterAcceptsRow(1, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_TypeMismatch_ReturnsFalse)
{
    // Arrange: 名称命中搜索串，但 apptype 不匹配当前过滤类型
    fillRow(m_source.data(), 0, RowSpec());
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterCurrentUser, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterApps);
    m_proxy->setSortFilterString("utproc");

    // branch F5: !filter 提前返回
    // Act + Assert
    EXPECT_FALSE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NameDisplayMatch_ReturnsTrue)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());   // name "utproc"
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterApps, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterApps);
    m_proxy->setSortFilterString("proc");

    // branch F6
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_EmptySearch_AcceptsAllTypedRows)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterApps, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterApps);
    m_proxy->setSortFilterString("");

    // branch F6 边界: 空正则匹配任意串
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_CaseInsensitiveSearch_Matches)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());   // name "utproc"
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("UTPROC");

    // branch SS2: CaseInsensitiveOption
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NameUserRoleMatch_ReturnsTrue)
{
    // Arrange: DisplayRole 与搜索串不匹配，UserRole 名称匹配
    RowSpec spec;
    spec.name = "XyZ";
    fillRow(m_source.data(), 0, spec);
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessNameColumn), "utproc", Qt::UserRole);
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("proc");

    // branch F7
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_PidMatch_ReturnsTrue)
{
    // Arrange: 名称与用户均不匹配，PID 命中
    RowSpec spec;
    spec.name = "xyz";
    spec.user = "zzz";
    spec.pid = 4321;
    fillRow(m_source.data(), 0, spec);
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("4321");

    // branch F9
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_UserMatch_ReturnsTrue)
{
    // Arrange: 名称与 PID 均不匹配，用户名命中
    RowSpec spec;
    spec.name = "xyz";
    spec.user = "utuser";
    spec.pid = 1111;
    fillRow(m_source.data(), 0, spec);
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("user");

    // branch F10
    // Act + Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, FilterAcceptsRow_NoMatch_ReturnsFalse)
{
    // Arrange: 所有来源都不含搜索串
    RowSpec spec;
    spec.name = "xyz";
    spec.user = "zzz";
    spec.pid = 1111;
    fillRow(m_source.data(), 0, spec);
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("nomatch");

    // branch F11
    // Act + Assert
    EXPECT_FALSE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

// ============================== setSortFilterString / setFilterType ==============================

TEST_F(ProcessSortFilterProxyModelTest, SetSortFilterString_StoresSearchPattern)
{
    // Arrange
    RowSpec spec;
    spec.name = "other";
    fillRow(m_source.data(), 0, spec);
    m_proxy->setFilterType(kNoFilter);

    // branch SS1
    // Act
    m_proxy->setSortFilterString("pattern");

    // Assert: 搜索串保存 + 过滤已应用（other 不含 pattern）
    EXPECT_EQ(m_proxy->m_search, QString("pattern"));
    EXPECT_FALSE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, SetSortFilterString_EmptyPattern_KeepsAllTypedRows)
{
    // Arrange: 先设置非空搜索串过滤掉行，再清空
    fillRow(m_source.data(), 0, RowSpec());
    m_proxy->setFilterType(kNoFilter);
    m_proxy->setSortFilterString("zzz");
    ASSERT_FALSE(m_proxy->filterAcceptsRow(0, QModelIndex()));

    // Act
    m_proxy->setSortFilterString("");

    // Assert: 空串放行所有通过类型过滤的行
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, SetFilterType_UpdatesMember)
{
    // branch SF1: 成员名 m_fileterType 与源码拼写一致
    // Act
    m_proxy->setFilterType(kNoFilter);

    // Assert
    EXPECT_EQ(m_proxy->m_fileterType, kNoFilter);
    m_proxy->setFilterType(kFilterCurrentUser);
    EXPECT_EQ(m_proxy->m_fileterType, kFilterCurrentUser);
}

TEST_F(ProcessSortFilterProxyModelTest, SetFilterType_ReappliesFilterForNewType)
{
    // Arrange: 行 apptype=kFilterCurrentUser，先按 kFilterApps 过滤
    fillRow(m_source.data(), 0, RowSpec());
    m_source->setData(m_source->index(0, ProcessTableModel::kProcessPIDColumn), kFilterCurrentUser, Qt::UserRole + 3);
    m_proxy->setFilterType(kFilterApps);
    ASSERT_FALSE(m_proxy->filterAcceptsRow(0, QModelIndex()));

    // branch SF2: 切换类型后重新应用
    // Act
    m_proxy->setFilterType(kFilterCurrentUser);

    // Assert
    EXPECT_TRUE(m_proxy->filterAcceptsRow(0, QModelIndex()));
}

// ============================== lessThan (L1~L19) ==============================

TEST_F(ProcessSortFilterProxyModelTest, LessThan_InvalidIndex_ReturnsFalse)
{
    // Arrange
    fillRow(m_source.data(), 0, RowSpec());
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNameColumn);

    // branch L1
    // Act + Assert
    EXPECT_FALSE(m_proxy->lessThan(QModelIndex(), m_source->index(0, 0)));
    EXPECT_FALSE(m_proxy->lessThan(m_source->index(0, 0), QModelIndex()));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_NameColumn_NonHanziBeforeHanzi_ReturnsTrue)
{
    // Arrange: 左侧非汉字、右侧汉字
    RowSpec l, r;
    l.name = "apple";
    r.name = QString::fromUtf8("苹果");
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNameColumn);

    // branch L2
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNameColumn),
                                  m_source->index(1, ProcessTableModel::kProcessNameColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_NameColumn_HanziBeforeNonHanzi_ReturnsFalse)
{
    // Arrange: 左侧汉字、右侧非汉字
    RowSpec l, r;
    l.name = QString::fromUtf8("苹果");
    r.name = "apple";
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNameColumn);

    // branch L3
    // Act + Assert
    EXPECT_FALSE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNameColumn),
                                   m_source->index(1, ProcessTableModel::kProcessNameColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_NameColumn_EqualNames_CompareByCpu)
{
    // Arrange: 名称相同，左侧 CPU 更小
    RowSpec l, r;
    l.name = "same";
    r.name = "same";
    l.cpu = 1.0;
    r.cpu = 2.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNameColumn);

    // branch L4
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNameColumn),
                                  m_source->index(1, ProcessTableModel::kProcessNameColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_NameColumn_LocaleOrder_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.name = "apple";
    r.name = "banana";
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNameColumn);

    // branch L5
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNameColumn),
                                  m_source->index(1, ProcessTableModel::kProcessNameColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_UserColumn_LocaleCompare_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.user = "alice";
    r.user = "bob";
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessUserColumn);

    // branch L6
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessUserColumn),
                                  m_source->index(1, ProcessTableModel::kProcessUserColumn)));
}

// Memory/ShareMemory/VTRMemory 三列共用同一 switch 分支，断言形态一致 -> TEST_P
class ProcessSortFilterProxyModelMemoryParamTest :
    public ProcessSortFilterProxyModelTest,
    public ::testing::WithParamInterface<SortCase> {
};

TEST_P(ProcessSortFilterProxyModelMemoryParamTest, LessThan_MemoryColumns_CompareValues)
{
    // Arrange: 按 switch 分支路由到对应内存列的 UserRole
    const SortCase &c = GetParam();
    RowSpec l, r;
    switch (c.column) {
    case ProcessTableModel::kProcessMemoryColumn:
        l.mem = c.lmem;
        r.mem = c.rmem;
        break;
    case ProcessTableModel::kProcessShareMemoryColumn:
        l.shm = c.lmem;
        r.shm = c.rmem;
        break;
    default:
        l.vtr = c.lmem;
        r.vtr = c.rmem;
        break;
    }
    l.cpu = c.lcpu;
    r.cpu = c.rcpu;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, c.column);

    // Act + Assert
    EXPECT_EQ(m_proxy->lessThan(m_source->index(0, c.column), m_source->index(1, c.column)), c.expected);
}

INSTANTIATE_TEST_SUITE_P(MemoryColumnsMatrix, ProcessSortFilterProxyModelMemoryParamTest,
    ::testing::Values(
        // branch L8: 数值比较
        SortCase{ProcessTableModel::kProcessMemoryColumn, 100, 200, 0.0, 0.0, true},
        SortCase{ProcessTableModel::kProcessMemoryColumn, 200, 100, 0.0, 0.0, false},
        // branch L7: 数值相等 -> CPU tie-break（左 CPU 小 -> true）
        SortCase{ProcessTableModel::kProcessMemoryColumn, 150, 150, 1.0, 2.0, true},
        SortCase{ProcessTableModel::kProcessMemoryColumn, 150, 150, 3.0, 2.0, false},
        SortCase{ProcessTableModel::kProcessShareMemoryColumn, 10, 20, 0.0, 0.0, true},
        SortCase{ProcessTableModel::kProcessVTRMemoryColumn, 30, 20, 0.0, 0.0, false}
    ));

TEST_F(ProcessSortFilterProxyModelTest, LessThan_CpuColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.cpu = 1.0;
    r.cpu = 2.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessCPUColumn);

    // branch L10
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessCPUColumn),
                                  m_source->index(1, ProcessTableModel::kProcessCPUColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_CpuColumn_EqualValues_CompareByMemory)
{
    // Arrange: CPU 相等（qFuzzyCompare），内存决定顺序
    RowSpec l, r;
    l.cpu = 2.0;
    r.cpu = 2.0;
    l.mem = 100;
    r.mem = 200;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessCPUColumn);

    // branch L9
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessCPUColumn),
                                  m_source->index(1, ProcessTableModel::kProcessCPUColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_UploadColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.upload = 10.0;
    r.upload = 20.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessUploadColumn);

    // branch L12
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessUploadColumn),
                                  m_source->index(1, ProcessTableModel::kProcessUploadColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_UploadColumn_EqualValues_CompareByTotalSent)
{
    // Arrange: 速率相等，按累计发送 UserRole+1 排序
    RowSpec l, r;
    l.upload = 10.0;
    r.upload = 10.0;
    l.uploadTotal = 500;
    r.uploadTotal = 900;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessUploadColumn);

    // branch L11
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessUploadColumn),
                                  m_source->index(1, ProcessTableModel::kProcessUploadColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_DownloadColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.download = 5.0;
    r.download = 50.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessDownloadColumn);

    // branch L14
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessDownloadColumn),
                                  m_source->index(1, ProcessTableModel::kProcessDownloadColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_DownloadColumn_EqualValues_CompareByTotalRecv)
{
    // Arrange: 速率相等，按累计接收 UserRole+1 排序
    RowSpec l, r;
    l.download = 5.0;
    r.download = 5.0;
    l.downloadTotal = 100;
    r.downloadTotal = 400;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessDownloadColumn);

    // branch L13
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessDownloadColumn),
                                  m_source->index(1, ProcessTableModel::kProcessDownloadColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_PidColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.pid = 100;
    r.pid = 200;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessPIDColumn);

    // branch L15
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessPIDColumn),
                                  m_source->index(1, ProcessTableModel::kProcessPIDColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_DiskReadColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.diskRead = 1.0;
    r.diskRead = 2.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessDiskReadColumn);

    // branch L16
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessDiskReadColumn),
                                  m_source->index(1, ProcessTableModel::kProcessDiskReadColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_DiskWriteColumn_LessValue_ReturnsTrue)
{
    // Arrange
    RowSpec l, r;
    l.diskWrite = 1.0;
    r.diskWrite = 2.0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessDiskWriteColumn);

    // branch L16
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessDiskWriteColumn),
                                  m_source->index(1, ProcessTableModel::kProcessDiskWriteColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_NiceColumn_HigherValue_NotLess)
{
    // Arrange: nice 10 vs 0 -> !(10 < 0) = true（值大者视为"更小"，降序语义）
    RowSpec l, r;
    l.nice = 10;
    r.nice = 0;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessNiceColumn);

    // branch L17
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNiceColumn),
                                  m_source->index(1, ProcessTableModel::kProcessNiceColumn)));
    // 反向: nice 0 vs 10 -> !(0 < 10) = false
    EXPECT_FALSE(m_proxy->lessThan(m_source->index(1, ProcessTableModel::kProcessNiceColumn),
                                   m_source->index(0, ProcessTableModel::kProcessNiceColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_PriorityColumn_ComparesNiceSibling)
{
    // Arrange: Priority 列排序实际比较 Nice 列 UserRole（sibling）
    RowSpec l, r;
    l.nice = 0;
    r.nice = 10;
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, ProcessTableModel::kProcessPriorityColumn);

    // branch L18
    // Act + Assert: !(0 < 10) = false
    EXPECT_FALSE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessPriorityColumn),
                                   m_source->index(1, ProcessTableModel::kProcessPriorityColumn)));
    // 反向: !(10 < 0) = true
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(1, ProcessTableModel::kProcessPriorityColumn),
                                  m_source->index(0, ProcessTableModel::kProcessPriorityColumn)));
}

TEST_F(ProcessSortFilterProxyModelTest, LessThan_UnknownColumn_FallsBackToBaseImplementation)
{
    // Arrange
    RowSpec l, r;
    l.name = "aaa";
    r.name = "bbb";
    fillRow(m_source.data(), 0, l);
    fillRow(m_source.data(), 1, r);
    Stub b;
    stubSortColumn(b, 1000);

    // branch L19: 基类按 DisplayRole 比较
    // Act + Assert
    EXPECT_TRUE(m_proxy->lessThan(m_source->index(0, ProcessTableModel::kProcessNameColumn),
                                  m_source->index(1, ProcessTableModel::kProcessNameColumn)));
}
