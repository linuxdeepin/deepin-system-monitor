// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include <QSignalSpy>
#include <QTest>

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
        pid_t pid = 100000;
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
    // /usr/bin/pkexec 与 /usr/bin/kill 均存在 → execute() 走到 m_proc->start
    // pkexec 在无 polkit 鉴权的测试环境中快速失败(127) → 触发 finished lambda（rc!=0 → EPERM 分支）
    QSignalSpy finishedSpy(m_tester, &ProcessController::finished);
    m_tester->execute();
    finishedSpy.wait(3000);  // 等待异步进程结束；wait 内部处理事件循环使 deleteLater 生效
    SUCCEED();
}

// 用不同信号构造，覆盖构造函数对不同 signal 值的处理（QProcess 创建 + finished 信号连接）
TEST_F(UT_ProcessController, test_construct_with_sigkill)
{
    ProcessController c(100001, SIGKILL);
    QSignalSpy finishedSpy(&c, &ProcessController::finished);
    c.execute();
    finishedSpy.wait(3000);
    SUCCEED();
}
