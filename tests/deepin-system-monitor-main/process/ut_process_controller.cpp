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
#include "process/process_controller.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QIODevice>

static QString m_Sresult;
/***************************************STUB begin*********************************************/

void stub_prio_execute_start(){
    m_Sresult = "start";
    return;
}

/***************************************STUB end**********************************************/

class UT_ProcessController : public ::testing::Test
{
public:
    UT_ProcessController() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        pid_t pid = getpid();
        int signal = SIGCONT;
        m_tester = new ProcessController(pid,signal);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessController *m_tester;
};

TEST_F(UT_ProcessController, initTest)
{

}

TEST_F(UT_ProcessController, test_execute_001)
{
    
}
