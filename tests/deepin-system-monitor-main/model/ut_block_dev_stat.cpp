// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/block_dev_stat_model.h"
#include "common/time_period.h"
#include "common/sample.h"
#include "addr_pri.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QDebug>
#include <QModelIndex>
#include <QVariant>
using namespace common::core;

// 注入私有 IOSample/IOPSSample，使 data() 命中真实的取值分支
using IOSampleDB = std::unique_ptr<IOSample>;
using IOPSSampleDB = std::unique_ptr<IOPSSample>;
ACCESS_PRIVATE_FIELD(BlockDevStatModel, IOSampleDB, m_ioSampleDB)
ACCESS_PRIVATE_FIELD(BlockDevStatModel, IOPSSampleDB, m_iopsSampleDB)

static QString m_Sresult;
/***************************************STUB begin*********************************************/
bool stub_data_isValid(){
    m_Sresult = "index is valid";
    return true;
}

int stub_data_row(){

    return 0;
}

int stub_data_count() {
    m_Sresult = "sample count";
    return 1;
}

int stub_data_column1()
{
    m_Sresult = "index column kStatTotalRead";
    return BlockDevStatModel::kStatTotalRead;
}

int stub_data_column2()
{
    m_Sresult = "index column kStatTotalWrite";
    return BlockDevStatModel::kStatTotalWrite;
}

int stub_data_column3()
{
    m_Sresult = "index column kStatReadSpeed";
    return BlockDevStatModel::kStatReadSpeed;
}

int stub_data_column4()
{
    m_Sresult = "index column kStatWriteSpeed";
    return BlockDevStatModel::kStatWriteSpeed;
}

/***************************************STUB end**********************************************/
class UT_BlockDevStatModel: public ::testing::Test
{
public:
    UT_BlockDevStatModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        TimePeriod period(TimePeriod::kNoPeriod, {2, 0});
        m_tester = new BlockDevStatModel(period,nullptr);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDevStatModel *m_tester;
};

TEST_F(UT_BlockDevStatModel, initTest)
{

}

TEST_F(UT_BlockDevStatModel, test_rowCount_001)
{
    int count = m_tester->rowCount();
    EXPECT_EQ(count,0);
}

TEST_F(UT_BlockDevStatModel, test_rowCount_002)
{
    Stub b1;
    b1.set(ADDR(IOSample,count),stub_data_count);
    Stub b2;
    b1.set(ADDR(IOPSSample,count),stub_data_count);
    m_tester->rowCount();
}

TEST_F(UT_BlockDevStatModel, test_columnCount_001)
{
    int count = m_tester->columnCount();
    EXPECT_EQ(count,BlockDevStatModel::kStatPropMax);
}

TEST_F(UT_BlockDevStatModel, test_data_001)
{
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_002)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_003)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column1);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_004)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column2);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_005)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column3);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_006)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);  
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column4);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,Qt::DisplayRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_007)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column1);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,BlockDevStatModel::kValueRole);
    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_008)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column2);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,BlockDevStatModel::kValueRole);
    delete index;
}


TEST_F(UT_BlockDevStatModel, test_data_009)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column3);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,BlockDevStatModel::kValueRole);

    delete index;
}

TEST_F(UT_BlockDevStatModel, test_data_010)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_data_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_data_row);
    Stub b3;
    b3.set(ADDR(Sample<IOPS>,count),stub_data_count);
    Stub b4;
    b4.set(ADDR(QModelIndex,column),stub_data_column4);
    QModelIndex *index = new QModelIndex();
    m_tester->data(*index,BlockDevStatModel::kValueRole);

    delete index;
}

// ========== 新增：注入真实 IOSample/IOPSSample，覆盖 data() 真实取值分支 ==========

// 辅助：注入一帧 IO + IOPS 样本
static void injectIOSample(BlockDevStatModel *m, qulonglong inB, qulonglong outB,
                            qreal inBps, qreal outBps)
{
    timeval tv {1, 0};
    IO io {inB, outB};
    IOPS iops {inBps, outBps};
    auto &ioDB = access_private_field::BlockDevStatModelm_ioSampleDB(*m);
    auto &iopsDB = access_private_field::BlockDevStatModelm_iopsSampleDB(*m);
    ioDB->addSample(new IOSampleFrame(tv, io));
    iopsDB->addSample(new IOPSSampleFrame(iops));
}

TEST_F(UT_BlockDevStatModel, test_data_displayRole_allColumns_withRealSamples)
{
    injectIOSample(m_tester, 1024, 2048, 100.0, 200.0);
    ASSERT_EQ(m_tester->rowCount(), 1);

    struct Col { int col; };
    Col cols[] = {
        {BlockDevStatModel::kStatTotalRead},
        {BlockDevStatModel::kStatTotalWrite},
        {BlockDevStatModel::kStatReadSpeed},
        {BlockDevStatModel::kStatWriteSpeed},
    };
    for (auto &c : cols) {
        QModelIndex idx = m_tester->index(0, c.col);
        QVariant v = m_tester->data(idx, Qt::DisplayRole);
        EXPECT_TRUE(v.isValid()) << "col=" << c.col;
    }
}

TEST_F(UT_BlockDevStatModel, test_data_valueRole_allColumns_withRealSamples)
{
    injectIOSample(m_tester, 4096, 8192, 50.5, 75.25);
    ASSERT_EQ(m_tester->rowCount(), 1);

    QModelIndex idxR = m_tester->index(0, BlockDevStatModel::kStatTotalRead);
    EXPECT_EQ(m_tester->data(idxR, BlockDevStatModel::kValueRole).toULongLong(), 4096ull);
    QModelIndex idxW = m_tester->index(0, BlockDevStatModel::kStatTotalWrite);
    EXPECT_EQ(m_tester->data(idxW, BlockDevStatModel::kValueRole).toULongLong(), 8192ull);
    QModelIndex idxRS = m_tester->index(0, BlockDevStatModel::kStatReadSpeed);
    EXPECT_DOUBLE_EQ(m_tester->data(idxRS, BlockDevStatModel::kValueRole).toDouble(), 50.5);
    QModelIndex idxWS = m_tester->index(0, BlockDevStatModel::kStatWriteSpeed);
    EXPECT_DOUBLE_EQ(m_tester->data(idxWS, BlockDevStatModel::kValueRole).toDouble(), 75.25);
}

TEST_F(UT_BlockDevStatModel, test_data_AccessibleTextRole_withRealSamples)
{
    injectIOSample(m_tester, 100, 200, 1.0, 2.0);
    QModelIndex idx = m_tester->index(0, BlockDevStatModel::kStatTotalRead);
    QVariant v = m_tester->data(idx, Qt::AccessibleTextRole);
    EXPECT_TRUE(v.isValid());
}

TEST_F(UT_BlockDevStatModel, test_data_defaultColumnBranch)
{
    // kStatPropMax 命中 switch default 分支
    injectIOSample(m_tester, 1, 2, 3.0, 4.0);
    QModelIndex idx = m_tester->createIndex(0, BlockDevStatModel::kStatPropMax);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_BlockDevStatModel, test_data_otherRoleReturnsInvalid)
{
    injectIOSample(m_tester, 1, 2, 3.0, 4.0);
    QModelIndex idx = m_tester->index(0, BlockDevStatModel::kStatTotalRead);
    QVariant v = m_tester->data(idx, Qt::FontRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_BlockDevStatModel, test_data_rowOutOfBounds_withRealSamples)
{
    injectIOSample(m_tester, 1, 2, 3.0, 4.0);
    QModelIndex idx = m_tester->createIndex(100, 0);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_BlockDevStatModel, test_rowCount_countMismatchReturnsZero)
{
    // 只注入 IO 不注入 IOPS，count 不一致 → 0
    timeval tv {1, 0};
    IO io {1, 2};
    auto &ioDB = access_private_field::BlockDevStatModelm_ioSampleDB(*m_tester);
    ioDB->addSample(new IOSampleFrame(tv, io));
    EXPECT_EQ(m_tester->rowCount(), 0);
}

