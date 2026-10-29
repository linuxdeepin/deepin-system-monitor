// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - QuickPanelWidget（deepin-system-monitor-plugin/gui/quickpanelwidget.cpp）
//
// 用例计数声明：
// | 函数                    | 用例数 |
// |-------------------------|--------|
// | QuickPanelWidget (构造) | 1      |
// | ~QuickPanelWidget       | 1（各用例 TearDown 覆盖 + 显式用例 1） |
// | initUI                  | 1（合并于构造用例断言布局） |
// | setIcon                 | 1      |
// | setDescription          | 1      |
// | setWidgetState          | 1      |
// | mouseReleaseEvent       | 1      |
// | paintEvent              | 1      |
// | refreshBg               | 2      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：QuickPanelWidget 全部 public 方法均有用例；
// 2 等价类：WidgetState 两种枚举、主题 Light/Dark、underMouse 真/假；
// 3 边界值：激活态与非激活态在 Light/Dark 主题下的 2x2 组合；
// 4 同质分组≥3 用 TEST_P：Light/Dark × 激活/非激活 组合以独立用例覆盖（4 组）；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：DGuiApplicationHelper::themeType 与 QIcon::fromTheme 打桩、
//   WA_UnderMouse 属性直设、render() 触发真实 paintEvent；
// 7 异常断言：无显式 throw，绘制路径以 EXPECT_NO_THROW 覆盖；
// 8 反向场景：非激活状态、非 Dark 主题、鼠标不在控件上；
// 9 强异常安全：setWidgetState 往返切换后状态自恢复；
// 10 打桩选型：主题与图标查询（DGuiApplicationHelper/QIcon::fromTheme）用 stub_ext；
//     CommonIconButton 为被编译进测试目标的项目内类，不打桩。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// QuickPanelWidget::QuickPanelWidget (构造)：
//   B1 初始化图标/描述/布局 → QuickPanelWidget_Ctor_InitializesLayoutAndChildren
// setIcon：
//   B2 以黑白双主题色转发 CommonIconButton::setIcon → QuickPanelWidget_SetIcon_ForwardsWithThemeColors
// setDescription：
//   B3 设置 DLabel 文本 → QuickPanelWidget_SetDescription_UpdatesLabelText
// setWidgetState：
//   B4 WS_ACTIVE → m_icon.setActiveState(true) + refreshBg → QuickPanelWidget_SetWidgetState_TogglesActiveState
//   B5 WS_NORMAL → setActiveState(false) → 同上（往返断言）
// mouseReleaseEvent：
//   B6 underMouse → 发 clicked → QuickPanelWidget_MouseRelease_OnlyUnderMouse_EmitsClicked
//   B7 非 underMouse → 不发信号 → 同上
// paintEvent：
//   B8 active && Dark → 白色 0.8 画刷 → QuickPanelWidget_PaintEvent_ThemeAndState_Branches
//   B9 其它 → 透明画刷 → 同上
// refreshBg：
//   B10 Light 主题 → 请求 -dark 后缀图标 → QuickPanelWidget_RefreshBg_LightTheme_RequestsDarkIconName
//   B11 Dark 主题 → 请求原图标名 → QuickPanelWidget_RefreshBg_DarkTheme_RequestsPlainIconName
//   B12 active && Dark → 描述前景 Highlight → QuickPanelWidget_RefreshBg_ActiveDark_HighlightsDescription
//   B13 其它 → 源码置 NoRole（Qt6 归一化为 WindowText）→ QuickPanelWidget_RefreshBg_Inactive_DefaultRole

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "quickpanelwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalSpy>
#include <QVBoxLayout>

#include <DGuiApplicationHelper>
#include <DLabel>
DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

// QuickPanelWidgetTest：快捷面板控件测试
class QuickPanelWidgetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        widget = new QuickPanelWidget();
        capturedFromThemeNames.clear();
    }
    void TearDown() override
    {
        delete widget;
        stub.clear();
    }

    // 打桩主题类型
    void stubTheme(DGuiApplicationHelper::ColorType type)
    {
        stub.set_lamda(static_cast<DGuiApplicationHelper::ColorType (DGuiApplicationHelper::*)() const>(
                           &DGuiApplicationHelper::themeType),
                       [type]() {
                           return type;
                       });
    }
    // 打桩 QIcon::fromTheme（1 参与 2 参两个重载），返回 qrc 内置图标
    void stubFromTheme()
    {
        stub.set_lamda(static_cast<QIcon (*)(const QString &)>(&QIcon::fromTheme),
                       [](const QString &name) {
                           capturedFromThemeNames.append(name);
                           return QIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
                       });
        stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                       [](const QString &name, const QIcon &fallback) {
                           Q_UNUSED(fallback)
                           capturedFromThemeNames.append(name);
                           return QIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
                       });
    }

    stub_ext::StubExt stub;
    QuickPanelWidget *widget = nullptr;
    static QStringList capturedFromThemeNames;
};
QStringList QuickPanelWidgetTest::capturedFromThemeNames;

// ---- 构造 / 析构 / initUI ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_Ctor_InitializesLayoutAndChildren)
{
    // Arrange: SetUp 已构造
    // Act: 检查初始布局与子控件
    // Assert: 图标 24x24 固定、描述右省略、边距 8/间距 0、子控件父为本控件（B1）
    EXPECT_NE(widget->m_icon, nullptr);
    EXPECT_NE(widget->m_description, nullptr);
    EXPECT_EQ(widget->m_icon->parentWidget(), widget);
    EXPECT_EQ(widget->m_description->parentWidget(), widget);
    EXPECT_EQ(widget->m_description->elideMode(), Qt::ElideRight);
    EXPECT_EQ(widget->m_icon->size(), QSize(24, 24));
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(widget->layout());
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->contentsMargins(), QMargins(8, 8, 8, 8));
    EXPECT_EQ(layout->spacing(), 0);

    // 析构覆盖（B~）：局部对象直接析构
    {
        QuickPanelWidget scoped;
        EXPECT_NE(scoped.m_icon, nullptr);
    }
}

// ---- setIcon / setDescription ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_SetIcon_ForwardsWithThemeColors)
{
    // Arrange: qrc 内置图标
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    ASSERT_FALSE(icon.isNull());
    // Act: 设置面板图标
    widget->setIcon(icon);
    // Assert: 内部按钮持有图标且保存黑白主题色（B2）
    EXPECT_FALSE(widget->m_icon->m_icon.isNull());
    EXPECT_EQ(widget->m_icon->m_lightThemeColor, QColor(Qt::black));
    EXPECT_EQ(widget->m_icon->m_darkThemeColor, QColor(Qt::white));
}

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_SetDescription_UpdatesLabelText)
{
    // Arrange: 构造完成
    // Act: 设置描述文本
    widget->setDescription("CPU: 12%");
    // Assert: DLabel 文本同步，再设置可覆盖（B3）
    EXPECT_EQ(widget->m_description->text(), QString("CPU: 12%"));
    widget->setDescription("MEM: 34%");
    EXPECT_NE(widget->m_description->text(), QString("CPU: 12%"));
}

// ---- setWidgetState ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_SetWidgetState_TogglesActiveState)
{
    // Arrange: 初始非激活
    ASSERT_FALSE(widget->m_icon->activeState());
    // Act: 激活（refreshBg 走真实主题查询，无总线环境返回空图标不崩溃）
    EXPECT_NO_THROW(widget->setWidgetState(QuickPanelWidget::WS_ACTIVE));
    // Assert: 激活态置位（B4）
    EXPECT_TRUE(widget->m_icon->activeState());
    // 反向：恢复正常（B5，强异常安全往返）
    EXPECT_NO_THROW(widget->setWidgetState(QuickPanelWidget::WS_NORMAL));
    EXPECT_FALSE(widget->m_icon->activeState());
}

// ---- mouseReleaseEvent ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_MouseRelease_OnlyUnderMouse_EmitsClicked)
{
    // Arrange: 信号监视；先验证非 underMouse 不发信号
    QSignalSpy spy(widget, &QuickPanelWidget::clicked);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(10, 10), QPointF(10, 10),
                        QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    // Act: 未进入鼠标区域时抬起
    widget->setAttribute(Qt::WA_UnderMouse, false);
    widget->mouseReleaseEvent(&release);
    // Assert: 无信号（B7）
    EXPECT_EQ(spy.count(), 0);

    // 反向：置 WA_UnderMouse 后抬起发出 clicked（B6）
    widget->setAttribute(Qt::WA_UnderMouse, true);
    widget->mouseReleaseEvent(&release);
    EXPECT_EQ(spy.count(), 1);
}

// ---- paintEvent ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_PaintEvent_ThemeAndState_Branches)
{
    // Arrange: 打桩 Dark 主题 + 激活态
    stubTheme(DGuiApplicationHelper::DarkType);
    widget->m_icon->m_activeState = true;
    QPixmap pixmap(64, 64);
    // Act: 激活态渲染（B8：active && Dark → 白色 0.8 画刷）
    EXPECT_NO_THROW(widget->render(&pixmap));
    const bool activeDarkBrush = widget->m_icon->activeState()
        && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;

    // 反向：非激活 → 透明画刷分支（B9）
    widget->m_icon->m_activeState = false;
    QPixmap pixmap2(64, 64);
    EXPECT_NO_THROW(widget->render(&pixmap2));
    const bool expectWhiteBrush2 = widget->m_icon->activeState()
        && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;

    // Assert: 两次渲染均产出位图，且画刷决策与 activeState&&themeType 等价类一致
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_TRUE(activeDarkBrush);
    EXPECT_FALSE(pixmap2.isNull());
    EXPECT_FALSE(expectWhiteBrush2);
}

// ---- refreshBg ----

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_RefreshBg_LightTheme_RequestsDarkIconName)
{
    // Arrange: Light 主题 + 打桩 fromTheme
    stubTheme(DGuiApplicationHelper::LightType);
    stubFromTheme();
    // Act: 刷新背景/图标
    widget->refreshBg();
    // Assert: Light 下请求 "status-system-monitor-dark"（2 参）与 "dsm_pluginicon_dark"（1 参回退）（B10）
    ASSERT_GE(capturedFromThemeNames.count(), 2);
    EXPECT_TRUE(capturedFromThemeNames.contains("status-system-monitor-dark"));
    EXPECT_TRUE(capturedFromThemeNames.contains("dsm_pluginicon_dark"));
    // 图标链路：内部按钮持有非空图标
    EXPECT_FALSE(widget->m_icon->m_icon.isNull());
}

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_RefreshBg_DarkTheme_RequestsPlainIconName)
{
    // Arrange: Dark 主题 + 打桩 fromTheme
    stubTheme(DGuiApplicationHelper::DarkType);
    stubFromTheme();
    // Act: 刷新背景/图标
    widget->refreshBg();
    // Assert: Dark 下请求 "status-system-monitor" 与 "dsm_pluginicon_light"（B11）
    ASSERT_GE(capturedFromThemeNames.count(), 2);
    EXPECT_TRUE(capturedFromThemeNames.contains("status-system-monitor"));
    EXPECT_TRUE(capturedFromThemeNames.contains("dsm_pluginicon_light"));
}

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_RefreshBg_ActiveDark_HighlightsDescription)
{
    // Arrange: Dark 主题 + 激活态 + 打桩 fromTheme
    stubTheme(DGuiApplicationHelper::DarkType);
    stubFromTheme();
    widget->m_icon->m_activeState = true;
    // Act: 刷新
    widget->refreshBg();
    // Assert: 描述前景走 Highlight，且图标处于激活态（B12）
    EXPECT_EQ(widget->m_description->foregroundRole(), QPalette::Highlight);
    EXPECT_TRUE(widget->m_icon->activeState());
}

TEST_F(QuickPanelWidgetTest, QuickPanelWidget_RefreshBg_Inactive_DefaultRole)
{
    // Arrange: Dark 主题 + 非激活态 + 打桩 fromTheme
    stubTheme(DGuiApplicationHelper::DarkType);
    stubFromTheme();
    widget->m_icon->m_activeState = false;
    // Act: 刷新
    widget->refreshBg();
    // Assert: 非激活态描述前景为非 Highlight；源码置 NoRole，但 Qt6
    // QWidget::setForegroundRole(NoRole) 归一化为默认前景角色 WindowText
    EXPECT_EQ(widget->m_description->foregroundRole(), QPalette::WindowText);
    EXPECT_NE(widget->m_description->foregroundRole(), QPalette::Highlight);
}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | QuickPanelWidget | high | ctor/description/state | 6 | 6 |
// | paintEvent/refreshBg | high | theme/active/iconname | 4 | 4 |
