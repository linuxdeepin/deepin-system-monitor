// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for model::CPUStatModel
// (deepin-system-monitor-main/model/cpu_stat_model.cpp) against
// upstream/master @ 00005c4e.
//
// The legacy half of this file carried the suite's review failure (numbered
// shell cases, no AAA markers, QModelIndex::isValid function stubbing that
// cannot work in Qt6); that half is removed. The injection-driven tail
// survives with branch-mapped names and thickened assertions.
//
// Rows are driven through private-member injection (addr_pri.h): stat and
// usage SampleFrames are appended to the model's sample databases, so the
// suite is hermetic without function stubbing and without /proc access.
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |CPUStatModel()|low|默认空样本库|1|1|
// |rowCount|mid|空库×等量样本×计数不一致×增长|2|4|
// |columnCount|low|常量列数|1|1|
// |data|high|索引无效×行越界×12列×角色×default列|3|6|
// 本 TU 共 12 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::CPUStatModel 全部方法
//       @ model/cpu_stat_model.cpp:12-107
// ctor : m_statSampleDB(new CPUStatSample(period)),
//        m_usageSampleDB(new CPUUsageSample(period))
// B1   : rowCount -> stat.count()==usage.count() ? usage.count() : 0
// B2   : columnCount -> kStatPropMax (常量 12)
// B3   : data !index.isValid() || 样本库为空 -> {}
// B4   : data row<0 || row>=usage.count() -> {}
// B5   : data stat/usage 样本指针为空 -> {}
// B6   : data Display/Accessible + 12 列 switch
//        (kStatUserTime..kStatGuestNice 读 stat->stat->*,
//         kStatTotalUsage/kStatIdleUsage 读 usage->stat->*)
// B7   : data Display/Accessible + default 列 -> {}
// B8   : data 其它角色 -> {}（switch 外直接 return {}）
// 用例映射:
//   Construction_DefaultState_SampleDBsEmpty          -> ctor + B1(0) + B2
//   RowCount_EqualSampleCounts_ReturnsUsageCount      -> B1 (1/1)
//   RowCount_AfterSecondSample_GrowsToTwo             -> B1 (2/2)
//   RowCount_SampleCountMismatch_ReturnsZero          -> B1 (2/1)
//   ColumnCount_Always_ReturnsKStatPropMax            -> B2
//   Data_InvalidIndex_ReturnsInvalidVariant           -> B3
//   Data_RowOutOfBounds_ReturnsInvalidVariant         -> B4
//   Data_NegativeRow_ReturnsInvalidVariant            -> B4 (row<0)
//   Data_DisplayRoleAllColumns_ReturnsSampleValues    -> B6 (12 列)
//   Data_AccessibleTextRole_SharesDisplayBranch       -> B6
//   Data_UnknownRole_ReturnsInvalidVariant            -> B8
//   Data_DefaultColumnSwitchBranch_ReturnsInvalid     -> B7
// ====================================================================

#include <gtest/gtest.h>

#include <QModelIndex>
#include <QVariant>

#include "model/cpu_stat_model.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"

using namespace common::core;
using namespace common::time;

using StatSampleDB = std::unique_ptr<Sample<cpu_stat_t>>;
using UsageSampleDB = std::unique_ptr<Sample<cpu_usage_t>>;

ACCESS_PRIVATE_FIELD(CPUStatModel, StatSampleDB, m_statSampleDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, UsageSampleDB, m_usageSampleDB)

namespace {

// Append one stat/usage sample frame pair to the model's sample databases.
void injectSample(CPUStatModel *model, const cpu_stat_t &statVal, const cpu_usage_t &usageVal)
{
    timeval tv {1, 0};
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*model);
    auto &usageDB = access_private_field::CPUStatModelm_usageSampleDB(*model);
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(statVal))));
    usageDB->addSample(new SampleFrame<cpu_usage_t>(tv, CPUUsage(std::make_shared<cpu_usage_t>(usageVal))));
}

class UT_CPUStatModel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Arrange: every case starts from a fresh model with empty sample DBs
        m_model.reset(new CPUStatModel(TimePeriod(), nullptr));
    }

    void TearDown() override
    {
        m_model.reset();
    }

    QScopedPointer<CPUStatModel> m_model;
};

// ---------------------------------------------------------------------------
// ctor + B1 + B2: a fresh model has zero rows and the constant column count.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Construction_DefaultState_SampleDBsEmpty)
{
    // Arrange: SetUp built the model with empty sample databases
    ASSERT_NE(m_model, nullptr);

    // Act
    const int rows = m_model->rowCount();
    const int cols = m_model->columnCount();

    // Assert
    EXPECT_EQ(0, rows);
    EXPECT_EQ(CPUStatModel::kStatPropMax, cols);
}

// ---------------------------------------------------------------------------
// B1 (equal counts): rowCount returns the usage sample count.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, RowCount_EqualSampleCounts_ReturnsUsageCount)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);

    // Act
    const int rows = m_model->rowCount();

    // Assert: one stat frame + one usage frame -> single model row
    EXPECT_EQ(1, rows);
    EXPECT_EQ(1, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B1 (equal counts): the row count follows the sample growth and the
// ring-buffer window keeps stat/usage counts equal when full.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, RowCount_AfterSecondSample_GrowsToTwo)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);
    ASSERT_EQ(1, m_model->rowCount());

    // Act: fill the default two-frame window
    injectSample(m_model.data(), st, us);
    const int rowsAfterTwo = m_model->rowCount();

    // Assert: both sample stores report the same count
    EXPECT_EQ(2, rowsAfterTwo);
    EXPECT_EQ(2, int(access_private_field::CPUStatModelm_usageSampleDB(*m_model)->count()));

    // Act: a third frame evicts the oldest (window stays at two frames)
    injectSample(m_model.data(), st, us);
    const int rowsAfterThree = m_model->rowCount();

    // Assert: counts stay equal, so the model row count is unchanged
    EXPECT_EQ(2, rowsAfterThree);
    EXPECT_EQ(2, int(access_private_field::CPUStatModelm_statSampleDB(*m_model)->count()));
}

// ---------------------------------------------------------------------------
// B1 (mismatched counts): stat/usage imbalance collapses rowCount to zero.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, RowCount_SampleCountMismatch_ReturnsZero)
{
    // Arrange: append a stat frame but no matching usage frame
    cpu_stat_t st {};
    timeval tv {1, 0};
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*m_model);
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(st))));
    ASSERT_EQ(1, int(access_private_field::CPUStatModelm_statSampleDB(*m_model)->count()));
    ASSERT_EQ(0, int(access_private_field::CPUStatModelm_usageSampleDB(*m_model)->count()));

    // Act
    const int rows = m_model->rowCount();

    // Assert: the guard demands equal stat/usage counts
    EXPECT_EQ(0, rows);
    EXPECT_EQ(0, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B2: columnCount is the constant kStatPropMax regardless of content.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, ColumnCount_Always_ReturnsKStatPropMax)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);

    // Act
    const int cols = m_model->columnCount();

    // Assert: 12 stat/usage columns enumerated in the header
    EXPECT_EQ(CPUStatModel::kStatPropMax, cols);
    EXPECT_EQ(12, cols);
}

// ---------------------------------------------------------------------------
// B3: an invalid index short-circuits before the sample lookups.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_InvalidIndex_ReturnsInvalidVariant)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);
    const QModelIndex invalid;

    // Act
    const QVariant value = m_model->data(invalid, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B4 (row >= usage count): an out-of-bounds row is rejected.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_RowOutOfBounds_ReturnsInvalidVariant)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);
    const QModelIndex tooFar = m_model->index(1000, CPUStatModel::kStatUserTime);

    // Act
    const QVariant value = m_model->data(tooFar, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B4 (row < 0): a negative row is rejected the same way.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_NegativeRow_ReturnsInvalidVariant)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);

    // Act: createIndex bypasses QAbstractItemModel's own index validation
    const QModelIndex negative = m_model->createIndex(-1, CPUStatModel::kStatUserTime);
    const QVariant value = m_model->data(negative, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B6: every one of the 12 columns maps to its sample field under DisplayRole.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_DisplayRoleAllColumns_ReturnsSampleValues)
{
    // Arrange: distinctive field values prove the column-to-field mapping
    cpu_stat_t st {};
    st.user = 11; st.nice = 12; st.sys = 13; st.idle = 14; st.iowait = 15;
    st.hardirq = 16; st.softirq = 17; st.steal = 18; st.guest = 19; st.guest_nice = 20;
    cpu_usage_t us {};
    us.total = 100; us.idle = 30;
    injectSample(m_model.data(), st, us);
    ASSERT_EQ(1, m_model->rowCount());

    struct Col { int col; qulonglong expect; };
    const Col cols[] = {
        {CPUStatModel::kStatUserTime, 11},   {CPUStatModel::kStatNiceTime, 12},
        {CPUStatModel::kStatSystemTime, 13}, {CPUStatModel::kStatIdleTime, 14},
        {CPUStatModel::kStatIOWaitTime, 15}, {CPUStatModel::kStatHardIRQ, 16},
        {CPUStatModel::kStatSoftIRQ, 17},    {CPUStatModel::kStatStealTime, 18},
        {CPUStatModel::kStatGuestTime, 19},  {CPUStatModel::kStatGuestNice, 20},
        {CPUStatModel::kStatTotalUsage, 100},{CPUStatModel::kStatIdleUsage, 30}
    };

    // Act: read every column under DisplayRole into a lookup table
    QMap<int, QVariant> rendered;
    for (const auto &c : cols)
        rendered.insert(c.col, m_model->data(m_model->index(0, c.col), Qt::DisplayRole));

    // Assert: each column resolves through its switch case
    for (const auto &c : cols) {
        EXPECT_TRUE(rendered.value(c.col).isValid()) << "col=" << c.col;
        EXPECT_EQ(c.expect, rendered.value(c.col).toULongLong()) << "col=" << c.col;
    }
    EXPECT_EQ(12, rendered.count());
}

// ---------------------------------------------------------------------------
// B6: AccessibleTextRole shares the display switch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_AccessibleTextRole_SharesDisplayBranch)
{
    // Arrange
    cpu_stat_t st {};
    st.user = 7;
    cpu_usage_t us {};
    us.total = 50; us.idle = 10;
    injectSample(m_model.data(), st, us);
    const QModelIndex idx = m_model->index(0, CPUStatModel::kStatUserTime);

    // Act
    const QVariant accessible = m_model->data(idx, Qt::AccessibleTextRole);
    const QVariant display = m_model->data(idx, Qt::DisplayRole);

    // Assert: identical value across the two roles
    EXPECT_EQ(display.toULongLong(), accessible.toULongLong());
    EXPECT_EQ(7ull, accessible.toULongLong());
}

// ---------------------------------------------------------------------------
// B8: roles outside the display pair return an invalid variant.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_UnknownRole_ReturnsInvalidVariant)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);
    const QModelIndex idx = m_model->index(0, CPUStatModel::kStatUserTime);

    // Act
    const QVariant alignment = m_model->data(idx, Qt::TextAlignmentRole);
    const QVariant font = m_model->data(idx, Qt::FontRole);

    // Assert: the switch is display-only, everything else falls to {}
    EXPECT_FALSE(alignment.isValid());
    EXPECT_EQ(QVariant(), font);
}

// ---------------------------------------------------------------------------
// B7: a column at kStatPropMax falls through the display switch.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUStatModel, Data_DefaultColumnSwitchBranch_ReturnsInvalid)
{
    // Arrange
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_model.data(), st, us);
    const QModelIndex defaultCol = m_model->createIndex(0, CPUStatModel::kStatPropMax);

    // Act
    const QVariant value = m_model->data(defaultCol, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

} // namespace
