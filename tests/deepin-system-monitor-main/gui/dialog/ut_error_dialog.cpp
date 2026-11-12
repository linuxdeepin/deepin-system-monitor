// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "dialog/error_dialog.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//Qt
#include <QLabel>
#include <QPushButton>
#include <QWidget>

/***************************************STUB begin*********************************************/
// exec() 打桩计数：验证静态 show() 确实以模态方式呈现对话框（exec 恰被调用一次），
// 同时避免测试进入真实事件循环。
static int g_execCallCount = 0;
static void stub_exec_counter(void *)
{
    ++g_execCallCount;
}
/***************************************STUB end**********************************************/

class UT_ErrorDialog : public ::testing::Test
{
public:
    UT_ErrorDialog() : m_tester(nullptr) {}

    static void SetUpTestSuite()
    {
        g_execCallCount = 0;
    }

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new ErrorDialog("error-message", "error-detail", &wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ErrorDialog *m_tester;
};

// 用例覆盖说明（源码 gui/dialog/error_dialog.cpp）：
// - ErrorDialog()    ：私有构造，委托 initUI() 完成全部 UI 装配
// - initUI()         ：图标/属性/最大宽度/详情标签/OK 按钮（无运行时分支）
// - show()           ：静态入口，new ErrorDialog + exec()（exec 打桩隔离）
// 无 if/switch/throw 运行时分支（#if QT_VERSION 为编译期分支），无需分支清单。

TEST_F(UT_ErrorDialog, Ctor_DefaultStrings_InitializesDialogContentAndProps)
{
    // Arrange：构造参数与父窗口
    static QWidget wid;

    // Act：构造对话框（initUI 在 ctor 内执行）
    ErrorDialog dlg("error-message", "error-detail", &wid);

    // Assert：详情标签文本、错误成员、窗口属性、按钮装配逐项核验
    EXPECT_EQ(dlg.m_detail, QStringLiteral("error-detail"));
    EXPECT_EQ(dlg.m_errMsg, QStringLiteral("error-message"));
    ASSERT_NE(dlg.m_detailLabel, nullptr);
    EXPECT_EQ(dlg.m_detailLabel->text(), QStringLiteral("error-detail"));
    EXPECT_TRUE(dlg.m_detailLabel->wordWrap());
    EXPECT_EQ(dlg.maximumWidth(), 720);
    EXPECT_TRUE(dlg.testAttribute(Qt::WA_DeleteOnClose));
    EXPECT_TRUE(dlg.windowFlags() & Qt::BypassWindowManagerHint);
    EXPECT_EQ(dlg.buttonCount(), 1);
}

TEST_F(UT_ErrorDialog, Ctor_EmptyStrings_StillBuildsUsableDialog)
{
    // Arrange：空串输入（负面等价类）
    static QWidget wid;

    // Act
    ErrorDialog emptyDlg("", "", &wid);

    // Assert：空输入不崩溃，骨架仍然装配完整（强异常安全：状态未损坏）
    EXPECT_TRUE(emptyDlg.m_errMsg.isEmpty());
    EXPECT_TRUE(emptyDlg.m_detail.isEmpty());
    ASSERT_NE(emptyDlg.m_detailLabel, nullptr);
    EXPECT_TRUE(emptyDlg.m_detailLabel->text().isEmpty());
    EXPECT_EQ(emptyDlg.buttonCount(), 1);
    EXPECT_EQ(emptyDlg.maximumWidth(), 720);
}

TEST_F(UT_ErrorDialog, Show_StaticEntry_ExecutesDialogExactlyOnce)
{
    // Arrange：打桩 DDialog::exec 计数，避免真实模态事件循环
    typedef int (*fptr)(DDialog *);
    fptr DDialog_exec = (fptr)(&DDialog::exec);
    Stub stub;
    stub.set(DDialog_exec, stub_exec_counter);
    const int before = g_execCallCount;
    static QWidget wid;

    // Act：静态 show 分别以常规输入与空串输入呈现对话框
    ErrorDialog::show(&wid, "msg", "detail");
    const int afterFirst = g_execCallCount;
    ErrorDialog::show(&wid, "", "");

    // Assert：每次调用 exec 恰被触发一次（模态呈现语义，空串输入同样成立）
    EXPECT_EQ(afterFirst, before + 1);
    EXPECT_EQ(g_execCallCount, before + 2);
}

TEST_F(UT_ErrorDialog, InitUI_ReInitAfterCtor_AppendsSecondButtonOnly)
{
    // Arrange：ctor 已执行一次 initUI（buttonCount == 1）
    ASSERT_EQ(m_tester->buttonCount(), 1);

    // Act：再次调用 initUI（重复初始化路径）
    m_tester->initUI();

    // Assert：仅追加按钮，已装配内容不被破坏
    EXPECT_EQ(m_tester->buttonCount(), 2);
    EXPECT_EQ(m_tester->m_detailLabel->text(), QStringLiteral("error-detail"));
    EXPECT_EQ(m_tester->maximumWidth(), 720);
}
