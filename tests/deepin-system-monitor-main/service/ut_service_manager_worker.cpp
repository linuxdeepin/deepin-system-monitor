/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     xuezifan<xuezifan@uniontech.com>
*
* Maintainer: xuezifan<xuezifan@uniontech.com>
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
#include "service/service_manager_worker.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

static QString m_Sresult;
/***************************************STUB begin*********************************************/

int stub_readUnitDescriptionFromUnitFile_open1(){
    m_Sresult = "open success";
    return 1;
}

int stub_readUnitDescriptionFromUnitFile_open2(){
    m_Sresult = "open failed";
    return -1;
}

ssize_t stub_readUnitDescriptionFromUnitFile_read1(){
    m_Sresult = "read success";
    return 1;
}

ssize_t stub_readUnitDescriptionFromUnitFile_read2(){
    m_Sresult = "read failed";
    return -1;
}

int stub_readUnitDescriptionFromUnitFile_close(){
    m_Sresult = "close";
    return 1;
}

/***************************************STUB end**********************************************/

class UT_ServiceManagerWorker : public ::testing::Test
{
public:
    UT_ServiceManagerWorker() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ServiceManagerWorker();
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    ServiceManagerWorker *m_tester;
};

TEST_F(UT_ServiceManagerWorker, initTest)
{

}

TEST_F(UT_ServiceManagerWorker, test_startJob_01)
{
    m_tester->startJob();
}
