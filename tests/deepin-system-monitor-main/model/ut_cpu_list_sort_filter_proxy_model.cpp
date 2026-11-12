// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for model::CPUListSortFilterProxyModel
// (deepin-system-monitor-main/model/cpu_list_sort_filter_proxy_model.cpp)
// against upstream/master @ 00005c4e.
//
// The legacy suite was pure shell (initTest, four no-assertion numbered
// cases driving the methods for coverage only); it is fully rewritten
// with a real CPUListModel source built through the same addr_pri
// injection matrix used by ut_cpu_list_model.cpp, so filterAcceptsRow
// and lessThan run against genuine rows instead of QModelIndex stubs.
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |CPUListSortFilterProxyModel()|low|默认构造|1|1|
// |setSortKey|low|两枚举键|2|2|
// |filterAcceptsRow|mid|匹配×不匹配×行无效|3|3|
// |filterAcceptsColumn|low|委托基类|1|1|
// |lessThan|high|按索引×按使用率×未知键|3|3|
// 本 TU 共 10 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::CPUListSortFilterProxyModel 全部方法
//       @ model/cpu_list_sort_filter_proxy_model.cpp:13-87
// ctor : m_sortKey = kSortByIndex
// B1   : setSortKey(key) -> m_sortKey = key（inline）
// B2   : filterAcceptsRow idx.isValid() -> lgIdx 命中 filter pattern
//        -> ok |= pattern.contains(lgIdx)
// B3   : filterAcceptsRow idx 无效 -> ok 保持 false
// B4   : filterAcceptsColumn -> 委托基类
// B5   : lessThan kSortByIndex -> 按 kPropCPUIndex 逻辑索引比较
// B6   : lessThan kSortByUsage -> 按 kPropCPUUsage/kValueRole 比率比较
// B7   : lessThan default -> 委托 QSortFilterProxyModel::lessThan
// 用例映射:
//   Construction_DefaultState_SortKeyIsIndex          -> ctor
//   SetSortKey_ByUsageAndBackToIndex_StoresKey        -> B1
//   FilterAcceptsRow_LogicalIndexInPattern_ReturnsTrue -> B2
//   FilterAcceptsRow_LogicalIndexNotInPattern_ReturnsFalse -> B2 (未命中)
//   FilterAcceptsRow_SourceIndexInvalid_ReturnsFalse  -> B3
//   FilterAcceptsColumn_AnyColumn_DelegatesToBase     -> B4
//   LessThan_SortByIndex_ComparesLogicalIndex         -> B5
//   LessThan_SortByUsage_ComparesUsageRatio           -> B6
//   LessThan_UnknownSortKey_DelegatesToBase           -> B7
//   Construction_ProxyOverSource_MapsRowsUnchanged    -> 代理映射一致性
// ====================================================================

#include <gtest/gtest.h>

#include <QModelIndex>
#include <QRegularExpression>
#include <QSharedDataPointer>
#include <QVariant>

#include "model/cpu_list_sort_filter_proxy_model.h"
#include "model/cpu_list_model.h"
#include "model/cpu_stat_model.h"
#include "system/cpu.h"
#include "system/private/cpu_p.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"

using namespace common::core;
using namespace common::time;

using CPUStatModelDB = QMap<QByteArray, std::shared_ptr<CPUStatModel>>;
using StatSampleDB = std::unique_ptr<Sample<cpu_stat_t>>;
using UsageSampleDB = std::unique_ptr<Sample<cpu_usage_t>>;

ACCESS_PRIVATE_FIELD(CPUListModel, QList<CPUInfo>, m_cpuInfoDB)
ACCESS_PRIVATE_FIELD(CPUListModel, CPUStatModelDB, m_statModelDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, StatSampleDB, m_statSampleDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, UsageSampleDB, m_usageSampleDB)
ACCESS_PRIVATE_FIELD(CPUInfo, QSharedDataPointer<CPUInfoPrivate>, d)
ACCESS_PRIVATE_FIELD(CPUListSortFilterProxyModel, CPUListSortFilterProxyModel::SortKey, m_sortKey)

namespace {

// Build a stat model whose usage frame renders `usage` percent
// (total=100, idle=100-usage) so kValueRole returns usage/100.
std::shared_ptr<CPUStatModel> makeUsageStatModel(int usage)
{
    auto model = std::make_shared<CPUStatModel>(TimePeriod(TimePeriod::kNoPeriod, {2, 0}));
    timeval tv {1, 0};
    cpu_usage_t us {};
    us.total = 100;
    us.idle = qulonglong(100 - usage);
    auto &usageDB = access_private_field::CPUStatModelm_usageSampleDB(*model);
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*model);
    usageDB->addSample(new SampleFrame<cpu_usage_t>(tv, CPUUsage(std::make_shared<cpu_usage_t>(us))));
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(cpu_stat_t {}))));
    return model;
}

// Build a CPUListModel with `logicalIndexes` rows; each row carries its
// logical index (for kPropCPUIndex) and a usage percent (for kValueRole).
CPUListModel *makeSourceModel(const QList<int> &logicalIndexes, const QList<int> &usages)
{
    auto *model = new CPUListModel(nullptr);
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*model);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*model);
    for (int i = 0; i < logicalIndexes.count(); ++i) {
        CPUInfo info(logicalIndexes[i]);
        auto &priv = access_private_field::CPUInfod(info);
        priv->cpu = QByteArray("cpu" + QByteArray::number(logicalIndexes[i]));
        cpuInfoDB.append(info);
        statModelDB.insert(priv->cpu, makeUsageStatModel(usages[i]));
    }
    return model;
}

class UT_CPUListSortFilterProxyModel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Arrange: every case starts from a fresh proxy model
        m_proxy.reset(new CPUListSortFilterProxyModel(nullptr));
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_source.reset();
    }

    // Wire a fresh source model with the given logical indexes / usages.
    void setSource(const QList<int> &logicalIndexes, const QList<int> &usages)
    {
        m_source.reset(makeSourceModel(logicalIndexes, usages));
        m_proxy->setSourceModel(m_source.data());
    }

    QScopedPointer<CPUListSortFilterProxyModel> m_proxy;
    QScopedPointer<CPUListModel> m_source;
};

// ---------------------------------------------------------------------------
// ctor: the default sort key is kSortByIndex.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, Construction_DefaultState_SortKeyIsIndex)
{
    // Arrange: SetUp built the proxy
    ASSERT_NE(m_proxy, nullptr);

    // Act
    const CPUListSortFilterProxyModel::SortKey key =
        access_private_field::CPUListSortFilterProxyModelm_sortKey(*m_proxy);

    // Assert: the constructor initialises kSortByIndex, not kSortByUsage
    EXPECT_EQ(CPUListSortFilterProxyModel::kSortByIndex, key);
    EXPECT_NE(CPUListSortFilterProxyModel::kSortByUsage, key);
}

// ---------------------------------------------------------------------------
// B1: setSortKey stores both enum keys.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, SetSortKey_ByUsageAndBackToIndex_StoresKey)
{
    // Arrange
    ASSERT_EQ(CPUListSortFilterProxyModel::kSortByIndex,
              access_private_field::CPUListSortFilterProxyModelm_sortKey(*m_proxy));

    // Act
    m_proxy->setSortKey(CPUListSortFilterProxyModel::kSortByUsage);
    const CPUListSortFilterProxyModel::SortKey afterUsage =
        access_private_field::CPUListSortFilterProxyModelm_sortKey(*m_proxy);
    m_proxy->setSortKey(CPUListSortFilterProxyModel::kSortByIndex);
    const CPUListSortFilterProxyModel::SortKey afterIndex =
        access_private_field::CPUListSortFilterProxyModelm_sortKey(*m_proxy);

    // Assert
    EXPECT_EQ(CPUListSortFilterProxyModel::kSortByUsage, afterUsage);
    EXPECT_EQ(CPUListSortFilterProxyModel::kSortByIndex, afterIndex);
}

// ---------------------------------------------------------------------------
// B2 (hit): a row whose logical index appears in the filter pattern passes.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, FilterAcceptsRow_LogicalIndexInPattern_ReturnsTrue)
{
    // Arrange: CPUs 0..2, filter matches the pattern "0,1"
    setSource({0, 1, 2}, {10, 20, 30});
    m_proxy->setFilterRegularExpression(QRegularExpression("0,1"));

    // Act
    const bool row0 = m_proxy->filterAcceptsRow(0, QModelIndex());
    const bool row1 = m_proxy->filterAcceptsRow(1, QModelIndex());

    // Assert
    EXPECT_EQ(true, row0);
    EXPECT_EQ(true, row1);
}

// ---------------------------------------------------------------------------
// B2 (miss): a logical index outside the pattern is filtered out.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, FilterAcceptsRow_LogicalIndexNotInPattern_ReturnsFalse)
{
    // Arrange: CPU 2 is not part of the pattern "0,1"
    setSource({0, 1, 2}, {10, 20, 30});
    m_proxy->setFilterRegularExpression(QRegularExpression("0,1"));

    // Act
    const bool row2 = m_proxy->filterAcceptsRow(2, QModelIndex());

    // Act: contrast row 1, whose logical index is in the pattern
    const bool row1 = m_proxy->filterAcceptsRow(1, QModelIndex());

    // Assert
    EXPECT_EQ(false, row2);
    EXPECT_EQ(true, row1);
}

// ---------------------------------------------------------------------------
// B3: a row beyond the source model has no index and is rejected.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, FilterAcceptsRow_SourceIndexInvalid_ReturnsFalse)
{
    // Arrange
    setSource({0, 1, 2}, {10, 20, 30});
    m_proxy->setFilterRegularExpression(QRegularExpression("0,1,2"));

    // Act
    const bool beyond = m_proxy->filterAcceptsRow(100, QModelIndex());

    // Assert: invalid source index keeps ok=false even with a wide pattern
    EXPECT_EQ(false, beyond);
    EXPECT_EQ(true, m_proxy->filterAcceptsRow(2, QModelIndex()));
}

// ---------------------------------------------------------------------------
// B4: filterAcceptsColumn always delegates to the base implementation.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, FilterAcceptsColumn_AnyColumn_DelegatesToBase)
{
    // Arrange: no column filter is set, so the base accepts every column
    setSource({0, 1}, {10, 20});

    // Act
    const bool col0 = m_proxy->filterAcceptsColumn(CPUListModel::kPropCPUIndex, QModelIndex());
    const bool col1 = m_proxy->filterAcceptsColumn(CPUListModel::kPropCPUUsage, QModelIndex());

    // Assert
    EXPECT_EQ(true, col0);
    EXPECT_EQ(true, col1);
}

// ---------------------------------------------------------------------------
// B5: kSortByIndex compares the logical CPU indexes of the two rows.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, LessThan_SortByIndex_ComparesLogicalIndex)
{
    // Arrange: row order 5, 3 — descending by logical index
    setSource({5, 3}, {10, 20});
    m_proxy->setSortKey(CPUListSortFilterProxyModel::kSortByIndex);
    const QModelIndex left = m_source->index(0, CPUListModel::kPropCPUIndex);
    const QModelIndex right = m_source->index(1, CPUListModel::kPropCPUIndex);

    // Act
    const bool leftFirst = m_proxy->lessThan(left, right);
    const bool rightFirst = m_proxy->lessThan(right, left);

    // Assert: 5 < 3 is false, 3 < 5 is true
    EXPECT_EQ(false, leftFirst);
    EXPECT_EQ(true, rightFirst);
}

// ---------------------------------------------------------------------------
// B6: kSortByUsage compares the kValueRole usage ratios of the two rows.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, LessThan_SortByUsage_ComparesUsageRatio)
{
    // Arrange: row0 usage 75%, row1 usage 25%
    setSource({0, 1}, {75, 25});
    m_proxy->setSortKey(CPUListSortFilterProxyModel::kSortByUsage);
    const QModelIndex left = m_source->index(0, CPUListModel::kPropCPUUsage);
    const QModelIndex right = m_source->index(1, CPUListModel::kPropCPUUsage);

    // Act
    const bool lowFirst = m_proxy->lessThan(right, left);
    const bool highFirst = m_proxy->lessThan(left, right);

    // Assert: 0.25 < 0.75 is true, 0.75 < 0.25 is false
    EXPECT_EQ(true, lowFirst);
    EXPECT_EQ(false, highFirst);
}

// ---------------------------------------------------------------------------
// B7: an unknown sort key falls through to the base lessThan, which compares
// the DisplayRole payloads (the logical indexes as int variants).
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, LessThan_UnknownSortKey_DelegatesToBase)
{
    // Arrange: rows carry logical indexes 5 and 3
    setSource({5, 3}, {10, 20});
    m_proxy->setSortKey(CPUListSortFilterProxyModel::kSortKeyMax);
    const QModelIndex left = m_source->index(0, CPUListModel::kPropCPUIndex);
    const QModelIndex right = m_source->index(1, CPUListModel::kPropCPUIndex);

    // Act
    const bool leftFirst = m_proxy->lessThan(left, right);
    const bool rightFirst = m_proxy->lessThan(right, left);

    // Assert: base variant comparison of 5 vs 3
    EXPECT_EQ(false, leftFirst);
    EXPECT_EQ(true, rightFirst);
}

// ---------------------------------------------------------------------------
// Proxy mapping: rows accepted by a pattern covering every logical index map
// one-to-one to source rows with stable logical indexes. Note the override
// ignores the base empty-pattern behaviour: with no pattern set every row
// would be rejected, so the pattern must name each accepted CPU.
// ---------------------------------------------------------------------------
TEST_F(UT_CPUListSortFilterProxyModel, Construction_ProxyOverSource_MapsRowsUnchanged)
{
    // Arrange: an explicit pattern accepting all three logical indexes
    setSource({0, 1, 2}, {10, 20, 30});
    m_proxy->setFilterRegularExpression(QRegularExpression("0,1,2"));

    // Act
    const int proxyRows = m_proxy->rowCount();
    const QVariant idx0 = m_proxy->index(0, CPUListModel::kPropCPUIndex).data(Qt::DisplayRole);
    const QVariant idx2 = m_proxy->index(2, CPUListModel::kPropCPUIndex).data(Qt::DisplayRole);

    // Assert: no filter pattern set -> all rows pass through in order
    EXPECT_EQ(3, proxyRows);
    EXPECT_EQ(0, idx0.toInt());
    EXPECT_EQ(2, idx2.toInt());
}

} // namespace
