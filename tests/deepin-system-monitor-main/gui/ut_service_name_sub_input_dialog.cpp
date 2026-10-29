// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "service_name_sub_input_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_ServiceNameSubInputDialog : public ::testing::Test
{
public:
    UT_ServiceNameSubInputDialog() : m_tester(nullptr) {}

    void SetUp() override
    {
        m_tester = new ServiceNameSubInputDialog();
    }

    void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    // 定位内部输入框
    DLineEdit *lineEdit()
    {
        return m_tester->findChild<DLineEdit *>("NameLineEdit");
    }

    // 定位 OK 按钮
    QPushButton *okButton()
    {
        return dynamic_cast<QPushButton *>(m_tester->getButton(m_tester->getButtonIndexByText("OK")));
    }

protected:
    ServiceNameSubInputDialog *m_tester;
};

// MAX_SERVICE_NAME 在源文件中定义为 30
static const int kMaxServiceName = 30;

// 用例计数声明表（方法 | level | factors | min | actual）
// | ServiceNameSubInputDialog | mid | lines:50;in_degree:8 | 2 | 2 |
// | onButtonClicked | low | - | 1 | 7 |
// | textChanged | low | - | 1 | 4 |

// 构造函数创建输入框与 OK/Cancel 按钮，OK 默认不可用
TEST_F(UT_ServiceNameSubInputDialog, Constructor_DefaultState_EnablesOkOnlyAfterInput)
{
    // Arrange
    const QString kInputBoxName = "NameLineEdit";
    // Act: 查询构造完成的对话框内部控件状态
    DLineEdit *edit = m_tester->findChild<DLineEdit *>(kInputBoxName);
    QPushButton *ok = okButton();
    // Assert
    EXPECT_NE(edit, nullptr);
    EXPECT_NE(ok, nullptr);
    EXPECT_FALSE(ok->isEnabled());
}

// 构造函数注册 OK 与 Cancel 两个按钮
TEST_F(UT_ServiceNameSubInputDialog, Constructor_DefaultButtons_OkAndCancelRegistered)
{
    // Arrange
    const QString kOkText = "OK";
    const QString kCancelText = "Cancel";
    // Act: 查询构造完成的对话框按钮注册情况
    int okIndex = m_tester->getButtonIndexByText(kOkText);
    int cancelIndex = m_tester->getButtonIndexByText(kCancelText);
    // Assert
    EXPECT_GE(okIndex, 0);
    EXPECT_GE(cancelIndex, 0);
    EXPECT_NE(okIndex, cancelIndex);
}

// 输入非空文本后 OK 按钮可用且无告警
TEST_F(UT_ServiceNameSubInputDialog, TextChanged_NonEmptyText_EnablesOkButton)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    const QString kText = "svc-01";
    // Act
    lineEdit()->setText(kText);
    // Assert
    EXPECT_EQ(lineEdit()->text().length(), kText.length());
    EXPECT_TRUE(okButton()->isEnabled());
    EXPECT_FALSE(lineEdit()->isAlert());
}

// 清空文本后 OK 按钮恢复不可用且无告警
TEST_F(UT_ServiceNameSubInputDialog, TextChanged_EmptyText_DisablesOkButton)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    lineEdit()->setText("svc-01");
    // Act
    lineEdit()->setText("");
    // Assert
    EXPECT_EQ(lineEdit()->text().length(), 0);
    EXPECT_FALSE(okButton()->isEnabled());
    EXPECT_FALSE(lineEdit()->isAlert());
}

// 文本长度达到上限时输入框进入告警状态且 OK 仍可用
TEST_F(UT_ServiceNameSubInputDialog, TextChanged_TextAtMaxLength_SetsAlert)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    const QString kMaxText = QString(kMaxServiceName, QLatin1Char('a'));
    // Act
    lineEdit()->setText(kMaxText);
    // Assert
    EXPECT_EQ(lineEdit()->text().length(), kMaxServiceName);
    EXPECT_TRUE(lineEdit()->isAlert());
    EXPECT_TRUE(okButton()->isEnabled());
}

// 文本长度未达上限时输入框不进入告警状态
TEST_F(UT_ServiceNameSubInputDialog, TextChanged_TextBelowMaxLength_NoAlert)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    const QString kBelowMaxText = QString(kMaxServiceName - 1, QLatin1Char('a'));
    // Act
    lineEdit()->setText(kBelowMaxText);
    // Assert
    EXPECT_EQ(lineEdit()->text().length(), kMaxServiceName - 1);
    EXPECT_FALSE(lineEdit()->isAlert());
    EXPECT_TRUE(okButton()->isEnabled());
}

// 点击确认按钮保存子名称并返回 Ok 结果
TEST_F(UT_ServiceNameSubInputDialog, OnButtonClicked_OkIndex_SavesNameAndSetsOkResult)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    lineEdit()->setText("instance-a");
    // Act
    m_tester->onButtonClicked(0, "OK");
    // Assert
    EXPECT_EQ(m_tester->getServiceSubName(), QString("instance-a"));
    EXPECT_EQ(m_tester->result(), QMessageBox::Ok);
}

// 点击取消按钮不保存子名称并返回 Cancel 结果
TEST_F(UT_ServiceNameSubInputDialog, OnButtonClicked_CancelIndex_KeepsNameAndSetsCancelResult)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    lineEdit()->setText("instance-a");
    // Act
    m_tester->onButtonClicked(1, "Cancel");
    // Assert
    EXPECT_EQ(m_tester->result(), QMessageBox::Cancel);
    EXPECT_TRUE(m_tester->getServiceSubName().isEmpty());
}

namespace
{
struct BtnResultParam
{
    int index;
    int expected;
    bool savesName;
};

std::ostream &operator<<(std::ostream &os, const BtnResultParam &p)
{
    return os << "index=" << p.index;
}
}

// 按钮索引到结果的映射：0 为 Ok，其余索引一律 Cancel
class OnButtonClicked_ResultMapping_Test
    : public UT_ServiceNameSubInputDialog,
      public ::testing::WithParamInterface<BtnResultParam>
{
};

TEST_P(OnButtonClicked_ResultMapping_Test, OnButtonClicked_IndexToResultMapping_YieldsExpectedResult)
{
    // Arrange
    const auto &param = GetParam();
    ASSERT_NE(lineEdit(), nullptr);
    lineEdit()->setText("instance-b");
    // Act
    m_tester->onButtonClicked(param.index, "btn");
    // Assert
    EXPECT_EQ(m_tester->result(), param.expected);
    if (param.savesName) {
        EXPECT_EQ(m_tester->getServiceSubName(), QString("instance-b"));
    } else {
        EXPECT_TRUE(m_tester->getServiceSubName().isEmpty());
    }
}

INSTANTIATE_TEST_SUITE_P(ServiceNameSubInputDialog,
                         OnButtonClicked_ResultMapping_Test,
                         ::testing::Values(
                             BtnResultParam { 0, QMessageBox::Ok, true },
                             BtnResultParam { 1, QMessageBox::Cancel, false },
                             BtnResultParam { 2, QMessageBox::Cancel, false },
                             BtnResultParam { -1, QMessageBox::Cancel, false }));

// 点击按钮信号通过构造时的 connect 路由到 onButtonClicked
TEST_F(UT_ServiceNameSubInputDialog, ButtonClickedSignal_RoutedToOnButtonClicked_UpdatesResult)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    lineEdit()->setText("instance-c");
    // Act
    QMetaObject::invokeMethod(m_tester, "buttonClicked",
                              Q_ARG(int, 0), Q_ARG(QString, "OK"));
    // Assert
    EXPECT_EQ(m_tester->result(), QMessageBox::Ok);
    EXPECT_EQ(m_tester->getServiceSubName(), QString("instance-c"));
}

// setServiceSubName/getServiceSubName 存取一致
TEST_F(UT_ServiceNameSubInputDialog, ServiceSubName_SetterGetter_RoundTripConsistent)
{
    // Arrange
    const QString kName = "round-trip";
    // Act
    m_tester->setServiceSubName(kName);
    // Assert
    EXPECT_EQ(m_tester->getServiceSubName(), kName);
    EXPECT_FALSE(m_tester->getServiceSubName().isEmpty());
}

// 显示后焦点落在内部输入框（窗口内 focusWidget，不依赖平台窗口激活态）
TEST_F(UT_ServiceNameSubInputDialog, ShowEvent_ShownDialog_FocusesLineEditor)
{
    // Arrange
    ASSERT_NE(lineEdit(), nullptr);
    m_tester->show();
    // Act
    QShowEvent ev;
    m_tester->showEvent(&ev);
    // Assert
    EXPECT_TRUE(m_tester->isVisible());
    EXPECT_EQ(lineEdit()->lineEdit()->window()->focusWidget(), lineEdit()->lineEdit());
    EXPECT_NE(m_tester->getButtons().size(), 0);
}
