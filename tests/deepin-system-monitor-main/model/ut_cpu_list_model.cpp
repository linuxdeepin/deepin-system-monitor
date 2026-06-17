// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/cpu_list_model.h"
#include "model/cpu_stat_model.h"
#include "common/sample.h"
#include "common/time_period.h"
#include "addr_pri.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QMap>
#include <QModelIndex>
#include <QVariant>
#include <cmath>

using namespace common::core;
using namespace common::time;

// typedef 避免宏参数中的逗号
using CPUStatModelDB = QMap<QByteArray, std::shared_ptr<CPUStatModel>>;
using StatSampleDB = std::unique_ptr<Sample<cpu_stat_t>>;
using UsageSampleDB = std::unique_ptr<Sample<cpu_usage_t>>;

// 注入私有成员以驱动 data() 的 kPropCPUIndex / kPropCPUUsage 分支
ACCESS_PRIVATE_FIELD(CPUListModel, QList<CPUInfo>, m_cpuInfoDB)
ACCESS_PRIVATE_FIELD(CPUListModel, CPUStatModelDB, m_statModelDB)
ACCESS_PRIVATE_FIELD(CPUListModel, int, m_nr)
ACCESS_PRIVATE_FIELD(CPUStatModel, StatSampleDB, m_statSampleDB)
ACCESS_PRIVATE_FIELD(CPUStatModel, UsageSampleDB, m_usageSampleDB)

static QString m_Sresult;
using myMap = QMap<QByteArray, std::shared_ptr<CPUStatModel>> ;
/***************************************STUB begin*********************************************/
bool stub_cpudata_isValid(){
    m_Sresult = "index is valid";
    return true;
}

int stub_cpudata_row(){

    return 1;
}

int stub_cpudata_column1()
{
    m_Sresult = "index column kStatTotalRead";
    return CPUListModel::kPropCPUIndex;
}

int stub_cpudata_column2()
{
    m_Sresult = "index column kPropCPUUsage";
    return CPUListModel::kPropCPUUsage;
}

int stub_cpudata_count(){
    return 1;
}
/***************************************STUB end**********************************************/
class UT_CPUListModel: public ::testing::Test
{
public:
    UT_CPUListModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUListModel(nullptr);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUListModel *m_tester;
};

TEST_F(UT_CPUListModel, initTest)
{

}

TEST_F(UT_CPUListModel, test_rowCount_001)
{
    m_tester->rowCount();

}

TEST_F(UT_CPUListModel, test_columnCount_001)
{
    int count = m_tester->columnCount();
    EXPECT_EQ(count,CPUListModel::kPropCount);

}

TEST_F(UT_CPUListModel, test_data_001)
{
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}


TEST_F(UT_CPUListModel, test_data_002)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpudata_isValid);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_CPUListModel, test_fetchMore_001)
{
    QModelIndex *index = new QModelIndex();
    m_tester->fetchMore(*index);
    delete index;
}

TEST_F(UT_CPUListModel, test_fetchMore_002)
{
    QModelIndex *index = new QModelIndex();
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpudata_isValid);
    m_tester->fetchMore(*index);
    delete index;
}

TEST_F(UT_CPUListModel, test_canFetchMore_001)
{
    QModelIndex *index = new QModelIndex();
    m_tester->canFetchMore(*index);
    delete index;
}

TEST_F(UT_CPUListModel, test_canFetchMore_002)
{
    QModelIndex *index = new QModelIndex();
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpudata_isValid);
    m_tester->canFetchMore(*index);
    delete index;
}

TEST_F(UT_CPUListModel, test_statModel_001)
{
    QByteArray cpu="cpu0";
    m_tester->statModel(cpu);
}

// 扩展测试
TEST_F(UT_CPUListModel, test_statModel_multiple)
{
    // 测试获取多个CPU的statModel
    m_tester->statModel("cpu0");
    m_tester->statModel("cpu1");
    m_tester->statModel("cpu2");
    m_tester->statModel("");  // 空名称
}

TEST_F(UT_CPUListModel, test_columnEnums)
{
    // 测试列枚举值
    EXPECT_EQ(CPUListModel::kPropCPUIndex, 0);
    EXPECT_EQ(CPUListModel::kPropCPUUsage, 1);
    EXPECT_EQ(CPUListModel::kPropCount, 2);
}

TEST_F(UT_CPUListModel, test_data_invalidIndex)
{
    // 测试无效索引
    QModelIndex invalid;
    QVariant result = m_tester->data(invalid, Qt::DisplayRole);
    EXPECT_FALSE(result.isValid());
}

TEST_F(UT_CPUListModel, test_data_UserRole)
{
    // 测试 UserRole
    Stub b1;
    b1.set(ADDR(QModelIndex, isValid), stub_cpudata_isValid);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUListModel, test_fetchMore_withValidIndex)
{
    // 测试带有有效索引的fetchMore
    Stub b1;
    b1.set(ADDR(QModelIndex, isValid), stub_cpudata_isValid);
    QModelIndex index;
    m_tester->fetchMore(index);
}

TEST_F(UT_CPUListModel, test_rowCount_withData)
{
    // 多次调用rowCount
    m_tester->rowCount();
    m_tester->rowCount();
    m_tester->rowCount();
}

TEST_F(UT_CPUListModel, test_columnCount_multiple)
{
    // 多次调用columnCount
    m_tester->columnCount();
    m_tester->columnCount();
}

// ========== 新增：注入真实 CPUInfo + CPUStatModel，覆盖 data() switch 与 fetchMore/canFetchMore ==========

// 辅助：注入一帧 stat 样本到 statModel 的 stat/usage DB
static void injectStatModelSample(CPUStatModel *m)
{
    timeval tv {1, 0};
    cpu_stat_t st {};
    cpu_usage_t us {};
    us.total = 100; us.idle = 25;
    auto &statDB = access_private_field::CPUStatModelm_statSampleDB(*m);
    auto &usageDB = access_private_field::CPUStatModelm_usageSampleDB(*m);
    statDB->addSample(new SampleFrame<cpu_stat_t>(tv, CPUStat(std::make_shared<cpu_stat_t>(st))));
    usageDB->addSample(new SampleFrame<cpu_usage_t>(tv, CPUUsage(std::make_shared<cpu_usage_t>(us))));
}

TEST_F(UT_CPUListModel, test_data_displayRole_CPUIndex_withInjectedCPU)
{
    // 注入一帧 CPUInfo + 对应 statModel，覆盖 kPropCPUIndex 分支
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    CPUInfo cpu(3);
    cpuInfoDB.append(cpu);
    // data() 的行越界检查基于 m_statModelDB.count()，必须同步非空
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(TimePeriod(TimePeriod::kNoPeriod, {2, 0})));

    QModelIndex idx = m_tester->index(0, CPUListModel::kPropCPUIndex);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(v.toInt(), 3);
}

TEST_F(UT_CPUListModel, test_data_displayRole_CPUUsage_withInjectedData)
{
    // 注入 CPUInfo + CPUStatModel，覆盖 kPropCPUUsage 计算分支
    // CPUInfo 默认 logicalName() 为空串 ""，故 statModelDB 用 "" 作为 key
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    CPUInfo cpu(0);
    cpuInfoDB.append(cpu);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    auto statModel = std::make_shared<CPUStatModel>(period);
    injectStatModelSample(statModel.get());
    statModelDB.insert("", statModel);

    QModelIndex idx = m_tester->index(0, CPUListModel::kPropCPUUsage);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_TRUE(v.isValid());
    EXPECT_TRUE(v.toString().contains('%'));
}

TEST_F(UT_CPUListModel, test_data_valueRole_CPUUsage)
{
    // 覆盖 kValueRole 分支
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    CPUInfo cpu(0);
    cpuInfoDB.append(cpu);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    auto statModel = std::make_shared<CPUStatModel>(period);
    injectStatModelSample(statModel.get());
    statModelDB.insert("", statModel);

    QModelIndex idx = m_tester->index(0, CPUListModel::kPropCPUUsage);
    QVariant v = m_tester->data(idx, CPUListModel::kValueRole);
    EXPECT_TRUE(v.isValid());
    // 注意：CPUListModel::data 内部以 kStatTotalUsage(=10) 作为 role 传给
    // CPUStatModel::data，而后者只认 DisplayRole(0)/AccessibleTextRole(11)，
    // 故 total/idle 取到 0，结果为 nan。这是源码既有行为，测试只断言分支被执行。
    EXPECT_TRUE(std::isnan(v.toDouble()));
}

TEST_F(UT_CPUListModel, test_data_valueRole_CPUIndex_returnsInvalid)
{
    // kValueRole 下 kPropCPUIndex 命中 default 分支，返回无效
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(TimePeriod(TimePeriod::kNoPeriod, {2, 0})));

    QModelIndex idx = m_tester->index(0, CPUListModel::kPropCPUIndex);
    QVariant v = m_tester->data(idx, CPUListModel::kValueRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_CPUListModel, test_data_displayRole_defaultColumn)
{
    // DisplayRole + 列号落在 default 分支
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    cpuInfoDB.append(CPUInfo(0));
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(TimePeriod(TimePeriod::kNoPeriod, {2, 0})));

    QModelIndex idx = m_tester->createIndex(0, CPUListModel::kPropCount);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_CPUListModel, test_data_rowOutOfBounds)
{
    auto &cpuInfoDB = access_private_field::CPUListModelm_cpuInfoDB(*m_tester);
    cpuInfoDB.append(CPUInfo(0));
    // statModelDB 为空 → row 0 也越界

    QModelIndex idx = m_tester->createIndex(100, 0);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_CPUListModel, test_fetchMore_actuallyFetches)
{
    // fetchMore 的 left = m_statModelDB.count() - m_nr，需要 statModelDB 非空
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    for (int i = 0; i < 40; ++i) {
        statModelDB.insert(QByteArray("cpu" + QByteArray::number(i)),
                           std::make_shared<CPUStatModel>(period));
    }

    QModelIndex parent;
    m_tester->fetchMore(parent);
    int nr = access_private_field::CPUListModelm_nr(*m_tester);
    EXPECT_EQ(nr, 32);

    m_tester->fetchMore(parent);
    nr = access_private_field::CPUListModelm_nr(*m_tester);
    EXPECT_EQ(nr, 40);

    m_tester->fetchMore(parent);
    nr = access_private_field::CPUListModelm_nr(*m_tester);
    EXPECT_EQ(nr, 40);
}

TEST_F(UT_CPUListModel, test_canFetchMore_withData)
{
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(period));
    statModelDB.insert("cpu1", std::make_shared<CPUStatModel>(period));

    QModelIndex parent;
    EXPECT_TRUE(m_tester->canFetchMore(parent));

    access_private_field::CPUListModelm_nr(*m_tester) = 2;
    EXPECT_FALSE(m_tester->canFetchMore(parent));
}

TEST_F(UT_CPUListModel, test_rowCount_withInjectedCPUs)
{
    auto &statModelDB = access_private_field::CPUListModelm_statModelDB(*m_tester);
    TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
    statModelDB.insert("cpu0", std::make_shared<CPUStatModel>(period));
    statModelDB.insert("cpu1", std::make_shared<CPUStatModel>(period));
    statModelDB.insert("cpu2", std::make_shared<CPUStatModel>(period));
    EXPECT_EQ(m_tester->rowCount(), 3);
}

