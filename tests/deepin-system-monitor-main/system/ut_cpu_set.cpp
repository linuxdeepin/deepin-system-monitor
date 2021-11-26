/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      baohaifeng <baohaifeng@uniontech.com>
* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//self
#include "system/cpu_set.h"
#include "system/cpu.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QString>
#include <QFile>
#include <QMap>

using namespace core::system;

/***************************************STUB begin*********************************************/

FILE *stub_fopen (const char *__restrict __filename,
 const char *__restrict __modes)
{
    return nullptr;
}

/***************************************STUB end**********************************************/


class UT_CPUSet : public ::testing::Test
{
public:
    UT_CPUSet() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUSet();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUSet *m_tester;
};

TEST_F(UT_CPUSet, initTest)
{
}

TEST_F(UT_CPUSet, test_copy)
{
    CPUSet copy(*m_tester);
}

TEST_F(UT_CPUSet, test_copy_02)
{
    CPUSet copy1;
    CPUSet copy2 = copy1;
}

TEST_F(UT_CPUSet, test_modelName)
{
    m_tester->update();
    QString retString = m_tester->modelName();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_vendor)
{
    m_tester->update();
    QString retString = m_tester->vendor();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_cpuCount)
{
    m_tester->update();
    int iCpuCount = m_tester->cpuCount();
    EXPECT_NE(iCpuCount, 0);
}

TEST_F(UT_CPUSet, test_socketCount)
{
    m_tester->update();
    int iSocketCount = m_tester->socketCount();
    EXPECT_NE(iSocketCount, 0);
}

TEST_F(UT_CPUSet, test_virtualization)
{
    m_tester->update();
    QString retString = m_tester->virtualization();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_curFreq)
{
    m_tester->update();
    QString retString = m_tester->curFreq();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_minFreq)
{
    m_tester->update();
    QString retString = m_tester->minFreq();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_maxFreq)
{
    m_tester->update();
    QString retString = m_tester->maxFreq();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_l1dCache)
{
    m_tester->update();
    QString retString = m_tester->l1dCache();
    EXPECT_NE(retString, "");
}


TEST_F(UT_CPUSet, test_l1iCache)
{
    m_tester->update();
    QString retString = m_tester->l1iCache();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_l2Cache)
{
    m_tester->update();
    QString retString = m_tester->l2Cache();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_l3Cache)
{
    m_tester->update();
    QString retString = m_tester->l3Cache();
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_coreId)
{
    m_tester->update();
    QString retString = m_tester->coreId(1);
    EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_usage)
{
    m_tester->update();
    core::system::CPUUsage Usage = m_tester->usage();
    EXPECT_NE(Usage->total, 1);
}

TEST_F(UT_CPUSet, test_stat)
{
    m_tester->update();
    core::system::CPUStat stat = m_tester->stat();
    EXPECT_NE(stat->idle, 0);
}

TEST_F(UT_CPUSet, test_cpuLogicName)
{
    m_tester->update();
    for (auto &cpuname : m_tester->cpuLogicName())
    {
        //cpu逻辑名不能为空
//        qInfo()<<"cpuname:"<<cpuname;
        EXPECT_NE(cpuname, "");
    }
}

TEST_F(UT_CPUSet, test_statDB)
{
    m_tester->update();
    QByteArray  TEST_F = "cpu0";
    core::system::CPUStat stat = m_tester->statDB(TEST_F);
    qInfo()<<"stat->user:"<<stat->user;
    EXPECT_NE(stat->user, 0);
}

TEST_F(UT_CPUSet, test_usageDB)
{
    m_tester->update();
    QByteArray  TEST_F = "cpu0";
    core::system::CPUUsage cPUUsage = m_tester->usageDB(TEST_F);
    qInfo()<<"cPUUsage->total:"<<cPUUsage->total;
    EXPECT_NE(cPUUsage->total, 0);
}

TEST_F(UT_CPUSet, test_update)
{
    m_tester->update();
    core::system::CPUStat stat = m_tester->stat();
    EXPECT_NE(stat->idle, 0);
}

TEST_F(UT_CPUSet, test_read_stats)
{
    m_tester->read_stats();
    core::system::CPUStat stat = m_tester->stat();
    EXPECT_NE(stat->idle, 0);
}

TEST_F(UT_CPUSet, test_read_stats_02)
{
    Stub stub;
    stub.set(fopen, stub_fopen);
    m_tester->read_stats();
    core::system::CPUStat stat = m_tester->stat();
    EXPECT_NE(stat->idle, -1);
}

TEST_F(UT_CPUSet, test_read_overall_info)
{
    m_tester->read_overall_info();
    QString retString = m_tester->l1iCache();
    EXPECT_NE(retString, "");
}


TEST_F(UT_CPUSet, test_read_lscpu_01)
{
    m_tester->read_lscpu();
    QString retString = m_tester->vendor();
    EXPECT_NE(retString, "");
}

 bool stub_isEmpty()
 {
     return true;
 }

TEST_F(UT_CPUSet, test_read_lscpu_02)
{
    Stub stub;
    stub.set(ADDR(QString, isEmpty), stub_isEmpty);
    m_tester->read_lscpu();
    QString retString = m_tester->vendor();
    EXPECT_NE(retString, "");
}

bool stub_contains()
{
    return false;
}

TEST_F(UT_CPUSet, test_read_lscpu_03)
{
//   Stub stub;
//   stub.set(ADDR(QMap, contains), stub_contains);
//   m_tester->read_lscpu();
//   QString retString = m_tester->vendor();
//   EXPECT_NE(retString, "");
}

TEST_F(UT_CPUSet, test_getUsageTotalDelta)
{
    qulonglong totalDelta = m_tester->getUsageTotalDelta();
    EXPECT_NE(totalDelta, 0);
}
