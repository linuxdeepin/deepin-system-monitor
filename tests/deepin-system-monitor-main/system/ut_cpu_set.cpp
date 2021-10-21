/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
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
#include "system/cpu_set.h"
#include "system/cpu.h"
#include <QString>

#include "ut_cpu_set.h"

Ut_CPUSet::Ut_CPUSet()
{
}

TEST(UT_CPUSet_modelName, UT_CPUSet_modelName_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->modelName();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_vendor, UT_CPUSet_vendor_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->vendor();
    EXPECT_NE(retString, "");
    delete cpuSet;
}

TEST(UT_CPUSet_cpuCount, UT_CPUSet_cpuCount_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    int iCpuCount = cpuSet->cpuCount();
    EXPECT_NE(iCpuCount, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_socketCount, UT_CPUSet_socketCount_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    int iSocketCount = cpuSet->socketCount();
    EXPECT_NE(iSocketCount, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_virtualization, UT_CPUSet_virtualization_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->virtualization();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_curFreq, UT_CPUSet_curFreq_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->curFreq();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_minFreq, UT_CPUSet_minFreq_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->minFreq();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_maxFreq, UT_CPUSet_maxFreq_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->maxFreq();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_l1dCache, UT_CPUSet_l1dCache_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->l1dCache();
    EXPECT_NE(retString, "");

    delete cpuSet;
}


TEST(UT_CPUSet_l1iCache, UT_CPUSet_l1iCache_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->l1iCache();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_l2Cache, UT_CPUSet_l2Cache_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->l2Cache();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_l3Cache, UT_CPUSet_l3Cache_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->l3Cache();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_coreId, UT_CPUSet_coreId_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QString retString = cpuSet->coreId(1);
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_usage, UT_CPUSet_usage_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    core::system::CPUUsage Usage = cpuSet->usage();
    EXPECT_NE(Usage->total, 1);

    delete cpuSet;
}

TEST(UT_CPUSet_stat, UT_CPUSet_stat_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    core::system::CPUStat stat = cpuSet->stat();
    EXPECT_NE(stat->idle, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_cpuLogicName, UT_CPUSet_cpuLogicName_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    for (auto &cpuname : cpuSet->cpuLogicName())
    {
        //cpu逻辑名不能为空
//        qInfo()<<"cpuname:"<<cpuname;
        EXPECT_NE(cpuname, "");
    }

    delete cpuSet;
}

TEST(UT_CPUSet_statDB, UT_CPUSet_statDB_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QByteArray  test = "cpu0";
    core::system::CPUStat stat = cpuSet->statDB(test);
    qInfo()<<"stat->user:"<<stat->user;
    EXPECT_NE(stat->user, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_usageDB, UT_CPUSet_usageDB_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    QByteArray  test = "cpu0";
    core::system::CPUUsage cPUUsage = cpuSet->usageDB(test);
    qInfo()<<"cPUUsage->total:"<<cPUUsage->total;
    EXPECT_NE(cPUUsage->total, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_update, UT_CPUSet_update_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->update();
    core::system::CPUStat stat = cpuSet->stat();
    EXPECT_NE(stat->idle, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_read_stats, UT_CPUSet_read_stats_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->read_stats();

    core::system::CPUStat stat = cpuSet->stat();
    EXPECT_NE(stat->idle, 0);

    delete cpuSet;
}

TEST(UT_CPUSet_read_overall_info, UT_CPUSet_read_overall_info_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->read_overall_info();

    QString retString = cpuSet->l1iCache();
    EXPECT_NE(retString, "");

    delete cpuSet;
}


TEST(UT_CPUSet_read_lscpu, UT_CPUSet_read_lscpu_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    cpuSet->read_lscpu();

    QString retString = cpuSet->vendor();
    EXPECT_NE(retString, "");

    delete cpuSet;
}

TEST(UT_CPUSet_getUsageTotalDelta, UT_CPUSet_getUsageTotalDelta_001)
{
    core::system::CPUSet *cpuSet = new core::system::CPUSet();
    qulonglong totalDelta = cpuSet->getUsageTotalDelta();
    EXPECT_NE(totalDelta, 0);

    delete cpuSet;
}
