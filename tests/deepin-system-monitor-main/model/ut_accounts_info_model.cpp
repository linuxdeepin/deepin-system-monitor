// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/accounts_info_model.h"
#include "model/user.h"
//gtest
#include "stub.h"
#include "addr_pri.h"
#include <gtest/gtest.h>
//Qt
#include <QString>
#include <QList>

// getCurrentUserType() 把私有成员 m_currentUserType (int) 映射到
// User::UserType 枚举，是 AccountsInfoModel 中唯一不依赖 D-Bus 的纯逻辑，
// 共 4 个分支。这里用 addr_pri 的 ACCESS_PRIVATE_FIELD 直接注入 m_currentUserType，
// 在不触发任何 D-Bus 方法调用的情况下遍历全部分支。

// 生成 access_private_field::AccountsInfoModelm_currentUserType(obj) 访问器
ACCESS_PRIVATE_FIELD(AccountsInfoModel, int, m_currentUserType)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, SessionInfoList, m_sessionList)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, QString, m_currentUserName)
using UserMap = QMap<QString, User *>;
using OnlineUserList = QStringList;
ACCESS_PRIVATE_FIELD(AccountsInfoModel, OnlineUserList, m_onlineUsers)
ACCESS_PRIVATE_FIELD(AccountsInfoModel, UserMap, m_userMap)

class UT_AccountsInfoModel : public ::testing::Test
{
public:
    UT_AccountsInfoModel() : m_model(nullptr) {}

public:
    virtual void SetUp()
    {
        // 构造时会连系统总线读取在线会话/用户列表；开发机 login1 正常运行时
        // 为毫秒级同步返回，与仓库其它 D-Bus 类测试 (ut_process_db 等) 同模式。
        m_model = new AccountsInfoModel();
    }

    virtual void TearDown()
    {
        if (m_model) {
            delete m_model;
            m_model = nullptr;
        }
    }

    // 注入 m_currentUserType 的私有值并断言 getCurrentUserType() 的映射结果
    void setCurrentUserType(int value)
    {
        access_private_field::AccountsInfoModelm_currentUserType(*m_model) = value;
    }

protected:
    AccountsInfoModel *m_model;
};

// 分支1: m_currentUserType == -1 → Customized
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_Uninitialized_ReturnsCustomized)
{
    setCurrentUserType(-1);
    EXPECT_EQ(m_model->getCurrentUserType(), User::UserType::Customized);
}

// 分支2: m_currentUserType == 0 → StandardUser
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_StandardUser)
{
    setCurrentUserType(0);
    EXPECT_EQ(m_model->getCurrentUserType(), User::UserType::StandardUser);
}

// 分支3: m_currentUserType == 1 → Administrator
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_Administrator)
{
    setCurrentUserType(1);
    EXPECT_EQ(m_model->getCurrentUserType(), User::UserType::Administrator);
}

// 分支4: m_currentUserType 为其它值 (如 2) → 兜底返回 StandardUser
TEST_F(UT_AccountsInfoModel, GetCurrentUserType_UnknownValue_FallbackToStandardUser)
{
    setCurrentUserType(2);
    EXPECT_EQ(m_model->getCurrentUserType(), User::UserType::StandardUser);

    setCurrentUserType(999);
    EXPECT_EQ(m_model->getCurrentUserType(), User::UserType::StandardUser);
}

// ========== 新增：覆盖 lockSessionByUserName / activateSessionByUserName / LogoutByUserName / userList ==========
// 通过注入 m_sessionList / m_currentUserName / m_userMap / m_onlineUsers 私有成员，
// 在不依赖真实 login1 D-Bus 应答的情况下遍历各分支。
// m_LoginInter->call() 会发起真实 D-Bus 调用但失败时仅返回错误 message，不致崩溃。

// 分支1: sessionList 非空且找到匹配用户 → 调用 LockSession 返回 true
TEST_F(UT_AccountsInfoModel, LockSessionByUserName_Found_ReturnsTrue)
{
    SessionInfo si;
    si.sessionId = "1";
    si.userName = "tester";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;

    EXPECT_TRUE(m_model->lockSessionByUserName("tester"));
}

// 分支2: sessionList 非空但未找到 → 返回 false
TEST_F(UT_AccountsInfoModel, LockSessionByUserName_NotFound_ReturnsFalse)
{
    SessionInfo si;
    si.userName = "other";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;

    EXPECT_FALSE(m_model->lockSessionByUserName("nobody"));
}

// 分支3: sessionList 为空 → 返回 false
TEST_F(UT_AccountsInfoModel, LockSessionByUserName_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(m_model->lockSessionByUserName("anyone"));
}

TEST_F(UT_AccountsInfoModel, ActivateSessionByUserName_Found_ReturnsTrue)
{
    SessionInfo si;
    si.sessionId = "2";
    si.userName = "tester";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;

    EXPECT_TRUE(m_model->activateSessionByUserName("tester"));
}

TEST_F(UT_AccountsInfoModel, ActivateSessionByUserName_NotFound_ReturnsFalse)
{
    SessionInfo si;
    si.userName = "other";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;

    EXPECT_FALSE(m_model->activateSessionByUserName("nobody"));
}

TEST_F(UT_AccountsInfoModel, ActivateSessionByUserName_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(m_model->activateSessionByUserName("anyone"));
}

// Logout: 当前用户且 sessionList 非空且找到 → true
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserFound_ReturnsTrue)
{
    access_private_field::AccountsInfoModelm_currentUserName(*m_model) = "curuser";
    SessionInfo si;
    si.userName = "curuser";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;

    EXPECT_TRUE(m_model->LogoutByUserName("curuser"));
}

// Logout: 当前用户但 sessionList 为空 → false
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserEmptyList_ReturnsFalse)
{
    access_private_field::AccountsInfoModelm_currentUserName(*m_model) = "curuser";
    EXPECT_FALSE(m_model->LogoutByUserName("curuser"));
}

// Logout: 当前用户 sessionList 非空但无匹配 → false
TEST_F(UT_AccountsInfoModel, LogoutByUserName_CurrentUserNotFound_ReturnsFalse)
{
    access_private_field::AccountsInfoModelm_currentUserName(*m_model) = "curuser";
    SessionInfo si;
    si.userName = "other";
    access_private_field::AccountsInfoModelm_sessionList(*m_model) << si;
    EXPECT_FALSE(m_model->LogoutByUserName("curuser"));
}

// Logout: 非当前用户，pkexec/pkill 存在 → 启动 QProcess 返回 true
TEST_F(UT_AccountsInfoModel, LogoutByUserName_OtherUser_ReturnsTrue)
{
    access_private_field::AccountsInfoModelm_currentUserName(*m_model) = "curuser";
    // 注意：此用例会真实启动 pkexec pkill -u <user>；pkexec 通常不存在或被拒，
    // 但 QProcess::start/waitForFinished 不会崩溃，函数仍返回 true。
    EXPECT_TRUE(m_model->LogoutByUserName("anotheruser"));
}

// userList: m_userMap 中存在在线用户 → 返回非空且 user->online=true
TEST_F(UT_AccountsInfoModel, UserList_OnlineUserReturned)
{
    auto &userMap = access_private_field::AccountsInfoModelm_userMap(*m_model);
    auto &onlineUsers = access_private_field::AccountsInfoModelm_onlineUsers(*m_model);
    User *u = new User;
    u->setName("onlineuser");
    userMap.insert("onlineuser", u);
    onlineUsers << "onlineuser";

    QList<User *> list = m_model->userList();
    // 注意：构造函数可能从真实 D-Bus 拉到其它在线用户，故不固定 size，
    // 只断言我们注入的用户在列表中且标记为在线。
    bool found = false;
    for (User *lu : list) {
        if (lu->name() == "onlineuser") { found = true; EXPECT_TRUE(lu->online()); }
    }
    EXPECT_TRUE(found);
}

TEST_F(UT_AccountsInfoModel, UserList_NoOnlineUser_ReturnsEmpty)
{
    auto &userMap = access_private_field::AccountsInfoModelm_userMap(*m_model);
    auto &onlineUsers = access_private_field::AccountsInfoModelm_onlineUsers(*m_model);
    User *u = new User;
    u->setName("offlineuser_x");
    userMap.insert("offlineuser_x", u);
    onlineUsers.clear();  // 确保无在线用户
    EXPECT_EQ(m_model->userList().size(), 0);
}
