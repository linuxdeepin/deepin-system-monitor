// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "kill_process_confirm_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QMessageBox>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_KillProcessConfirmDialog : public ::testing::Test
{
public:
    UT_KillProcessConfirmDialog() : m_tester(nullptr) {}

    void SetUp() override
    {
        m_tester = new KillProcessConfirmDialog();
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    KillProcessConfirmDialog *m_tester;
};

// 用例计数声明表（方法 | level | factors | min | actual）
// | KillProcessConfirmDialog | mid | in_degree:11 | 2 | 2 |
// | onButtonClicked | low | - | 1 | 7 |

// 初始结果为 0 且对话框未显示
TEST_F(UT_KillProcessConfirmDialog, Constructor_DefaultState_ResultIsZeroAndHidden)
{
    // Arrange
    const int kZeroResult = 0;
    // Act: 查询构造完成的对话框状态
    int result = m_tester->result();
    bool visible = m_tester->isVisible();
    // Assert
    EXPECT_EQ(result, kZeroResult);
    EXPECT_FALSE(visible);
}

// 构造函数未注册任何按钮，由外部调用方注册
TEST_F(UT_KillProcessConfirmDialog, Constructor_NoButtonsRegistered_GetButtonsEmpty)
{
    // Arrange
    const int kZeroResult = 0;
    // Act: 在用例内构造独立实例查询按钮注册情况
    KillProcessConfirmDialog *dlg = new KillProcessConfirmDialog();
    // Assert
    EXPECT_TRUE(dlg->getButtons().isEmpty());
    EXPECT_EQ(dlg->result(), kZeroResult);
    delete dlg;
}

// setResult/result 存取一致
TEST_F(UT_KillProcessConfirmDialog, Result_SetterGetter_RoundTripConsistent)
{
    // Arrange
    const int kInitial = m_tester->result();
    const int kOk = QMessageBox::Ok;
    EXPECT_EQ(kInitial, 0);
    // Act
    m_tester->setResult(kOk);
    // Assert
    EXPECT_EQ(m_tester->result(), kOk);
    EXPECT_NE(m_tester->result(), QMessageBox::Cancel);
}

// 点击确认（索引 1）后结果由 Cancel 变为 Ok
TEST_F(UT_KillProcessConfirmDialog, OnButtonClicked_OkIndex_SetsOkResult)
{
    // Arrange
    m_tester->onButtonClicked(0, "cancel");
    // Act
    m_tester->onButtonClicked(1, "kill");
    // Assert
    EXPECT_EQ(m_tester->result(), QMessageBox::Ok);
    EXPECT_NE(m_tester->result(), QMessageBox::Cancel);
}

namespace
{
struct BtnResultParam
{
    int index;
    int expected;
};

std::ostream &operator<<(std::ostream &os, const BtnResultParam &p)
{
    return os << "index=" << p.index;
}
}

// 按钮索引到结果的映射：1 为 Ok，其余索引一律 Cancel
class KillBtnResultMapping_Test
    : public UT_KillProcessConfirmDialog,
      public ::testing::WithParamInterface<BtnResultParam>
{
};

TEST_P(KillBtnResultMapping_Test, KillBtn_IndexToResultMapping_YieldsExpectedResult)
{
    // Arrange
    const auto &param = GetParam();
    // Act
    m_tester->onButtonClicked(param.index, "btn");
    // Assert
    EXPECT_EQ(m_tester->result(), param.expected);
    if (param.expected == QMessageBox::Ok) {
        EXPECT_NE(m_tester->result(), QMessageBox::Cancel);
    } else {
        EXPECT_NE(m_tester->result(), QMessageBox::Ok);
    }
}

INSTANTIATE_TEST_SUITE_P(KillProcessConfirmDialog,
                         KillBtnResultMapping_Test,
                         ::testing::Values(
                             BtnResultParam { 1, QMessageBox::Ok },
                             BtnResultParam { 0, QMessageBox::Cancel },
                             BtnResultParam { 2, QMessageBox::Cancel },
                             BtnResultParam { -1, QMessageBox::Cancel },
                             BtnResultParam { 100, QMessageBox::Cancel }));

// 按钮点击信号通过构造时的 connect 路由到 onButtonClicked
TEST_F(UT_KillProcessConfirmDialog, ButtonClickedSignal_RoutedToOnButtonClicked_UpdatesResult)
{
    // Arrange
    const int kOkIndex = 1;
    const int kCancelIndex = 0;
    // Act: 触发确认按钮（索引 1）信号
    QMetaObject::invokeMethod(m_tester, "buttonClicked",
                              Q_ARG(int, kOkIndex), Q_ARG(QString, "kill"));
    // Assert
    EXPECT_EQ(m_tester->result(), QMessageBox::Ok);
    // Act: 触发取消按钮（索引 0）信号
    QMetaObject::invokeMethod(m_tester, "buttonClicked",
                              Q_ARG(int, kCancelIndex), Q_ARG(QString, "cancel"));
    // Assert
    EXPECT_EQ(m_tester->result(), QMessageBox::Cancel);
    EXPECT_NE(m_tester->result(), QMessageBox::Ok);
}
