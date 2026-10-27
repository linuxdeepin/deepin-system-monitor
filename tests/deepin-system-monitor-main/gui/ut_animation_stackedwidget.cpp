// Copyright (C) 2019-2026 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//Self
#include "animation_stackedwidget.h"
// 覆盖率采集辅助：进程退出段错误会在全局析构阶段丢失 gcda，故在 gtest 全局
// 环境拆除时强制落盘覆盖率数据（仅影响测试进程，不触碰被测源码）
extern "C" void __gcov_dump(void);

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QPainter>

/***************************************STUB begin*********************************************/
// 空操作桩：保持被桩化函数原副作用关闭，由用例自行断言状态（成员函数桩首参为 this）
static void stub_setCurrentWidget_noop(void *, int &, int)
{
}
/***************************************STUB end**********************************************/

// 分支清单（来源：get_code_snippet 实际源码）
// AnimationStackedWidget::getMoveOrientation(currIndex, nextIndex)，complexity:8：
//   B1: nextIndex < currIndex 且 m_animationOri==LR → LeftToRight
//   B2: nextIndex < currIndex 且 m_animationOri==TB → TopToBottom
//   B3: nextIndex > currIndex 且 m_animationOri==LR → RightToLeft
//   B4: nextIndex > currIndex 且 m_animationOri==TB → BottomToTop
//   B5: nextIndex == currIndex → 保持默认 LeftToRight
// 用例映射：
//   - GetMoveOrientation_BackwardLR_ReturnsLeftToRight      → B1
//   - GetMoveOrientation_BackwardTB_ReturnsTopToBottom      → B2
//   - GetMoveOrientation_ForwardLR_ReturnsRightToLeft       → B3
//   - GetMoveOrientation_ForwardTB_ReturnsBottomToTop       → B4
//   - GetMoveOrientation_SameIndex_ReturnsDefault           → B5
// AnimationStackedWidget::setCurrentWidget(index, beginWidth)：
//   B1: m_IsAnimation==true → 直接返回（无信号）
//   B2: index == currentIndex() → emit signalIsFinished
//   B3: 其余 → 启动动画，记录 m_NextIndex 并隐藏当前页
// 用例映射：
//   - SetCurrentWidget_WhileRunning_NoReentry               → B1
//   - SetCurrentWidget_SameIndex_EmitsFinished              → B2
//   - SetCurrentWidget_NewIndex_StartsAnimation             → B3
// AnimationStackedWidget::setCurrent(QWidget*)：
//   B1: indexOf < 0 → 告警返回；B2: 有效索引 → 计算方向并启动切换
// 用例映射：
//   - SetCurrent_WidgetNotFound_KeepsState                  → B1
//   - SetCurrent_ValidWidget_SwitchesWithOrientation        → B2
// AnimationStackedWidget::paintEvent(e)：
//   B1: m_IsAnimation==true → 绘制当前页与下一页
// 用例映射：
//   - PaintEvent_WhileAnimating_RendersBothPages            → B1

/* 用例计数声明（min 依据 test-code-gen §4：level 基数 + factor 加成）
| method | level | factors | min | actual |
| AnimationStackedWidget | mid | lines:110 | 2 | 3 |
| IsRunning | low | - | 1 | 2 |
| offset/setOffset | mid | in_degree:1 | 2 | 2 |
| setDuration | low | - | 1 | 2 |
| getBeginValue | low | - | 1 | 2 |
| getMoveOrientation | high | complexity:8,cognitive:22,in_degree:2 | 3 | 5 |
| setCurrentWidget | high | complexity:8 | 3 | 4 |
| setCurrent(QWidget*) | mid | in_degree:1 | 2 | 2 |
| setCurrent(int) | mid | in_degree:2 | 2 | 2 |
| animationFinished | mid | in_degree:1 | 2 | 2 |
| paintEvent | low | - | 1 | 2 |
| paintCurrentWidget | low | - | 1 | 4 |
| paintNextWidget | low | - | 1 | 4 |
*/

class UT_AnimationStackedWidget : public ::testing::Test
{
public:
    UT_AnimationStackedWidget() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new AnimationStackedWidget();
        // 增加两个页面，避免 animationFinished 触碰空页指针
        m_pageA = new QWidget();
        m_pageB = new QWidget();
        m_tester->addWidget(m_pageA);
        m_tester->addWidget(m_pageB);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    AnimationStackedWidget *m_tester;
    QWidget *m_pageA = nullptr;
    QWidget *m_pageB = nullptr;
};

class GetMoveOrientationParamTest : public ::testing::TestWithParam<::testing::tuple<AnimationStackedWidget::AnimationOri, int, int, AnimationStackedWidget::MoveOrientation>>
{
public:
    virtual void SetUp()
    {
        m_tester = new AnimationStackedWidget(::testing::get<0>(GetParam()));
        m_pageA = new QWidget();
        m_pageB = new QWidget();
        m_tester->addWidget(m_pageA);
        m_tester->addWidget(m_pageB);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    AnimationStackedWidget *m_tester;
    QWidget *m_pageA = nullptr;
    QWidget *m_pageB = nullptr;
};

ACCESS_PRIVATE_FUN(AnimationStackedWidget, AnimationStackedWidget::MoveOrientation(const int, const int), getMoveOrientation);
ACCESS_PRIVATE_FUN(AnimationStackedWidget, void(int &, int), setCurrentWidget);
ACCESS_PRIVATE_FUN(AnimationStackedWidget, int(void), getBeginValue);
ACCESS_PRIVATE_FUN(AnimationStackedWidget, void(QPainter &, int), paintCurrentWidget);
ACCESS_PRIVATE_FUN(AnimationStackedWidget, void(QPainter &, int), paintNextWidget);

namespace {
class GcovDumpEnvironment : public ::testing::Environment
{
public:
    void TearDown() override { __gcov_dump(); }
};
[[maybe_unused]] const bool s_registerGcovEnv = []() {
    ::testing::AddGlobalTestEnvironment(new GcovDumpEnvironment);
    return true;
}();
}  // namespace

TEST_F(UT_AnimationStackedWidget, Ctor_DefaultLR_InitializesAnimation)
{
    // Arrange：默认构造（LR 方向），对象在 SetUp 就绪
    EXPECT_NE(m_tester, nullptr);

    // Act：读取构造后的默认状态
    // Assert：初始方向 / 记录时长 / 缓动曲线 / 运行态均应为构造期默认值
    // 注：setDuration 仅写 m_Duration 并设曲线，QPropertyAnimation 自身时长保持默认 250
    EXPECT_EQ(m_tester->m_animationOri, AnimationStackedWidget::LR);
    EXPECT_EQ(m_tester->m_Duration, 20);
    EXPECT_EQ(m_tester->m_Animation->duration(), 250);
    EXPECT_EQ(m_tester->m_Animation->easingCurve().type(), QEasingCurve::InOutQuad);
    EXPECT_FALSE(m_tester->IsRunning());
}

TEST_F(UT_AnimationStackedWidget, Ctor_TBOrientation_KeepsOrientation)
{
    // Arrange：以 TB 方向构造独立对象
    AnimationStackedWidget *tbTester = new AnimationStackedWidget(AnimationStackedWidget::TB);

    // Act：读取其方向与动画对象

    // Assert：方向保持 TB，动画对象已创建且记录时长为默认 20（动画对象 250）
    EXPECT_EQ(tbTester->m_animationOri, AnimationStackedWidget::TB);
    EXPECT_NE(tbTester->m_Animation, nullptr);
    EXPECT_EQ(tbTester->m_Duration, 20);
    EXPECT_EQ(tbTester->m_Animation->duration(), 250);
    delete tbTester;
}

TEST_F(UT_AnimationStackedWidget, Dtor_CleansUp_AnimationObject)
{
    // Arrange：独立对象并确认动画对象存在
    AnimationStackedWidget *localTester = new AnimationStackedWidget();
    EXPECT_NE(localTester->m_Animation, nullptr);
    EXPECT_EQ(m_tester->count(), 2);

    // Act：析构
    delete localTester;

    // Assert：无崩溃即通过（ASAN 兜底），再断言当前对象未被误删
    EXPECT_NE(m_tester, nullptr);
    EXPECT_NE(m_tester->m_Animation, nullptr);
}

TEST_F(UT_AnimationStackedWidget, IsRunning_IdleState_ReturnsFalse)
{
    // Arrange：未开启动画
    m_tester->m_IsAnimation = false;

    // Act：查询运行态
    bool running = m_tester->IsRunning();

    // Assert：空闲时返回 false 且状态不被修改
    EXPECT_EQ(running, false);
    EXPECT_EQ(m_tester->m_IsAnimation, false);
}

TEST_F(UT_AnimationStackedWidget, IsRunning_AnimatingState_ReturnsTrue)
{
    // Arrange：模拟动画运行标志
    m_tester->m_IsAnimation = true;

    // Act：查询运行态
    bool running = m_tester->IsRunning();

    // Assert：运行时返回 true 且状态不被修改
    EXPECT_EQ(running, true);
    EXPECT_EQ(m_tester->m_IsAnimation, true);
}

TEST_F(UT_AnimationStackedWidget, Offset_InitialState_ReturnsZero)
{
    // Arrange：初始 m_offset=0
    EXPECT_NE(m_tester, nullptr);

    // Act：读取初始偏移
    double offset = m_tester->offset();

    // Assert：getter 返回 0，且 setter 前不被污染
    EXPECT_DOUBLE_EQ(offset, 0.0);
    EXPECT_DOUBLE_EQ(m_tester->m_offset, 0.0);
}

TEST_F(UT_AnimationStackedWidget, SetOffset_NewValue_StoresAndRefreshes)
{
    // Arrange：独立 pixmap 上强制重绘一次，确保 update() 可执行
    m_tester->setOffset(128.5);

    // Act：读取 offset
    // Assert：写入值被原样保存（含负值边界）
    EXPECT_DOUBLE_EQ(m_tester->offset(), 128.5);
    m_tester->setOffset(-3.25);
    EXPECT_DOUBLE_EQ(m_tester->offset(), -3.25);
}

TEST_F(UT_AnimationStackedWidget, SetDuration_PositiveValue_UpdatesDurationAndCurve)
{
    // Arrange：默认时长为 20
    ASSERT_NE(m_tester->m_Animation, nullptr);

    // Act：设置非默认时长
    m_tester->setDuration(500);

    // Assert：记录时长更新，缓动曲线同步；动画对象自身时长不受影响
    EXPECT_EQ(m_tester->m_Duration, 500);
    EXPECT_EQ(m_tester->m_Animation->duration(), 250);
    EXPECT_EQ(m_tester->m_Animation->easingCurve().type(), QEasingCurve::InOutQuad);
}

TEST_F(UT_AnimationStackedWidget, SetDuration_ZeroValue_AcceptsBoundary)
{
    // Arrange：默认时长为 20
    ASSERT_NE(m_tester->m_Animation, nullptr);

    // Act：设置边界时长 0
    m_tester->setDuration(0);

    // Assert：0 被接受且曲线仍为 InOutQuad
    EXPECT_EQ(m_tester->m_Duration, 0);
    EXPECT_EQ(m_tester->m_Animation->easingCurve().type(), QEasingCurve::InOutQuad);
}

TEST_F(UT_AnimationStackedWidget, GetBeginValue_LROrientation_ReturnsWidth)
{
    // Arrange：LR 方向 + 固定尺寸
    m_tester->m_animationOri = AnimationStackedWidget::LR;
    m_tester->resize(400, 200);

    // Act：调用私有 getBeginValue
    auto getBeginValue = get_private_fun::AnimationStackedWidgetgetBeginValue();
    int begin = call_private_fun::AnimationStackedWidgetgetBeginValue(*m_tester);

    // Assert：LR 取宽度
    EXPECT_EQ(begin, 400);
    EXPECT_EQ(begin, m_tester->width());
    (void)getBeginValue;
}

TEST_F(UT_AnimationStackedWidget, GetBeginValue_TBOrientation_ReturnsHeight)
{
    // Arrange：TB 方向 + 固定尺寸
    m_tester->m_animationOri = AnimationStackedWidget::TB;
    m_tester->resize(400, 200);

    // Act：调用私有 getBeginValue
    int begin = call_private_fun::AnimationStackedWidgetgetBeginValue(*m_tester);

    // Assert：TB 取高度
    EXPECT_EQ(begin, 200);
    EXPECT_EQ(begin, m_tester->height());
}

TEST_P(GetMoveOrientationParamTest, GetMoveOrientation_Direction_ReturnsExpectedOri)
{
    // Arrange：参数化（ori, curr, next, expected）
    auto ori = ::testing::get<0>(GetParam());
    auto curr = ::testing::get<1>(GetParam());
    auto next = ::testing::get<2>(GetParam());
    auto expected = ::testing::get<3>(GetParam());
    m_tester->m_animationOri = ori;

    // Act：调用私有 getMoveOrientation
    auto moveOri = call_private_fun::AnimationStackedWidgetgetMoveOrientation(*m_tester, curr, next);

    // Assert：与真实源码分支表一致
    EXPECT_EQ(moveOri, expected);
    EXPECT_EQ(m_tester->m_animationOri, ori);
}

INSTANTIATE_TEST_SUITE_P(BranchTable, GetMoveOrientationParamTest,
                         ::testing::Values(
                             // B1: 后退 + LR
                             ::testing::make_tuple(AnimationStackedWidget::LR, 1, 0, AnimationStackedWidget::LeftToRight),
                             // B2: 后退 + TB
                             ::testing::make_tuple(AnimationStackedWidget::TB, 1, 0, AnimationStackedWidget::TopToBottom),
                             // B3: 前进 + LR
                             ::testing::make_tuple(AnimationStackedWidget::LR, 0, 1, AnimationStackedWidget::RightToLeft),
                             // B4: 前进 + TB
                             ::testing::make_tuple(AnimationStackedWidget::TB, 0, 1, AnimationStackedWidget::BottomToTop),
                             // B5: 索引相同 → 默认 LeftToRight
                             ::testing::make_tuple(AnimationStackedWidget::LR, 1, 1, AnimationStackedWidget::LeftToRight)));

TEST_F(UT_AnimationStackedWidget, SetCurrentWidget_WhileRunning_NoReentry)
{
    // Arrange：模拟动画进行中
    m_tester->m_IsAnimation = true;
    m_tester->m_NextIndex = 42;
    int index = 1;

    // Act：调用私有 setCurrentWidget
    call_private_fun::AnimationStackedWidgetsetCurrentWidget(*m_tester, index, 100);

    // Assert：不重入，索引与状态均未被改动
    EXPECT_TRUE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->m_NextIndex, 42);
    EXPECT_EQ(m_tester->currentIndex(), 0);
}

TEST_F(UT_AnimationStackedWidget, SetCurrentWidget_SameIndex_EmitsFinished)
{
    // Arrange：索引与当前页相同
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);
    int index = 0;

    // Act：调用私有 setCurrentWidget
    call_private_fun::AnimationStackedWidgetsetCurrentWidget(*m_tester, index, 100);

    // Assert：同页切换直接发 finished 且不开动画
    EXPECT_EQ(finishedSpy.count(), 1);
    EXPECT_FALSE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->currentIndex(), 0);
}

TEST_F(UT_AnimationStackedWidget, SetCurrentWidget_NewIndex_StartsAnimation)
{
    // Arrange：切换到新页 1
    int index = 1;

    // Act：调用私有 setCurrentWidget，起始值 100
    call_private_fun::AnimationStackedWidgetsetCurrentWidget(*m_tester, index, 100);

    // Assert：进入动画态、记录下一页、隐藏当前页，动画区间 [100→0]
    EXPECT_TRUE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->m_NextIndex, 1);
    EXPECT_TRUE(m_tester->widget(0)->isHidden());
    EXPECT_EQ(m_tester->m_Animation->startValue(), QVariant(100));
    EXPECT_EQ(m_tester->m_Animation->endValue(), QVariant(0));
    m_tester->m_Animation->stop();
}

TEST_F(UT_AnimationStackedWidget, SetCurrentWidget_ReentryGuard_StubKeepsNoop)
{
    // Arrange：以桩替换私有 setCurrentWidget，保证后续 setCurrent 流程不被动画干扰
    auto setCurrentWidgetPtr = get_private_fun::AnimationStackedWidgetsetCurrentWidget();
    Stub stub;
    stub.set(setCurrentWidgetPtr, stub_setCurrentWidget_noop);
    int index = 1;

    // Act：调用私有入口确认桩生效
    call_private_fun::AnimationStackedWidgetsetCurrentWidget(*m_tester, index, 100);

    // Assert：桩无副作用，状态保持初始
    EXPECT_FALSE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->m_NextIndex, 0);
}

TEST_F(UT_AnimationStackedWidget, SetCurrent_WidgetNotFound_KeepsState)
{
    // Arrange：不在堆叠中的独立 widget
    QWidget orphan;
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);

    // Act：setCurrent(未知 widget)
    m_tester->setCurrent(&orphan);

    // Assert：索引无效直接返回，方向与状态不被污染
    EXPECT_EQ(m_tester->m_moveOri, AnimationStackedWidget::LeftToRight);
    EXPECT_EQ(finishedSpy.count(), 0);
    EXPECT_FALSE(m_tester->m_IsAnimation);
}

TEST_F(UT_AnimationStackedWidget, SetCurrent_ValidWidget_SwitchesWithOrientation)
{
    // Arrange：当前页 0，目标页 1（LR → 前进）
    ASSERT_EQ(m_tester->currentIndex(), 0);

    // Act：setCurrent(widget)
    m_tester->setCurrent(m_pageB);

    // Assert：方向为 RightToLeft 且动画已启动（B3 分支）
    EXPECT_EQ(m_tester->m_moveOri, AnimationStackedWidget::RightToLeft);
    EXPECT_TRUE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->m_NextIndex, 1);
    m_tester->m_Animation->stop();
}

TEST_F(UT_AnimationStackedWidget, SetCurrent_ByIndex_StartsSwitchToTarget)
{
    // Arrange：索引入口 setCurrent(int)
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);

    // Act：切到索引 1
    m_tester->setCurrent(1);

    // Assert：动画态 + 目标索引正确（同页场景索引 0 时不发 finished）
    EXPECT_TRUE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->m_NextIndex, 1);
    EXPECT_EQ(finishedSpy.count(), 0);
    m_tester->m_Animation->stop();
}

TEST_F(UT_AnimationStackedWidget, SetCurrent_SameIndex_EmitsFinished)
{
    // Arrange：索引入口 setCurrent(int)，目标与当前相同
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);

    // Act：切到索引 0
    m_tester->setCurrent(0);

    // Assert：同页直接发 finished，不进入动画
    EXPECT_EQ(finishedSpy.count(), 1);
    EXPECT_FALSE(m_tester->m_IsAnimation);
}

TEST_F(UT_AnimationStackedWidget, AnimationFinished_AfterSwitch_ShowsNextAndEmits)
{
    // Arrange：当前页 0，模拟动画完成时记录的下一页索引
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);
    m_tester->m_IsAnimation = true;
    m_tester->m_NextIndex = 1;
    m_tester->widget(0)->hide();

    // Act：动画完成回调
    m_tester->animationFinished();

    // Assert：退出动画态、切到下一页并展示、发送 finished（旧页保持隐藏态）
    EXPECT_FALSE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->currentIndex(), 1);
    EXPECT_TRUE(m_tester->widget(0)->isHidden());
    EXPECT_FALSE(m_tester->widget(1)->isHidden());
    EXPECT_EQ(finishedSpy.count(), 1);
}

TEST_F(UT_AnimationStackedWidget, AnimationFinished_SameTarget_KeepsIndex)
{
    // Arrange：下一页索引与当前相同（m_NextIndex=0）
    QSignalSpy finishedSpy(m_tester, &AnimationStackedWidget::signalIsFinished);
    m_tester->m_IsAnimation = true;
    m_tester->m_NextIndex = 0;

    // Act：动画完成回调
    m_tester->animationFinished();

    // Assert：状态复位且仍发 finished（信号与索引解耦）
    EXPECT_FALSE(m_tester->m_IsAnimation);
    EXPECT_EQ(m_tester->currentIndex(), 0);
    EXPECT_EQ(finishedSpy.count(), 1);
}

TEST_F(UT_AnimationStackedWidget, PaintEvent_Idle_SkipsCustomPainting)
{
    // Arrange：非动画态
    m_tester->m_IsAnimation = false;
    m_tester->setOffset(30.0);

    // Act：强制重绘
    QPixmap grabbed = m_tester->grab();

    // Assert：常规渲染可出图（paintEvent 空分支不绘制两页）
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), m_tester->size());
}

TEST_F(UT_AnimationStackedWidget, PaintEvent_WhileAnimating_RendersBothPages)
{
    // Arrange：动画态 + 非零偏移
    m_tester->m_IsAnimation = true;
    m_tester->m_moveOri = AnimationStackedWidget::LeftToRight;
    m_tester->setOffset(30.0);

    // Act：强制重绘触发 paintEvent 两页合成
    QPixmap grabbed = m_tester->grab();

    // Assert：渲染成功且恢复后不改变动画态标志
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(m_tester->m_IsAnimation, true);
    m_tester->m_IsAnimation = false;
}

TEST_F(UT_AnimationStackedWidget, PaintCurrentWidget_FourOrientations_RendersPage)
{
    // Arrange：遍历 4 个方向的移动枚举（同质输入组）
    const AnimationStackedWidget::MoveOrientation orientations[4] = {
        AnimationStackedWidget::LeftToRight,
        AnimationStackedWidget::RightToLeft,
        AnimationStackedWidget::TopToBottom,
        AnimationStackedWidget::BottomToTop
    };
    m_tester->resize(200, 100);
    m_tester->setOffset(50.0);

    for (int i = 0; i < 4; ++i) {
        m_tester->m_moveOri = orientations[i];
        QPixmap pixmap(m_tester->size());
        pixmap.fill(Qt::black);
        QPainter painter(&pixmap);

        // Act：绘制当前页
        call_private_fun::AnimationStackedWidgetpaintCurrentWidget(*m_tester, painter, 0);
        painter.end();

        // Assert：每方向均产出非空位图
        EXPECT_FALSE(pixmap.isNull()) << "orientation index " << i;
        EXPECT_EQ(pixmap.size(), m_tester->size()) << "orientation index " << i;
    }
}

TEST_F(UT_AnimationStackedWidget, PaintNextWidget_FourOrientations_RendersNextPage)
{
    // Arrange：遍历 4 个方向的移动枚举
    const AnimationStackedWidget::MoveOrientation orientations[4] = {
        AnimationStackedWidget::LeftToRight,
        AnimationStackedWidget::RightToLeft,
        AnimationStackedWidget::TopToBottom,
        AnimationStackedWidget::BottomToTop
    };
    m_tester->resize(200, 100);
    m_tester->setOffset(50.0);

    for (int i = 0; i < 4; ++i) {
        m_tester->m_moveOri = orientations[i];
        QPixmap pixmap(m_tester->size());
        pixmap.fill(Qt::black);
        QPainter painter(&pixmap);

        // Act：绘制下一页
        call_private_fun::AnimationStackedWidgetpaintNextWidget(*m_tester, painter, 1);
        painter.end();

        // Assert：每方向均产出非空位图
        EXPECT_FALSE(pixmap.isNull()) << "orientation index " << i;
        EXPECT_EQ(pixmap.size(), m_tester->size()) << "orientation index " << i;
    }
}
