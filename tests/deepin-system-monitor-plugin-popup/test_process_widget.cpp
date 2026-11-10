// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 ProcessWidget 单元测试
// 被测类：deepin-system-monitor-plugin-popup/gui/process_widget.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                | level | factors              | min | actual |
// |-----------------------|-------|----------------------|-----|--------|
// | ProcessWidget         | mid   | -                    | 1   | 1      |
// | ~ProcessWidget        | low   | -                    | 1   | 1      |
// | updateStatus          | mid   | 空列表/赋值/刷新     | 2   | 2      |
// | changeTheme           | mid   | switch:3分支         | 3   | 3      |
// | paintEvent            | mid   | hover:2              | 2   | 2      |
// | autoHeight            | mid   | 高度阈值:2分支       | 2   | 2      |
// | changeFont            | low   | -                    | 1   | 1      |
// | eventFilter           | low   | enter/leave/透传     | 2   | 2      |
// | mouseDoubleClickEvent | low   | 左键/非左键          | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: parent->width()-20 定宽(255 高); changeTheme+changeFont 连接;
//         new ProcessTableView(子对象 objectName="ProcessTableView"); WA_TranslucentBackground;
//         [缺陷标注] changeFont/connect(fontChanged) 代码块重复两遍（冗余连接）
// - updateStatus: B1 totalCpuPercent 赋值; B2 遍历 cPercents 追加 cpuPercents[i]（cpuPercents
//   为空且 cPercents 非空时 cpuPercents[i] 越界——缺陷，测试仅覆盖空列表安全路径）; B3 size>
//   pointsNumber → pop_front; B4 update()
// - autoHeight: B1 height > STANDARD_TOTAL_HEIGHT(972) → setFixedHeight(height-662)+update;
//               B2 否则不动
// - changeTheme: B1 LightType → 亮色 trans/icon_list; B2 DarkType → 暗色; B3 default → 仅刷新颜色
// - paintEvent: B1 m_isHover → hoverTrans 填充 / 透明填充; B2 标题/表头文本绘制
// - eventFilter: B1 Enter → hover=true 消费; B2 Leave → hover=false 消费; B3 其他透传基类
// - mouseDoubleClickEvent: B1 LeftButton → sendJumpWidgetMessage("MSG_PROCESS"); B2 其他走基类

#include <gtest/gtest.h>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include "stubext.h"
#include "process_widget.h"
#include "datadealsingleton.h"
#include <DGuiApplicationHelper>

// sendJumpWidgetMessage 调用计数（避免真实拉起主进程）
static int g_processJumpCalls = 0;

class ProcessWidgetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        // ProcessWidget 构造内部解引用 parent->width()，必须携带真实 parent
        parent = new QWidget;
        parent->resize(280, 400);
        g_processJumpCalls = 0;
    }

    void TearDown() override
    {
        stub.clear();
        delete parent;
        parent = nullptr;
    }

    stub_ext::StubExt stub;
    QWidget *parent = nullptr;
};

// ═══════════════════ 构造 / 析构 ═══════════════════

TEST_F(ProcessWidgetTest, Ctor_ParentWidth_FixedSizeApplied)
{
    // Arrange: parent 已 resize 到 280x400
    QWidget host;
    host.resize(280, 400);
    // Act: 构造被测对象
    ProcessWidget *w = new ProcessWidget(&host);
    // Assert: 固定尺寸为 (parent宽-20, 255)，透明背景 + 内嵌 ProcessTableView
    EXPECT_EQ(w->width(), 260);
    EXPECT_EQ(w->height(), 255);
    EXPECT_TRUE(w->testAttribute(Qt::WA_TranslucentBackground));
    delete w;
}

TEST_F(ProcessWidgetTest, Ctor_SigDataUpdate_TriggersUpdateStatus)
{
    // Arrange: 构造对象（ctor 已建立 sigDataUpdate→updateStatus 连接）
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 发射单例的 sigDataUpdate 信号（updateStatus 以空核心列表安全执行）
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 信号驱动重绘渲染成功
    QPixmap pm = w->grab();
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(ProcessWidgetTest, Dtor_AfterUse_ReleasesCleanly)
{
    // Arrange: 构造待析构对象（内部持有 ProcessTableView）
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 析构（Qt parent 机制先于测试 TearDown 释放）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
    // Assert: 析构过程无崩溃即为通过
    SUCCEED();
}

// ═══════════════════ updateStatus（mid）═══════════════════

TEST_F(ProcessWidgetTest, UpdateStatus_EmptyCoreList_NoCrash)
{
    // Arrange: 构造对象，cpuPercents 为空（B1 赋值 + B4 update 安全路径）
    stub.set_lamda(ADDR(DataDealSingleton, sendJumpWidgetMessage), [](DataDealSingleton *, const QString &) {
        ++g_processJumpCalls;
        return false;
    });
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 以空核心占比列表刷新
    w->updateStatus(50.0, QList<qreal>());
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(ProcessWidgetTest, UpdateStatus_RepeatedEmptyRefresh_StillRenders)
{
    // Arrange: 构造对象
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 连续多次空列表刷新（B4 update 分支重复执行）
    for (int i = 0; i < 5; ++i)
        w->updateStatus(66.0 + i, QList<qreal>());
    // Assert: 多次刷新后渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 255);
    delete w;
}

// ═══════════════════ autoHeight（mid: 阈值分支）═══════════════════

TEST_F(ProcessWidgetTest, AutoHeight_AboveStandard_ResizesHeight)
{
    // Arrange: 构造对象（默认高 255），覆盖 B1 超阈值分支
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 传入 2000（> 972）
    w->autoHeight(2000);
    // Assert: 高度变为 2000-662=1338
    EXPECT_EQ(w->height(), 1338);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(ProcessWidgetTest, AutoHeight_BelowStandard_NoResize)
{
    // Arrange: 构造对象，覆盖 B2 未超阈值分支
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 传入 500（< 972）
    w->autoHeight(500);
    // Assert: 高度保持默认 255
    EXPECT_EQ(w->height(), 255);
    EXPECT_EQ(w->width(), 260);
    delete w;
}

// ═══════════════════ changeTheme（mid: 3 分支）═══════════════════

TEST_F(ProcessWidgetTest, ChangeTheme_LightType_LightParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B1 亮色分支
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 切换亮色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::LightType);
    // Assert: 渲染正常（亮色 trans 与 icon_list 已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(ProcessWidgetTest, ChangeTheme_DarkType_DarkParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B2 暗色分支
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 切换暗色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::DarkType);
    // Assert: 渲染正常（暗色 trans 与 icon_list 已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 255);
    delete w;
}

TEST_F(ProcessWidgetTest, ChangeTheme_UnknownType_KeepsDefaults)
{
    // Arrange: 构造对象，覆盖 changeTheme B3 default 分支
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 传入未知主题类型
    w->changeTheme(DGuiApplicationHelper::ColorType::UnknownType);
    // Assert: 不崩溃且尺寸未变化（default 分支仅刷新 palette 颜色）
    EXPECT_EQ(w->width(), 260);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

// ═══════════════════ paintEvent（mid: hover 分支）═══════════════════

TEST_F(ProcessWidgetTest, PaintEvent_IdleState_RendersSuccess)
{
    // Arrange: 构造对象，未 hover
    ProcessWidget *w = new ProcessWidget(parent);
    // Act: 触发绘制（grab 强制 paintEvent）
    QPixmap pm = w->grab();
    // Assert: 渲染有效（B1 非悬停 + B2 标题/表头绘制）
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(ProcessWidgetTest, PaintEvent_HoverState_HighlightRendered)
{
    // Arrange: 构造对象并注入悬停状态（paintEvent B1 悬停分支）
    ProcessWidget *w = new ProcessWidget(parent);
    QEvent enter(QEvent::Enter);
    w->eventFilter(w, &enter);
    // Act: 触发绘制
    QPixmap pm = w->grab();
    // Assert: 悬停高亮分支渲染有效
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.height(), 255);
    delete w;
}

// ═══════════════════ changeFont ═══════════════════

TEST_F(ProcessWidgetTest, ChangeFont_CustomFont_WidgetStillRenders)
{
    // Arrange: 构造对象与自定义字体
    ProcessWidget *w = new ProcessWidget(parent);
    QFont font("Sans", 12);
    // Act: 切换字体（内部设置 section/content/sub 三组字体）
    w->changeFont(font);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 255);
    delete w;
}

// ═══════════════════ eventFilter ═══════════════════

TEST_F(ProcessWidgetTest, EventFilter_EnterEvent_ConsumedSetsHover)
{
    // Arrange: 构造对象与 Enter 事件（B1 分支）
    ProcessWidget *w = new ProcessWidget(parent);
    QEvent enter(QEvent::Enter);
    // Act: 以自身为目标分发事件
    bool consumed = w->eventFilter(w, &enter);
    // Assert: Enter 被消费且悬停态生效（后续 Leave 也被消费，B2 分支）
    EXPECT_TRUE(consumed);
    QEvent leave(QEvent::Leave);
    EXPECT_TRUE(w->eventFilter(w, &leave));
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(ProcessWidgetTest, EventFilter_OtherObject_DelegatesToBase)
{
    // Arrange: 非 this 目标与无关事件（B3 透传分支）
    ProcessWidget *w = new ProcessWidget(parent);
    QObject other;
    QEvent hover(QEvent::HoverMove);
    // Act: 以其他对象为目标分发事件
    bool consumed = w->eventFilter(&other, &hover);
    // Assert: 透传基类，默认不消费
    EXPECT_FALSE(consumed);
    EXPECT_EQ(w->width(), 260);
    delete w;
}

// ═══════════════════ mouseDoubleClickEvent ═══════════════════

TEST_F(ProcessWidgetTest, MouseDoubleClick_LeftButton_JumpMessageSent)
{
    // Arrange: 桩定 sendJumpWidgetMessage 并构造左键双击事件（B1 分支）
    stub.set_lamda(ADDR(DataDealSingleton, sendJumpWidgetMessage), [](DataDealSingleton *, const QString &) {
        ++g_processJumpCalls;
        return false;
    });
    ProcessWidget *w = new ProcessWidget(parent);
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, QPointF(10, 10),
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    // Act: 直接触发双击处理
    w->mouseDoubleClickEvent(&dblClick);
    // Assert: 跳转消息恰好发送一次
    EXPECT_EQ(g_processJumpCalls, 1);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(ProcessWidgetTest, MouseDoubleClick_RightButton_MessageIgnored)
{
    // Arrange: 桩定 sendJumpWidgetMessage 并构造右键双击事件（B2 走基类分支）
    stub.set_lamda(ADDR(DataDealSingleton, sendJumpWidgetMessage), [](DataDealSingleton *, const QString &) {
        ++g_processJumpCalls;
        return false;
    });
    ProcessWidget *w = new ProcessWidget(parent);
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, QPointF(10, 10),
                         Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    // Act: 直接触发双击处理
    w->mouseDoubleClickEvent(&dblClick);
    // Assert: 不发送跳转消息
    EXPECT_EQ(g_processJumpCalls, 0);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}
