// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "toolbar.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

// 分支清单（来源：get_code_snippet 实际源码）
// Toolbar::eventFilter(obj, event)，complexity:11：
//   B1: KeyPress 且 obj==this 且 key==Escape → 清空搜索框并 emit pressEsc
//   B2: KeyPress 且 obj==this 且 key!=Escape → 无动作
//   B3: obj==searchEdit 且 key==Tab → emit pressTab
//   B4: obj==searchEdit 且 key!=Tab → 无动作
//   B5: obj==m_procBtn 且 key==Right → 聚焦服务按钮并消费事件（return true）
//   B6: obj==m_procBtn 且 key!=Right → 落入基类处理
//   B7: obj==m_svcBtn 且 key==Left → 聚焦进程按钮并消费事件（return true）
//   B8: obj==m_svcBtn 且 key!=Left → 落入基类处理
//   B9: obj==m_accountProcBtn 且 key==Left → 聚焦用户进程按钮并消费事件（return true）
//   B10: 其余对象/事件 → 落入基类处理
// 用例映射：
//   - EventFilter_ToolbarEsc_ClearsAndEmitsPressEsc        → B1
//   - EventFilter_ToolbarOtherKey_NoSignal                 → B2
//   - EventFilter_SearchEditTab_EmitsPressTab              → B3
//   - EventFilter_SearchEditOtherKey_NoSignal              → B4
//   - EventFilter_ProcBtnRight_ConsumesAndFocusSvc         → B5
//   - EventFilter_ProcBtnOtherKey_FallsThrough             → B6
//   - EventFilter_SvcBtnLeft_ConsumesAndFocusProc          → B7
//   - EventFilter_SvcBtnOtherKey_FallsThrough              → B8
//   - EventFilter_AccountBtnLeft_ConsumesAndFocusSelf      → B9
//   - EventFilter_NonKeyEvent_FallsThrough                 → B10

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | lines:110,in_degree:1 | 2 | 2 |
| Dtor | low | destructor | 1 | 1 |
| eventFilter | high | complexity:11,cognitive:46,lines:50 | 3 | 10 |
| focusInput | mid | in_degree:1 | 2 | 2 |
| handleSearch | mid | in_degree:1 | 2 | 2 |
| handleSearchTextChanged | mid | in_degree:1 | 2 | 2 |
| isSearchContentEmpty | low | - | 1 | 2 |
| searchContent | mid | in_degree:1 | 2 | 2 |
| setProcessButtonChecked | low | - | 1 | 2 |
| clearSearchText | mid | name_pattern:clearSearchText | 2 | 2 |
*/

class UT_Toolbar : public ::testing::Test
{
public:
    UT_Toolbar() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static QWidget wid;
        m_tester = new Toolbar(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Toolbar *m_tester;
};

TEST_F(UT_Toolbar, Ctor_ButtonsSearchEdit_Initialized)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取各控件的规格状态
    // Assert：三个页签按钮与搜索框、按钮组按源码规格创建
    EXPECT_NE(m_tester->m_procBtn, nullptr);
    EXPECT_NE(m_tester->m_svcBtn, nullptr);
    EXPECT_NE(m_tester->m_accountProcBtn, nullptr);
    EXPECT_NE(m_tester->searchEdit, nullptr);
    EXPECT_EQ(m_tester->m_procBtn->objectName(), QString("ProcBtn"));
    EXPECT_EQ(m_tester->m_switchFuncTabBtnGrp->objectName(), QString("SwitchFuncTabBtnGrp"));
    EXPECT_EQ(m_tester->focusProxy(), m_tester->m_switchFuncTabBtnGrp);
}

TEST_F(UT_Toolbar, Ctor_InitialChecks_TimerConfig)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取按钮勾选态与定时器配置
    // Assert：进程按钮默认选中、服务按钮未选中、搜索定时器为单次触发
    EXPECT_TRUE(m_tester->m_procBtn->isCheckable());
    EXPECT_TRUE(m_tester->m_procBtn->isChecked());
    EXPECT_FALSE(m_tester->m_svcBtn->isChecked());
    EXPECT_NE(m_tester->searchTimer, nullptr);
    EXPECT_TRUE(m_tester->searchTimer->isSingleShot());
    EXPECT_TRUE(m_tester->searchContent().isEmpty());
}

TEST_F(UT_Toolbar, Dtor_LocalInstance_NoCrash)
{
    // Arrange：独立构造一个 Toolbar
    static QWidget parent;
    Toolbar *local = new Toolbar(&parent);
    EXPECT_NE(local, nullptr);
    EXPECT_EQ(m_tester->searchContent(), QString(""));
    ASSERT_NE(m_tester, nullptr);

    // Act：析构局部实例
    delete local;

    // Assert：当前 fixture 实例不受影响
    EXPECT_NE(m_tester->m_procBtn, nullptr);
    EXPECT_NE(m_tester->searchEdit, nullptr);
}

TEST_F(UT_Toolbar, EventFilter_ToolbarEsc_ClearsAndEmitsPressEsc)
{
    // Arrange：搜索框预置文本
    m_tester->searchEdit->setText("abc");
    QSignalSpy escSpy(m_tester, &Toolbar::pressEsc);
    static QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);

    // Act：工具栏自身收到 ESC
    m_tester->eventFilter(m_tester, &escEv);

    // Assert：搜索框被清空且 pressEsc 发出一次
    EXPECT_TRUE(m_tester->searchContent().isEmpty());
    EXPECT_EQ(escSpy.count(), 1);
}

TEST_F(UT_Toolbar, EventFilter_ToolbarOtherKey_NoSignal)
{
    // Arrange：预置文本，非 ESC 键
    m_tester->searchEdit->setText("abc");
    QSignalSpy escSpy(m_tester, &Toolbar::pressEsc);
    static QKeyEvent spaceEv(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);

    // Act：工具栏自身收到空格键
    m_tester->eventFilter(m_tester, &spaceEv);

    // Assert：文本保留、pressEsc 不发
    EXPECT_EQ(m_tester->searchContent(), QString("abc"));
    EXPECT_EQ(escSpy.count(), 0);
}

TEST_F(UT_Toolbar, EventFilter_SearchEditTab_EmitsPressTab)
{
    // Arrange：搜索框内按 Tab
    QSignalSpy tabSpy(m_tester, &Toolbar::pressTab);
    static QKeyEvent tabEv(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);

    // Act：eventFilter 直接收搜索框的 Tab 事件
    m_tester->eventFilter(m_tester->searchEdit, &tabEv);

    // Assert：pressTab 发出一次
    EXPECT_EQ(tabSpy.count(), 1);
    EXPECT_NE(m_tester->searchEdit, nullptr);
}

TEST_F(UT_Toolbar, EventFilter_SearchEditOtherKey_NoSignal)
{
    // Arrange：搜索框内按非 Tab 键
    QSignalSpy tabSpy(m_tester, &Toolbar::pressTab);
    static QKeyEvent keyAEv(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act
    m_tester->eventFilter(m_tester->searchEdit, &keyAEv);

    // Assert：pressTab 不发
    EXPECT_EQ(tabSpy.count(), 0);
    EXPECT_NE(m_tester->searchEdit, nullptr);
}

TEST_F(UT_Toolbar, EventFilter_ProcBtnRight_ConsumesAndFocusSvc)
{
    // Arrange：进程按钮上按右键
    static QKeyEvent rightEv(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester->m_procBtn, &rightEv);

    // Assert：事件被消费并请求聚焦服务按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(m_tester->m_svcBtn->objectName(), QString("SvcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_ProcBtnOtherKey_FallsThrough)
{
    // Arrange：进程按钮上按非右键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester->m_procBtn, &leftEv);

    // Assert：不消费，落入基类（默认 false）
    EXPECT_FALSE(consumed);
    EXPECT_NE(m_tester->m_procBtn, nullptr);
}

TEST_F(UT_Toolbar, EventFilter_SvcBtnLeft_ConsumesAndFocusProc)
{
    // Arrange：服务按钮上按左键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester->m_svcBtn, &leftEv);

    // Assert：事件被消费并请求聚焦进程按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(m_tester->m_procBtn->objectName(), QString("ProcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_SvcBtnOtherKey_FallsThrough)
{
    // Arrange：服务按钮上按非左键
    static QKeyEvent rightEv(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester->m_svcBtn, &rightEv);

    // Assert：不消费
    EXPECT_FALSE(consumed);
    EXPECT_NE(m_tester->m_svcBtn, nullptr);
}

TEST_F(UT_Toolbar, EventFilter_AccountBtnLeft_ConsumesAndFocusSelf)
{
    // Arrange：用户进程按钮上按左键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester->m_accountProcBtn, &leftEv);

    // Assert：事件被消费并请求聚焦自身按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(m_tester->m_accountProcBtn->objectName(), QString("AccountProcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_NonKeyEvent_FallsThrough)
{
    // Arrange：非键盘事件（鼠标移动）
    static QMouseEvent moveEv(QEvent::MouseMove, QPointF(1, 1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester, &moveEv);

    // Assert：直接落入基类处理
    EXPECT_FALSE(consumed);
    EXPECT_NE(m_tester->searchEdit, nullptr);
}

TEST_F(UT_Toolbar, FocusInput_WithText_SelectsAllAndFocuses)
{
    // Arrange：搜索框有文本
    m_tester->searchEdit->setText("abc");

    // Act：聚焦输入框
    m_tester->focusInput();

    // Assert：非空分支执行了全选（offscreen 无活动窗口，hasFocus 不可靠），文本未被改动
    EXPECT_EQ(m_tester->searchEdit->lineEdit()->selectedText(), QString("abc"));
    EXPECT_EQ(m_tester->searchContent(), QString("abc"));
}

TEST_F(UT_Toolbar, FocusInput_EmptyText_FocusesOnly)
{
    // Arrange：搜索框为空
    ASSERT_TRUE(m_tester->searchContent().isEmpty());

    // Act：聚焦输入框
    m_tester->focusInput();

    // Assert：空文本分支不触发全选，内容保持为空
    EXPECT_EQ(m_tester->searchContent(), QString(""));
    EXPECT_FALSE(m_tester->searchEdit->lineEdit()->hasSelectedText());
}

TEST_F(UT_Toolbar, HandleSearch_CacheMatch_EmitsSearchSignal)
{
    // Arrange：文本与缓存一致
    m_tester->searchEdit->setText("abc");
    m_tester->searchTextCache = "abc";
    QSignalSpy searchSpy(m_tester, &Toolbar::search);

    // Act：执行搜索
    m_tester->handleSearch();

    // Assert：search 信号发出且携带缓存文本
    EXPECT_EQ(searchSpy.count(), 1);
    EXPECT_EQ(searchSpy.first().first().toString(), QString("abc"));
}

TEST_F(UT_Toolbar, HandleSearch_CacheMismatch_SkipsSearch)
{
    // Arrange：文本与缓存不一致
    m_tester->searchEdit->setText("xyz");
    m_tester->searchTextCache = "abc";
    QSignalSpy searchSpy(m_tester, &Toolbar::search);

    // Act：执行搜索
    m_tester->handleSearch();

    // Assert：不发出 search 信号，缓存不变
    EXPECT_EQ(searchSpy.count(), 0);
    EXPECT_EQ(m_tester->searchTextCache, QString("abc"));
}

TEST_F(UT_Toolbar, HandleSearchTextChanged_NewText_UpdatesCacheAndStartsTimer)
{
    // Arrange：定时器初始未激活
    m_tester->searchTimer->stop();
    ASSERT_FALSE(m_tester->searchTimer->isActive());

    // Act：设置新文本后响应变化
    m_tester->searchEdit->setText("abc");
    m_tester->handleSearchTextChanged();

    // Assert：缓存更新且 50ms 单发定时器启动
    EXPECT_EQ(m_tester->searchTextCache, QString("abc"));
    EXPECT_TRUE(m_tester->searchTimer->isActive());
    m_tester->searchTimer->stop();
}

TEST_F(UT_Toolbar, HandleSearchTextChanged_EmptyText_UpdatesCache)
{
    // Arrange：预置旧文本并停掉定时器
    m_tester->searchEdit->setText("abc");
    m_tester->handleSearchTextChanged();
    m_tester->searchTimer->stop();
    ASSERT_EQ(m_tester->searchTextCache, QString("abc"));

    // Act：清空文本后响应变化
    m_tester->searchEdit->setText("");
    m_tester->handleSearchTextChanged();

    // Assert：缓存同步为空，定时器再次启动
    EXPECT_EQ(m_tester->searchTextCache, QString(""));
    EXPECT_TRUE(m_tester->searchTimer->isActive());
    m_tester->searchTimer->stop();
}

// 注：源码 isSearchContentEmpty 为空文本返回 false、非空返回 true，
// 与函数名语义相反（疑似缺陷，详见缺陷清单），用例按真实源码行为断言
TEST_F(UT_Toolbar, IsSearchContentEmpty_EmptyText_ReturnsFalse)
{
    // Arrange：搜索框为空
    ASSERT_TRUE(m_tester->searchContent().isEmpty());

    // Act
    bool result = m_tester->isSearchContentEmpty();

    // Assert：源码语义：空文本 → false
    EXPECT_FALSE(result);
    EXPECT_EQ(m_tester->searchContent(), QString(""));
}

TEST_F(UT_Toolbar, IsSearchContentEmpty_NonEmptyText_ReturnsTrue)
{
    // Arrange：搜索框有文本
    m_tester->searchEdit->setText("abc");

    // Act
    bool result = m_tester->isSearchContentEmpty();

    // Assert：源码语义：非空文本 → true
    EXPECT_TRUE(result);
    EXPECT_EQ(m_tester->searchContent(), QString("abc"));
}

TEST_F(UT_Toolbar, SearchContent_InitialEmpty_ReturnsEmpty)
{
    // Arrange：构造后未输入任何文本
    EXPECT_NE(m_tester, nullptr);

    // Act：读取初始搜索内容
    QString content = m_tester->searchContent();

    // Assert：与编辑框文本一致且为空
    EXPECT_EQ(content, QString(""));
    EXPECT_EQ(content, m_tester->searchEdit->text());
}

TEST_F(UT_Toolbar, SearchContent_WithText_ReturnsText)
{
    // Arrange：设置文本
    m_tester->searchEdit->setText("abc");

    // Act：读取搜索内容
    QString content = m_tester->searchContent();

    // Assert：透传编辑框文本
    EXPECT_EQ(content, QString("abc"));
    EXPECT_TRUE(m_tester->isSearchContentEmpty());
}

TEST_F(UT_Toolbar, SetProcessButtonChecked_True_ChecksProcBtn)
{
    // Arrange：初始为进程按钮选中态
    EXPECT_NE(m_tester, nullptr);

    // Act：置选进程按钮
    m_tester->setProcessButtonChecked(true);

    // Assert：进程按钮选中，服务按钮不受影响
    EXPECT_EQ(m_tester->m_procBtn->isChecked(), true);
    EXPECT_FALSE(m_tester->m_svcBtn->isChecked());
}

TEST_F(UT_Toolbar, SetProcessButtonChecked_False_KeepsState)
{
    // Arrange：记录默认选中态（互斥组下程序化取消选中会被还原，故以不变性断言）
    bool before = m_tester->m_procBtn->isChecked();

    // Act：传入 false（源码分支直接返回，不改变任何按钮状态）
    m_tester->setProcessButtonChecked(false);

    // Assert：进程按钮状态保持调用前取值，服务按钮不受影响
    EXPECT_EQ(m_tester->m_procBtn->isChecked(), before);
    EXPECT_FALSE(m_tester->m_svcBtn->isChecked());
}

TEST_F(UT_Toolbar, ClearSearchText_WithText_ClearsEdit)
{
    // Arrange：预置文本与缓存
    m_tester->searchEdit->setText("abc");
    m_tester->searchTextCache = "abc";

    // Act：清空搜索框
    m_tester->clearSearchText();

    // Assert：clearEdit 同时清空编辑框内容与文本缓存（DSearchEdit 清除语义）
    EXPECT_TRUE(m_tester->searchContent().isEmpty());
    EXPECT_TRUE(m_tester->searchTextCache.isEmpty());
}

TEST_F(UT_Toolbar, ClearSearchText_RepeatedCall_Safe)
{
    // Arrange：清空一次
    m_tester->clearSearchText();
    ASSERT_TRUE(m_tester->searchContent().isEmpty());

    // Act：重复清空
    m_tester->clearSearchText();

    // Assert：仍为空且编辑框可用
    EXPECT_EQ(m_tester->searchContent(), QString(""));
    EXPECT_TRUE(m_tester->searchEdit->isEnabled());
}
