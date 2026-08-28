// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "process/process_set.h"
#include "process/process_db.h"
#include "process/private/process_p.h"
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
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }

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

TEST_F(UT_ProcessSet, test_mergeSubProcMemory_001)
{
    Process root(101);
    root.d->rss = 13;
    Process child1(102);
    child1.d->rss = 17;
    Process child2(103);
    child2.d->rss = 20;

    m_tester->m_set.insert(101, root);
    m_tester->m_set.insert(102, child1);
    m_tester->m_set.insert(103, child2);
    m_tester->m_pidPtoCMapping.insert(101, 102);
    m_tester->m_pidPtoCMapping.insert(101, 103);

    qulonglong memory = 0;
    m_tester->mergeSubProcMemory(101, memory);

    EXPECT_EQ(memory, 50U);
}

TEST_F(UT_ProcessSet, test_aggregateProcessGroup_001)
{
    Process representative(101);
    representative.setCpu(1);
    representative.setNetIoBps(2, 3);
    representative.d->rss = 13;

    Process child1(102);
    child1.setCpu(2);
    child1.setNetIoBps(4, 5);
    child1.d->rss = 17;

    Process child2(103);
    child2.setCpu(3);
    child2.setNetIoBps(6, 7);
    child2.d->rss = 20;

    m_tester->m_set.insert(representative.pid(), representative);
    m_tester->m_set.insert(child1.pid(), child1);
    m_tester->m_set.insert(child2.pid(), child2);

    m_tester->aggregateProcessGroup(representative.pid(), {101, 102, 103, 103});

    const Process result = m_tester->m_set.value(representative.pid());
    EXPECT_DOUBLE_EQ(result.cpu(), 1);
    EXPECT_DOUBLE_EQ(result.recvBps(), 2);
    EXPECT_DOUBLE_EQ(result.sentBps(), 3);
    EXPECT_EQ(result.memory(), 13U);

    const QMap<pid_t, ApplicationResource> resources = m_tester->getApplicationResources();
    ASSERT_TRUE(resources.contains(representative.pid()));
    const ApplicationResource resource = resources.value(representative.pid());
    EXPECT_DOUBLE_EQ(resource.cpu, 6);
    EXPECT_DOUBLE_EQ(resource.recvBps, 12);
    EXPECT_DOUBLE_EQ(resource.sentBps, 15);
    EXPECT_EQ(resource.memory, 50U);
}

TEST_F(UT_ProcessSet, test_collapseDesktopLaunchGroups_001)
{
    WMWindowList windowList;
    windowList.m_guiAppcache.emplace(102, WMWindow(new wm_window_t()));

    for (pid_t pid : {101, 102, 103}) {
        Process proc(pid);
        proc.d->ppid = 1;
        proc.d->uid = geteuid();
        proc.d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE", "/tmp/wxwork.desktop");
        proc.d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "100");
        proc.setAppType(kFilterApps);
        m_tester->m_set.insert(pid, proc);
    }

    Process separateLaunch(104);
    separateLaunch.d->ppid = 1;
    separateLaunch.d->uid = geteuid();
    separateLaunch.d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE", "/tmp/wxwork.desktop");
    separateLaunch.d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "200");
    separateLaunch.setAppType(kFilterApps);
    m_tester->m_set.insert(separateLaunch.pid(), separateLaunch);

    const QMap<pid_t, QList<pid_t>> groups =
            m_tester->collapseDesktopLaunchGroups(&windowList, geteuid());

    ASSERT_EQ(groups.size(), 1);
    ASSERT_TRUE(groups.contains(102));
    EXPECT_EQ(groups.value(102).size(), 3);
    EXPECT_FALSE(groups.value(102).contains(104));
    EXPECT_EQ(m_tester->m_set.value(102).appType(), kFilterApps);
    EXPECT_EQ(m_tester->m_set.value(101).appType(), kFilterCurrentUser);
    EXPECT_EQ(m_tester->m_set.value(103).appType(), kFilterCurrentUser);
}

TEST_F(UT_ProcessSet, test_collapseDesktopLaunchGroups_collapsesDuplicateAppsInSingleTree)
{
    WMWindowList windowList;
    windowList.m_guiAppcache.emplace(202, WMWindow(new wm_window_t()));
    Process root(201);
    root.d->ppid = 1;
    root.d->uid = geteuid();
    Process child(202);
    child.d->ppid = 201;
    child.d->uid = geteuid();

    for (Process *proc : {&root, &child}) {
        proc->d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE", "/tmp/app.desktop");
        proc->setAppType(kFilterApps);
        m_tester->m_set.insert(proc->pid(), *proc);
    }

    // An incomplete launch identity must never be used for grouping.
    EXPECT_TRUE(m_tester->collapseDesktopLaunchGroups(&windowList, geteuid()).isEmpty());

    m_tester->m_set[201].d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "200");
    m_tester->m_set[202].d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "200");
    m_tester->m_pidPtoCMapping.insert(201, 202);

    const QMap<pid_t, QList<pid_t>> groups =
            m_tester->collapseDesktopLaunchGroups(&windowList, geteuid());

    ASSERT_EQ(groups.size(), 1);
    ASSERT_TRUE(groups.contains(202));
    EXPECT_EQ(groups.value(202), QList<pid_t>({201, 202}));
    EXPECT_EQ(m_tester->m_set.value(201).appType(), kFilterCurrentUser);
    EXPECT_EQ(m_tester->m_set.value(202).appType(), kFilterApps);
}

TEST_F(UT_ProcessSet, test_collapseDesktopLaunchGroups_keepsOneAppCandidateInSingleTree)
{
    WMWindowList windowList;
    Process root(201);
    root.d->ppid = 1;
    root.d->uid = geteuid();
    root.setAppType(kFilterApps);
    Process child(202);
    child.d->ppid = 201;
    child.d->uid = geteuid();
    child.setAppType(kFilterCurrentUser);

    for (Process *proc : {&root, &child}) {
        proc->d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE", "/tmp/app.desktop");
        proc->d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "200");
        m_tester->m_set.insert(proc->pid(), *proc);
    }
    m_tester->m_pidPtoCMapping.insert(201, 202);

    EXPECT_TRUE(m_tester->collapseDesktopLaunchGroups(&windowList, geteuid()).isEmpty());
    EXPECT_EQ(m_tester->m_set.value(201).appType(), kFilterApps);
}

TEST_F(UT_ProcessSet, test_collapseDesktopLaunchGroups_includesDescendantsWithoutLaunchIdentity)
{
    WMWindowList windowList;
    Process root1(301);
    root1.d->ppid = 1;
    root1.d->uid = geteuid();
    root1.d->rss = 13;
    root1.setCpu(1);
    root1.setNetIoBps(2, 3);
    root1.setAppType(kFilterApps);

    Process root2(302);
    root2.d->ppid = 1;
    root2.d->uid = geteuid();
    root2.d->rss = 17;
    root2.setCpu(2);
    root2.setNetIoBps(4, 5);
    root2.setAppType(kFilterCurrentUser);

    for (Process *proc : {&root1, &root2}) {
        proc->d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE", "/tmp/app.desktop");
        proc->d->environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "300");
    }

    Process child(303);
    child.d->ppid = 301;
    child.d->uid = geteuid();
    child.d->rss = 20;
    child.setCpu(3);
    child.setNetIoBps(6, 7);
    child.setAppType(kFilterCurrentUser);

    m_tester->m_set.insert(root1.pid(), root1);
    m_tester->m_set.insert(root2.pid(), root2);
    m_tester->m_set.insert(child.pid(), child);
    m_tester->m_pidPtoCMapping.insert(root1.pid(), child.pid());

    const QMap<pid_t, QList<pid_t>> groups =
            m_tester->collapseDesktopLaunchGroups(&windowList, geteuid());

    ASSERT_EQ(groups.size(), 1);
    ASSERT_TRUE(groups.contains(root1.pid()));
    EXPECT_EQ(groups.value(root1.pid()), QList<pid_t>({301, 302, 303}));

    m_tester->aggregateProcessGroup(root1.pid(), groups.value(root1.pid()));
    const QMap<pid_t, ApplicationResource> resources = m_tester->getApplicationResources();
    ASSERT_TRUE(resources.contains(root1.pid()));
    const ApplicationResource resource = resources.value(root1.pid());
    EXPECT_DOUBLE_EQ(resource.cpu, 6);
    EXPECT_DOUBLE_EQ(resource.recvBps, 12);
    EXPECT_DOUBLE_EQ(resource.sentBps, 15);
    EXPECT_EQ(resource.memory, 50U);
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
