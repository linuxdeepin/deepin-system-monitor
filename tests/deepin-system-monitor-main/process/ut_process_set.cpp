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
    // Exercise the full scan with a deterministic process snapshot.
    QList<pid_t> scanProcesses(const QList<pid_t> &pids,
                              const QList<pid_t> &guiPids = {})
    {
        ProcessDB db;
        for (pid_t pid : guiPids)
            db.windowList()->m_guiAppcache.emplace(pid, WMWindow(new wm_window_t()));
        static ProcessDB *database;
        static QList<pid_t> pending, simpleReads;
        database = &db;
        pending = pids;
        simpleReads.clear();
        Stub stub;
        stub.set(&ProcessDB::instance, +[]() { return database; });
        stub.set(ADDR(ProcessSet::Iterator, hasNext),
                 +[](ProcessSet::Iterator *) { return !pending.isEmpty(); });
        stub.set(ADDR(ProcessSet::Iterator, next),
                 +[](ProcessSet::Iterator *) { return Process(pending.takeFirst()); });
        stub.set(ADDR(Process, readProcessSimpleInfo), +[](Process *proc) {
            simpleReads.append(proc->pid());
            proc->d->valid = true;
        });
        stub.set(ADDR(Process, readProcessVariableInfo), +[](Process *proc) {
            proc->d->valid = true;
        });
        m_tester->scanProcess();
        return simpleReads;
    }

    Process addProcess(pid_t pid, pid_t ppid, const char *name,
                       int type = kFilterApps,
                       const QString &prefix = "/home/test/.deepinwine/example",
                       const QString &package = "com.example.wine")
    {
        Process proc(pid);
        proc.d->ppid = ppid;
        proc.d->uid = geteuid();
        proc.d->name = name;
        proc.d->cmdline << name;
        proc.d->environ.insert("WINEPREFIX", prefix);
        proc.d->environ.insert("DEB_PACKAGE_NAME", package);
        proc.setAppType(type);
        m_tester->m_simpleSet.insert(pid, proc);
        m_tester->m_set.insert(pid, proc);
        m_tester->m_pidPtoCMapping.insert(ppid, pid);
        m_tester->m_pidCtoPMapping.insert(pid, ppid);
        return proc;
    }

    ProcessSet *m_tester;

};

TEST_F(UT_ProcessSet, initTest)
{

}

TEST_F(UT_ProcessSet, test_mergeSubProcResources_missingProcess)
{
    qreal cpu = 0, recvBps = 0, sendBps = 0;
    m_tester->mergeSubProcResources(101, cpu, recvBps, sendBps);
    EXPECT_DOUBLE_EQ(cpu, 0);
    EXPECT_DOUBLE_EQ(recvBps, 0);
    EXPECT_DOUBLE_EQ(sendBps, 0);
    EXPECT_TRUE(m_tester->m_set.isEmpty());
}

TEST_F(UT_ProcessSet, test_mergeSubProcResources_countsEachPidOnce)
{
    Process parent = addProcess(101, 1, "parent");
    Process child = addProcess(102, 101, "child", kFilterCurrentUser);
    parent.setCpu(1);
    parent.setNetIoBps(2, 3);
    child.setCpu(4);
    child.setNetIoBps(5, 6);
    m_tester->m_pidPtoCMapping.insert(101, 102);
    m_tester->m_pidPtoCMapping.insert(102, 101);

    qreal cpu = 0, recvBps = 0, sendBps = 0;
    m_tester->mergeSubProcResources(101, cpu, recvBps, sendBps);
    EXPECT_DOUBLE_EQ(cpu, 5);
    EXPECT_DOUBLE_EQ(recvBps, 7);
    EXPECT_DOUBLE_EQ(sendBps, 9);
}

TEST_F(UT_ProcessSet, test_collapseWineContainerGroups_001)
{
    WMWindowList windowList;
    windowList.m_guiAppcache.emplace(102, WMWindow(new wm_window_t()));

    addProcess(103, 1, "C:\\Program Files\\Example\\helper.exe");
    addProcess(101, 1, "C:\\Program Files\\Example\\example.exe");
    addProcess(102, 101, "C:\\windows\\system32\\services.exe");

    const QMap<pid_t, QList<pid_t>> groups =
            m_tester->collapseWineContainerGroups(&windowList, geteuid());

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups.value(102), QList<pid_t>({101, 102, 103}));
    EXPECT_EQ(m_tester->m_set.value(102).appType(), kFilterApps);
    EXPECT_EQ(m_tester->m_set.value(101).appType(), kFilterCurrentUser);
    EXPECT_EQ(m_tester->m_simpleSet.value(101).appType(), kFilterApps);
    EXPECT_EQ(m_tester->m_set.value(102).ppid(), 101);
    EXPECT_EQ(m_tester->m_pidCtoPMapping.value(102), 101);
    EXPECT_FALSE(m_tester->m_pidPtoCMapping.contains(101, 102));
    EXPECT_TRUE(m_tester->m_pidPtoCMapping.contains(102, 101));
}

TEST_F(UT_ProcessSet, test_collapseWineContainerGroups_excludesNativeProcess)
{
    WMWindowList windowList;
    windowList.m_guiAppcache.emplace(102, WMWindow(new wm_window_t()));

    addProcess(101, 1, "C:\\Program Files\\Example\\example.exe");
    addProcess(102, 1, "C:\\windows\\system32\\services.exe");
    addProcess(104, 1, "wineserver");
    Process native = addProcess(103, 1, "/usr/bin/dde-file-manager");
    native.d->cmdline << "/tmp/setup.exe";

    const QMap<pid_t, QList<pid_t>> groups =
            m_tester->collapseWineContainerGroups(&windowList, geteuid());

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups.value(102), QList<pid_t>({101, 102, 104}));
    EXPECT_EQ(m_tester->m_set.value(103).appType(), kFilterApps);
}

TEST_F(UT_ProcessSet, test_collapseWineContainerGroups_keepsHelperResourcesAndMemory)
{
    WMWindowList windowList;
    Process parent = addProcess(100, 1, "native-launcher");
    Process representative = addProcess(101, 100, "main.exe");
    Process server = addProcess(102, 1, "wineserver", kFilterCurrentUser);
    Process helper = addProcess(103, 102, "native-helper", kFilterCurrentUser);
    parent.setCpu(1);
    parent.setNetIoBps(1, 1);
    representative.setCpu(2);
    representative.setNetIoBps(2, 2);
    representative.d->rss = 13;
    representative.d->shm = 3;
    server.setCpu(3);
    server.setNetIoBps(3, 3);
    helper.setCpu(4);
    helper.setNetIoBps(4, 4);

    const auto groups = m_tester->collapseWineContainerGroups(&windowList, geteuid());
    ASSERT_EQ(groups.size(), 1);
    ASSERT_TRUE(groups.contains(101));
    qreal cpu = 0, recvBps = 0, sendBps = 0;
    m_tester->mergeSubProcResources(101, cpu, recvBps, sendBps);
    EXPECT_DOUBLE_EQ(cpu, 9);
    EXPECT_DOUBLE_EQ(recvBps, 9);
    EXPECT_DOUBLE_EQ(sendBps, 9);
    // Apply the same setters as scanProcess: memory must stay per-PID.
    representative.setCpu(cpu);
    representative.setNetIoBps(recvBps, sendBps);
    EXPECT_EQ(m_tester->getProcessById(101).memory(), 10U);
    EXPECT_EQ(m_tester->m_simpleSet.value(101).memory(), 10U);
    EXPECT_EQ(m_tester->getProcessById(101).sharememory(), 3U);

    cpu = recvBps = sendBps = 0;
    m_tester->mergeSubProcResources(100, cpu, recvBps, sendBps);
    EXPECT_DOUBLE_EQ(cpu, 1);
    EXPECT_DOUBLE_EQ(recvBps, 1);
    EXPECT_DOUBLE_EQ(sendBps, 1);
}

TEST_F(UT_ProcessSet, test_collapseWineContainerGroups_isolatesContainersAndUsers)
{
    WMWindowList windowList;
    addProcess(101, 1, "main.exe", kFilterApps, "/prefix/a", "package.a");
    addProcess(102, 101, "helper.exe", kFilterApps, "/prefix/a", "package.a");
    addProcess(201, 101, "main.exe", kFilterApps, "/prefix/b", "package.a");
    addProcess(202, 201, "helper.exe", kFilterApps, "/prefix/b", "package.a");
    addProcess(301, 1, "main.exe", kFilterApps, "/prefix/a", "package.b");
    addProcess(302, 301, "helper.exe", kFilterApps, "/prefix/a", "package.b");
    Process foreign = addProcess(401, 1, "other.exe", kFilterApps, "/prefix/a", "package.a");
    foreign.d->uid = geteuid() + 1;
    addProcess(501, 1, "no-prefix.exe", kFilterApps, "", "package.a");
    addProcess(502, 1, "no-package.exe", kFilterApps, "/prefix/a", "");
    for (auto it = m_tester->m_set.begin(); it != m_tester->m_set.end(); ++it)
        it->setCpu(1);

    const auto groups = m_tester->collapseWineContainerGroups(&windowList, geteuid());
    ASSERT_EQ(groups.size(), 3);
    EXPECT_EQ(groups.value(101), QList<pid_t>({101, 102}));
    EXPECT_EQ(groups.value(201), QList<pid_t>({201, 202}));
    EXPECT_EQ(groups.value(301), QList<pid_t>({301, 302}));
    for (pid_t pid : {101, 201, 301}) {
        qreal cpu = 0, recvBps = 0, sendBps = 0;
        m_tester->mergeSubProcResources(pid, cpu, recvBps, sendBps);
        EXPECT_DOUBLE_EQ(cpu, 2);
    }
    for (pid_t pid : {401, 501, 502})
        EXPECT_EQ(m_tester->getProcessById(pid).appType(), kFilterApps);
}

TEST_F(UT_ProcessSet, test_collapseWineContainerGroups_leavesSingleTreeAlone)
{
    WMWindowList windowList;
    addProcess(101, 1, "main.exe");
    addProcess(102, 101, "helper.exe", kFilterCurrentUser);
    const auto originalTree = m_tester->m_pidPtoCMapping;

    EXPECT_TRUE(m_tester->collapseWineContainerGroups(&windowList, geteuid()).isEmpty());
    EXPECT_EQ(m_tester->m_pidPtoCMapping, originalTree);
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

TEST_F(UT_ProcessSet, test_scanProcess_removesExitedPidsWithoutReaddingLiveProcesses)
{
    for (pid_t pid = 10; pid < 25; ++pid) {
        addProcess(pid, 1, "native-app");
        m_tester->m_prePid.append(pid);
        m_tester->m_pidMyApps.append(pid);
    }
    const QList<pid_t> livePids = m_tester->m_prePid.mid(3);
    EXPECT_TRUE(scanProcesses(livePids).isEmpty());
    EXPECT_EQ(m_tester->m_prePid, livePids);
    EXPECT_EQ(m_tester->m_simpleSet.keys(), livePids);
    EXPECT_EQ(m_tester->m_pidMyApps, livePids);
    EXPECT_EQ(m_tester->getPIDList(), livePids);
}

TEST_F(UT_ProcessSet, test_scanProcess_parentCycleTerminates)
{
    for (bool selfParent : {false, true}) {
        addProcess(101, 102, "native-app");
        addProcess(102, selfParent ? 102 : 103, "helper", kFilterCurrentUser);
        addProcess(103, 102, "helper", kFilterCurrentUser);
        m_tester->m_prePid = {101, 102, 103};
        m_tester->m_pidMyApps = {101};

        scanProcesses({101, 102, 103});
        EXPECT_EQ(m_tester->getProcessById(101).appType(), kFilterApps);
    }
}

TEST_F(UT_ProcessSet, test_scanProcess_findsGuiAncestorAndKeepsShellException)
{
    addProcess(101, 102, "native-app");
    addProcess(102, 103, "helper", kFilterCurrentUser);
    addProcess(103, 1, "gui-app");
    addProcess(104, 105, "native-app");
    addProcess(105, 103, "/bin/bash", kFilterCurrentUser);
    m_tester->m_prePid = {101, 102, 103, 104, 105};
    m_tester->m_pidMyApps = {101, 103, 104};

    scanProcesses({101, 102, 103, 104, 105}, {103});
    EXPECT_EQ(m_tester->getProcessById(101).appType(), kFilterCurrentUser);
    EXPECT_EQ(m_tester->getProcessById(103).appType(), kFilterApps);
    EXPECT_EQ(m_tester->getProcessById(104).appType(), kFilterApps);
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
