/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     lishiqi <lishiqi@uniontech.com>
*
* Maintainer: lishiqi  <lishiqi@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//self
#include "model/cpu_list_model.h"
#include "model/cpu_stat_model.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QMap>

static QString m_Sresult;
using myMap = QMap<QByteArray, std::shared_ptr<CPUStatModel>> ;
/***************************************STUB begin*********************************************/
bool stub_cpudata_isValid(){
    m_Sresult = "index is valid";
    return true;
}

int stub_cpudata_row(){

    return 0;
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

TEST_F(UT_CPUListModel, test_data_003)
{
    Stub b1;
    b1.set(ADDR(QModelIndex,isValid),stub_cpudata_isValid);
    Stub b2;
    b2.set(ADDR(QModelIndex,row),stub_cpudata_row);
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
