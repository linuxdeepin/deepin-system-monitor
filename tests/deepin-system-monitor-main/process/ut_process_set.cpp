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
#include "process/process_set.h"
#include "process/process_db.h"
#include "common/common.h"
#include "wm/wm_window_list.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::process;
/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/
class UT_ProcessSet : public ::testing::Test
{
public:
    UT_ProcessSet() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessSet();
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    ProcessSet *m_tester;

};

TEST_F(UT_ProcessSet, initTest)
{

}

TEST_F(UT_ProcessSet, test_mergeSubProcNetIO_001)
{
    pid_t ppid = getppid();
    qreal recvBps = 100;
    qreal sendBps = 100;
    m_tester->mergeSubProcNetIO(ppid,recvBps,sendBps);

}

TEST_F(UT_ProcessSet, test_mergeSubProcCpu_001)
{
    pid_t ppid = getppid();
    qreal cpu = 0;
    m_tester->mergeSubProcCpu(ppid,cpu);

}

TEST_F(UT_ProcessSet, test_refresh_001)
{
    m_tester->refresh();
}

TEST_F(UT_ProcessSet, test_scanProcess_001)
{
    Process *proc = new Process;
    m_tester->m_set.insert(proc->pid(),*proc);
    m_tester->scanProcess();
    delete proc;
}

TEST_F(UT_ProcessSet, test_hasNext_001)
{
    ProcessSet::Iterator *it = new ProcessSet::Iterator();
    it->hasNext();
    delete it;
}

TEST_F(UT_ProcessSet, test_next_001)
{
    ProcessSet::Iterator *it = new ProcessSet::Iterator();
    it->next();
    delete it;
}

TEST_F(UT_ProcessSet, test_advance_001)
{
    ProcessSet::Iterator *it = new ProcessSet::Iterator();
    it->advance();
    delete it;
}

TEST_F(UT_ProcessSet, test_getRecentProcStage_001)
{
    pid_t pid = getpid();
    m_tester->getRecentProcStage(pid);

}

TEST_F(UT_ProcessSet, test_getProcessById_001)
{
    pid_t pid = getpid();
    m_tester->getProcessById(pid);

}

TEST_F(UT_ProcessSet, test_getPIDList_001)
{
    m_tester->getPIDList();

}

TEST_F(UT_ProcessSet, test_removeProcess_001)
{
    pid_t pid = getpid();
    m_tester->removeProcess(pid);
}

TEST_F(UT_ProcessSet, test_updateProcessState_001)
{
    pid_t pid = getpid();
    m_tester->updateProcessState(pid,'Z');
}

TEST_F(UT_ProcessSet, test_updateProcessPriority_001)
{
    pid_t pid = getpid();
    m_tester->updateProcessPriority(pid,0);
}
