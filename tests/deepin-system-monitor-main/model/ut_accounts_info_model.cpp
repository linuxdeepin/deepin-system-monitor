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

// getCurrentUserType() 把私有成员 m_currentUserType (int) 映射到
// User::UserType 枚举，是 AccountsInfoModel 中唯一不依赖 D-Bus 的纯逻辑，
// 共 4 个分支。这里用 addr_pri 的 ACCESS_PRIVATE_FIELD 直接注入 m_currentUserType，
// 在不触发任何 D-Bus 方法调用的情况下遍历全部分支。

// 生成 access_private_field::AccountsInfoModelm_currentUserType(obj) 访问器
ACCESS_PRIVATE_FIELD(AccountsInfoModel, int, m_currentUserType)

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
