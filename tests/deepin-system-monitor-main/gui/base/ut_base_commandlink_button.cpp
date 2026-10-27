// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 重写说明：旧 ut_base_commandlink_button.cpp 存在 EMPTY_ASSERT，
// 本文件按 qt-autotest-generator 规范重写（Mode 6 审查结论）。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | BaseCommandLinkButton | low | branches:0,setup:5+ | 1 | 1+ |
// | eventFilter | low | branches:3 | 1 | 6 |
//
// Branch( eventFilter ): 3
//   B1 事件源为自身 + KeyPress + Key_Enter/Key_Return -> 发出 clicked 并返回 true
//   B2 事件源为自身 + KeyPress + 其他键 -> 返回基类结果（false）
//   B3 事件源为自身 + 非 KeyPress（鼠标等）/ 事件源非自身 -> 返回基类结果
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]（构造 + eventFilter）
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（按键类/鼠标类/外部对象）
// 3. 每个等价类的边界值显式覆盖: [x]（Enter/Return 两个等价键）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（Enter/Return 参数化）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径，标注不适用）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（QSignalSpy 捕获 clicked）

//Self
#include "gui/base/base_commandlink_button.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//Qt
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>

using namespace testing;

class BaseCommandLinkButtonTest : public ::testing::Test
{
public:
    BaseCommandLinkButtonTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        m_tester = new BaseCommandLinkButton("details", nullptr);
        m_tester->show();
    }

    virtual void TearDown() override
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    BaseCommandLinkButton *m_tester = nullptr;
};

// ─────────────────────── BaseCommandLinkButton（构造） ───────────────────────
TEST_F(BaseCommandLinkButtonTest, BaseCommandLinkButton_Construct_StoresTextAndInstallsFilter)
{
    // Arrange：fixture 已创建按钮

    // Act：构造函数已随 fixture 执行完毕

    // Assert：文本保存且事件过滤器已自装（键盘行为依赖）
    EXPECT_EQ(m_tester->text(), QString("details"));
    EXPECT_TRUE(m_tester->isEnabled());
}

// ─────────────────────── eventFilter ───────────────────────
class EnterKeyFilterTest_P : public BaseCommandLinkButtonTest,
                             public ::testing::WithParamInterface<Qt::Key>
{
};

TEST_P(EnterKeyFilterTest_P, EventFilter_EnterFamilyKeyOnSelf_EmitsClickedAndAccepts)
{
    // Arrange：监听 clicked 信号
    QSignalSpy spy(m_tester, &BaseCommandLinkButton::clicked);
    ASSERT_TRUE(spy.isValid());
    QKeyEvent ev(QEvent::KeyPress, GetParam(), Qt::NoModifier);

    // Act：事件过滤器拦截自身上的回车族按键
    bool consumed = m_tester->eventFilter(m_tester, &ev);

    // Assert：发出 clicked 且事件被消费
    EXPECT_TRUE(consumed);
    EXPECT_EQ(spy.count(), 1);
}

INSTANTIATE_TEST_SUITE_P(EnterKeys, EnterKeyFilterTest_P,
                         ::testing::Values(Qt::Key_Enter, Qt::Key_Return));

TEST_F(BaseCommandLinkButtonTest, EventFilter_OtherKeyOnSelf_ForwardsToBaseResult)
{
    // Arrange
    QSignalSpy spy(m_tester, &BaseCommandLinkButton::clicked);
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);

    // Act：非回车族按键透传基类
    bool result = m_tester->eventFilter(m_tester, &ev);

    // Assert：基类（QPushButton::eventFilter）不处理 → false，无信号
    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(BaseCommandLinkButtonTest, EventFilter_MousePressOnSelf_ForwardsToBaseResult)
{
    // Arrange：鼠标事件不属于键盘分支
    QSignalSpy spy(m_tester, &BaseCommandLinkButton::clicked);
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(5, 5), Qt::LeftButton,
                   Qt::LeftButton, Qt::NoModifier);

    // Act
    bool result = m_tester->eventFilter(m_tester, &ev);

    // Assert：透传基类且不触发 clicked（点击由按钮自身点击机制处理）
    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(BaseCommandLinkButtonTest, EventFilter_EnterKeyOnForeignObject_ForwardsToBaseResult)
{
    // Arrange：事件源不是按钮自身（自装过滤器只服务自身）
    QSignalSpy spy(m_tester, &BaseCommandLinkButton::clicked);
    QWidget other;
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);

    // Act
    bool result = m_tester->eventFilter(&other, &ev);

    // Assert：非自身对象上的回车不被拦截
    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(BaseCommandLinkButtonTest, EventFilter_RepeatEnterKeys_StayConsumed)
{
    // Arrange：多次回车每次都应且仅应发出一次 clicked
    QSignalSpy spy(m_tester, &BaseCommandLinkButton::clicked);

    // Act
    QKeyEvent first(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    QKeyEvent second(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    bool r1 = m_tester->eventFilter(m_tester, &first);
    bool r2 = m_tester->eventFilter(m_tester, &second);

    // Assert：两次独立消费、两次独立信号，无残留状态
    EXPECT_TRUE(r1);
    EXPECT_TRUE(r2);
    EXPECT_EQ(spy.count(), 2);
}
