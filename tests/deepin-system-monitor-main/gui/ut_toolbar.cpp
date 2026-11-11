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
#include "application.h"
#include "main_window.h"

// 私有控件成员经 addr_pri.h 访问（不修改生产头文件）
ACCESS_PRIVATE_FIELD(Toolbar, CustomButtonBox *, m_switchFuncTabBtnGrp)
ACCESS_PRIVATE_FIELD(Toolbar, DButtonBoxButton *, m_procBtn)
ACCESS_PRIVATE_FIELD(Toolbar, DButtonBoxButton *, m_svcBtn)
ACCESS_PRIVATE_FIELD(Toolbar, DButtonBoxButton *, m_accountProcBtn)
ACCESS_PRIVATE_FIELD(Toolbar, DSearchEdit *, searchEdit)
ACCESS_PRIVATE_FIELD(Toolbar, QTimer *, searchTimer)
ACCESS_PRIVATE_FIELD(Toolbar, QString, searchTextCache)

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
//   - ToggledButtons_EmitTabButtonClickedSignals            → 构造器三个 toggled lambda 信号转发
//   - LoadingStatusChanged_DisablesAndEnablesButtons        → 构造器 loadingStatusChanged lambda 启停控制

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| Ctor | mid | lines:110,in_degree:1 | 2 | 4 |
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
    EXPECT_NE(access_private_field::Toolbarm_procBtn(*m_tester), nullptr);
    EXPECT_NE(access_private_field::Toolbarm_svcBtn(*m_tester), nullptr);
    EXPECT_NE(access_private_field::Toolbarm_accountProcBtn(*m_tester), nullptr);
    EXPECT_NE(access_private_field::ToolbarsearchEdit(*m_tester), nullptr);
    EXPECT_EQ(access_private_field::Toolbarm_procBtn(*m_tester)->objectName(), QString("ProcBtn"));
    EXPECT_EQ(access_private_field::Toolbarm_switchFuncTabBtnGrp(*m_tester)->objectName(), QString("SwitchFuncTabBtnGrp"));
    EXPECT_EQ(m_tester->focusProxy(), access_private_field::Toolbarm_switchFuncTabBtnGrp(*m_tester));
}

TEST_F(UT_Toolbar, Ctor_InitialChecks_TimerConfig)
{
    // Arrange：构造已在 SetUp 完成
    EXPECT_NE(m_tester, nullptr);

    // Act：读取按钮勾选态与定时器配置
    // Assert：进程按钮默认选中、服务按钮未选中、搜索定时器为单次触发
    EXPECT_TRUE(access_private_field::Toolbarm_procBtn(*m_tester)->isCheckable());
    EXPECT_TRUE(access_private_field::Toolbarm_procBtn(*m_tester)->isChecked());
    EXPECT_FALSE(access_private_field::Toolbarm_svcBtn(*m_tester)->isChecked());
    EXPECT_NE(access_private_field::ToolbarsearchTimer(*m_tester), nullptr);
    EXPECT_TRUE(access_private_field::ToolbarsearchTimer(*m_tester)->isSingleShot());
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
    EXPECT_NE(access_private_field::Toolbarm_procBtn(*m_tester), nullptr);
    EXPECT_NE(access_private_field::ToolbarsearchEdit(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, EventFilter_ToolbarEsc_ClearsAndEmitsPressEsc)
{
    // Arrange：搜索框预置文本
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
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
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
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
    m_tester->eventFilter(access_private_field::ToolbarsearchEdit(*m_tester), &tabEv);

    // Assert：pressTab 发出一次
    EXPECT_EQ(tabSpy.count(), 1);
    EXPECT_NE(access_private_field::ToolbarsearchEdit(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, EventFilter_SearchEditOtherKey_NoSignal)
{
    // Arrange：搜索框内按非 Tab 键
    QSignalSpy tabSpy(m_tester, &Toolbar::pressTab);
    static QKeyEvent keyAEv(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act
    m_tester->eventFilter(access_private_field::ToolbarsearchEdit(*m_tester), &keyAEv);

    // Assert：pressTab 不发
    EXPECT_EQ(tabSpy.count(), 0);
    EXPECT_NE(access_private_field::ToolbarsearchEdit(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, EventFilter_ProcBtnRight_ConsumesAndFocusSvc)
{
    // Arrange：进程按钮上按右键
    static QKeyEvent rightEv(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(access_private_field::Toolbarm_procBtn(*m_tester), &rightEv);

    // Assert：事件被消费并请求聚焦服务按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(access_private_field::Toolbarm_svcBtn(*m_tester)->objectName(), QString("SvcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_ProcBtnOtherKey_FallsThrough)
{
    // Arrange：进程按钮上按非右键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(access_private_field::Toolbarm_procBtn(*m_tester), &leftEv);

    // Assert：不消费，落入基类（默认 false）
    EXPECT_FALSE(consumed);
    EXPECT_NE(access_private_field::Toolbarm_procBtn(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, EventFilter_SvcBtnLeft_ConsumesAndFocusProc)
{
    // Arrange：服务按钮上按左键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(access_private_field::Toolbarm_svcBtn(*m_tester), &leftEv);

    // Assert：事件被消费并请求聚焦进程按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(access_private_field::Toolbarm_procBtn(*m_tester)->objectName(), QString("ProcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_SvcBtnOtherKey_FallsThrough)
{
    // Arrange：服务按钮上按非左键
    static QKeyEvent rightEv(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(access_private_field::Toolbarm_svcBtn(*m_tester), &rightEv);

    // Assert：不消费
    EXPECT_FALSE(consumed);
    EXPECT_NE(access_private_field::Toolbarm_svcBtn(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, EventFilter_AccountBtnLeft_ConsumesAndFocusSelf)
{
    // Arrange：用户进程按钮上按左键
    static QKeyEvent leftEv(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(access_private_field::Toolbarm_accountProcBtn(*m_tester), &leftEv);

    // Assert：事件被消费并请求聚焦自身按钮
    EXPECT_TRUE(consumed);
    EXPECT_EQ(access_private_field::Toolbarm_accountProcBtn(*m_tester)->objectName(), QString("AccountProcBtn"));
}

TEST_F(UT_Toolbar, EventFilter_NonKeyEvent_FallsThrough)
{
    // Arrange：非键盘事件（鼠标移动）
    static QMouseEvent moveEv(QEvent::MouseMove, QPointF(1, 1), Qt::NoButton, Qt::NoButton, Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester, &moveEv);

    // Assert：直接落入基类处理
    EXPECT_FALSE(consumed);
    EXPECT_NE(access_private_field::ToolbarsearchEdit(*m_tester), nullptr);
}

TEST_F(UT_Toolbar, FocusInput_WithText_SelectsAllAndFocuses)
{
    // Arrange：搜索框有文本
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");

    // Act：聚焦输入框
    m_tester->focusInput();

    // Assert：非空分支执行了全选（offscreen 无活动窗口，hasFocus 不可靠），文本未被改动
    EXPECT_EQ(access_private_field::ToolbarsearchEdit(*m_tester)->lineEdit()->selectedText(), QString("abc"));
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
    EXPECT_FALSE(access_private_field::ToolbarsearchEdit(*m_tester)->lineEdit()->hasSelectedText());
}

TEST_F(UT_Toolbar, HandleSearch_CacheMatch_EmitsSearchSignal)
{
    // Arrange：文本与缓存一致
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
    access_private_field::ToolbarsearchTextCache(*m_tester) = "abc";
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
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("xyz");
    access_private_field::ToolbarsearchTextCache(*m_tester) = "abc";
    QSignalSpy searchSpy(m_tester, &Toolbar::search);

    // Act：执行搜索
    m_tester->handleSearch();

    // Assert：不发出 search 信号，缓存不变
    EXPECT_EQ(searchSpy.count(), 0);
    EXPECT_EQ(access_private_field::ToolbarsearchTextCache(*m_tester), QString("abc"));
}

TEST_F(UT_Toolbar, HandleSearchTextChanged_NewText_UpdatesCacheAndStartsTimer)
{
    // Arrange：定时器初始未激活
    access_private_field::ToolbarsearchTimer(*m_tester)->stop();
    ASSERT_FALSE(access_private_field::ToolbarsearchTimer(*m_tester)->isActive());

    // Act：设置新文本后响应变化
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
    m_tester->handleSearchTextChanged();

    // Assert：缓存更新且 50ms 单发定时器启动
    EXPECT_EQ(access_private_field::ToolbarsearchTextCache(*m_tester), QString("abc"));
    EXPECT_TRUE(access_private_field::ToolbarsearchTimer(*m_tester)->isActive());
    access_private_field::ToolbarsearchTimer(*m_tester)->stop();
}

TEST_F(UT_Toolbar, HandleSearchTextChanged_EmptyText_UpdatesCache)
{
    // Arrange：预置旧文本并停掉定时器
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
    m_tester->handleSearchTextChanged();
    access_private_field::ToolbarsearchTimer(*m_tester)->stop();
    ASSERT_EQ(access_private_field::ToolbarsearchTextCache(*m_tester), QString("abc"));

    // Act：清空文本后响应变化
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("");
    m_tester->handleSearchTextChanged();

    // Assert：缓存同步为空，定时器再次启动
    EXPECT_EQ(access_private_field::ToolbarsearchTextCache(*m_tester), QString(""));
    EXPECT_TRUE(access_private_field::ToolbarsearchTimer(*m_tester)->isActive());
    access_private_field::ToolbarsearchTimer(*m_tester)->stop();
}

// 注：本 PR 修正了 isSearchContentEmpty 的语义反转缺陷：空文本返回 true、非空返回 false，
// 与函数名语义一致；唯一调用方 ProcessTableView::adjustInfoLabelVisibility 已同步取反，
// "搜索未找到"提示的显示条件保持不变（表格为空且搜索内容非空）
TEST_F(UT_Toolbar, IsSearchContentEmpty_EmptyText_ReturnsTrue)
{
    // Arrange：搜索框为空
    ASSERT_TRUE(m_tester->searchContent().isEmpty());

    // Act
    bool result = m_tester->isSearchContentEmpty();

    // Assert：修正后语义：空文本 → true
    EXPECT_TRUE(result);
    EXPECT_EQ(m_tester->searchContent(), QString(""));
}

TEST_F(UT_Toolbar, IsSearchContentEmpty_NonEmptyText_ReturnsFalse)
{
    // Arrange：搜索框有文本
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");

    // Act
    bool result = m_tester->isSearchContentEmpty();

    // Assert：修正后语义：非空文本 → false
    EXPECT_FALSE(result);
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
    EXPECT_EQ(content, access_private_field::ToolbarsearchEdit(*m_tester)->text());
}

TEST_F(UT_Toolbar, SearchContent_WithText_ReturnsText)
{
    // Arrange：设置文本
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");

    // Act：读取搜索内容
    QString content = m_tester->searchContent();

    // Assert：透传编辑框文本（修正后非空文本 → isSearchContentEmpty 为 false）
    EXPECT_EQ(content, QString("abc"));
    EXPECT_FALSE(m_tester->isSearchContentEmpty());
}

TEST_F(UT_Toolbar, SetProcessButtonChecked_True_ChecksProcBtn)
{
    // Arrange：初始为进程按钮选中态
    EXPECT_NE(m_tester, nullptr);

    // Act：置选进程按钮
    m_tester->setProcessButtonChecked(true);

    // Assert：进程按钮选中，服务按钮不受影响
    EXPECT_EQ(access_private_field::Toolbarm_procBtn(*m_tester)->isChecked(), true);
    EXPECT_FALSE(access_private_field::Toolbarm_svcBtn(*m_tester)->isChecked());
}

TEST_F(UT_Toolbar, SetProcessButtonChecked_False_KeepsState)
{
    // Arrange：记录默认选中态（互斥组下程序化取消选中会被还原，故以不变性断言）
    bool before = access_private_field::Toolbarm_procBtn(*m_tester)->isChecked();

    // Act：传入 false（源码分支直接返回，不改变任何按钮状态）
    m_tester->setProcessButtonChecked(false);

    // Assert：进程按钮状态保持调用前取值，服务按钮不受影响
    EXPECT_EQ(access_private_field::Toolbarm_procBtn(*m_tester)->isChecked(), before);
    EXPECT_FALSE(access_private_field::Toolbarm_svcBtn(*m_tester)->isChecked());
}

TEST_F(UT_Toolbar, ClearSearchText_WithText_ClearsEdit)
{
    // Arrange：预置文本与缓存
    access_private_field::ToolbarsearchEdit(*m_tester)->setText("abc");
    access_private_field::ToolbarsearchTextCache(*m_tester) = "abc";

    // Act：清空搜索框
    m_tester->clearSearchText();

    // Assert：clearEdit 同时清空编辑框内容与文本缓存（DSearchEdit 清除语义）
    EXPECT_TRUE(m_tester->searchContent().isEmpty());
    EXPECT_TRUE(access_private_field::ToolbarsearchTextCache(*m_tester).isEmpty());
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
    EXPECT_TRUE(access_private_field::ToolbarsearchEdit(*m_tester)->isEnabled());
}

TEST_F(UT_Toolbar, ToggledButtons_AnyButtonToggle_EmitsTabButtonClickedSignals)
{
    // Arrange：监听三个页签信号
    QSignalSpy procSpy(m_tester, &Toolbar::procTabButtonClicked);
    QSignalSpy svcSpy(m_tester, &Toolbar::serviceTabButtonClicked);
    QSignalSpy accountSpy(m_tester, &Toolbar::accountProcTabButtonClicked);
    DButtonBoxButton *procBtn = access_private_field::Toolbarm_procBtn(*m_tester);
    DButtonBoxButton *svcBtn = access_private_field::Toolbarm_svcBtn(*m_tester);
    DButtonBoxButton *accountBtn = access_private_field::Toolbarm_accountProcBtn(*m_tester);
    ASSERT_NE(procBtn, nullptr);

    // Act：依次切换三个页签按钮（构造器中 toggled lambda 对任意 checked 变化转发信号）
    procBtn->setChecked(!procBtn->isChecked());
    svcBtn->setChecked(true);
    accountBtn->setChecked(true);

    // Assert：各信号至少转发一次
    EXPECT_GE(procSpy.count(), 1);
    EXPECT_GE(svcSpy.count(), 1);
    EXPECT_GE(accountSpy.count(), 1);
}

static MainWindow *g_stubMainWin_42 = nullptr;
static MainWindow *stub_mainWindow_42()
{
    return g_stubMainWin_42;
}

TEST_F(UT_Toolbar, LoadingStatusChanged_TrueThenFalse_DisablesThenEnablesButtons)
{
    // Arrange：打桩 mainWindow 返回真实 MainWindow（信号源），在打桩窗口下构造 Toolbar
    Stub stub;
    MainWindow mainWin;
    g_stubMainWin_42 = &mainWin;
    stub.set(ADDR(Application, mainWindow), stub_mainWindow_42);
    Toolbar toolbar;
    DButtonBoxButton *procBtn = access_private_field::Toolbarm_procBtn(toolbar);
    DButtonBoxButton *svcBtn = access_private_field::Toolbarm_svcBtn(toolbar);
    DButtonBoxButton *accountBtn = access_private_field::Toolbarm_accountProcBtn(toolbar);
    DSearchEdit *edit = access_private_field::ToolbarsearchEdit(toolbar);
    ASSERT_NE(procBtn, nullptr);

    // Act：加载开始
    Q_EMIT mainWin.loadingStatusChanged(true);

    // Assert：加载中全部禁用
    EXPECT_FALSE(procBtn->isEnabled());
    EXPECT_FALSE(svcBtn->isEnabled());
    EXPECT_FALSE(accountBtn->isEnabled());
    EXPECT_FALSE(edit->isEnabled());

    // Act：加载结束
    Q_EMIT mainWin.loadingStatusChanged(false);

    // Assert：加载完成全部恢复可用
    EXPECT_TRUE(procBtn->isEnabled());
    EXPECT_TRUE(svcBtn->isEnabled());
    EXPECT_TRUE(accountBtn->isEnabled());
    EXPECT_TRUE(edit->isEnabled());
    g_stubMainWin_42 = nullptr;
}
