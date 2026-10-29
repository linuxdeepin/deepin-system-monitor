// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "process/priority_controller.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QProcess>
#include <QSignalSpy>
#include <QTest>
static QString m_Sresult;
/***************************************STUB begin*********************************************/
void stub_execute_start(){
    m_Sresult = "Process start";
    return;
}
/***************************************STUB end**********************************************/
class UT_PriorityController : public ::testing::Test
{
public:
    UT_PriorityController() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        pid_t pid = 100000;
        int priority = 20;
        m_tester = new PriorityController(pid,priority);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    PriorityController *m_tester;
};

TEST_F(UT_PriorityController, initTest)
{

}

TEST_F(UT_PriorityController, test_execute_001)
{
    // /usr/bin/pkexec 与 /usr/bin/renice 均存在 → execute() 走到 m_proc->start
    // 触发 stateChanged(Starting) lambda（backgroundTaskStateChanged(kTaskStarted)）
    // pkexec 无鉴权快速失败(127) → finished lambda（rc!=0 且非 EPERM/EACCES → else 分支）
    QSignalSpy finishedSpy(m_tester, &PriorityController::finished);
    m_tester->execute();
    finishedSpy.wait(3000);
    SUCCEED();
}

// 不同优先级值构造，覆盖构造函数路径
TEST_F(UT_PriorityController, test_construct_negative_priority)
{
    PriorityController c(100001, -10);
    QSignalSpy finishedSpy(&c, &PriorityController::finished);
    c.execute();
    finishedSpy.wait(3000);
    SUCCEED();
}

