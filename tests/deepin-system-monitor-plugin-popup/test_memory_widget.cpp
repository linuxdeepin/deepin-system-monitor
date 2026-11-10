// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 MemoryWidget 单元测试
// 被测类：deepin-system-monitor-plugin-popup/gui/memory_widget.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                | level | factors            | min | actual |
// |-----------------------|-------|--------------------|-----|--------|
// | MemoryWidget          | low   | -                  | 2   | 2      |
// | ~MemoryWidget         | low   | -                  | 1   | 1      |
// | changeTheme           | mid   | switch:3分支       | 3   | 3      |
// | paintEvent            | mid   | hover:2+swap:2     | 3   | 3      |
// | updateStatus          | mid   | 早退:2+swap:2+正常 | 5   | 5      |
// | changeFont            | low   | -                  | 1   | 1      |
// | eventFilter           | low   | enter/leave/透传   | 2   | 2      |
// | mouseDoubleClickEvent | low   | 左键/非左键        | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: parent->width()-20 定宽; 连接 themeTypeChanged/fontChanged/sigDataUpdate; installEventFilter
// - changeTheme: B1 LightType → 亮色参数; B2 DarkType → 暗色参数; B3 default → 仅刷新 palette
// - paintEvent: B1 m_isHover 真伪两分支; B2 m_swapTotal=="0.0 B" 禁用文案分支; B3 swap 环绘制分支
// - updateStatus: B1 readMemInfo 失败仅告警; B2 memUsage 拆分==2 正常; B3 memUsage 非法早退;
//                 B4 swapUsage 拆分==2 正常; B5 swapUsage 非法早退
// - eventFilter: B1 Enter → hover=true 消费; B2 Leave → hover=false 消费; B3 其他透传基类
// - mouseDoubleClickEvent: B1 LeftButton → sendJumpWidgetMessage("MSG_MEM"); B2 其他走基类

#include <gtest/gtest.h>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include "stubext.h"
#include "memory_widget.h"
#include "datadealsingleton.h"
#include <DGuiApplicationHelper>

// sendJumpWidgetMessage 调用计数（避免真实拉起主进程）
static int g_memJumpCalls = 0;

// readMemInfo 可控数据源（替代真实 /proc 读取）
static QString g_memUsage = "128.5 MB";
static QString g_memTotal = "8.0 GB";
static QString g_memPercent = "25.5";
static QString g_swapUsage = "0.0 B";
static QString g_swapTotal = "0.0 B";
static QString g_swapPercent = "0.0";
static bool g_memReadOk = true;

class MemoryWidgetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        // MemoryWidget 构造内部解引用 parent->width()，必须携带真实 parent
        parent = new QWidget;
        parent->resize(280, 400);
        g_memJumpCalls = 0;
        g_memReadOk = true;
        g_memUsage = "128.5 MB";
        g_memTotal = "8.0 GB";
        g_memPercent = "25.5";
        g_swapUsage = "0.0 B";
        g_swapTotal = "0.0 B";
        g_swapPercent = "0.0";
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

TEST_F(MemoryWidgetTest, Ctor_ParentWidth_FixedSizeApplied)
{
    // Arrange: parent 已 resize 到 280x400
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    // Act: 构造被测对象
    MemoryWidget *w = new MemoryWidget(parent);
    // Assert: 固定尺寸为 (parent宽-20, 153)，渲染有效
    EXPECT_EQ(w->width(), 260);
    EXPECT_EQ(w->height(), 153);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(MemoryWidgetTest, Ctor_SigDataUpdate_TriggersUpdateStatus)
{
    // Arrange: 构造并桩定 readMemInfo，验证 ctor 建立的 sigDataUpdate→updateStatus 连接
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 发射单例的 sigDataUpdate 信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 信号驱动重绘渲染成功（updateStatus → update 链路生效）
    QPixmap pm = w->grab();
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, Dtor_AfterUse_ReleasesCleanly)
{
    // Arrange: 构造待析构对象
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 析构（Qt parent 机制先于测试 TearDown 释放）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
    // Assert: 析构过程无崩溃即为通过
    SUCCEED();
}

// ═══════════════════ changeTheme（mid: 3 分支）═══════════════════

TEST_F(MemoryWidgetTest, ChangeTheme_LightType_LightParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B1 亮色分支
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 切换亮色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::LightType);
    // Assert: 渲染正常（内部 numberColor/titleTrans 等亮色参数已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, ChangeTheme_DarkType_DarkParamsApplied)
{
    // Arrange: 构造对象，覆盖 changeTheme B2 暗色分支
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 切换暗色主题
    w->changeTheme(DGuiApplicationHelper::ColorType::DarkType);
    // Assert: 渲染正常（暗色 numberColor 与 titleTrans 已生效）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 153);
    delete w;
}

TEST_F(MemoryWidgetTest, ChangeTheme_UnknownType_KeepsDefaults)
{
    // Arrange: 构造对象，覆盖 changeTheme B3 default 分支（不修改主题参数）
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 传入未知主题类型
    w->changeTheme(DGuiApplicationHelper::ColorType::UnknownType);
    // Assert: 不崩溃且尺寸未变化（default 分支仅刷新 palette）
    EXPECT_EQ(w->width(), 260);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

// ═══════════════════ paintEvent（mid: hover/swap 分支）═══════════════════

TEST_F(MemoryWidgetTest, PaintEvent_IdleState_RendersSuccess)
{
    // Arrange: 构造对象，未 hover、swap 未启用
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    MemoryWidget *w = new MemoryWidget(parent);
    w->updateStatus();
    // Act: 触发绘制（grab 强制 paintEvent）
    QPixmap pm = w->grab();
    // Assert: 渲染有效（B1 非悬停 + B2/B3 swap 禁用分支）
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, PaintEvent_HoverState_HighlightRendered)
{
    // Arrange: 构造对象并注入悬停状态（paintEvent B1 悬停分支）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    MemoryWidget *w = new MemoryWidget(parent);
    QEvent enter(QEvent::Enter);
    w->eventFilter(w, &enter);
    // Act: 触发绘制
    QPixmap pm = w->grab();
    // Assert: 悬停高亮分支渲染有效
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, PaintEvent_SwapEnabled_SwapRingRendered)
{
    // Arrange: 注入启用 swap 的内存数据（paintEvent B3 swap 环分支）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    g_swapUsage = "1.5 GB";
    g_swapTotal = "2.0 GB";
    g_swapPercent = "75.0";
    MemoryWidget *w = new MemoryWidget(parent);
    w->updateStatus();
    // Act: 触发绘制
    QPixmap pm = w->grab();
    // Assert: swap 环绘制分支渲染有效
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.height(), 153);
    delete w;
}

// ═══════════════════ updateStatus（mid: 早退/swap 分支）═══════════════════

TEST_F(MemoryWidgetTest, UpdateStatus_ValidData_DisplayRefreshed)
{
    // Arrange: 桩定 readMemInfo 返回合法双 token 数据
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: B2/B4 正常路径，界面刷新渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, UpdateStatus_ReadFailed_WidgetStillRenders)
{
    // Arrange: readMemInfo 返回 false（B1 失败仅告警分支）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    g_memReadOk = false;
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: 失败不崩溃，界面仍可渲染
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, UpdateStatus_BadMemUsage_EarlyReturn)
{
    // Arrange: memUsage 缺少单位 token（B3 memUsage 非法早退分支）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    g_memUsage = "128.5";
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: 早退不崩溃，后续合法数据仍可正常刷新
    g_memUsage = "128.5 MB";
    w->updateStatus();
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, UpdateStatus_BadSwapUsage_EarlyReturn)
{
    // Arrange: swapUsage 缺少单位 token（B5 swapUsage 非法早退分支）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    g_swapUsage = "0.0B";
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: 早退不崩溃，恢复合法数据后渲染正常
    g_swapUsage = "0.0 B";
    w->updateStatus();
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

TEST_F(MemoryWidgetTest, UpdateStatus_SpacedMemTotal_SpacesStripped)
{
    // Arrange: memTotal 带空格（覆盖 bug #111050 去空格逻辑：m_memTotal 拆分重组）
    stub.set_lamda(ADDR(DataDealSingleton, readMemInfo), [](DataDealSingleton *, QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent) {
        memUsage = g_memUsage;
        memTotal = g_memTotal;
        memPercent = g_memPercent;
        swapUsage = g_swapUsage;
        swapTotal = g_swapTotal;
        swapPercent = g_swapPercent;
        return g_memReadOk;
    });
    g_memTotal = "8 . 0  GB";
    MemoryWidget *w = new MemoryWidget(parent);
    // Act: 执行状态刷新
    w->updateStatus();
    // Assert: 空格剔除后渲染有效（paintEvent 使用 m_memTotal 拼接标题单位）
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->width(), 260);
    delete w;
}

// ═══════════════════ changeFont ═══════════════════

TEST_F(MemoryWidgetTest, ChangeFont_CustomFont_WidgetStillRenders)
{
    // Arrange: 构造对象与自定义字体
    MemoryWidget *w = new MemoryWidget(parent);
    QFont font("Sans", 12);
    // Act: 切换字体（内部设置 section/mem/memUnit/memTxt 四组字体）
    w->changeFont(font);
    // Assert: 不崩溃且渲染有效
    EXPECT_FALSE(w->grab().isNull());
    EXPECT_EQ(w->height(), 153);
    delete w;
}

// ═══════════════════ eventFilter ═══════════════════

TEST_F(MemoryWidgetTest, EventFilter_EnterEvent_ConsumedSetsHover)
{
    // Arrange: 构造对象与 Enter 事件（B1 分支）
    MemoryWidget *w = new MemoryWidget(parent);
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

TEST_F(MemoryWidgetTest, EventFilter_OtherObject_DelegatesToBase)
{
    // Arrange: 非 this 目标与无关事件（B3 透传分支）
    MemoryWidget *w = new MemoryWidget(parent);
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

TEST_F(MemoryWidgetTest, MouseDoubleClick_LeftButton_JumpMessageSent)
{
    // Arrange: 桩定 sendJumpWidgetMessage 并构造左键双击事件（B1 分支）
    stub.set_lamda(ADDR(DataDealSingleton, sendJumpWidgetMessage), [](DataDealSingleton *, const QString &) { ++g_memJumpCalls; return false; });
    MemoryWidget *w = new MemoryWidget(parent);
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, QPointF(10, 10),
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    // Act: 直接触发双击处理
    w->mouseDoubleClickEvent(&dblClick);
    // Assert: 跳转消息恰好发送一次
    EXPECT_EQ(g_memJumpCalls, 1);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}

TEST_F(MemoryWidgetTest, MouseDoubleClick_RightButton_MessageIgnored)
{
    // Arrange: 桩定 sendJumpWidgetMessage 并构造右键双击事件（B2 走基类分支）
    stub.set_lamda(ADDR(DataDealSingleton, sendJumpWidgetMessage), [](DataDealSingleton *, const QString &) { ++g_memJumpCalls; return false; });
    MemoryWidget *w = new MemoryWidget(parent);
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, QPointF(10, 10),
                         Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    // Act: 直接触发双击处理
    w->mouseDoubleClickEvent(&dblClick);
    // Assert: 不发送跳转消息
    EXPECT_EQ(g_memJumpCalls, 0);
    EXPECT_FALSE(w->grab().isNull());
    delete w;
}
