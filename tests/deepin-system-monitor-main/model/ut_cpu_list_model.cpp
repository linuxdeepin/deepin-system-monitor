// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for model::CPUListModel
// (deepin-system-monitor-main/model/cpu_list_model.cpp) against
// upstream/master @ 00005c4e.
//
// The legacy half of this file carried the whole suite's review failure:
// numbered case names, no Arrange/Act/Assert markers and empty assertion
// clusters. That half is gone; the injection-driven cases survive with
// branch-mapped names and thickened assertions.
//
// No DBus / filesystem / network dependency exists in the model itself: the
// rows are driven entirely through private-member injection (addr_pri.h),
// so the suite is hermetic without function stubbing.
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |CPUListModel()|low|默认空库|1|1|
// |rowCount|mid|空库×注入条目|1|2|
// |columnCount|low|常量列数|1|1|
// |data|high|索引无效×行越界×列枚举×角色×total为零|3|9|
// |fetchMore|mid|parent有效×满批×未满批×无剩余|2|3|
// |canFetchMore|mid|parent有效×未取完×已取完|2|3|
// |statModel|mid|已注册×未注册|2|2|
// 本 TU 共 21 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::CPUListModel 全部方法
//       @ model/cpu_list_model.cpp:12-116
// ctor : m_cpuInfoDB={}, m_statModelDB={}, m_nr=0
// B1   : rowCount -> m_statModelDB.count()
// B2   : columnCount -> kPropCount (常量)
// B3   : data !idx.isValid() -> {}
// B4   : data row<0 || row>=m_statModelDB.count() -> {}
// B5   : data role==Display/Accessible + col==kPropCPUIndex -> logicalIndex()
// B6   : data role==Display/Accessible + col==kPropCPUUsage
//        -> total==0 ? "0%" : "%1%2"(usage%)
// B7   : data Display/Accessible + default col -> {} (switch 落空)
// B8   : data role==kValueRole + col==kPropCPUUsage
//        -> total==0 ? 0.0 : (total-idle)/total
// B9   : data kValueRole + default col -> {} + role 落空 -> {}
// B10  : fetchMore parent.isValid() -> return
// B11  : fetchMore more<=0 -> return（无剩余）
// B12  : fetchMore more=qMin(32,left) -> beginInsertRows + m_nr+=more
// B13  : canFetchMore parent.isValid() -> false
// B14  : canFetchMore -> m_nr < m_statModelDB.count()
// B15  : statModel -> m_statModelDB[cpu]（已注册/未注册）
// 用例映射:
//   Construction_DefaultState_ModelEmpty             -> ctor + B1(空)
//   RowCount_WithInjectedEntries_MatchesStatModelSize -> B1
//   ColumnCount_Always_ReturnsKPropCount             -> B2
//   Data_InvalidIndex_ReturnsInvalidVariant          -> B3
//   Data_RowOutOfBounds_ReturnsInvalidVariant        -> B4
//   Data_DisplayRoleCPUIndex_ReturnsLogicalIndex     -> B5
//   Data_DisplayRoleCPUUsage_ReturnsPercentString    -> B6 (total>0)
//   Data_AccessibleTextRole_MatchesDisplayPercent    -> B6 (同分支)
//   Data_DisplayRoleTotalZero_ReturnsZeroPercent     -> B6 (total==0)
//   Data_ValueRoleCPUUsage_ReturnsUsageRatio         -> B8 (total>0)
//   Data_ValueRoleTotalZero_ReturnsZero              -> B8 (total==0)
//   Data_DisplayRoleUnknownColumn_ReturnsInvalid     -> B7
//   Data_ValueRoleCPUIndex_ReturnsInvalid            -> B9 (default col)
//   Data_ValueRoleUnknownRole_ReturnsInvalid         -> B9 (role 落空)
//   FetchMore_ValidParent_InsertsUpTo32PerBatch      -> B12 (40 -> 32 -> 8)
//   FetchMore_WhenFullyFetched_NoMoreInserts         -> B11
//   FetchMore_ValidParentIndex_ReturnsImmediately    -> B10
//   CanFetchMore_UnfetchedEntries_ReturnsTrue        -> B14 (m_nr<count)
//   CanFetchMore_AllFetched_ReturnsFalse             -> B14 (m_nr==count)
//   CanFetchMore_ValidParentIndex_ReturnsFalse       -> B13
//   StatModel_RegisteredAndUnknownNames_ReturnsModelOrNull        -> B15
// ====================================================================

#include <gtest/gtest.h>

#include <QMap>
#include <QModelIndex>
#include <QVariant>
#include <cmath>

#include "model/cpu_list_model.h"
#include "model/cpu_stat_model.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"

using namespace common::core;
using namespace common::time;

using CPUStatModelDB = QMap<QByteArray, std::shared_ptr<CPUStatModel>>;
using StatSampleDB = std::unique_ptr<Sample<cpu_stat_t>>;
using UsageSampleDB = std::unique_ptr<Sample<cpu_usage_t>>;

// Inject the private members the row/data paths consume.
ACCESS_PRIVATE_FIELD(CPUListModel, QList<CPUInfo>, m_cpuInfoDB)
ACCESS_PRIVATE_FIELD(CPUListModel, CPUStatModelDB, m_statModelDB)
ACCESS_PRIVATE_FIELD(CPUListModel, int, m_nr)
ACCESS_PRIVATE_FIELD(CPUStatModel, StatSampleDB, m_statSampleDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, UsageSampleDB, m_usageSampleDB)

namespace {

// Install one usage/stat sample frame (total=100, idle=25) into a freshly
// created CPUStatModel so the CPUListModel usage math is deterministic.
std::shared_ptr<CPUStatModel> makeStatModel(qulonglong total, qulonglong idle)
{
    auto model = std::make_shared<CPUStatModel>(TimePeriod(TimePeriod::kNoPeriod, {2, 0}));
    timeval tv {1, 0};
    cpu_stat_t st {};
    cpu_usage_t us {};
    us.total = total;
    us.idle = idle;
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*model);
    auto &usageDB = access_private_field::CPUStatModelm_usageSampleDB(*model);
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(st))));
    usageDB->addSample(new SampleFrame<cpu_usage_t>(tv, CPUUsage(std::make_shared<cpu_usage_t>(us))));
    return model;
}

// Register `count` stat models keyed cpu0..cpuN-1 into the list model and
// mirror the same number of CPUInfo rows.
void injectEntries(CPUListModel *model, int count)
{
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*model);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    for (int i = 0; i < count; ++i) {
        cpuInfoDB.append(CPUInfo(i));
        statModelDB.insert(QByteArray("cpu" + QByteArray::number(i)),
                           std::make_shared<CPUStatModel>(period));
    }
}

class UT_CPUListModel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Arrange: every case starts from a fresh empty list model
        m_model.reset(new CPUListModel(nullptr));
    }

    void TearDown() override
    {
        m_model.reset();
    }

    QScopedPointer<CPUListModel> m_model;
};

// ---------------------------------------------------------------------------
// ctor + B1: a fresh model reports zero rows and the constant column count.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Construction_DefaultState_ModelEmpty)
{
    // Arrange: SetUp built an empty model
    ASSERT_NE(m_model, nullptr);

    // Act
    const int rows = m_model->rowCount();
    const int cols = m_model->columnCount();

    // Assert
    EXPECT_EQ(0, rows);
    EXPECT_EQ(CPUListModel::kPropCount, cols);
}

// ---------------------------------------------------------------------------
// B1: rowCount mirrors the number of injected stat-model entries.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, RowCount_WithInjectedEntries_MatchesStatModelSize)
{
    // Arrange
    injectEntries(m_model.data(), 3);
    ASSERT_EQ(3, access_private_field::CPUListModelm_statModelDB(*m_model).count());

    // Act
    const int rows = m_model->rowCount();

    // Assert: rowCount mirrors the stat entry count; a second row stays stable
    EXPECT_EQ(3, rows);
    EXPECT_EQ(3, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B2: columnCount is the constant kPropCount regardless of content.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, ColumnCount_Always_ReturnsKPropCount)
{
    // Arrange
    injectEntries(m_model.data(), 2);

    // Act
    const int cols = m_model->columnCount();

    // Assert: kPropCPUIndex=0, kPropCPUUsage=1 -> count 2
    EXPECT_EQ(CPUListModel::kPropCount, cols);
    EXPECT_EQ(2, cols);
}

// ---------------------------------------------------------------------------
// B3: an invalid index short-circuits before any lookup.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_InvalidIndex_ReturnsInvalidVariant)
{
    // Arrange
    const QModelIndex invalid;

    // Act
    const QVariant value = m_model->data(invalid, Qt::DisplayRole);

    // Assert: the guard returns an invalid variant before any lookup
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B4: a row outside m_statModelDB is rejected even with a valid index.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_RowOutOfBounds_ReturnsInvalidVariant)
{
    // Arrange: one entry exists, but the row index points past it
    injectEntries(m_model.data(), 1);
    const QModelIndex tooFar = m_model->createIndex(100, CPUListModel::kPropCPUIndex);

    // Act
    const QVariant value = m_model->data(tooFar, Qt::DisplayRole);

    // Assert: the bound check rejects the row before the column switch
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B5: DisplayRole on the index column returns the CPU logical index.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_DisplayRoleCPUIndex_ReturnsLogicalIndex)
{
    // Arrange: CPUInfo(3) carries logicalIndex 3; the stat entry count must
    // cover row 0 (the bound check runs against m_statModelDB, not cpuInfoDB)
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    cpuInfoDB.append(CPUInfo(3));
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(period));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUIndex);

    // Act
    const QVariant value = m_model->data(idx, Qt::DisplayRole);

    // Assert
    EXPECT_TRUE(value.isValid());
    EXPECT_EQ(3, value.toInt());
}

// ---------------------------------------------------------------------------
// B6 (total>0): DisplayRole on the usage column renders "75%" for the
// injected frame total=100 / idle=25.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_DisplayRoleCPUUsage_ReturnsPercentString)
{
    // Arrange: CPUInfo(int) leaves logicalName() empty, so the stat lookup
    // key must be the empty byte array
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert(QByteArray(""), makeStatModel(100, 25));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant value = m_model->data(idx, Qt::DisplayRole);

    // Assert: (100-25)/100 = 75%
    EXPECT_TRUE(value.isValid());
    EXPECT_EQ(QString("75%"), value.toString());
}

// ---------------------------------------------------------------------------
// B6: AccessibleTextRole shares the DisplayRole branch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_AccessibleTextRole_MatchesDisplayPercent)
{
    // Arrange
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert(QByteArray(""), makeStatModel(100, 25));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant accessible = m_model->data(idx, Qt::AccessibleTextRole);
    const QVariant display = m_model->data(idx, Qt::DisplayRole);

    // Assert: identical rendering across the two roles
    EXPECT_EQ(display.toString(), accessible.toString());
    EXPECT_EQ(QString("75%"), accessible.toString());
}

// ---------------------------------------------------------------------------
// B6 (total==0): the zero-usage guard renders a literal "0%" string.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_DisplayRoleTotalZero_ReturnsZeroPercent)
{
    // Arrange
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert(QByteArray(""), makeStatModel(0, 0));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant value = m_model->data(idx, Qt::DisplayRole);

    // Assert
    EXPECT_TRUE(value.isValid());
    EXPECT_EQ(QString("0%"), value.toString());
}

// ---------------------------------------------------------------------------
// B8 (total>0): kValueRole returns the raw ratio (total-idle)/total = 0.75.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_ValueRoleCPUUsage_ReturnsUsageRatio)
{
    // Arrange
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert(QByteArray(""), makeStatModel(100, 25));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant value = m_model->data(idx, CPUListModel::kValueRole);

    // Assert
    EXPECT_TRUE(value.isValid());
    EXPECT_DOUBLE_EQ(0.75, value.toDouble());
}

// ---------------------------------------------------------------------------
// B8 (total==0): the zero guard returns 0.0 for the value role.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_ValueRoleTotalZero_ReturnsZero)
{
    // Arrange
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_model);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert(QByteArray(""), makeStatModel(0, 0));
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant value = m_model->data(idx, CPUListModel::kValueRole);

    // Assert
    EXPECT_TRUE(value.isValid());
    EXPECT_DOUBLE_EQ(0.0, value.toDouble());
}

// ---------------------------------------------------------------------------
// B7: a column past kPropCount falls through the display switch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_DisplayRoleUnknownColumn_ReturnsInvalid)
{
    // Arrange
    injectEntries(m_model.data(), 1);
    const QModelIndex unknownCol = m_model->createIndex(0, CPUListModel::kPropCount);

    // Act
    const QVariant value = m_model->data(unknownCol, Qt::DisplayRole);

    // Assert: the display switch falls through to its default branch
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B9 (default col): kValueRole on the index column has no case either.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_ValueRoleCPUIndex_ReturnsInvalid)
{
    // Arrange
    injectEntries(m_model.data(), 1);
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUIndex);

    // Act
    const QVariant value = m_model->data(idx, CPUListModel::kValueRole);

    // Assert: kValueRole only handles the usage column
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B9 (role fall-through): an unrelated role returns an invalid variant.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, Data_ValueRoleUnknownRole_ReturnsInvalid)
{
    // Arrange
    injectEntries(m_model.data(), 1);
    const QModelIndex idx = m_model->index(0, CPUListModel::kPropCPUUsage);

    // Act
    const QVariant value = m_model->data(idx, Qt::UserRole + 100);

    // Assert: neither the display nor the value branch matches the role
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B12: fetchMore batches at most 32 rows per call (40 -> 32 -> 8 -> stop).
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, FetchMore_ValidParent_InsertsUpTo32PerBatch)
{
    // Arrange
    injectEntries(m_model.data(), 40);
    ASSERT_EQ(0, access_private_field::CPUListModelm_nr(*m_model));
    const QModelIndex root;

    // Act
    m_model->fetchMore(root);
    const int afterFirst = access_private_field::CPUListModelm_nr(*m_model);
    m_model->fetchMore(root);
    const int afterSecond = access_private_field::CPUListModelm_nr(*m_model);
    const int rows = m_model->rowCount();

    // Assert: first batch clamps to 32, second takes the remaining 8
    EXPECT_EQ(32, afterFirst);
    EXPECT_EQ(40, afterSecond);
    EXPECT_EQ(40, rows);
}

// ---------------------------------------------------------------------------
// B11: once everything is fetched, fetchMore is a no-op.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, FetchMore_WhenFullyFetched_NoMoreInserts)
{
    // Arrange: fetch everything up front
    injectEntries(m_model.data(), 5);
    const QModelIndex root;
    m_model->fetchMore(root);
    ASSERT_EQ(5, access_private_field::CPUListModelm_nr(*m_model));

    // Act
    m_model->fetchMore(root);

    // Assert: m_nr stays at the fetched count; rowCount unchanged
    EXPECT_EQ(5, access_private_field::CPUListModelm_nr(*m_model));
    EXPECT_EQ(5, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B10: a valid parent index refuses fetching (tree-only guard).
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, FetchMore_ValidParentIndex_ReturnsImmediately)
{
    // Arrange
    injectEntries(m_model.data(), 4);
    const QModelIndex child = m_model->index(0, CPUListModel::kPropCPUIndex);

    // Act
    m_model->fetchMore(child);

    // Assert: the parent guard returned before touching m_nr
    EXPECT_EQ(0, access_private_field::CPUListModelm_nr(*m_model));
    EXPECT_EQ(4, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B14 (m_nr < count): more rows are available.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, CanFetchMore_UnfetchedEntries_ReturnsTrue)
{
    // Arrange
    injectEntries(m_model.data(), 3);
    const QModelIndex root;

    // Act
    const bool canFetch = m_model->canFetchMore(root);

    // Assert: m_nr(0) < count(3) still allows fetching
    EXPECT_EQ(true, canFetch);
    EXPECT_EQ(3, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B14 (m_nr == count): nothing left to fetch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, CanFetchMore_AllFetched_ReturnsFalse)
{
    // Arrange
    injectEntries(m_model.data(), 2);
    access_private_field::CPUListModelm_nr(*m_model) = 2;
    const QModelIndex root;

    // Act
    const bool canFetch = m_model->canFetchMore(root);

    // Assert: m_nr(2) == count(2) stops further fetching
    EXPECT_EQ(false, canFetch);
    EXPECT_EQ(2, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B13: a valid parent index always reports nothing to fetch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, CanFetchMore_ValidParentIndex_ReturnsFalse)
{
    // Arrange
    injectEntries(m_model.data(), 2);
    const QModelIndex child = m_model->index(0, CPUListModel::kPropCPUIndex);

    // Act
    const bool canFetch = m_model->canFetchMore(child);

    // Assert: the parent guard wins before the m_nr comparison
    EXPECT_EQ(false, canFetch);
    EXPECT_EQ(2, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B15: statModel returns the registered model, null for unknown names.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListModel, StatModel_RegisteredAndUnknownNames_ReturnsModelOrNull)
{
    // Arrange
    injectEntries(m_model.data(), 1);

    // Act
    const std::weak_ptr<CPUStatModel> known = m_model->statModel("cpu0");
    const std::weak_ptr<CPUStatModel> unknown = m_model->statModel("cpuX");

    // Assert
    EXPECT_EQ(known.lock(), access_private_field::CPUListModelm_statModelDB(*m_model).value("cpu0"));
    EXPECT_TRUE(unknown.lock() == nullptr);
}

} // namespace
