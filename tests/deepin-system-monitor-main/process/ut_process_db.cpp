// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "process/process_db.h"
#include "process/process_set.h"
#include "process/priority_controller.h"
#include "process/private/process_p.h"
#include "process/desktop_entry_cache.h"
#include "wm/wm_window_list.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::process;
static QString m_Sresult;
/***************************************STUB begin*********************************************/
pid_t stub_isCurrentProcess_getpid(){
    return 50000;
}

void stub_endProcess_sendSignalToProcess(){
    m_Sresult = "endProcess signal";
    return;
}

void stub_pauseProcess_sendSignalToProcess(){
    m_Sresult = "pauseProcess signal";
    return;
}

void stub_resumeProcess_sendSignalToProcess(){
    m_Sresult = "resumeProcess signal";
    return;
}

void stub_killProcess_sendSignalToProcess(){
    m_Sresult = "killProcess signal";
    return;
}

void stub_update_updateCache(){
    return ;
}

void stub_update_updateWindowListCache(){
    return;
}

void stub_update_refresh(){
    m_Sresult = "update";
    return;
}

void stub_setProcessPriority_signalProcessPrioritysetChangede(){
        return;
}

void stub_onProcessPrioritysetChanged_processPriorityChanged(){
        return;
}

void stub_onProcessPrioritysetChanged_processControlResultReady(){
        return;
}

void stub_onProcessPrioritysetChanged_priorityPromoteResultReady(){
        return;
}

void stub_onProcessPrioritysetChanged_execute(){
        return;
}
/***************************************STUB end**********************************************/


class UT_ProcessDB : public ::testing::Test
{
public:
    UT_ProcessDB() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessDB();
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessDB *m_tester;
};

TEST_F(UT_ProcessDB, initTest)
{

}

TEST_F(UT_ProcessDB, test_instance_001)
{
    ProcessDB::instance();
}

TEST_F(UT_ProcessDB, test_processEuid_001)
{
    uid_t processEuid = m_tester->processEuid();
    EXPECT_EQ(processEuid,m_tester->m_euid);
}

TEST_F(UT_ProcessDB, test_processSet_001)
{
    ProcessSet* processSet = m_tester->processSet();
    QList<pid_t> list = processSet->getPIDList();
    EXPECT_EQ(list,m_tester->m_procSet->getPIDList());
}

TEST_F(UT_ProcessDB, test_isCurrentProcess_001)
{
    pid_t pid = 50000;
    Stub b;
    b.set(getpid,stub_isCurrentProcess_getpid);
    bool isCurrentProcess = m_tester->isCurrentProcess(pid);

    EXPECT_TRUE(isCurrentProcess);
}

TEST_F(UT_ProcessDB, test_endProcess_001)
{
    pid_t pid = 50000;
    Stub stub;
    stub.set(ADDR(ProcessDB,sendSignalToProcess), stub_endProcess_sendSignalToProcess);

    m_tester->endProcess(pid);
    EXPECT_EQ(m_Sresult,"endProcess signal");
}

TEST_F(UT_ProcessDB, test_pauseProcess_001)
{
    pid_t pid = 50000;
    Stub stub;
    stub.set(ADDR(ProcessDB,sendSignalToProcess), stub_pauseProcess_sendSignalToProcess);

    m_tester->pauseProcess(pid);
    EXPECT_EQ(m_Sresult,"pauseProcess signal");
}

TEST_F(UT_ProcessDB, test_resumeProcess_001)
{
    pid_t pid = 50000;
    Stub stub;
    stub.set(ADDR(ProcessDB,sendSignalToProcess), stub_resumeProcess_sendSignalToProcess);

    m_tester->resumeProcess(pid);
    EXPECT_EQ(m_Sresult,"resumeProcess signal");
}

TEST_F(UT_ProcessDB, test_killProcess_001)
{
    pid_t pid = 50000;
    Stub stub;
    stub.set(ADDR(ProcessDB,sendSignalToProcess), stub_killProcess_sendSignalToProcess);

    m_tester->resumeProcess(pid);
    EXPECT_EQ(m_Sresult,"killProcess signal");
}

TEST_F(UT_ProcessDB, test_update_001)
{
    Stub b1;
    b1.set(ADDR(DesktopEntryCache,updateCache), stub_update_updateCache);
    Stub b2;
    b1.set(ADDR(WMWindowList,updateWindowListCache), stub_update_updateWindowListCache);
    Stub b3;
    b3.set(ADDR(ProcessSet,refresh), stub_update_refresh);

    m_tester->update();
    EXPECT_EQ(m_Sresult,"update");
}

TEST_F(UT_ProcessDB, test_setProcessPriority_001)
{
    Stub b1;
    b1.set(ADDR(ProcessDB,signalProcessPrioritysetChanged), stub_setProcessPriority_signalProcessPrioritysetChangede);
    m_tester->setProcessPriority(50000,20);
}

TEST_F(UT_ProcessDB, test_onProcessPrioritysetChanged_001)
{
    Stub b1;
    b1.set(ADDR(ProcessDB,processPriorityChanged), stub_onProcessPrioritysetChanged_processPriorityChanged);
    Stub b2;
    b2.set(ADDR(ProcessDB,processControlResultReady), stub_onProcessPrioritysetChanged_processControlResultReady);
    Stub b3;
    b3.set(ADDR(ProcessDB,priorityPromoteResultReady), stub_onProcessPrioritysetChanged_priorityPromoteResultReady);
    Stub b4;
    b4.set(ADDR(PriorityController,execute),stub_onProcessPrioritysetChanged_execute);

    m_tester->onProcessPrioritysetChanged(50000,20);
}

TEST_F(UT_ProcessDB, test_sendSignalToProcess_001)
{
    m_tester->sendSignalToProcess(100000,SIGCONT);
}

// 扩展 sendSignalToProcess 测试
TEST_F(UT_ProcessDB, test_sendSignalToProcess_SIGTERM)
{
    // 测试 SIGTERM 信号（会先发 SIGCONT）
    m_tester->sendSignalToProcess(100000, SIGTERM);
}

TEST_F(UT_ProcessDB, test_sendSignalToProcess_SIGKILL)
{
    // 测试 SIGKILL 信号（会先发 SIGCONT）
    m_tester->sendSignalToProcess(100000, SIGKILL);
}

TEST_F(UT_ProcessDB, test_sendSignalToProcess_SIGSTOP)
{
    // 测试 SIGSTOP 信号
    m_tester->sendSignalToProcess(100000, SIGSTOP);
}

TEST_F(UT_ProcessDB, test_windowList)
{
    // 测试 windowList() 方法
    auto* wl = m_tester->windowList();
    EXPECT_NE(wl, nullptr);
}

TEST_F(UT_ProcessDB, test_desktopEntryCache)
{
    // 测试 desktopEntryCache() 方法
    auto* cache = m_tester->desktopEntryCache();
    EXPECT_NE(cache, nullptr);
}

TEST_F(UT_ProcessDB, test_onProcessPrioritysetChanged_priorityChange)
{
    // 测试 onProcessPrioritysetChanged 的优先级设置分支
    // sched_priority == 0 (dynamic priority) 时走 setpriority 分支
    Stub b1;
    b1.set(ADDR(ProcessDB, processPriorityChanged), stub_onProcessPrioritysetChanged_processPriorityChanged);
    Stub b2;
    b2.set(ADDR(ProcessDB, processControlResultReady), stub_onProcessPrioritysetChanged_processControlResultReady);

    m_tester->onProcessPrioritysetChanged(1, 0);  // PID 1 通常有 dynamic priority
}

TEST_F(UT_ProcessDB, test_onProcessPrioritysetChanged_permissionDenied)
{
    // 测试 onProcessPrioritysetChanged 权限不足时的分支
    Stub b1;
    b1.set(ADDR(ProcessDB, priorityPromoteResultReady), stub_onProcessPrioritysetChanged_priorityPromoteResultReady);
    Stub b2;
    b2.set(ADDR(PriorityController, execute), stub_onProcessPrioritysetChanged_execute);

    // 使用无效 PID 触发权限错误分支
    m_tester->onProcessPrioritysetChanged(1, 19);  // 最高优先级
}

