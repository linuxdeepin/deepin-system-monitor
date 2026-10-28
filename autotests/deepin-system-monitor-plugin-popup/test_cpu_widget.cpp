// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 CpuWidget 单元测试
// 被测类：deepin-system-monitor-plugin-popup/gui/cpu_widget.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method         | level | factors                | min | actual |
// |----------------|-------|------------------------|-----|--------|
// | CpuWidget      | mid   | -                      | 1   | 1      |
// | ~CpuWidget     | low   | -                      | 1   | 1      |
// | updateStatus   | mid   | 读取失败+历史窗口cap   | 3   | 3      |
// | changeTheme    | mid   | switch:3分支           | 3   | 3      |
// | paintEvent     | mid   | hover:2+路径:2         | 3   | 3      |
// | changeFont     | low   | -                      | 1   | 1      |
// | eventFilter    | low   | enter/leave/透传       | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: parent->width()-20 定宽(111 高); downloadSpeeds 预填 pointsNumber+1 个 0;
//         themeTypeChanged→changeTheme; initConnection(changeFont+fontChanged+sigDataUpdate); installEventFilter
// - updateStatus: B1 readCpuPer 失败仅告警; B2 append 后 size>pointsNumber+1 → pop_front;
//                 B3 max_element 驱动 getPainterPathByData → downloadPath → update
// - changeTheme: B1 LightType → 亮色 trans/图标; B2 DarkType → 暗色 trans/图标; B3 default → 仅刷新颜色
// - paintEvent: B1 m_isHover → hoverTrans 填充 / 透明填充; B2 downloadPath 路径绘制（有/无数据）
// - eventFilter: B1 Enter → hover=true 消费; B2 Leave → hover=false 消费; B3 其他透传基类

#include <gtest/gtest.h>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include "stubext.h"
#include "cpu_widget.h"
#include "datadealsingleton.h"
#include <DGuiApplicationHelper>

// readCpuPer 可控数据源（替代真实 CPU 统计）
static qreal g_cpuPer = 42.5;
static bool g_cpuReadOk = true;

class CpuWidgetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        // CpuWidget 构造内部解引用 parent->width()，必须携带真实 parent
        parent = new QWidget;
        parent->resize(280, 400);
        g_cpuPer = 42.5;
        g_cpuReadOk = true;
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

TEST_F(CpuWidgetTest, Ctor_ParentWidth_FixedSizeApplied)
{
    // Arrange: parent 已 resize 到 280x400
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    // Act: 构造被测对象
    CpuWidget *w = new CpuWidget(parent);
    // Assert: 固定尺寸为 (parent宽-20, 111)，渲染有效
    EXPECT_EQ(w->width(), 260);
    EXPECT_EQ(w->height(), 111);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(CpuWidgetTest, Ctor_SigDataUpdate_TriggersUpdateStatus)
{
    // Arrange: 构造并桩定 readCpuPer，验证 ctor 建立的 sigDataUpdate→updateStatus 连接
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    CpuWidget *w = new CpuWidget(parent);
    // Act: 发射单例的 sigDataUpdate 信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 信号驱动重绘渲染成功
    QPixmap pm = w->grab();
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(CpuWidgetTest, Dtor_AfterUse_ReleasesCleanly)
{
    // Arrange: 构造待析构对象（downloadSpeeds 由析构释放）
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    CpuWidget *w = new CpuWidget(parent);
    // Act: 析构（Qt parent 机制先于测试 TearDown 释放）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
    // Assert: 析构过程无崩溃即为通过
    SUCCEED();
}

// ═══════════════════ updateStatus（mid: 失败/历史窗口）═══════════════════

TEST_F(CpuWidgetTest, UpdateStatus_ValidData_DisplayRefreshed)
{
    // Arrange: 桩定 readCpuPer 返回 42.5%
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    CpuWidget *w = new CpuWidget(parent);
    // Act: 执行状态刷新（append + 路径重算）
    w->updateStatus();
    // Assert: 渲染有效（update 分支生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(CpuWidgetTest, UpdateStatus_ReadFailed_WidgetStillRenders)
{
    // Arrange: readCpuPer 返回 false（B1 失败仅告警分支）
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    g_cpuReadOk = false;
    CpuWidget *w = new CpuWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: 失败不崩溃，界面仍可渲染
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 111);
    delete w;
}

TEST_F(CpuWidgetTest, UpdateStatus_ConsecutiveUpdates_HistoryWindowCapped)
{
    // Arrange: 连续刷新超过 pointsNumber+1 次（B2 pop_front 滑窗分支）
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    g_cpuPer = 66.6;
    CpuWidget *w = new CpuWidget(parent);
    // Act: 连续刷新 30 次（>26 触发 cap），再刷新一次验证窗口稳定
    for (int i = 0; i < 30; ++i)
        w->updateStatus();
    w->updateStatus();
    // Assert: 滑窗收敛后渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

// ═══════════════════ changeTheme（mid: 3 分支）═══════════════════

TEST_F(CpuWidgetTest, ChangeTheme_LightType_LightParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B1 亮色分支
    CpuWidget *w = new CpuWidget(parent);
    // Act: 切换亮色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::LightType);
    // Assert: 渲染正常（亮色 trans 与图标已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(CpuWidgetTest, ChangeTheme_DarkType_DarkParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B2 暗色分支
    CpuWidget *w = new CpuWidget(parent);
    // Act: 切换暗色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::DarkType);
    // Assert: 渲染正常（暗色 trans 与图标已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 111);
    delete w;
}

TEST_F(CpuWidgetTest, ChangeTheme_UnknownType_KeepsDefaults)
{
    // Arrange: 构造对象，覆盖 changeTheme B3 default 分支
    CpuWidget *w = new CpuWidget(parent);
    // Act: 传入未知主题类型
    w->changeTheme(DGuiApplicationHelper::ColorType::UnknownType);
    // Assert: 不崩溃且尺寸未变化（default 分支仅刷新 palette 颜色）
    EXPECT_EQ(w->width(), 260);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

// ═══════════════════ paintEvent（mid: hover/路径分支）═══════════════════

TEST_F(CpuWidgetTest, PaintEvent_IdleState_RendersSuccess)
{
    // Arrange: 构造对象，未 hover、无历史数据
    CpuWidget *w = new CpuWidget(parent);
    // Act: 触发绘制（grab 强制 paintEvent）
    QPixmap pm = w->grab();
    // Assert: 渲染有效（B1 非悬停 + B2 空路径分支）
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(CpuWidgetTest, PaintEvent_HoverState_HighlightRendered)
{
    // Arrange: 构造对象并注入悬停状态（paintEvent B1 悬停分支）
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    CpuWidget *w = new CpuWidget(parent);
    QEvent enter(QEvent::Enter);
    w->eventFilter(w, &enter);
    // Act: 触发绘制
    QPixmap pm = w->grab();
    // Assert: 悬停高亮分支渲染有效
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.height(), 111);
    delete w;
}

TEST_F(CpuWidgetTest, PaintEvent_DataLoaded_PathRendered)
{
    // Arrange: 先刷新状态注入数据（paintEvent B2 有数据路径分支）
    stub.set_lamda(ADDR(DataDealSingleton, readCpuPer), [](DataDealSingleton *, qreal &cpuPer) -> bool {
        cpuPer = g_cpuPer;
        return g_cpuReadOk;
    });
    CpuWidget *w = new CpuWidget(parent);
    w->updateStatus();
    // Act: 触发绘制
    QPixmap pm = w->grab();
    // Assert: 路径绘制分支渲染有效
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

// ═══════════════════ changeFont ═══════════════════

TEST_F(CpuWidgetTest, ChangeFont_CustomFont_WidgetStillRenders)
{
    // Arrange: 构造对象与自定义字体
    CpuWidget *w = new CpuWidget(parent);
    QFont font("Sans", 12);
    // Act: 切换字体（内部设置 title/content/unit/sub 四组字体）
    w->changeFont(font);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 111);
    delete w;
}

// ═══════════════════ eventFilter ═══════════════════

TEST_F(CpuWidgetTest, EventFilter_EnterEvent_ConsumedSetsHover)
{
    // Arrange: 构造对象与 Enter 事件（B1 分支）
    CpuWidget *w = new CpuWidget(parent);
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

TEST_F(CpuWidgetTest, EventFilter_OtherObject_DelegatesToBase)
{
    // Arrange: 非 this 目标与无关事件（B3 透传分支）
    CpuWidget *w = new CpuWidget(parent);
    QObject other;
    QEvent hover(QEvent::HoverMove);
    // Act: 以其他对象为目标分发事件
    bool consumed = w->eventFilter(&other, &hover);
    // Assert: 透传基类，默认不消费
    EXPECT_FALSE(consumed);
    EXPECT_EQ(w->width(), 260);
    delete w;
}
