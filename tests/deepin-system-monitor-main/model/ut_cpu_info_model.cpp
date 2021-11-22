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
#include "model/cpu_info_model.h"
#include "system/cpu_set.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
/***************************************STUB begin*********************************************/
QList<QByteArray> stub_updateModel_cpuLogicName(){
    QList<QByteArray> cpuset;
    cpuset << "cpu0";
    return cpuset;
}
void stub_updateModel_addSample(){
    return;
}
/***************************************STUB end**********************************************/
class UT_CPUInfoModel: public ::testing::Test
{
public:
    UT_CPUInfoModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUInfoModel();
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUInfoModel *m_tester;
};

TEST_F(UT_CPUInfoModel, initTest)
{

}

TEST_F(UT_CPUInfoModel, test_instance_001)
{
    m_tester->instance();
}

TEST_F(UT_CPUInfoModel, test_uptime_001)
{
    m_tester->uptime();
}

TEST_F(UT_CPUInfoModel, test_sysInfo_001)
{
    m_tester->sysInfo();
}

TEST_F(UT_CPUInfoModel, test_cpuSet_001)
{
    m_tester->cpuSet();
}

TEST_F(UT_CPUInfoModel, test_updateModel_001)
{
    m_tester->updateModel();
}

TEST_F(UT_CPUInfoModel, test_cpuPercentList_001)
{
    m_tester->cpuPercentList();
}

TEST_F(UT_CPUInfoModel, test_cpuAllPercent_001)
{
    m_tester->cpuAllPercent();
}

TEST_F(UT_CPUInfoModel, test_loadavg_001)
{
    m_tester->loadavg();
}

TEST_F(UT_CPUInfoModel, test_nProcesses_001)
{
    m_tester->nProcesses();
}

TEST_F(UT_CPUInfoModel, test_nThreads_001)
{
    m_tester->nThreads();
}

TEST_F(UT_CPUInfoModel, test_nFileDescriptors_001)
{
    m_tester->nFileDescriptors();
}

TEST_F(UT_CPUInfoModel, test_hostname_001)
{
    m_tester->nFileDescriptors();
}

TEST_F(UT_CPUInfoModel, test_osType_001)
{
    m_tester->osType();
}

TEST_F(UT_CPUInfoModel, test_osVersion_001)
{
    m_tester->osVersion();
}
