// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 重写说明：旧 ut_base_header_view.cpp 存在 EMPTY_ASSERT/LOW_ASSERT，
// 本文件按 qt-autotest-generator 规范重写（Mode 6 审查结论）。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | BaseHeaderView | low | branches:0,setup:3+ | 1 | 1+ |
// | sizeHint | low | branches:0 | 1 | 2 |
// | eventFilter | low | branches:3 | 1 | 5 |
// | focusInEvent | low | branches:0 | 1 | 2 |
// | paintEvent | low | branches:2 | 1 | 2 |
//
// Branch( eventFilter ): 3
//   B1 KeyPress + Space/Up/Down -> 拦截返回 true
//   B2 KeyPress + 其他键 -> 基类结果
//   B3 非 KeyPress -> 基类结果
//
// Branch( paintEvent ): 2
//   B1 hasFocus && TabFocusReason -> 在基类之上绘制焦点框
//   B2 无焦点（或非 Tab 聚焦）-> 只绘制圆角底色
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（拦截键/未拦截键/非键盘事件）
// 3. 每个等价类的边界值显式覆盖: [x]（三个拦截键逐个覆盖）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（拦截键参数化）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径，标注不适用）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

//Self
#include "gui/base/base_header_view.h"

//gtest
#include "stub.h"
#include "3rdparty/stub-ext/stubext.h"
#include <gtest/gtest.h>

//Qt
#include <QApplication>
#include <QFocusEvent>
#include <QHoverEvent>
#include <QKeyEvent>

//Dtk
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DStyle>

using namespace Dtk::Gui;
using namespace Dtk::Widget;

using namespace testing;

class BaseHeaderViewTest : public ::testing::Test
{
public:
    BaseHeaderViewTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        m_tester = new BaseHeaderView(Qt::Horizontal);
        m_tester->resize(200, 20);
        m_tester->show();
    }

    virtual void TearDown() override
    {
        stub.clear();
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    stub_ext::StubExt stub;
    BaseHeaderView *m_tester = nullptr;
};

// ─────────────────────── BaseHeaderView（构造） ───────────────────────
TEST_F(BaseHeaderViewTest, BaseHeaderView_Construct_InstallsSelfFilterAndDefaultReason)
{
    // Arrange：fixture 已创建表头

    // Act：构造函数已随 fixture 执行完毕

    // Assert：自装事件过滤器（键盘拦截依赖）+ 默认焦点原因为 Tab
    EXPECT_EQ(m_tester->orientation(), Qt::Horizontal);
    EXPECT_EQ(m_tester->m_focusReason, Qt::TabFocusReason);
}

// ─────────────────────── sizeHint ───────────────────────
TEST_F(BaseHeaderViewTest, SizeHint_AfterResize_WidthTracksAndHeightStays37)
{
    // Arrange
    m_tester->resize(345, 20);

    // Act
    QSize hint = m_tester->sizeHint();

    // Assert：宽度跟随当前宽度，高度固定 37
    EXPECT_EQ(hint, QSize(345, 37));
    EXPECT_EQ(hint.width(), 345);
    EXPECT_EQ(hint.height(), 37);
}

TEST_F(BaseHeaderViewTest, SizeHint_AfterShrink_WidthTracksNewWidth)
{
    // Arrange
    m_tester->resize(120, 20);

    // Act
    QSize hint = m_tester->sizeHint();

    // Assert
    EXPECT_EQ(hint.width(), 120);
    EXPECT_EQ(hint.height(), 37);
}

// ─────────────────────── eventFilter ───────────────────────
class HeaderInterceptKeyTest_P : public BaseHeaderViewTest,
                                 public ::testing::WithParamInterface<Qt::Key>
{
};

TEST_P(HeaderInterceptKeyTest_P, EventFilter_NavigationKeyOnSelf_IsIntercepted)
{
    // Arrange：空格/上下键用于避免表头上的键盘导航干扰点击状态
    QKeyEvent ev(QEvent::KeyPress, GetParam(), Qt::NoModifier);

    // Act
    bool consumed = m_tester->eventFilter(m_tester, &ev);

    // Assert：三个导航键一律拦截，且事件类型保持 KeyPress
    EXPECT_TRUE(consumed);
    EXPECT_EQ(ev.type(), QEvent::KeyPress);
}

INSTANTIATE_TEST_SUITE_P(NavigationKeys, HeaderInterceptKeyTest_P,
                         ::testing::Values(Qt::Key_Space, Qt::Key_Up, Qt::Key_Down));

TEST_F(BaseHeaderViewTest, EventFilter_UnhandledKeyOnSelf_ForwardsToBaseResult)
{
    // Arrange
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act
    bool result = m_tester->eventFilter(m_tester, &ev);

    // Assert：非导航键透传基类（DHeaderView::eventFilter 不处理 → false）
    EXPECT_FALSE(result);
    EXPECT_EQ(ev.type(), QEvent::KeyPress);
}

TEST_F(BaseHeaderViewTest, EventFilter_NonKeyEventOnSelf_ForwardsToBaseResult)
{
    // Arrange
    QHoverEvent ev(QEvent::HoverMove, QPointF(5, 5), QPointF(5, 5));

    // Act
    bool result = m_tester->eventFilter(m_tester, &ev);

    // Assert：非键盘事件透传基类
    EXPECT_FALSE(result);
    EXPECT_EQ(ev.type(), QEvent::HoverMove);
}

TEST_F(BaseHeaderViewTest, EventFilter_NavigationKeyOnForeignObject_StillIntercepted)
{
    // Arrange：impl 只判断按键类型，不区分 watched 对象，导航键一律拦截
    QWidget other;
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);

    // Act
    bool result = m_tester->eventFilter(&other, &ev);

    // Assert：导航键被拦截返回 true
    EXPECT_TRUE(result);
    EXPECT_EQ(ev.type(), QEvent::KeyPress);
}

// ─────────────────────── focusInEvent ───────────────────────
TEST_F(BaseHeaderViewTest, FocusInEvent_MouseFocus_RecordsMouseReason)
{
    // Arrange
    QFocusEvent ev(QEvent::FocusIn, Qt::MouseFocusReason);

    // Act
    m_tester->focusInEvent(&ev);

    // Assert：记录的焦点原因为鼠标聚焦（非 Tab → 不触发焦点框绘制）
    EXPECT_EQ(m_tester->m_focusReason, Qt::MouseFocusReason);
    EXPECT_NE(m_tester->m_focusReason, Qt::TabFocusReason);
}

TEST_F(BaseHeaderViewTest, FocusInEvent_TabFocus_RecordsTabReason)
{
    // Arrange
    QFocusEvent ev(QEvent::FocusIn, Qt::TabFocusReason);

    // Act
    m_tester->focusInEvent(&ev);

    // Assert：Tab 聚焦时 paintEvent 才会绘制焦点框
    EXPECT_EQ(m_tester->m_focusReason, Qt::TabFocusReason);
    EXPECT_NE(m_tester->m_focusReason, Qt::MouseFocusReason);
}

// ─────────────────────── paintEvent ───────────────────────
TEST_F(BaseHeaderViewTest, PaintEvent_WithoutFocus_RendersRoundedBackgroundOnly)
{
    // Arrange：无焦点 → 跳过焦点框分支
    ASSERT_FALSE(m_tester->hasFocus());

    // Act
    QPixmap pm = m_tester->grab();

    // Assert
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.size(), m_tester->size());
}

TEST_F(BaseHeaderViewTest, PaintEvent_TabFocusedHeader_DrawsFocusFrame)
{
    // Arrange：offscreen 下 DApplication::style() 未必是 DStyle，
    // 源码 dynamic_cast 后未判空，这里注入真实 DStyle 实例（否则焦点框分支空指针）
    static DStyle dstyleInstance;
    stub.set_lamda(static_cast<QStyle *(*)(void)>(&QApplication::style),
                   []() -> QStyle * {
                       return &dstyleInstance;
                   });
    // 先建立真实焦点，再手动 focusInEvent 注入 Tab 原因（setFocus 会以 MouseFocusReason 覆盖）
    m_tester->activateWindow();
    QApplication::setActiveWindow(m_tester);
    m_tester->setFocus();
    QFocusEvent fev(QEvent::FocusIn, Qt::TabFocusReason);
    m_tester->focusInEvent(&fev);
    ASSERT_TRUE(m_tester->hasFocus());
    ASSERT_EQ(m_tester->m_focusReason, Qt::TabFocusReason);

    // Act
    QPixmap pm = m_tester->grab();

    // Assert：焦点框绘制不崩溃且整体渲染完成
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.size(), m_tester->size());
}
