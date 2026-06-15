// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/user.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QString>

// 被测类 User 是一个纯数据承载对象 (QObject 无信号)，所有 setter 均带
// “值未变化即提前返回”的守卫；displayName() 是唯一含分支的关键纯逻辑。
// 这里对 model/user.cpp 中 0% 覆盖的全部公共函数做覆盖。

class UT_User : public ::testing::Test
{
public:
    UT_User() : m_user(nullptr) {}

public:
    virtual void SetUp()
    {
        m_user = new User();
    }

    virtual void TearDown()
    {
        if (m_user) {
            delete m_user;
            m_user = nullptr;
        }
    }

protected:
    User *m_user;
};

// 构造函数：默认状态。对应 user.cpp 中成员初始化列表 (m_isCurrentUser=false 等)。
TEST_F(UT_User, Constructor_DefaultState)
{
    EXPECT_FALSE(m_user->isCurrentUser());
    EXPECT_FALSE(m_user->online());
    EXPECT_EQ(m_user->userType(), User::StandardUser);   // m_userType 初始化为 0
    EXPECT_TRUE(m_user->name().isEmpty());
    EXPECT_TRUE(m_user->fullname().isEmpty());
    EXPECT_TRUE(m_user->iconFile().isEmpty());
    EXPECT_TRUE(m_user->userUid().isEmpty());
}

// name / setName 往返，并覆盖守卫的“相同值提前返回”分支。
TEST_F(UT_User, Name_SetAndGet)
{
    m_user->setName("alice");
    EXPECT_EQ(m_user->name(), QString("alice"));

    // 再次设置相同值：走守卫提前返回分支，值保持不变
    m_user->setName("alice");
    EXPECT_EQ(m_user->name(), QString("alice"));

    // 设置不同值：走赋值分支
    m_user->setName("bob");
    EXPECT_EQ(m_user->name(), QString("bob"));
}

// fullname / setFullname
TEST_F(UT_User, Fullname_SetAndGet)
{
    m_user->setFullname("Alice Wonder");
    EXPECT_EQ(m_user->fullname(), QString("Alice Wonder"));

    // 相同值提前返回
    m_user->setFullname("Alice Wonder");
    EXPECT_EQ(m_user->fullname(), QString("Alice Wonder"));
}

// iconFile / setIconFile
TEST_F(UT_User, IconFile_SetAndGet)
{
    m_user->setIconFile("/var/lib/AccountsService/icons/alice.png");
    EXPECT_EQ(m_user->iconFile(), QString("/var/lib/AccountsService/icons/alice.png"));

    // 相同值提前返回
    m_user->setIconFile("/var/lib/AccountsService/icons/alice.png");
    EXPECT_EQ(m_user->iconFile(), QString("/var/lib/AccountsService/icons/alice.png"));
}

// userUid / setUserUid
TEST_F(UT_User, UserUid_SetAndGet)
{
    m_user->setUserUid("1000");
    EXPECT_EQ(m_user->userUid(), QString("1000"));

    // 相同值提前返回
    m_user->setUserUid("1000");
    EXPECT_EQ(m_user->userUid(), QString("1000"));
}

// isCurrentUser / setIsCurrentUser：默认 false，翻转后为 true，覆盖守卫两分支。
TEST_F(UT_User, IsCurrentUser_SetAndGet)
{
    EXPECT_FALSE(m_user->isCurrentUser());

    m_user->setIsCurrentUser(true);
    EXPECT_TRUE(m_user->isCurrentUser());

    // 相同值提前返回分支
    m_user->setIsCurrentUser(true);
    EXPECT_TRUE(m_user->isCurrentUser());

    m_user->setIsCurrentUser(false);
    EXPECT_FALSE(m_user->isCurrentUser());
}

// online / setOnline
TEST_F(UT_User, Online_SetAndGet)
{
    EXPECT_FALSE(m_user->online());

    m_user->setOnline(true);
    EXPECT_TRUE(m_user->online());

    // 相同值提前返回分支
    m_user->setOnline(true);
    EXPECT_TRUE(m_user->online());

    m_user->setOnline(false);
    EXPECT_FALSE(m_user->online());
}

// userType / setUserType：枚举值往返，覆盖守卫两分支。
TEST_F(UT_User, UserType_SetAndGet)
{
    EXPECT_EQ(m_user->userType(), User::StandardUser);

    m_user->setUserType(User::Administrator);
    EXPECT_EQ(m_user->userType(), User::Administrator);

    // 相同值提前返回分支
    m_user->setUserType(User::Administrator);
    EXPECT_EQ(m_user->userType(), User::Administrator);

    m_user->setUserType(User::Customized);
    EXPECT_EQ(m_user->userType(), User::Customized);
}

// displayName() 关键分支：fullname 为空时回退到 name。
TEST_F(UT_User, DisplayName_FallbackToName_WhenFullnameEmpty)
{
    m_user->setName("alice");
    // fullname 默认为空 → 返回 name
    EXPECT_EQ(m_user->displayName(), QString("alice"));
}

// displayName() 另一分支：fullname 非空时优先返回 fullname。
TEST_F(UT_User, DisplayName_PreferFullname_WhenNonEmpty)
{
    m_user->setName("alice");
    m_user->setFullname("Alice Wonder");
    EXPECT_EQ(m_user->displayName(), QString("Alice Wonder"));
}
