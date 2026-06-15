// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "common/error_context.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_ErrorContext : public ::testing::Test
{
public:
    UT_ErrorContext() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ErrorContext();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ErrorContext *m_tester;
};

TEST_F(UT_ErrorContext, initTest)
{

}

TEST_F(UT_ErrorContext, test_ErrorContext_01)
{
    ErrorContext();
}

TEST_F(UT_ErrorContext, test_getCode_01)
{
    m_tester->getCode();
}

TEST_F(UT_ErrorContext, test_setCode_01)
{
    m_tester->setCode(1);
}

TEST_F(UT_ErrorContext, test_getSubCode_01)
{
    m_tester->getSubCode();
}

TEST_F(UT_ErrorContext, test_setSubCode_01)
{
    m_tester->setSubCode(1);
}

TEST_F(UT_ErrorContext, test_getErrorName_01)
{
    m_tester->getErrorName();
}

TEST_F(UT_ErrorContext, test_setErrorName_01)
{
    m_tester->setErrorName("1");
}

TEST_F(UT_ErrorContext, test_getErrorMessage_01)
{
    m_tester->getErrorMessage();
}

TEST_F(UT_ErrorContext, test_setErrorMessage_01)
{
    m_tester->setErrorMessage("1");
}

TEST_F(UT_ErrorContext, test_operator_01)
{
    const ErrorContext other;
    m_tester->operator==(other);
}

TEST_F(UT_ErrorContext, test_operator_02)
{
    ErrorContext copy(*m_tester);
}

// 覆盖带参构造函数 + isValid/bool 取真分支
TEST_F(UT_ErrorContext, test_construct_with_params_01)
{
    ErrorContext ec(ErrorContext::kErrorTypeSystem, 2, "EPerm", "permission denied");
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), 2);
    EXPECT_EQ(ec.getErrorName(), QString("EPerm"));
    EXPECT_EQ(ec.getErrorMessage(), QString("permission denied"));
    EXPECT_TRUE(ec.isValid());
    EXPECT_TRUE(static_cast<bool>(ec));
}

// 覆盖 operator= 赋值运算符
TEST_F(UT_ErrorContext, test_assignment_01)
{
    ErrorContext src(ErrorContext::kErrorTypeDBus, 5, "name", "msg");
    *m_tester = src;
    EXPECT_EQ(m_tester->getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(m_tester->getSubCode(), 5);
    EXPECT_EQ(m_tester->getErrorName(), QString("name"));
}

// 覆盖 operator== 相等分支
TEST_F(UT_ErrorContext, test_operator_eq_true_01)
{
    ErrorContext a(1, 2, "n", "m");
    ErrorContext b(1, 2, "n", "m");
    EXPECT_TRUE(a == b);
}

// 覆盖 operator== 不等分支（code 不同即短路返回 false）
TEST_F(UT_ErrorContext, test_operator_eq_false_01)
{
    ErrorContext a(1, 2, "n", "m");
    ErrorContext b(9, 9, "x", "y");
    EXPECT_FALSE(a == b);
}

// 覆盖 isValid / operator bool / operator! 的真假两分支
TEST_F(UT_ErrorContext, test_validity_and_bool_ops_01)
{
    // 默认构造 code=subcode=0 → 无效(无错误)
    EXPECT_FALSE(m_tester->isValid());
    EXPECT_FALSE(static_cast<bool>(*m_tester));
    EXPECT_TRUE(!(*m_tester));  // operator! : 无错误 → true

    m_tester->setCode(1);
    // 设置错误码后 → 有效(有错误)
    EXPECT_TRUE(m_tester->isValid());
    EXPECT_TRUE(static_cast<bool>(*m_tester));
    EXPECT_FALSE(!(*m_tester));
}

// 覆盖 reset()：清空所有字段回到无效状态
TEST_F(UT_ErrorContext, test_reset_01)
{
    m_tester->setCode(5);
    m_tester->setSubCode(6);
    m_tester->setErrorName("err");
    m_tester->setErrorMessage("msg");
    ASSERT_TRUE(m_tester->isValid());

    m_tester->reset();
    EXPECT_FALSE(m_tester->isValid());
    EXPECT_EQ(m_tester->getCode(), 0);
    EXPECT_EQ(m_tester->getSubCode(), 0);
    EXPECT_EQ(m_tester->getErrorName(), QString());
}
