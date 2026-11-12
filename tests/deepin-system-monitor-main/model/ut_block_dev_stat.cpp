// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for model::BlockDevStatModel
// (deepin-system-monitor-main/model/block_dev_stat_model.cpp) against
// upstream/master @ 00005c4e.
//
// The legacy half of this file carried the suite's review failure (numbered
// shell cases, no AAA markers, QModelIndex member stubbing that cannot bind
// in Qt6); that half is removed. The injection-driven tail survives with
// branch-mapped names and exact display/value expectations.
//
// Rows are driven through private-member injection (addr_pri.h): IO and IOPS
// sample frames are appended to the model's sample databases, so the suite is
// hermetic without stubbing and without real block devices.
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |BlockDevStatModel()|low|默认空样本库|1|1|
// |rowCount|mid|空库×等量样本×计数不一致|2|2|
// |columnCount|low|常量列数|1|1|
// |data|high|索引无效×行越界×4列×两角色组×default列|3|7|
// 说明: data() 内 if(io)/if(iops) 空指针分支在样本库计数相等时不可达
// （sample(row) 均返回有效帧），该分支标注为 injection-unreachable。
// 本 TU 共 11 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::BlockDevStatModel 全部方法
//       @ model/block_dev_stat_model.cpp:15-120
// ctor : m_ioSampleDB(new IOSample(period)), m_iopsSampleDB(new IOPSSample(period))
// B1   : rowCount -> iops.count()==io.count() ? iops.count() : 0
// B2   : columnCount -> kStatPropMax (常量 4)
// B3   : data !index.isValid() || 样本库为空 -> {}
// B4   : data row<0 || row>=iops.count() -> {}
// B5   : data Display/Accessible + 4 列 switch
//        (kStatTotalRead/Write -> formatUnit_memory_disk(io->data.*),
//         kStatReadSpeed/WriteSpeed -> formatUnit_memory_disk(iops->data.*,B,1,true))
// B6   : data kValueRole + 4 列 switch -> 原始 qulonglong/qreal
// B7   : data 其它角色 -> {}
// B8   : data default 列 -> {}
// 用例映射:
//   Construction_DefaultState_SampleDBsEmpty          -> ctor + B1(0) + B2
//   RowCount_EqualSampleCounts_ReturnsUsageCount      -> B1 (1/1)
//   RowCount_SampleCountMismatch_ReturnsZero          -> B1 (1/0)
//   ColumnCount_Always_ReturnsKStatPropMax            -> B2
//   Data_InvalidIndex_ReturnsInvalidVariant           -> B3
//   Data_RowOutOfBounds_ReturnsInvalidVariant         -> B4
//   Data_DisplayRoleAllColumns_ReturnsFormattedUnits  -> B5 (4 列)
//   Data_AccessibleTextRole_SharesDisplayBranch       -> B5
//   Data_ValueRoleAllColumns_ReturnsRawSampleValues   -> B6 (4 列)
//   Data_UnknownRole_ReturnsInvalidVariant            -> B7
//   Data_DefaultColumnSwitchBranch_ReturnsInvalid     -> B8
// ====================================================================

#include <gtest/gtest.h>

#include <QModelIndex>
#include <QVariant>

#include "model/block_dev_stat_model.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"

using namespace common::core;

using IOSampleDB = std::unique_ptr<IOSample>;
using IOPSSampleDB = std::unique_ptr<IOPSSample>;

ACCESS_PRIVATE_FIELD(BlockDevStatModel, IOSampleDB, m_ioSampleDB)
ACCESS_PRIVATE_FIELD(BlockDevStatModel, IOPSSampleDB, m_iopsSampleDB)

namespace {

// Append one IO + IOPS sample frame pair to the model's sample databases.
void injectIOSample(BlockDevStatModel *model, qulonglong inB, qulonglong outB,
                    qreal inBps, qreal outBps)
{
    timeval tv {1, 0};
    IO io {inB, outB};
    IOPS iops {inBps, outBps};
    auto &ioDB = access_private_field::BlockDevStatModelm_ioSampleDB(*model);
    auto &iopsDB = access_private_field::BlockDevStatModelm_iopsSampleDB(*model);
    ioDB->addSample(new IOSampleFrame(tv, io));
    iopsDB->addSample(new IOPSSampleFrame(iops));
}

class UT_BlockDevStatModel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Arrange: every case starts from a fresh model with empty sample DBs
        m_model.reset(new BlockDevStatModel(TimePeriod(), nullptr));
    }

    void TearDown() override
    {
        m_model.reset();
    }

    QScopedPointer<BlockDevStatModel> m_model;
};

// ---------------------------------------------------------------------------
// ctor + B1 + B2: a fresh model has zero rows and the constant column count.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Construction_DefaultState_SampleDBsEmpty)
{
    // Arrange: SetUp built the model with empty sample databases
    ASSERT_NE(m_model, nullptr);

    // Act
    const int rows = m_model->rowCount();
    const int cols = m_model->columnCount();

    // Assert
    EXPECT_EQ(0, rows);
    EXPECT_EQ(BlockDevStatModel::kStatPropMax, cols);
}

// ---------------------------------------------------------------------------
// B1 (equal counts): rowCount returns the sample count.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, RowCount_EqualSampleCounts_ReturnsUsageCount)
{
    // Arrange
    injectIOSample(m_model.data(), 1024, 2048, 100.0, 200.0);

    // Act
    const int rows = m_model->rowCount();

    // Assert
    EXPECT_EQ(1, rows);
    EXPECT_EQ(1, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B1 (mismatched counts): an IO frame without its IOPS twin collapses to zero.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, RowCount_SampleCountMismatch_ReturnsZero)
{
    // Arrange: append an IO frame but no matching IOPS frame
    timeval tv {1, 0};
    auto &ioDB = access_private_field::BlockDevStatModelm_ioSampleDB(*m_model);
    ioDB->addSample(new IOSampleFrame(tv, IO {1, 2}));
    ASSERT_EQ(1, int(access_private_field::BlockDevStatModelm_ioSampleDB(*m_model)->count()));
    ASSERT_EQ(0, int(access_private_field::BlockDevStatModelm_iopsSampleDB(*m_model)->count()));

    // Act
    const int rows = m_model->rowCount();

    // Assert
    EXPECT_EQ(0, rows);
    EXPECT_EQ(0, m_model->rowCount());
}

// ---------------------------------------------------------------------------
// B2: columnCount is the constant kStatPropMax regardless of content.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, ColumnCount_Always_ReturnsKStatPropMax)
{
    // Arrange
    injectIOSample(m_model.data(), 1, 2, 3.0, 4.0);

    // Act
    const int cols = m_model->columnCount();

    // Assert: kStatTotalRead/Write/ReadSpeed/WriteSpeed
    EXPECT_EQ(BlockDevStatModel::kStatPropMax, cols);
    EXPECT_EQ(4, cols);
}

// ---------------------------------------------------------------------------
// B3: an invalid index short-circuits before the sample lookups.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_InvalidIndex_ReturnsInvalidVariant)
{
    // Arrange
    injectIOSample(m_model.data(), 1, 2, 3.0, 4.0);
    const QModelIndex invalid;

    // Act
    const QVariant value = m_model->data(invalid, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B4: a row past the IOPS sample count is rejected.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_RowOutOfBounds_ReturnsInvalidVariant)
{
    // Arrange
    injectIOSample(m_model.data(), 1, 2, 3.0, 4.0);
    const QModelIndex tooFar = m_model->createIndex(100, BlockDevStatModel::kStatTotalRead);

    // Act
    const QVariant value = m_model->data(tooFar, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

// ---------------------------------------------------------------------------
// B5: all four columns render their sample fields through formatUnit.
// 1024 B stays "1024.0 B"; 2048 B scales to "2.0 KB"; speeds get the "/s".
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_DisplayRoleAllColumns_ReturnsFormattedUnits)
{
    // Arrange
    injectIOSample(m_model.data(), 1024, 2048, 100.0, 200.0);
    ASSERT_EQ(1, m_model->rowCount());

    // Act: render the four columns under DisplayRole
    const QVariant read = m_model->data(m_model->index(0, BlockDevStatModel::kStatTotalRead), Qt::DisplayRole);
    const QVariant write = m_model->data(m_model->index(0, BlockDevStatModel::kStatTotalWrite), Qt::DisplayRole);
    const QVariant readSpeed = m_model->data(m_model->index(0, BlockDevStatModel::kStatReadSpeed), Qt::DisplayRole);
    const QVariant writeSpeed = m_model->data(m_model->index(0, BlockDevStatModel::kStatWriteSpeed), Qt::DisplayRole);

    // Assert: exact formatted strings for the injected frame
    EXPECT_EQ(QString("1024.0 B"), read.toString());
    EXPECT_EQ(QString("2.0 KB"), write.toString());
    EXPECT_EQ(QString("100.0 B/s"), readSpeed.toString());
    EXPECT_EQ(QString("200.0 B/s"), writeSpeed.toString());
}

// ---------------------------------------------------------------------------
// B5: AccessibleTextRole shares the display switch.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_AccessibleTextRole_SharesDisplayBranch)
{
    // Arrange
    injectIOSample(m_model.data(), 100, 200, 1.0, 2.0);
    const QModelIndex idx = m_model->index(0, BlockDevStatModel::kStatTotalRead);

    // Act
    const QVariant accessible = m_model->data(idx, Qt::AccessibleTextRole);
    const QVariant display = m_model->data(idx, Qt::DisplayRole);

    // Assert: identical rendering across the two roles
    EXPECT_EQ(display.toString(), accessible.toString());
    EXPECT_EQ(QString("100.0 B"), accessible.toString());
}

// ---------------------------------------------------------------------------
// B6: kValueRole returns the raw numeric sample fields.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_ValueRoleAllColumns_ReturnsRawSampleValues)
{
    // Arrange
    injectIOSample(m_model.data(), 4096, 8192, 50.5, 75.25);
    ASSERT_EQ(1, m_model->rowCount());

    // Act
    const QVariant read = m_model->data(m_model->index(0, BlockDevStatModel::kStatTotalRead), BlockDevStatModel::kValueRole);
    const QVariant write = m_model->data(m_model->index(0, BlockDevStatModel::kStatTotalWrite), BlockDevStatModel::kValueRole);
    const QVariant readSpeed = m_model->data(m_model->index(0, BlockDevStatModel::kStatReadSpeed), BlockDevStatModel::kValueRole);
    const QVariant writeSpeed = m_model->data(m_model->index(0, BlockDevStatModel::kStatWriteSpeed), BlockDevStatModel::kValueRole);

    // Assert: byte counters as u64, bandwidths as doubles
    EXPECT_EQ(4096ull, read.toULongLong());
    EXPECT_EQ(8192ull, write.toULongLong());
    EXPECT_DOUBLE_EQ(50.5, readSpeed.toDouble());
    EXPECT_DOUBLE_EQ(75.25, writeSpeed.toDouble());
}

// ---------------------------------------------------------------------------
// B7: roles outside the display/value pair return an invalid variant.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_UnknownRole_ReturnsInvalidVariant)
{
    // Arrange
    injectIOSample(m_model.data(), 1, 2, 3.0, 4.0);
    const QModelIndex idx = m_model->index(0, BlockDevStatModel::kStatTotalRead);

    // Act
    const QVariant font = m_model->data(idx, Qt::FontRole);

    // Assert: neither display nor value branch matches the role
    EXPECT_FALSE(font.isValid());
    EXPECT_EQ(QVariant(), font);
}

// ---------------------------------------------------------------------------
// B8: a column at kStatPropMax falls through the display switch.
// ---------------------------------------------------------------------------
TEST_F(UT_BlockDevStatModel, Data_DefaultColumnSwitchBranch_ReturnsInvalid)
{
    // Arrange
    injectIOSample(m_model.data(), 1, 2, 3.0, 4.0);
    const QModelIndex defaultCol = m_model->createIndex(0, BlockDevStatModel::kStatPropMax);

    // Act
    const QVariant value = m_model->data(defaultCol, Qt::DisplayRole);

    // Assert
    EXPECT_FALSE(value.isValid());
    EXPECT_EQ(QVariant(), value);
}

} // namespace
