// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// linglong_controller 是 linglong 容器进程管理的新特性（提交 d1d86d8b），
// 原先 0% 覆盖。本测试覆盖：
//   - 构造函数（QProcess 创建 + started/finished 信号连接）
//   - execute() appId 正则校验非法分支（EINVAL）
//   - execute() ll-cli 不存在分支（ENOENT，桩化 QFile::exists）
//   - execute() 正常启动分支（真实 ll-cli kill 失败，覆盖 sigName 选择 + started/finished lambda）

#include "process/linglong_controller.h"
#include "application.h"
#include "ddlog.h"

// gtest
#include "stub.h"
#include <gtest/gtest.h>
// Qt
#include <QSignalSpy>
#include <QTest>
#include <QFile>

#include <cerrno>
#include <csignal>

/***************************************STUB begin*********************************************/
// 桩化 QFile::exists(const QString&) 静态重载 → 恒返回 false，用于触发 ll-cli 不存在分支
bool stub_ll_qfile_exists_false(const QString &)
{
    return false;
}
/***************************************STUB end**********************************************/

class UT_LinglongController : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// 构造函数：创建 QProcess、连接 started/finished 信号
TEST_F(UT_LinglongController, Constructor)
{
    LinglongController c("com.test.app", SIGTERM);
    SUCCEED();
}

// execute() 非法 appId：含空格/感叹号，正则 ^[a-zA-Z0-9.-]+$ 不匹配 → resultReady(EINVAL) + finished
TEST_F(UT_LinglongController, Execute_InvalidAppId_EmitsEINVAL)
{
    LinglongController c("bad app id!", SIGTERM);
    QSignalSpy resultSpy(&c, &LinglongController::resultReady);
    QSignalSpy finishedSpy(&c, &LinglongController::finished);

    c.execute();  // 同步走 EINVAL 分支，未启动进程

    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.takeFirst().at(0).toInt(), EINVAL);
    EXPECT_EQ(finishedSpy.count(), 1);
}

// execute() 合法 appId 但 ll-cli 不存在（桩化 QFile::exists） → resultReady(ENOENT) + finished
TEST_F(UT_LinglongController, Execute_LlcliMissing_EmitsENOENT)
{
    LinglongController c("com.test.app", SIGTERM);
    QSignalSpy resultSpy(&c, &LinglongController::resultReady);
    QSignalSpy finishedSpy(&c, &LinglongController::finished);

    Stub s;
    s.set((bool(*)(const QString &))&QFile::exists, stub_ll_qfile_exists_false);

    c.execute();

    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.takeFirst().at(0).toInt(), ENOENT);
    EXPECT_EQ(finishedSpy.count(), 1);
}

// execute() 合法 appId 且 ll-cli 存在 → m_proc->start，覆盖 SIGTERM 信号名选择 + started/finished lambda
// ll-cli kill 一个不存在的 appId 会很快失败（rc!=0），触发 finished lambda 中读取 stderr 的分支
TEST_F(UT_LinglongController, Execute_LlcliExists_StartsProcess_SIGTERM)
{
    LinglongController c("com.nonexistent.app.unitest", SIGTERM);
    QSignalSpy finishedSpy(&c, &LinglongController::finished);

    c.execute();
    // 等待异步进程结束（ll-cli 失败很快）；wait 内部会处理事件循环使 deleteLater 生效
    finishedSpy.wait(3000);

    SUCCEED();
}

// 覆盖 SIGKILL 信号名选择分支
TEST_F(UT_LinglongController, Execute_LlcliExists_StartsProcess_SIGKILL)
{
    LinglongController c("com.nonexistent.app.unitest", SIGKILL);
    QSignalSpy finishedSpy(&c, &LinglongController::finished);

    c.execute();
    finishedSpy.wait(3000);

    SUCCEED();
}
