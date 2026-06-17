// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/cpu_stat_model.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QModelIndex>
#include <QVariant>

using namespace common::core;
using namespace common::time;

// ACCESS_PRIVATE_FIELD 注入：直接操作 CPUStatModel 的私有 unique_ptr 成员，
// 用于向 statSampleDB / usageSampleDB 注入真实 SampleFrame，避免现有测试因
// sample(row) 返回 nullptr 而无法命中 data() switch 分支。
ACCESS_PRIVATE_FIELD(CPUStatModel, std::unique_ptr<Sample<cpu_stat_t>>, m_statSampleDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, std::unique_ptr<Sample<cpu_usage_t>>, m_usageSampleDB)

static QString m_Sresult;
/***************************************STUB begin*********************************************/
bool stub_cpustatdata_isValid(){
    m_Sresult = "index is valid";
    return true;
}

int stub_cpustatdata_row(){

    return 0;
}

int stub_cpustatdata_column1()
{
    m_Sresult = "index column kStatTotalRead";
    return CPUStatModel::kStatUserTime;
}

int stub_cpustatdata_column2()
{
    m_Sresult = "index column kPropCPUUsage";
    return CPUStatModel::kStatNiceTime;
}

int stub_cpustatdata_count(){
    return 1;
}

/***************************************STUB end**********************************************/
class UT_CPUStatModel: public ::testing::Test
{
public:
    UT_CPUStatModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        TimePeriod time {};
        m_tester = new CPUStatModel(time);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUStatModel *m_tester;
};

TEST_F(UT_CPUStatModel, initTest)
{

}

TEST_F(UT_CPUStatModel, test_rowCount_001)
{
    m_tester->rowCount();

}

TEST_F(UT_CPUStatModel, test_columnCount_001)
{
    int expect = m_tester->columnCount();
    EXPECT_EQ(expect,CPUStatModel::kStatPropMax);
}

TEST_F(UT_CPUStatModel, test_data_001)
{
    QModelIndex *index = new QModelIndex;
    int role = Qt::DisplayRole;

    m_tester->data(*index,role);

    delete index;

}

TEST_F(UT_CPUStatModel, test_data_002)
{
    QModelIndex *index = new QModelIndex;
    int role = Qt::DisplayRole;

    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpustatdata_isValid);

    m_tester->data(*index,role);
    delete index;

}

TEST_F(UT_CPUStatModel, test_data_003)
{
    QModelIndex *index = new QModelIndex;
    int role = Qt::DisplayRole;

    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpustatdata_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_cpustatdata_row);
    Stub b3;
    b3.set(ADDR(Sample<cpu_usage_t>,count),stub_cpustatdata_count);

    m_tester->data(*index,role);
    delete index;

}



TEST_F(UT_CPUStatModel, test_data_004)
{
    QModelIndex *index = new QModelIndex;
    int role = Qt::DisplayRole;

    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpustatdata_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_cpustatdata_row);
    Stub b3;
    b3.set(ADDR(Sample<cpu_usage_t>,count),stub_cpustatdata_count);

    m_tester->data(*index,role);
    delete index;

}

// 扩展测试：测试所有列枚举值
TEST_F(UT_CPUStatModel, test_columnEnums)
{
    // 测试 kStatPropMax 值
    EXPECT_EQ(CPUStatModel::kStatPropMax, 12);

    // 测试各枚举值存在
    EXPECT_EQ(CPUStatModel::kStatUserTime, 0);
    EXPECT_EQ(CPUStatModel::kStatNiceTime, 1);
    EXPECT_EQ(CPUStatModel::kStatSystemTime, 2);
    EXPECT_EQ(CPUStatModel::kStatIdleTime, 3);
    EXPECT_EQ(CPUStatModel::kStatIOWaitTime, 4);
    EXPECT_EQ(CPUStatModel::kStatHardIRQ, 5);
    EXPECT_EQ(CPUStatModel::kStatSoftIRQ, 6);
    EXPECT_EQ(CPUStatModel::kStatStealTime, 7);
    EXPECT_EQ(CPUStatModel::kStatGuestTime, 8);
    EXPECT_EQ(CPUStatModel::kStatGuestNice, 9);
    EXPECT_EQ(CPUStatModel::kStatTotalUsage, 10);
    EXPECT_EQ(CPUStatModel::kStatIdleUsage, 11);
}

TEST_F(UT_CPUStatModel, test_data_invalidIndex)
{
    // 测试无效索引
    QModelIndex invalidIndex;
    EXPECT_FALSE(invalidIndex.isValid());

    // 用无效索引调用 data
    QVariant result = m_tester->data(invalidIndex, Qt::DisplayRole);
    EXPECT_FALSE(result.isValid());
}

TEST_F(UT_CPUStatModel, test_data_outOfBounds)
{
    // 测试超出范围的行索引
    QModelIndex outOfBoundsIndex = m_tester->index(1000, 0);
    QVariant result = m_tester->data(outOfBoundsIndex, Qt::DisplayRole);
    EXPECT_FALSE(result.isValid());
}

TEST_F(UT_CPUStatModel, test_data_negativeRow)
{
    // 测试负数行索引
    QModelIndex negativeIndex = m_tester->index(-1, 0);
    QVariant result = m_tester->data(negativeIndex, Qt::DisplayRole);
    EXPECT_FALSE(result.isValid());
}

TEST_F(UT_CPUStatModel, test_data_AccessibleTextRole)
{
    // 测试 AccessibleTextRole
    QModelIndex index = m_tester->index(0, CPUStatModel::kStatUserTime);
    if (index.isValid() || m_tester->rowCount() > 0) {
        QVariant result = m_tester->data(index, Qt::AccessibleTextRole);
        // 即使结果无效，也覆盖了代码路径
    }
}

TEST_F(UT_CPUStatModel, test_rowCount_noData)
{
    // 在没有数据的情况下测试 rowCount
    TimePeriod time {};
    CPUStatModel model(time);
    // 如果 statSampleDB 和 usageSampleDB 都为空或计数不同，返回 0
    int rows = model.rowCount();
    EXPECT_GE(rows, 0);
}

// ========== 新增：注入真实 Sample，覆盖 data() switch 全部分支 ==========

// 辅助：向 m_tester 注入一帧 stat/usage 样本
static void injectSample(CPUStatModel *m, const cpu_stat_t &stVal, const cpu_usage_t &usVal)
{
    timeval tv {1, 0};
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*m);
    auto &usageDB = access_private_field::CPUStatModelm_usageSampleDB(*m);
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(stVal))));
    usageDB->addSample(new SampleFrame<cpu_usage_t>(tv, CPUUsage(std::make_shared<cpu_usage_t>(usVal))));
}

TEST_F(UT_CPUStatModel, test_data_allStatColumns_DisplayRole)
{
    cpu_stat_t st {};
    st.user = 11; st.nice = 12; st.sys = 13; st.idle = 14; st.iowait = 15;
    st.hardirq = 16; st.softirq = 17; st.steal = 18; st.guest = 19; st.guest_nice = 20;
    cpu_usage_t us {};
    us.total = 100; us.idle = 30;
    injectSample(m_tester, st, us);

    ASSERT_EQ(m_tester->rowCount(), 1);
    struct Col { int col; qulonglong expect; };
    Col cols[] = {
        {CPUStatModel::kStatUserTime, 11},   {CPUStatModel::kStatNiceTime, 12},
        {CPUStatModel::kStatSystemTime, 13}, {CPUStatModel::kStatIdleTime, 14},
        {CPUStatModel::kStatIOWaitTime, 15}, {CPUStatModel::kStatHardIRQ, 16},
        {CPUStatModel::kStatSoftIRQ, 17},    {CPUStatModel::kStatStealTime, 18},
        {CPUStatModel::kStatGuestTime, 19},  {CPUStatModel::kStatGuestNice, 20},
        {CPUStatModel::kStatTotalUsage, 100},{CPUStatModel::kStatIdleUsage, 30},
    };
    for (auto &c : cols) {
        QModelIndex idx = m_tester->index(0, c.col);
        QVariant v = m_tester->data(idx, Qt::DisplayRole);
        EXPECT_TRUE(v.isValid()) << "col=" << c.col;
        EXPECT_EQ(v.toULongLong(), c.expect) << "col=" << c.col;
    }
}

TEST_F(UT_CPUStatModel, test_data_allStatColumns_AccessibleTextRole)
{
    // AccessibleTextRole 与 DisplayRole 走同一 switch 分支，确保覆盖
    cpu_stat_t st {};
    st.user = 7; st.nice = 8; st.sys = 9;
    cpu_usage_t us {};
    us.total = 50; us.idle = 10;
    injectSample(m_tester, st, us);

    QModelIndex idx = m_tester->index(0, CPUStatModel::kStatUserTime);
    QVariant v = m_tester->data(idx, Qt::AccessibleTextRole);
    EXPECT_EQ(v.toULongLong(), 7ull);
}

TEST_F(UT_CPUStatModel, test_data_otherRolesReturnInvalid)
{
    // 非显示/Accessible 角色，data() 应直接返回空 QVariant
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_tester, st, us);

    QModelIndex idx = m_tester->index(0, CPUStatModel::kStatUserTime);
    QVariant v = m_tester->data(idx, Qt::TextAlignmentRole);
    EXPECT_FALSE(v.isValid());
    v = m_tester->data(idx, Qt::FontRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_CPUStatModel, test_data_defaultColumnSwitchBranch)
{
    // 列号落在 switch 的 default 分支（kStatPropMax 本身）
    cpu_stat_t st {};
    cpu_usage_t us {};
    injectSample(m_tester, st, us);

    QModelIndex idx = m_tester->createIndex(0, CPUStatModel::kStatPropMax);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_CPUStatModel, test_rowCount_countMismatchReturnsZero)
{
    // stat/usage 样本数不一致 → rowCount 返回 0
    cpu_stat_t st {};
    cpu_usage_t us {};
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*m_tester);
    timeval tv {1, 0};
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(st))));
    // usageDB 仍为空
    EXPECT_EQ(m_tester->rowCount(), 0);
}


