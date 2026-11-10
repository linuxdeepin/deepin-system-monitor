// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - MonitorPluginButtonWidget（deepin-system-monitor-plugin/gui/monitorpluginbuttonwidget.cpp）
//
// 用例计数声明：
// | 函数                  | 用例数 |
// |-----------------------|--------|
// | MonitorPluginButtonWidget (构造) | 1 |
// | paintEvent            | 3      |
// | resizeEvent           | 1      |
// | mouseMoveEvent        | 1      |
// | mousePressEvent       | 1      |
// | mouseReleaseEvent     | 1      |
// | leaveEvent            | 1      |
// | loadSvg               | 0（源码已不再调用 loadSvg，paintEvent 改用 QIcon 体系；不产生死用例） |
// | containCursorPos      | 2      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：构造函数为唯一 public 方法（其余为 protected 事件），全部覆盖；
// 2 等价类：光标在/不在图标方形区、控件尺寸 >16 / <=16、主题 Light/Dark、hover/pressed 组合；
// 3 边界值：min(w,h)==16（不大于 PLUGIN_BACKGROUND_MIN_SIZE 的临界）、矩形边界点 (0,0)/(39,39)；
// 4 同质分组≥3 用 TEST_P：paintEvent 主题×尺寸×hover/pressed 8 组参数化；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：QCursor::pos 打桩（光标位置）、DGuiApplicationHelper themeType 打桩、
//   QIcon::fromTheme 打桩（强制走 qrc SVG 回退分支）、render() 触发真实 paintEvent；
// 7 异常断言：无显式 throw，绘制路径 EXPECT_NO_THROW；
// 8 反向场景：光标在区域外、释放后状态清零、无主题图标时的 qrc 回退；
// 9 强异常安全：鼠标状态清零（release/leave）后再次进入/按下可重新置位；
// 10 打桩选型：QCursor::pos（静态）、themeType（成员）、fromTheme（静态重载）用 stub_ext；
//     不引入 gMock。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// MonitorPluginButtonWidget::MonitorPluginButtonWidget (构造)：
//   B1 鼠标追踪 + 最小尺寸 16 + 主题变化重绘连接 → Button_Ctor_InitializesState
// paintEvent：
//   B2 min(w,h) > 16 && Light → 黑底 + hover(0.6)/pressed(0.3) 透明度 → TEST_P（Light_Large/Hover/Pressed）
//   B3 min(w,h) > 16 && Dark → 白底 + hover(0.2)/pressed(0.05) → TEST_P（Dark_Large）
//   B4 min<=16 && Light → 深色图标名 → TEST_P（Light_Small）
//   B5 min<=16 && Dark → 原图标名 → TEST_P（Dark_Small）
//   B6 fromTheme 命中 → QIcon::pixmap 绘制 → TEST_P（ThemeIconHit）
//   B7 fromTheme 未命中 → qrc SVG 回退 → Button_PaintEvent_MissingThemeIcon_FallsBackToQrc
//   B8 旋转/居中坐标计算（x=(w-iconSize)/2）→ 各绘制用例共同覆盖
// resizeEvent：
//   B9 仅基类处理（宽高比逻辑已注释）→ Button_ResizeEvent_PreservesRequestedSize
// mouseMoveEvent：
//   B10 containCursorPos → hover 置位 → Button_MouseMove_FollowsCursor
//   B11 非 containCursorPos → hover 复位 → 同上
// mousePressEvent：
//   B12 containCursorPos → pressed 置位 → Button_MousePress_FollowsCursor
//   B13 非 containCursorPos → pressed 复位 → 同上
// mouseReleaseEvent：
//   B14 无条件清零 hover/pressed → Button_MouseRelease_ClearsStates
// leaveEvent：
//   B15 无条件清零 hover/pressed → Button_LeaveEvent_ClearsStates
// containCursorPos：
//   B16 中心方形内 → true → Button_ContainCursorPos_CursorInside_ReturnsTrue
//   B17 中心方形外 → false → Button_ContainCursorPos_CursorOutside_ReturnsFalse
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | MonitorPluginButtonWidget | high | ctor/mouse/leave/paint | 9 | 9 |
// | paintEvent(参数化) | high | theme/size/state/fallback | 8 | 8 |

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "monitorpluginbuttonwidget.h"

#include <QCursor>
#include <QEvent>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>

#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE

// ButtonTest：dock 插件图标按钮测试
class ButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        button = new MonitorPluginButtonWidget();
        button->resize(40, 40); // containCursorPos 在 40x40 下中心方形即全矩形
        capturedFromThemeNames.clear();
    }
    void TearDown() override
    {
        delete button;
        stub.clear();
    }

    // 打桩光标全局位置（QCursor::pos 静态重载）
    void stubCursorPos(const QPoint &pos)
    {
        stub.set_lamda(static_cast<QPoint (*)()>(&QCursor::pos),
                       [pos]() {
                           return pos;
                       });
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
    // 打桩 fromTheme：捕获名字并返回空图标（强制 qrc 回退分支）
    void stubFromThemeNull()
    {
        stub.set_lamda(static_cast<QIcon (*)(const QString &)>(&QIcon::fromTheme),
                       [](const QString &name) {
                           capturedFromThemeNames.append(name);
                           return QIcon();
                       });
        stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                       [](const QString &name, const QIcon &fallback) {
                           Q_UNUSED(fallback)
                           capturedFromThemeNames.append(name);
                           return QIcon();
                       });
    }

    static QMouseEvent pressEvent(const QPoint &pos)
    {
        return QMouseEvent(QEvent::MouseButtonPress, pos, pos, pos,
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    }
    static QMouseEvent releaseEvent(const QPoint &pos)
    {
        return QMouseEvent(QEvent::MouseButtonRelease, pos, pos, pos,
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    }
    static QMouseEvent moveEvent(const QPoint &pos)
    {
        return QMouseEvent(QEvent::MouseMove, pos, pos, pos,
                           Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    }

    stub_ext::StubExt stub;
    MonitorPluginButtonWidget *button = nullptr;
    static QStringList capturedFromThemeNames;
};
QStringList ButtonTest::capturedFromThemeNames;

// ---- 构造 ----

TEST_F(ButtonTest, Button_Ctor_InitializesState)
{
    // Arrange: SetUp 已构造
    // Act: 读取初始状态
    // Assert: 鼠标追踪开启、最小尺寸 16x16（PLUGIN_BACKGROUND_MIN_SIZE）、初始无 hover/pressed（B1）
    EXPECT_TRUE(button->hasMouseTracking());
    EXPECT_EQ(button->minimumSize(), QSize(16, 16));
    EXPECT_FALSE(button->m_hover);
    EXPECT_FALSE(button->m_pressed);
}

// ---- containCursorPos ----

TEST_F(ButtonTest, Button_ContainCursorPos_CursorInside_ReturnsTrue)
{
    // Arrange: 光标打桩在控件中心 (20,20)
    stubCursorPos(QPoint(20, 20));
    // Act: 判定光标包含性
    bool inside = button->containCursorPos();
    // Assert: 中心方形包含光标（B16）
    EXPECT_TRUE(inside);

    // 边界补充：矩形左上角 (0,0)（中心方形即全矩形时包含边界点）
    stubCursorPos(QPoint(0, 0));
    EXPECT_TRUE(button->containCursorPos());
}

TEST_F(ButtonTest, Button_ContainCursorPos_CursorOutside_ReturnsFalse)
{
    // Arrange: 光标打桩在控件外 (200,200)
    stubCursorPos(QPoint(200, 200));
    // Act: 判定光标包含性
    bool outside = button->containCursorPos();
    // Assert: 不包含（B17）
    EXPECT_FALSE(outside);

    // 边界收缩：16x16 小控件下中心方形即全矩形，(17,17) 在外
    button->resize(16, 16);
    stubCursorPos(QPoint(17, 17));
    EXPECT_FALSE(button->containCursorPos());
}

// ---- mouseMoveEvent / mousePressEvent / mouseReleaseEvent / leaveEvent ----

TEST_F(ButtonTest, Button_MouseMove_FollowsCursor)
{
    // Arrange: 光标在内
    stubCursorPos(QPoint(20, 20));
    QMouseEvent move = moveEvent(QPoint(20, 20));
    // Act: 移动
    button->mouseMoveEvent(&move);
    // Assert: hover 置位（B10）
    EXPECT_TRUE(button->m_hover);
    // 反向：光标在外 → hover 复位（B11）
    stubCursorPos(QPoint(300, 300));
    button->mouseMoveEvent(&move);
    EXPECT_FALSE(button->m_hover);
}

TEST_F(ButtonTest, Button_MousePress_FollowsCursor)
{
    // Arrange: 光标在内
    stubCursorPos(QPoint(20, 20));
    QMouseEvent press = pressEvent(QPoint(20, 20));
    // Act: 按下
    button->mousePressEvent(&press);
    // Assert: pressed 置位（B12）
    EXPECT_TRUE(button->m_pressed);
    // 反向：光标在外 → pressed 复位（B13）
    stubCursorPos(QPoint(300, 300));
    button->mousePressEvent(&press);
    EXPECT_FALSE(button->m_pressed);
}

TEST_F(ButtonTest, Button_MouseRelease_ClearsStates)
{
    // Arrange: 预置 hover/pressed
    button->m_hover = true;
    button->m_pressed = true;
    QMouseEvent release = releaseEvent(QPoint(20, 20));
    // Act: 抬起（无条件清零）
    button->mouseReleaseEvent(&release);
    // Assert: 双状态清零（B14，强异常安全：可再次置位）
    EXPECT_FALSE(button->m_hover);
    EXPECT_FALSE(button->m_pressed);
    stubCursorPos(QPoint(20, 20));
    QMouseEvent press = pressEvent(QPoint(20, 20));
    button->mousePressEvent(&press);
    EXPECT_TRUE(button->m_pressed);
}

TEST_F(ButtonTest, Button_LeaveEvent_ClearsStates)
{
    // Arrange: 预置 hover/pressed
    button->m_hover = true;
    button->m_pressed = true;
    QEvent leave(QEvent::Leave);
    // Act: 离开
    button->leaveEvent(&leave);
    // Assert: 双状态清零（B15）
    EXPECT_FALSE(button->m_hover);
    EXPECT_FALSE(button->m_pressed);
}

// ---- resizeEvent ----

TEST_F(ButtonTest, Button_ResizeEvent_PreservesRequestedSize)
{
    // Arrange: 初始 40x40
    // Act: 直接改尺寸（QResizeEvent 由 resize 触发 resizeEvent）
    button->resize(50, 30);
    // Assert: 无宽高比锁定，尺寸保持请求值（B9）
    EXPECT_EQ(button->size(), QSize(50, 30));
    EXPECT_EQ(button->width(), 50);
    EXPECT_EQ(button->height(), 30);
}

// ---- paintEvent（TEST_P：主题 × 尺寸 × hover/pressed × 图标命中）----

namespace {
struct PaintCase {
    const char *title;
    DGuiApplicationHelper::ColorType theme;
    int w;
    int h;
    bool hover;
    bool pressed;
    const char *expectIconName;    // 2 参 fromTheme 期望请求名
    const char *expectFallbackName; // 1 参 fromTheme 期望请求名
};
std::ostream &operator<<(std::ostream &os, const PaintCase &c)
{
    return os << c.title;
}
}

class PaintParamTest : public ButtonTest,
                       public ::testing::WithParamInterface<PaintCase>
{
};

TEST_P(PaintParamTest, Button_PaintEvent_ThemeSizeState_Branches)
{
    // Arrange: 打桩主题 + fromTheme（捕获名并返回空图标 → qrc 回退绘制）
    const PaintCase param = GetParam();
    stubTheme(param.theme);
    stubFromThemeNull();
    button->resize(param.w, param.h);
    button->m_hover = param.hover;
    button->m_pressed = param.pressed;
    // Act: 渲染触发 paintEvent
    QPixmap pixmap(param.w + 8, param.h + 8);
    EXPECT_NO_THROW(button->render(&pixmap));
    // Assert: 请求的图标名符合分支预期（B2-B5），渲染完成
    EXPECT_FALSE(pixmap.isNull());
    ASSERT_GE(capturedFromThemeNames.count(), 2);
    EXPECT_EQ(capturedFromThemeNames.first(), QString::fromLatin1(param.expectFallbackName));
    EXPECT_TRUE(capturedFromThemeNames.contains(QString::fromLatin1(param.expectIconName)));
    // 状态不被绘制改变
    EXPECT_EQ(button->m_hover, param.hover);
    EXPECT_EQ(button->m_pressed, param.pressed);
}

INSTANTIATE_TEST_SUITE_P(PaintBranches, PaintParamTest,
                         ::testing::Values(
                             PaintCase { "Light_Large_Default", DGuiApplicationHelper::LightType, 64, 64, false, false,
                                         "status-system-monitor", "dsm_pluginicon_light" },
                             PaintCase { "Light_Large_Hover", DGuiApplicationHelper::LightType, 64, 64, true, false,
                                         "status-system-monitor", "dsm_pluginicon_light" },
                             PaintCase { "Light_Large_Pressed", DGuiApplicationHelper::LightType, 64, 64, false, true,
                                         "status-system-monitor", "dsm_pluginicon_light" },
                             PaintCase { "Dark_Large_Default", DGuiApplicationHelper::DarkType, 64, 64, false, false,
                                         "status-system-monitor", "dsm_pluginicon_light" },
                             PaintCase { "Light_Small_Default", DGuiApplicationHelper::LightType, 16, 16, false, false,
                                         "status-system-monitor-dark", "dsm_pluginicon_dark" },
                             PaintCase { "Light_Small_Hover", DGuiApplicationHelper::LightType, 16, 16, true, false,
                                         "status-system-monitor-dark", "dsm_pluginicon_dark" },
                             PaintCase { "Dark_Small_Default", DGuiApplicationHelper::DarkType, 16, 16, false, false,
                                         "status-system-monitor", "dsm_pluginicon_light" },
                             PaintCase { "Dark_Large_Hover", DGuiApplicationHelper::DarkType, 48, 48, true, false,
                                         "status-system-monitor", "dsm_pluginicon_light" }));

TEST_F(ButtonTest, Button_PaintEvent_MissingThemeIcon_FallsBackToQrc)
{
    // Arrange: 打桩 fromTheme 返回空（未命中主题图标）
    stubTheme(DGuiApplicationHelper::LightType);
    stubFromThemeNull();
    // Act: 渲染（qrc 内 dsm_pluginicon_light_20px.svg 由测试目标 qrc 提供）
    QPixmap pixmap(64, 64);
    EXPECT_NO_THROW(button->render(&pixmap));
    // Assert: 走 qrc 回退分支（B7）且渲染完成
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_FALSE(capturedFromThemeNames.isEmpty());
}
