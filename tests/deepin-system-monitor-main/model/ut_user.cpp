// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for model::User (deepin-system-monitor-main/model/user.cpp)
// against upstream/master @ 00005c4e.
//
// User is a pure data holder (QObject without signals); every setter guards
// against unchanged values with an early return, and displayName() is the
// only method with real branching (fullname empty -> fall back to name).
// No DBus / filesystem / network access happens, so no stubbing is needed.
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |User()|low|默认成员初始化|1|2|
// |setName|low|守卫相同×不同|1|1|
// |setFullname|low|守卫相同×不同|1|1|
// |displayName|high|fullname空×非空×name空|2|3|
// |setIsCurrentUser|low|守卫相同×翻转|1|1|
// |setUserType|low|守卫相同×枚举遍历|1|1|
// |setIconFile|low|守卫相同×不同|1|1|
// |setUserUid|low|守卫相同×不同|1|1|
// |setOnline|low|守卫相同×翻转|1|1|
// 本 TU 共 10 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::User 全部方法 @ model/user.cpp:31-114
// ctor : 成员初始化列表 m_isCurrentUser=false, m_online=false, m_userType=0,
//        m_name/m_fullname/m_iconFile 均空
// B1   : setName(name != m_name)          -> 赋值 | 相同提前返回
// B2   : setFullname(fullname != m_fullname) -> 赋值 | 相同提前返回
// B3   : displayName -> m_fullname.isEmpty() ? m_name : m_fullname
// B4   : setIsCurrentUser(isCurrentUser == m_isCurrentUser) -> return | 赋值
// B5   : setUserType(m_userType == userType) -> return | 赋值
// B6   : setIconFile(m_iconFile == iconFile) -> return | 赋值
// B7   : setUserUid(m_uid == uid) -> return | 赋值
// B8   : setOnline(m_online != online) -> 赋值 | 相同提前返回
// 用例映射:
//   Construction_DefaultState_MembersInitialized       -> ctor
//   Construction_DefaultUid_IsEmpty                    -> ctor (m_uid)
//   Name_SetDifferentValue_StoresAndGetterMirrors      -> B1
//   Fullname_SetDifferentValue_StoresAndGetterMirrors  -> B2
//   DisplayName_FullnameEmpty_FallsBackToName          -> B3 (empty)
//   DisplayName_FullnameNonEmpty_PreferFullname        -> B3 (non-empty)
//   DisplayName_BothEmpty_ReturnsEmpty                 -> B3 (both empty)
//   IsCurrentUser_ToggleAndGuard_MirrorsState          -> B4
//   UserType_EnumCycleAndGuard_MirrorsState            -> B5
//   IconFile_SetDifferentValue_StoresAndGetterMirrors  -> B6
//   UserUid_SetDifferentValue_StoresAndGetterMirrors   -> B7
//   Online_ToggleAndGuard_MirrorsState                 -> B8
// ====================================================================

#include <gtest/gtest.h>

#include <QString>

#include "model/user.h"

namespace {

class UT_User : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Arrange: each case starts from a freshly constructed data holder
        m_user.reset(new User());
    }

    void TearDown() override
    {
        // release the fixture object before the next case rebuilds it
        m_user.reset();
    }

    QScopedPointer<User> m_user;
};

// ---------------------------------------------------------------------------
// ctor: every member uses the documented default from the init list.
// ---------------------------------------------------------------------------
TEST_F(UT_User, Construction_DefaultState_MembersInitialized)
{
    // Arrange: SetUp constructed the user
    ASSERT_NE(m_user, nullptr);

    // Act: read the initial state through the public getters
    const bool currentFlag = m_user->isCurrentUser();
    const bool onlineFlag = m_user->online();
    const int type = m_user->userType();
    const QString name = m_user->name();
    const QString fullname = m_user->fullname();
    const QString iconFile = m_user->iconFile();

    // Assert: every member matches the documented default
    EXPECT_FALSE(currentFlag);
    EXPECT_FALSE(onlineFlag);
    EXPECT_EQ(User::StandardUser, type);
    EXPECT_EQ(QString(), name);
    EXPECT_EQ(QString(), fullname);
    EXPECT_EQ(QString(), iconFile);
}

// ---------------------------------------------------------------------------
// ctor: m_uid is an implicit QString default (not in the init list).
// ---------------------------------------------------------------------------
TEST_F(UT_User, Construction_DefaultUid_IsEmpty)
{
    // Arrange: SetUp constructed the user
    ASSERT_NE(m_user, nullptr);

    // Act: read the uid through the getter
    const QString uid = m_user->userUid();

    // Assert: uid mirrors the implicit QString default, null string included
    EXPECT_EQ(QString(), uid);
    EXPECT_EQ(0, uid.size());
}

// ---------------------------------------------------------------------------
// B1: setName stores a new value; re-storing the same value keeps it.
// ---------------------------------------------------------------------------
TEST_F(UT_User, Name_SetDifferentValue_StoresAndGetterMirrors)
{
    // Arrange
    ASSERT_TRUE(m_user->name().isEmpty());

    // Act
    m_user->setName("alice");

    // Assert
    EXPECT_EQ(QString("alice"), m_user->name());

    // Act: guard branch - same value is a no-op
    m_user->setName("alice");

    // Assert: value unchanged
    EXPECT_EQ(QString("alice"), m_user->name());
}

// ---------------------------------------------------------------------------
// B2: setFullname stores a new value; the guard keeps repeats stable.
// ---------------------------------------------------------------------------
TEST_F(UT_User, Fullname_SetDifferentValue_StoresAndGetterMirrors)
{
    // Arrange
    ASSERT_TRUE(m_user->fullname().isEmpty());

    // Act
    m_user->setFullname("Alice Wonder");

    // Assert
    EXPECT_EQ(QString("Alice Wonder"), m_user->fullname());

    // Act: guard branch
    m_user->setFullname("Alice Wonder");

    // Assert: value unchanged
    EXPECT_EQ(QString("Alice Wonder"), m_user->fullname());
}

// ---------------------------------------------------------------------------
// B3: displayName falls back to the login name while fullname is empty.
// ---------------------------------------------------------------------------
TEST_F(UT_User, DisplayName_FullnameEmpty_FallsBackToName)
{
    // Arrange
    m_user->setName("alice");
    ASSERT_TRUE(m_user->fullname().isEmpty());

    // Act
    const QString shown = m_user->displayName();

    // Assert: fallback picks the login name, not the (empty) fullname
    EXPECT_EQ(QString("alice"), shown);
    EXPECT_EQ(m_user->name(), shown);
}

// ---------------------------------------------------------------------------
// B3: displayName prefers a non-empty fullname over the login name.
// ---------------------------------------------------------------------------
TEST_F(UT_User, DisplayName_FullnameNonEmpty_PreferFullname)
{
    // Arrange
    m_user->setName("alice");
    m_user->setFullname("Alice Wonder");

    // Act
    const QString shown = m_user->displayName();

    // Assert: fullname wins over the login name
    EXPECT_EQ(QString("Alice Wonder"), shown);
    EXPECT_NE(m_user->name(), shown);
}

// ---------------------------------------------------------------------------
// B3: with both fields empty the display name stays empty.
// ---------------------------------------------------------------------------
TEST_F(UT_User, DisplayName_BothEmpty_ReturnsEmpty)
{
    // Arrange
    ASSERT_TRUE(m_user->name().isEmpty());
    ASSERT_TRUE(m_user->fullname().isEmpty());

    // Act
    const QString shown = m_user->displayName();

    // Assert: empty on both sides resolves to an empty string
    EXPECT_EQ(QString(), shown);
    EXPECT_EQ(0, shown.size());
}

// ---------------------------------------------------------------------------
// B4: setIsCurrentUser toggles the flag; equal writes are ignored.
// ---------------------------------------------------------------------------
TEST_F(UT_User, IsCurrentUser_ToggleAndGuard_MirrorsState)
{
    // Arrange
    ASSERT_FALSE(m_user->isCurrentUser());

    // Act: false -> true
    m_user->setIsCurrentUser(true);

    // Assert
    EXPECT_EQ(true, m_user->isCurrentUser());

    // Act: guard (same value) keeps the flag untouched
    m_user->setIsCurrentUser(true);

    // Assert
    EXPECT_EQ(true, m_user->isCurrentUser());

    // Act: true -> false
    m_user->setIsCurrentUser(false);

    // Assert
    EXPECT_EQ(false, m_user->isCurrentUser());
}

// ---------------------------------------------------------------------------
// B5: setUserType cycles the enum; equal writes are ignored.
// ---------------------------------------------------------------------------
TEST_F(UT_User, UserType_EnumCycleAndGuard_MirrorsState)
{
    // Arrange
    ASSERT_EQ(User::StandardUser, m_user->userType());

    // Act: standard -> administrator
    m_user->setUserType(User::Administrator);

    // Assert
    EXPECT_EQ(User::Administrator, m_user->userType());

    // Act: guard (same value) keeps the type untouched
    m_user->setUserType(User::Administrator);

    // Assert
    EXPECT_EQ(User::Administrator, m_user->userType());

    // Act: administrator -> customized
    m_user->setUserType(User::Customized);

    // Assert
    EXPECT_EQ(User::Customized, m_user->userType());
}

// ---------------------------------------------------------------------------
// B6: setIconFile stores the marker value; the guard keeps repeats stable.
// ---------------------------------------------------------------------------
TEST_F(UT_User, IconFile_SetDifferentValue_StoresAndGetterMirrors)
{
    // Arrange: a non-existent marker string, never touched on disk
    const QString marker = QString("ut-icon-%1.png").arg(quintptr(this));

    // Act
    m_user->setIconFile(marker);

    // Assert
    EXPECT_EQ(marker, m_user->iconFile());

    // Act: guard branch
    m_user->setIconFile(marker);

    // Assert: value unchanged
    EXPECT_EQ(marker, m_user->iconFile());
}

// ---------------------------------------------------------------------------
// B7: setUserUid stores the value; the guard keeps repeats stable.
// ---------------------------------------------------------------------------
TEST_F(UT_User, UserUid_SetDifferentValue_StoresAndGetterMirrors)
{
    // Arrange
    ASSERT_TRUE(m_user->userUid().isEmpty());

    // Act
    m_user->setUserUid("1000");

    // Assert
    EXPECT_EQ(QString("1000"), m_user->userUid());

    // Act: guard branch
    m_user->setUserUid("1000");

    // Assert: value unchanged
    EXPECT_EQ(QString("1000"), m_user->userUid());
}

// ---------------------------------------------------------------------------
// B8: setOnline toggles the flag; equal writes are ignored.
// ---------------------------------------------------------------------------
TEST_F(UT_User, Online_ToggleAndGuard_MirrorsState)
{
    // Arrange
    ASSERT_FALSE(m_user->online());

    // Act: false -> true
    m_user->setOnline(true);

    // Assert
    EXPECT_EQ(true, m_user->online());

    // Act: guard (same value) keeps the flag untouched
    m_user->setOnline(true);

    // Assert
    EXPECT_EQ(true, m_user->online());

    // Act: true -> false
    m_user->setOnline(false);

    // Assert
    EXPECT_EQ(false, m_user->online());
}

} // namespace
