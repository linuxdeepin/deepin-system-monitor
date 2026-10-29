// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "detailwidgetmanager.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QSignalSpy>

/***************************************STUB begin*********************************************/
// 无外部依赖打桩需求：DetailWidgetManager 为纯事件转发单例，无 DBus/网络/文件依赖
/***************************************STUB end***********************************************/

// 分支清单（源自 detailwidgetmanager.cpp）:
// 1. getInstance: 双检锁（QAtomicPointer testAndSetOrdered）保证单例唯一
// 2. jumpDetailWidget: lockForRead -> emit sigJumpToDetailWidget(msgCode) -> unlock
// 3. jumpProcessWidget: lockForRead -> emit sigJumpToProcessWidget(msgCode) -> unlock
// 注: 私有构造通过 -fno-access-control 由 getInstance 内部创建

class DetailWidgetManagerTest : public ::testing::Test
{
public:
    virtual void SetUp() override
    {
        // Arrange: 取单例（首次调用时创建）
        m_manager = &DetailWidgetManager::getInstance();
        // Assert: 实例有效
        ASSERT_NE(m_manager, nullptr);
    }

    virtual void TearDown() override
    {
        // 单例随进程退出，无需销毁
    }

protected:
    DetailWidgetManager *m_manager = nullptr;
};

// ───────────────────────── getInstance ─────────────────────────
TEST_F(DetailWidgetManagerTest, GetInstance_CalledRepeatedly_ReturnsSameInstance)
{
    // Arrange: SetUp 已获取单例
    // Act: 再次获取
    DetailWidgetManager &second = DetailWidgetManager::getInstance();

    // Assert: 双检锁单例，两次获取同一实例
    EXPECT_EQ(&second, m_manager);
    EXPECT_NE(&second, nullptr);
    EXPECT_TRUE(&second == &DetailWidgetManager::getInstance());
}

// ───────────────────────── jumpDetailWidget ─────────────────────────
TEST_F(DetailWidgetManagerTest, JumpDetailWidget_WithMsgCode_EmitsSignalWithCode)
{
    // Arrange
    QSignalSpy spy(m_manager, &DetailWidgetManager::sigJumpToDetailWidget);

    // Act
    m_manager->jumpDetailWidget("MSG_CPU");

    // Assert: 信号携带原始 msgCode 发出
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().size(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_CPU"));
}

TEST_F(DetailWidgetManagerTest, JumpDetailWidget_WithEmptyMsg_EmitsEmptyString)
{
    // Arrange
    QSignalSpy spy(m_manager, &DetailWidgetManager::sigJumpToDetailWidget);

    // Act: 空串原样透传，不经过任何转换分支
    m_manager->jumpDetailWidget(QString(""));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString(""));
}

// ───────────────────────── jumpProcessWidget ─────────────────────────
TEST_F(DetailWidgetManagerTest, JumpProcessWidget_WithMsgCode_EmitsSignalWithCode)
{
    // Arrange
    QSignalSpy spy(m_manager, &DetailWidgetManager::sigJumpToProcessWidget);

    // Act
    m_manager->jumpProcessWidget("MSG_PROCESS");

    // Assert: 信号携带原始 msgCode 发出
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().size(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_PROCESS"));
}

TEST_F(DetailWidgetManagerTest, JumpProcessWidget_ConsecutiveJumps_EmitsPerInvocation)
{
    // Arrange
    QSignalSpy spy(m_manager, &DetailWidgetManager::sigJumpToProcessWidget);

    // Act: 连续两次跳转
    m_manager->jumpProcessWidget("MSG_PROCESS");
    m_manager->jumpProcessWidget("MSG_NET");

    // Assert: 每次调用各发射一次，顺序与参数一致
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(spy.at(0).first().toString(), QString("MSG_PROCESS"));
    EXPECT_EQ(spy.at(1).first().toString(), QString("MSG_NET"));
}

// ───────────────────────── 双信号互不干扰 ─────────────────────────
TEST_F(DetailWidgetManagerTest, JumpToDifferentTargets_TwoSignalTypes_EmitsIndependently)
{
    // Arrange: 同时监听两类信号，验证路由互不串扰
    QSignalSpy detailSpy(m_manager, &DetailWidgetManager::sigJumpToDetailWidget);
    QSignalSpy processSpy(m_manager, &DetailWidgetManager::sigJumpToProcessWidget);

    // Act
    m_manager->jumpDetailWidget("MSG_DISK");
    m_manager->jumpProcessWidget("MSG_PROCESS");

    // Assert: 各自独立发射
    EXPECT_EQ(detailSpy.count(), 1);
    EXPECT_EQ(processSpy.count(), 1);
    EXPECT_EQ(detailSpy.first().first().toString(), QString("MSG_DISK"));
    EXPECT_EQ(processSpy.first().first().toString(), QString("MSG_PROCESS"));
}
