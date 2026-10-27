// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// =====================================================================================
// 用例计数声明表
// | 方法 | 等级 | factors | min | actual |
// |---|---|---|---|---|
// | ProcessAttributeDialog | mid | cx:0 | 1 | 3 |
// | closeEvent | low | cx:0 | 1 | 1 |
// | eventFilter | high | cx:8 | 1 | 7 |
// | initUI | mid | cx:0 | 1 | 2 |
// | resizeEvent | low | cx:0 | 1 | 1 |
// | resizeItemWidget | low | cx:0 | 1 | 1 |
// =====================================================================================

// 分支清单（来源：process_attribute_dialog.cpp ProcessAttributeDialog::eventFilter）
//   B1: type == Show → resizeItemWidget 重算条目尺寸 [EventFilter_ShowEvent_ResizeItemWidgets]
//   B2: type == FontChange → resizeItemWidget 重算条目尺寸 [EventFilter_FontChangeEvent_ResizeItemWidgets]
//   B3: type == KeyPress 分支进入 [EventFilter_EscKey_ClosesDialog]
//   B4: 键事件目标 obj == this → 校验 ESC [EventFilter_OtherKey_NotIntercepted]
//   B5: matches(Cancel) → close 并拦截（return true） [EventFilter_EscKey_ClosesDialog]
//   B6: type == MouseButtonPress 分支进入 [EventFilter_LeftClickNameViewport_ClearsSelections]
//   B7: LeftButton → 清名称/命令行选区 [EventFilter_LeftClickNameViewport_ClearsSelections]
//   B8: 点击对象父级为 DTextBrowser → setFocus 转移焦点 [EventFilter_LeftClickNameViewport_ClearsSelections]
//   B9: 开始时间有选区且 obj != 其 viewport → 一并清除（点 viewport 则保留, 见反例用例） [EventFilter_LeftClickStartViewport_KeepsStartSelection]

#include "process_attribute_dialog.h"
#include "settings.h"

#include <QScrollBar>

#include <gtest/gtest.h>

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMouseEvent>

class UT_ProcessAttributeDialog : public ::testing::Test
{
public:
    UT_ProcessAttributeDialog() : m_tester1(nullptr), m_tester2(nullptr) {}

protected:
    virtual void SetUp()
    {
        // Arrange(夹具): 两个对话框分别携带/不携带命令行, 挂在同一静态父窗口上
        static QWidget wid;
        QIcon icon;
        m_tester1 = new ProcessAttributeDialog(1, "1", "1", "1", icon, 1, &wid);
        m_tester2 = new ProcessAttributeDialog(1, "1", "1", "", icon, 1, &wid);
    }

    virtual void TearDown()
    {
        if (m_tester1) {
            m_tester1->setAttribute(Qt::WA_DeleteOnClose, false);
            delete m_tester1;
            m_tester1 = nullptr;
        }
        if (m_tester2) {
            m_tester2->setAttribute(Qt::WA_DeleteOnClose, false);
            delete m_tester2;
            m_tester2 = nullptr;
        }
    }

    ProcessAttributeDialog *m_tester1;
    ProcessAttributeDialog *m_tester2;
};

TEST_F(UT_ProcessAttributeDialog, Constructor_DefaultState_InitializesMembers)
{
    // Arrange: 夹具已以 pid=1, startTime=1 构造 m_tester1
    ASSERT_NE(m_tester1, nullptr);

    // Act: 复查夹具构造产物（对象与文本框均已建立）
    ASSERT_NE(m_tester1->m_procNameText, nullptr);

    // Assert: 名称/命令行/开始时间来自入参, 最小尺寸与关闭即删属性符合约定
    EXPECT_EQ(m_tester1->m_procNameText->toPlainText(), QString("1"));
    EXPECT_EQ(m_tester1->m_procCmdText->toPlainText(), QString("1"));
    EXPECT_EQ(m_tester1->m_procStartText->toPlainText().left(10), QString("1970-01-01"));
    EXPECT_EQ(m_tester1->minimumSize(), QSize(600, 480));
    EXPECT_TRUE(m_tester1->testAttribute(Qt::WA_DeleteOnClose));
}

TEST_F(UT_ProcessAttributeDialog, Constructor_EmptyCmdline_HidesCmdLabel)
{
    // Arrange: 夹具已以空命令行构造 m_tester2
    ASSERT_NE(m_tester2, nullptr);

    // Act: 复查夹具构造产物（对象与命令行框均已建立）
    ASSERT_NE(m_tester2->m_procCmdText, nullptr);

    // Assert: 空命令行 → 命令行标签隐藏, 文本为空; 非空命令行对话框标签可见
    EXPECT_EQ(m_tester2->m_procCmdText->toPlainText(), QString());
    EXPECT_TRUE(m_tester2->m_procCmdLabel->isHidden());
    EXPECT_FALSE(m_tester1->m_procCmdLabel->isHidden());
}

TEST_F(UT_ProcessAttributeDialog, Constructor_RestoresSavedSize_RestoresSize)
{
    // Arrange: 预写历史窗口尺寸到备份配置
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogWidth, 640);
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogHeight, 520);
    static QWidget holder;
    QIcon icon;

    // Act: 新建对话框触发 initUI 的尺寸恢复逻辑
    ProcessAttributeDialog dlg(1, "1", "1", "", icon, 1, &holder);

    // Assert: 窗口尺寸来自备份配置而非默认值
    EXPECT_EQ(dlg.width(), 640);
    EXPECT_EQ(dlg.height(), 520);

    // 还原备份配置, 避免影响后续用例
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogWidth, 600);
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogHeight, 480);
}

TEST_F(UT_ProcessAttributeDialog, CloseEvent_Closing_SavesDialogSize)
{
    // Arrange: 设定窗口尺寸
    m_tester1->resize(700, 520);
    QCloseEvent closeEv;

    // Act: 触发关闭事件
    m_tester1->closeEvent(&closeEv);

    // Assert: 当前尺寸已写入备份配置
    EXPECT_EQ(Settings::instance()->getOption(kSettingKeyProcessAttributeDialogWidth, 0).toInt(), 700);
    EXPECT_EQ(Settings::instance()->getOption(kSettingKeyProcessAttributeDialogHeight, 0).toInt(), 520);

    // 还原备份配置, 避免影响后续用例
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogWidth, 600);
    Settings::instance()->setOption(kSettingKeyProcessAttributeDialogHeight, 480);
}

TEST_F(UT_ProcessAttributeDialog, ResizeEvent_NewWidth_UpdatesShadowWidth)
{
    // Arrange: 先显示对话框, 使 resize 事件原生投递
    ASSERT_NE(m_tester1, nullptr);
    m_tester1->show();

    // Act: 改变窗口尺寸触发 resizeEvent
    m_tester1->resize(700, 500);

    // Assert: 阴影条宽度与中央框架同步, 高度保持固定 10
    EXPECT_EQ(m_tester1->m_tbShadow->width(), 700);
    EXPECT_EQ(m_tester1->m_tbShadow->width(), m_tester1->m_frame->width());
    EXPECT_EQ(m_tester1->m_tbShadow->height(), 10);
}

TEST_F(UT_ProcessAttributeDialog, ResizeItemWidget_PlainCall_FixesLabelSizes)
{
    // Arrange: 夹具已构造默认对话框
    ASSERT_NE(m_tester1, nullptr);

    // Act: 直接调用条目尺寸重算
    m_tester1->resizeItemWidget();

    // Assert: 标签宽度等于文本渲染宽度, 命令行滚动条复位, 开始时间文本有高度
    EXPECT_EQ(m_tester1->m_procNameLabel->width(),
              m_tester1->m_procNameLabel->fontMetrics().horizontalAdvance(m_tester1->m_procNameLabel->text()));
    EXPECT_EQ(m_tester1->m_procCmdText->verticalScrollBar()->value(), 0);
    EXPECT_GT(m_tester1->m_procStartText->height(), 0);
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_ShowEvent_ResizeItemWidgets)
{
    // Arrange: 先显示对话框使布局生效, 再派发 Show 事件重算条目尺寸
    m_tester1->show();
    QEvent showEv(QEvent::Show);
    const int expectW = m_tester1->m_procNameLabel->fontMetrics().horizontalAdvance(m_tester1->m_procNameLabel->text());

    // Act: 以对话框自身为过滤对象派发 Show 事件
    const bool handled = m_tester1->eventFilter(m_tester1, &showEv);

    // Assert: 条目尺寸已重算, 事件未被吞掉
    EXPECT_EQ(m_tester1->m_procNameLabel->width(), expectW);
    EXPECT_GT(m_tester1->m_procNameText->height(), 0);
    EXPECT_EQ(handled, false);
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_FontChangeEvent_ResizeItemWidgets)
{
    // Arrange: 先显示对话框使布局生效, 再派发 FontChange 事件
    m_tester1->show();
    QEvent fontEv(QEvent::FontChange);
    const int expectNameW = m_tester1->m_procNameLabel->fontMetrics().horizontalAdvance(m_tester1->m_procNameLabel->text());
    const int expectCmdW = m_tester1->m_procCmdLabel->fontMetrics().horizontalAdvance(m_tester1->m_procCmdLabel->text());

    // Act: 派发 FontChange 事件
    const bool handled = m_tester1->eventFilter(m_tester1, &fontEv);

    // Assert: 字体变化同样触发条目尺寸重算, 事件未被吞掉
    EXPECT_EQ(m_tester1->m_procNameLabel->width(), expectNameW);
    EXPECT_EQ(m_tester1->m_procCmdLabel->width(), expectCmdW);
    EXPECT_EQ(handled, false);
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_EscKey_ClosesDialog)
{
    // Arrange: 构造 ESC 按键事件; 先显示窗口, 并关闭"关闭即删"以便断言后复用
    QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    m_tester1->setAttribute(Qt::WA_DeleteOnClose, false);
    m_tester1->show();
    ASSERT_TRUE(m_tester1->isVisible());

    // Act: ESC 过滤逻辑
    const bool intercepted = m_tester1->eventFilter(m_tester1, &escEv);

    // Assert: ESC 被拦截并关闭窗口; Show 分支已同步条目尺寸
    EXPECT_TRUE(intercepted);
    EXPECT_FALSE(m_tester1->isVisible());
    EXPECT_EQ(m_tester1->m_procNameLabel->width(),
              m_tester1->m_procNameLabel->fontMetrics().horizontalAdvance(m_tester1->m_procNameLabel->text()));
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_OtherKey_NotIntercepted)
{
    // Arrange: 构造普通字母按键事件
    QKeyEvent keyEv(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act
    const bool intercepted = m_tester1->eventFilter(m_tester1, &keyEv);

    // Assert: 非 ESC 按键不被拦截, 对话框内容不受影响
    EXPECT_EQ(intercepted, false);
    EXPECT_EQ(m_tester1->m_procNameText->toPlainText(), QString("1"));
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_LeftClickNameViewport_ClearsSelections)
{
    // Arrange: 显示并激活对话框, 让三个文本框全部制造选区
    m_tester1->show();
    m_tester1->activateWindow();
    for (DTextBrowser *tb : { m_tester1->m_procNameText, m_tester1->m_procCmdText, m_tester1->m_procStartText }) {
        QTextCursor cursor = tb->textCursor();
        cursor.select(QTextCursor::Document);
        tb->setTextCursor(cursor);
    }
    QMouseEvent pressEv(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act: 以名称文本框 viewport 为过滤对象派发左键按下
    m_tester1->eventFilter(m_tester1->m_procNameText->viewport(), &pressEv);

    // Assert: 三处选区全部清除, 且名称文本框获得焦点
    EXPECT_FALSE(m_tester1->m_procNameText->textCursor().hasSelection());
    EXPECT_FALSE(m_tester1->m_procCmdText->textCursor().hasSelection());
    EXPECT_FALSE(m_tester1->m_procStartText->textCursor().hasSelection());
    EXPECT_EQ(m_tester1->focusWidget(), m_tester1->m_procNameText);
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_LeftClickStartViewport_KeepsStartSelection)
{
    // Arrange: 显示并激活对话框, 让三个文本框全部制造选区
    m_tester1->show();
    m_tester1->activateWindow();
    for (DTextBrowser *tb : { m_tester1->m_procNameText, m_tester1->m_procCmdText, m_tester1->m_procStartText }) {
        QTextCursor cursor = tb->textCursor();
        cursor.select(QTextCursor::Document);
        tb->setTextCursor(cursor);
    }
    QMouseEvent pressEv(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act: 以开始时间文本框 viewport 为过滤对象派发左键按下
    m_tester1->eventFilter(m_tester1->m_procStartText->viewport(), &pressEv);

    // Assert: 名称/命令行选区被清除, 开始时间选区因 obj 即其 viewport 而保留, 焦点转到开始时间框
    EXPECT_FALSE(m_tester1->m_procNameText->textCursor().hasSelection());
    EXPECT_FALSE(m_tester1->m_procCmdText->textCursor().hasSelection());
    EXPECT_TRUE(m_tester1->m_procStartText->textCursor().hasSelection());
    EXPECT_EQ(m_tester1->focusWidget(), m_tester1->m_procStartText);
}

TEST_F(UT_ProcessAttributeDialog, EventFilter_RightClickNameViewport_KeepsAllSelections)
{
    // Arrange: 三个文本框全部制造选区, 使用右键事件
    for (DTextBrowser *tb : { m_tester1->m_procNameText, m_tester1->m_procCmdText, m_tester1->m_procStartText }) {
        QTextCursor cursor = tb->textCursor();
        cursor.select(QTextCursor::Document);
        tb->setTextCursor(cursor);
    }
    QMouseEvent pressEv(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5),
                        Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    // Act: 派发右键按下
    m_tester1->eventFilter(m_tester1->m_procNameText->viewport(), &pressEv);

    // Assert: 非左键不触发选区清理, 焦点也不转移到名称文本框
    EXPECT_TRUE(m_tester1->m_procNameText->textCursor().hasSelection());
    EXPECT_TRUE(m_tester1->m_procCmdText->textCursor().hasSelection());
    EXPECT_TRUE(m_tester1->m_procStartText->textCursor().hasSelection());
    EXPECT_NE(QApplication::focusWidget(), m_tester1->m_procNameText);
}
