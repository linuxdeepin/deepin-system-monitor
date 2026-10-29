// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - CommonIconButton（deepin-system-monitor-plugin/gui/commoniconbutton.cpp）
//
// 用例计数声明：
// | 函数                        | 用例数 |
// |-----------------------------|--------|
// | CommonIconButton (构造函数)  | 2      |
// | setStateIconMapping          | 1（与 setState 合并覆盖） |
// | setState                     | 3      |
// | setActiveState               | 3      |
// | activeState                  | 1（合并于 setActiveState） |
// | setIcon(QString)             | 4      |
// | setIcon(QIcon,颜色)          | 2      |
// | setHoverIcon                 | 1      |
// | setClickable                 | 1      |
// | setRotatable                 | 1      |
// | startRotate                  | 2      |
// | stopRotate                   | 1      |
// | event                        | 1      |
// | paintEvent                   | 2      |
// | mousePressEvent              | 1      |
// | mouseReleaseEvent            | 4      |
// | refreshIcon                  | 1      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：CommonIconButton 全部 public 方法均有用例；
// 2 等价类：主题 Light/Dark、状态映射命中/未命中、颜色有效/无效；
// 3 边界值：旋转角 0/54/162/378（>=360 停转）、点击位置 inside/outside/corner；
// 4 同质分组≥3 用 TEST_P：Light/Dark 主题 ×（含后缀/不含后缀）文件名变换（4 组）；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：QIcon::fromTheme 打桩捕获请求名、DGuiApplicationHelper themeType 打桩、
//   render() 触发真实 paintEvent、直接派发 QMouseEvent/QEvent；
// 7 异常断言：无显式 throw；paintEvent/render 以 EXPECT_NO_HEAP_LEAKS 语义（不崩溃）覆盖；
// 8 反向场景：不可点击、点击区域外、旋转进行中点击、空图标绘制；
// 9 强异常安全：setRotatable(false) 清理定时器后再次 startRotate 重建定时器；
// 10 打桩选型：DGuiApplicationHelper::themeType 与 QIcon::fromTheme（静态/成员）用
//    stub_ext；不引入 gMock（无接口类需要 mock）。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// CommonIconButton::CommonIconButton (构造)：
//   B1 无 parent → 固定 24x24（dpr=1 下 qCeil(24*1)）→ CommonIconButton_Ctor_Default_InitializesState
//   B2 有 parent → 继承 foregroundRole → CommonIconButton_Ctor_WithParent_InheritsForegroundRole
// setStateIconMapping：
//   B3 存表 → CommonIconButton_SetState_MappedState_UsesMapping
// setState：
//   B4 mapping 命中 → setIcon(pair) → CommonIconButton_SetState_MappedState_UsesMapping
//   B5 m_icon 非空 → 以当前颜色重设 → CommonIconButton_SetState_UnmappedWithValidIcon_ReappliesIcon
//   B6 m_icon 为空 → 跳过重设 → CommonIconButton_SetState_UnmappedNullIcon_NoLookup
// setActiveState：
//   B7 颜色有效 → 重设图标 → CommonIconButton_SetActiveState_WithColors_ReappliesPalette
//   B8 颜色无效 → setForegroundRole(Highlight/NoRole) → CommonIconButton_SetActiveState_NoColors_TogglesRole
// setIcon(QString...)：
//   B9 Default 映射缺失 → 记录原始 (icon,fallback) → CommonIconButton_SetIcon_String_StoresDefaultMapping
//   B10 Light 主题 → addDarkMark → TEST_P
//   B11 非 Light 主题 → 原名 → TEST_P
//   B12 文件含 suffix → replace → TEST_P
//   B13 文件不含 suffix → append "-dark" → TEST_P
// setIcon(QIcon, light, dark)：
//   B14 双色有效 → 应用调色板（active 时 Highlight）→ CommonIconButton_SetIcon_QIconWithColors_AppliesPalette
//   B15 颜色无效 → 跳过调色板 → CommonIconButton_SetIcon_QIconWithColors_AppliesPalette
// setRotatable：
//   B16 false → 删除定时器 → CommonIconButton_SetRotatable_False_DeletesTimer
// startRotate：
//   B17 定时器为空 → 新建并启动 → CommonIconButton_MouseRelease_Rotatable_ClickStartsRotation
//   B18 角度 >=360 → stopRotate → CommonIconButton_StartRotate_AccumulatesToFullTurn_Stops
// stopRotate：
//   B19 停表 + 归零 → CommonIconButton_StopRotate_ResetsAngle
// event：
//   B20 Enter → hover=true → CommonIconButton_Event_EnterLeave_TogglesHover
//   B21 Leave → hover=false → 同上
//   B22 其它事件 → 透传 → 同上
// paintEvent：
//   B23 rotateAngle!=0 → 旋转变换 → CommonIconButton_PaintEvent_WithIcon_RendersAndRotation
//   B24 hover 且 hoverIcon 非空 → 画 hover 图标 → CommonIconButton_PaintEvent_NullIcon_RendersNoCrash
//   B25 hoverIcon 空 → 画主图标 → CommonIconButton_PaintEvent_WithIcon_RendersAndRotation
//   B26 两图标皆空 → 不绘制 → CommonIconButton_PaintEvent_NullIcon_RendersNoCrash
// mousePressEvent：
//   B27 记录按压点 → CommonIconButton_MousePress_StoresPressPos
// mouseReleaseEvent：
//   B28 clickable && 区域内 && 定时器未跑 → clicked → CommonIconButton_MouseRelease_ClickableInside_EmitsClicked
//   B29 不可点击 → 透传 → CommonIconButton_MouseRelease_NotClickable_NoEmit
//   B30 按压点在区域外 → 透传 → CommonIconButton_MouseRelease_Outside_NoEmit
//   B31 旋转进行中 → 透传（防重复触发）→ CommonIconButton_MouseRelease_DuringRotation_NoEmit
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | CommonIconButton | high | ctor/clickable/hover/event | 8 | 8 |
// | setIcon/refreshIcon/setActiveState | high | theme/suffix/mapping | 8 | 8 |
// | paintEvent/mouseEvent | high | icon/state/rotation | 8 | 8 |

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "commoniconbutton.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalSpy>
#include <QTimer>
#include <QEvent>

#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE

// CommonIconButtonTest：普通图标按钮（QWidget 派生）行为测试
class CommonIconButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        button = new CommonIconButton();
        requestedThemeNames.clear();
        requestedFallbackNames.clear();
    }
    void TearDown() override
    {
        delete button;
        stub.clear();
    }

    // 派发一次鼠标事件（Qt6 构造：local/scene/global + 按键 + 修饰键）
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

    stub_ext::StubExt stub;
    CommonIconButton *button = nullptr;
    // 捕获 QIcon::fromTheme 收到的图标名（主题分支断言用）
    static QStringList requestedThemeNames;
    static QStringList requestedFallbackNames;   // 一参重载 QIcon::fromTheme(name) 的请求名
};
QStringList CommonIconButtonTest::requestedThemeNames;
QStringList CommonIconButtonTest::requestedFallbackNames;

// ---- 构造 ----

TEST_F(CommonIconButtonTest, CommonIconButton_Ctor_Default_InitializesState)
{
    // Arrange: SetUp 已构造
    // Act: 读取初始状态
    // Assert: accessible 名、24x24（offscreen dpr=1 → qCeil(24*1)）、各开关初值
    EXPECT_EQ(button->accessibleName(), QString("IconButton"));
    EXPECT_EQ(button->size(), QSize(24, 24));
    EXPECT_FALSE(button->activeState());
    EXPECT_EQ(button->m_state, CommonIconButton::Default);
    EXPECT_FALSE(button->m_clickable);
    EXPECT_FALSE(button->m_rotatable);
    EXPECT_FALSE(button->m_hover);
    EXPECT_EQ(button->m_rotateAngle, 0);
    EXPECT_TRUE(button->m_icon.isNull());
    EXPECT_TRUE(button->m_hoverIcon.isNull());
    EXPECT_EQ(button->m_refreshTimer, nullptr);
}

TEST_F(CommonIconButtonTest, CommonIconButton_Ctor_WithParent_InheritsForegroundRole)
{
    // Arrange: 构造带调色板的父控件
    QWidget parent;
    QPalette pa = parent.palette();
    pa.setColor(parent.foregroundRole(), Qt::red);
    parent.setPalette(pa);
    // Act: 以 parent 创建子按钮
    CommonIconButton child(&parent);
    // Assert: 子按钮继承父控件前景角色，且调色板同色
    EXPECT_EQ(child.foregroundRole(), parent.foregroundRole());
    EXPECT_EQ(child.palette().color(child.foregroundRole()), QColor(Qt::red));
}

// ---- setClickable / setRotatable / setHoverIcon ----

TEST_F(CommonIconButtonTest, CommonIconButton_SetClickable_TogglesState)
{
    // Arrange: 默认不可点击
    ASSERT_FALSE(button->m_clickable);
    // Act: 开启后关闭
    button->setClickable(true);
    const bool afterEnable = button->m_clickable;
    button->setClickable(false);
    // Assert: 开/关往返（B24）
    EXPECT_TRUE(afterEnable);
    EXPECT_FALSE(button->m_clickable);
}

TEST_F(CommonIconButtonTest, CommonIconButton_SetHoverIcon_StoresHoverIcon)
{
    // Arrange: qrc 内置图标（构造非空 QIcon）
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    ASSERT_FALSE(icon.isNull());
    // Act: 设置 hover 图标
    button->setHoverIcon(icon);
    // Assert: 保存且相等
    EXPECT_FALSE(button->m_hoverIcon.isNull());
    EXPECT_EQ(button->m_hoverIcon.cacheKey(), icon.cacheKey());
}

TEST_F(CommonIconButtonTest, CommonIconButton_SetRotatable_False_DeletesTimer)
{
    // Arrange: 先启动一次旋转以创建定时器
    button->setRotatable(true);
    button->startRotate();
    ASSERT_NE(button->m_refreshTimer, nullptr);
    // Act: 关闭可旋转（应删除定时器；角度不清零——记录源码行为）
    button->setRotatable(false);
    // Assert: 定时器为空，角度保持残留值 54
    EXPECT_EQ(button->m_refreshTimer, nullptr);
    EXPECT_EQ(button->m_rotateAngle, 54);

    // 强异常安全：再次 startRotate 应重建定时器
    button->startRotate();
    EXPECT_NE(button->m_refreshTimer, nullptr);
    button->stopRotate(); // 归位，避免影响后续用例
}

// ---- setIcon(QString) 主题分支（TEST_P：主题 × 后缀）----

namespace {
struct ThemeNameCase {
    const char *title;
    DGuiApplicationHelper::ColorType theme;
    const char *icon;
    const char *fallback;
    const char *expectIcon;
    const char *expectFallback;
};
std::ostream &operator<<(std::ostream &os, const ThemeNameCase &c)
{
    return os << c.title;
}
}

class ThemeNameParamTest : public CommonIconButtonTest,
                           public ::testing::WithParamInterface<ThemeNameCase>
{
};

TEST_P(ThemeNameParamTest, CommonIconButton_SetIcon_String_ThemeNameTransform)
{
    // Arrange: 打桩主题类型与 fromTheme，捕获请求名
    stub.set_lamda(static_cast<DGuiApplicationHelper::ColorType (DGuiApplicationHelper::*)() const>(
                       &DGuiApplicationHelper::themeType),
                   [p = GetParam()]() {
                       return p.theme;
                   });
    stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                   [](const QString &name, const QIcon &fallback) {
                       Q_UNUSED(fallback)
                       requestedThemeNames.append(name);
                       return QIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
                   });
    // 源码内层 QIcon::fromTheme(tmpFallback) 走一参重载，需单独打桩才能捕获
    stub.set_lamda(static_cast<QIcon (*)(const QString &)>(&QIcon::fromTheme),
                   [](const QString &name) {
                       requestedFallbackNames.append(name);
                       return QIcon();
                   });
    const ThemeNameCase param = GetParam();
    // Act: 字符串图标设置
    button->setIcon(QString::fromLatin1(param.icon), QString::fromLatin1(param.fallback));
    // Assert: 主题名变换符合预期（含后缀 replace / 不含后缀 append / Dark 不变）
    // 外层二参请求主名，内层一参请求回退名，分列表断言避免参数求值顺序影响
    ASSERT_EQ(requestedThemeNames.count(), 1);
    EXPECT_EQ(requestedThemeNames.first(), QString::fromLatin1(param.expectIcon));
    ASSERT_EQ(requestedFallbackNames.count(), 1);
    EXPECT_EQ(requestedFallbackNames.first(), QString::fromLatin1(param.expectFallback));
    EXPECT_FALSE(button->m_icon.isNull());
}

INSTANTIATE_TEST_SUITE_P(ThemeNameTransform, ThemeNameParamTest,
                         ::testing::Values(
                             ThemeNameCase { "Light_WithSuffix", DGuiApplicationHelper::LightType,
                                             "icon.svg", "fb.svg", "icon-dark.svg", "fb-dark.svg" },
                             ThemeNameCase { "Light_NoSuffix", DGuiApplicationHelper::LightType,
                                             "icon", "fb", "icon-dark", "fb-dark" },
                             ThemeNameCase { "Dark_WithSuffix", DGuiApplicationHelper::DarkType,
                                             "icon.svg", "fb.svg", "icon.svg", "fb.svg" },
                             ThemeNameCase { "Dark_NoSuffix", DGuiApplicationHelper::DarkType,
                                             "icon", "fb", "icon", "fb" }));

TEST_F(CommonIconButtonTest, CommonIconButton_SetIcon_String_StoresDefaultMapping)
{
    // Arrange: 任意主题（不打桩，不依赖主题分支）
    // Act: 字符串图标设置
    button->setIcon("my-icon", "my-fallback");
    // Assert: Default 槽记录原始（未变暗的）名字对
    ASSERT_TRUE(button->m_fileMapping.contains(CommonIconButton::Default));
    EXPECT_EQ(button->m_fileMapping.value(CommonIconButton::Default).first, QString("my-icon"));
    EXPECT_EQ(button->m_fileMapping.value(CommonIconButton::Default).second, QString("my-fallback"));
}

// ---- setIcon(QIcon, 颜色) ----

TEST_F(CommonIconButtonTest, CommonIconButton_SetIcon_QIconWithColors_AppliesPalette)
{
    // Arrange: 打桩主题为 Light
    stub.set_lamda(static_cast<DGuiApplicationHelper::ColorType (DGuiApplicationHelper::*)() const>(
                       &DGuiApplicationHelper::themeType),
                   []() {
                       return DGuiApplicationHelper::LightType;
                   });
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    // Act: 双色有效设置
    button->setIcon(icon, Qt::black, Qt::white);
    // Assert: 主图标保存、颜色保存、Light 主题下窗口文本用 light 色
    EXPECT_FALSE(button->m_icon.isNull());
    EXPECT_EQ(button->m_lightThemeColor, QColor(Qt::black));
    EXPECT_EQ(button->m_darkThemeColor, QColor(Qt::white));
    EXPECT_EQ(button->palette().color(QPalette::WindowText), QColor(Qt::black));

    // 分支 B15：颜色无效 → 不改调色板
    CommonIconButton other;
    other.setIcon(icon);
    EXPECT_FALSE(other.m_lightThemeColor.isValid());
    EXPECT_FALSE(other.m_darkThemeColor.isValid());
}

// ---- setState / setStateIconMapping ----

TEST_F(CommonIconButtonTest, CommonIconButton_SetState_MappedState_UsesMapping)
{
    // Arrange: 打桩 fromTheme 捕获 + 配置 On/Off 映射
    stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                   [](const QString &name, const QIcon &fallback) {
                       Q_UNUSED(fallback)
                       requestedThemeNames.append(name);
                       return QIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
                   });
    QMap<CommonIconButton::State, QPair<QString, QString>> mapping;
    mapping.insert(CommonIconButton::On, qMakePair(QString("icon-on"), QString("fb-on")));
    mapping.insert(CommonIconButton::Off, qMakePair(QString("icon-off"), QString("fb-off")));
    button->setStateIconMapping(mapping);
    // Act: 切到 On 状态
    button->setState(CommonIconButton::On);
    // Assert: 状态保存，fromTheme 收到映射名（主题非 Light 时不变换）
    EXPECT_EQ(button->m_state, CommonIconButton::On);
    ASSERT_FALSE(requestedThemeNames.isEmpty());
    EXPECT_TRUE(requestedThemeNames.first().startsWith("icon-on"));
}

TEST_F(CommonIconButtonTest, CommonIconButton_SetState_UnmappedNullIcon_NoLookup)
{
    // Arrange: 无映射、无主图标；打桩 fromTheme 以便统计调用
    int fromThemeCalls = 0;
    stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                   [&fromThemeCalls](const QString &name, const QIcon &fallback) {
                       Q_UNUSED(name)
                       Q_UNUSED(fallback)
                       ++fromThemeCalls;
                       return QIcon();
                   });
    // Act: 未命中映射的空图标切换
    button->setState(CommonIconButton::Off);
    // Assert: 状态保存；映射未命中且 m_icon 为空 → 不发生任何 fromTheme 查询（B6）
    EXPECT_EQ(button->m_state, CommonIconButton::Off);
    EXPECT_EQ(fromThemeCalls, 0);
    EXPECT_TRUE(button->m_icon.isNull());
}

TEST_F(CommonIconButtonTest, CommonIconButton_SetState_UnmappedWithValidIcon_ReappliesIcon)
{
    // Arrange: 先设置非空主图标
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    button->setIcon(icon, Qt::black, Qt::white);
    ASSERT_FALSE(button->m_icon.isNull());
    // Act: 切到未配置映射的 Default 状态
    button->setState(CommonIconButton::Default);
    // Assert: 状态保存，主图标仍有效（B5 重设路径不丢图标）
    EXPECT_EQ(button->m_state, CommonIconButton::Default);
    EXPECT_FALSE(button->m_icon.isNull());
}

// ---- setActiveState ----

TEST_F(CommonIconButtonTest, CommonIconButton_SetActiveState_NoColors_TogglesRole)
{
    // Arrange: 无主题颜色（默认构造）
    // Act: 开启激活态
    button->setActiveState(true);
    // Assert: 激活态保存，前景角色 Highlight（B8）
    EXPECT_TRUE(button->activeState());
    EXPECT_EQ(button->foregroundRole(), QPalette::Highlight);

    // 反向：关闭激活态 → 源码置 NoRole；Qt6 QWidget::setForegroundRole(NoRole)
    // 会归一化为默认前景角色 WindowText，故以 WindowText 且非 Highlight 断言
    button->setActiveState(false);
    EXPECT_FALSE(button->activeState());
    EXPECT_EQ(button->foregroundRole(), QPalette::WindowText);
    EXPECT_NE(button->foregroundRole(), QPalette::Highlight);
}

TEST_F(CommonIconButtonTest, CommonIconButton_SetActiveState_WithColors_ReappliesPalette)
{
    // Arrange: 打桩主题为 Light，先设置双色图标
    stub.set_lamda(static_cast<DGuiApplicationHelper::ColorType (DGuiApplicationHelper::*)() const>(
                       &DGuiApplicationHelper::themeType),
                   []() {
                       return DGuiApplicationHelper::LightType;
                   });
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    button->setIcon(icon, Qt::black, Qt::white);
    // Act: 激活
    button->setActiveState(true);
    // Assert: 激活时窗口文本走 Highlight 色（B7）
    EXPECT_TRUE(button->activeState());
    EXPECT_EQ(button->palette().color(QPalette::WindowText),
              button->palette().color(QPalette::Highlight));
    // 反向：取消激活后恢复主题色
    button->setActiveState(false);
    EXPECT_EQ(button->palette().color(QPalette::WindowText), QColor(Qt::black));
}

// ---- event（hover 状态）----

TEST_F(CommonIconButtonTest, CommonIconButton_Event_EnterLeave_TogglesHover)
{
    // Arrange: 初始非 hover
    QEvent enter(QEvent::Enter);
    QEvent leave(QEvent::Leave);
    QEvent polish(QEvent::PolishRequest);
    ASSERT_FALSE(button->m_hover);
    // Act: Enter → Leave → Polish
    button->event(&enter);
    const bool afterEnter = button->m_hover;
    button->event(&leave);
    const bool afterLeave = button->m_hover;
    const bool polishHandled = button->event(&polish);
    // Assert: Enter → true；Leave → false；其它事件透传且不改变状态（B20/B21/B22）
    EXPECT_TRUE(afterEnter);
    EXPECT_FALSE(afterLeave);
    EXPECT_TRUE(polishHandled);
    EXPECT_FALSE(button->m_hover);
}

// ---- 鼠标事件 ----

TEST_F(CommonIconButtonTest, CommonIconButton_MousePress_StoresPressPos)
{
    // Arrange: 事件位置 (7,9)，初始按压点为原点
    QMouseEvent press = pressEvent(QPoint(7, 9));
    ASSERT_EQ(button->m_pressPos, QPoint(0, 0));
    // Act: 按压
    button->mousePressEvent(&press);
    // Assert: 按压点记录（B27）
    EXPECT_EQ(button->m_pressPos, QPoint(7, 9));
    EXPECT_NE(button->m_pressPos, QPoint(0, 0));
}

TEST_F(CommonIconButtonTest, CommonIconButton_MouseRelease_ClickableInside_EmitsClicked)
{
    // Arrange: 可点击；区域内按下并抬起
    button->setClickable(true);
    QSignalSpy spy(button, &CommonIconButton::clicked);
    QMouseEvent press = pressEvent(QPoint(5, 5));
    QMouseEvent release = releaseEvent(QPoint(5, 5));
    // Act: 按下 + 抬起
    button->mousePressEvent(&press);
    button->mouseReleaseEvent(&release);
    // Assert: clicked 发出一次；不可旋转 → 无定时器（B28）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(button->m_refreshTimer, nullptr);
}

TEST_F(CommonIconButtonTest, CommonIconButton_MouseRelease_NotClickable_NoEmit)
{
    // Arrange: 默认不可点击
    QSignalSpy spy(button, &CommonIconButton::clicked);
    QMouseEvent press = pressEvent(QPoint(5, 5));
    QMouseEvent release = releaseEvent(QPoint(5, 5));
    // Act: 按下 + 抬起
    button->mousePressEvent(&press);
    button->mouseReleaseEvent(&release);
    // Assert: 无信号，按压点仍被记录（B29）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(button->m_pressPos, QPoint(5, 5));
}

TEST_F(CommonIconButtonTest, CommonIconButton_MouseRelease_Outside_NoEmit)
{
    // Arrange: 可点击；区域内按下，区域外抬起
    button->setClickable(true);
    QSignalSpy spy(button, &CommonIconButton::clicked);
    QMouseEvent press = pressEvent(QPoint(5, 5));
    QMouseEvent release = releaseEvent(QPoint(200, 200));
    // Act: 按下 + 区域外抬起
    button->mousePressEvent(&press);
    button->mouseReleaseEvent(&release);
    // Assert: 区域外抬起不发射，按压点保持按下时记录的坐标（B30）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(button->m_pressPos, QPoint(5, 5));
}

TEST_F(CommonIconButtonTest, CommonIconButton_MouseRelease_DuringRotation_NoEmit)
{
    // Arrange: 可点击可旋转，先注入一次旋转使定时器运行
    button->setClickable(true);
    button->setRotatable(true);
    button->startRotate();
    ASSERT_TRUE(button->m_refreshTimer->isActive());
    QSignalSpy spy(button, &CommonIconButton::clicked);
    QMouseEvent press = pressEvent(QPoint(5, 5));
    QMouseEvent release = releaseEvent(QPoint(5, 5));
    // Act: 旋转中点击
    button->mousePressEvent(&press);
    button->mouseReleaseEvent(&release);
    // Assert: 防重复触发：无信号、角度不再累计（B31）
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(button->m_rotateAngle, 54);
    button->stopRotate();
}

TEST_F(CommonIconButtonTest, CommonIconButton_MouseRelease_Rotatable_ClickStartsRotation)
{
    // Arrange: 可点击可旋转
    button->setClickable(true);
    button->setRotatable(true);
    QSignalSpy spy(button, &CommonIconButton::clicked);
    QMouseEvent press = pressEvent(QPoint(5, 5));
    QMouseEvent release = releaseEvent(QPoint(5, 5));
    // Act: 点击
    button->mousePressEvent(&press);
    button->mouseReleaseEvent(&release);
    // Assert: 信号发出 + 定时器建立并运行 + 起始角 54（B17）
    EXPECT_EQ(spy.count(), 1);
    EXPECT_NE(button->m_refreshTimer, nullptr);
    EXPECT_TRUE(button->m_refreshTimer->isActive());
    EXPECT_EQ(button->m_refreshTimer->interval(), 50);
    EXPECT_EQ(button->m_rotateAngle, 54);
    button->stopRotate();
}

// ---- startRotate / stopRotate ----

TEST_F(CommonIconButtonTest, CommonIconButton_StartRotate_AccumulatesToFullTurn_Stops)
{
    // Arrange: 可旋转
    button->setRotatable(true);
    // Act: 旋转 3 次（54×3=162，未满一圈）
    for (int i = 0; i < 3; ++i) {
        button->startRotate();
    }
    // Assert: 中途角度累计且定时器运行（B18 未满一圈不停）
    EXPECT_EQ(button->m_rotateAngle, 162);
    ASSERT_NE(button->m_refreshTimer, nullptr);
    EXPECT_TRUE(button->m_refreshTimer->isActive());
    // Act: 再转 4 次（累计 378 >= 360 自动停）
    for (int i = 0; i < 4; ++i) {
        button->startRotate();
    }
    // Assert: 停转归零，定时器停止（B18）
    EXPECT_EQ(button->m_rotateAngle, 0);
    EXPECT_FALSE(button->m_refreshTimer->isActive());
}

TEST_F(CommonIconButtonTest, CommonIconButton_StopRotate_ResetsAngle)
{
    // Arrange: 先启动一次
    button->setRotatable(true);
    button->startRotate();
    ASSERT_EQ(button->m_rotateAngle, 54);
    // Act: 手动停转
    button->stopRotate();
    // Assert: 角度归零，定时器停止（B19）
    EXPECT_EQ(button->m_rotateAngle, 0);
    EXPECT_FALSE(button->m_refreshTimer->isActive());
}

// ---- paintEvent / refreshIcon ----

TEST_F(CommonIconButtonTest, CommonIconButton_PaintEvent_NullIcon_RendersNoCrash)
{
    // Arrange: 空图标 + hover 图标非空且 hover 态（走 hover 绘制分支）
    const QIcon hoverIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    button->setHoverIcon(hoverIcon);
    button->m_hover = true;
    QPixmap pixmap(48, 48);
    // Act: 渲染触发 paintEvent（B24）
    EXPECT_NO_THROW(button->render(&pixmap));
    // Assert: 渲染目标可用
    EXPECT_FALSE(pixmap.isNull());

    // 反向：hover 图标清空 + 主图标为空 → 不绘制不崩溃（B26）
    button->setHoverIcon(QIcon());
    button->m_hover = false;
    QPixmap pixmap2(48, 48);
    EXPECT_NO_THROW(button->render(&pixmap2));
    EXPECT_FALSE(pixmap2.isNull());
}

TEST_F(CommonIconButtonTest, CommonIconButton_PaintEvent_WithIcon_RendersAndRotation)
{
    // Arrange: 设置主图标（qrc 图标非空）
    const QIcon icon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
    button->setIcon(icon);
    ASSERT_FALSE(button->m_icon.isNull());
    // Act: 常规渲染
    QPixmap pixmap(48, 48);
    EXPECT_NO_THROW(button->render(&pixmap));
    // Act: 旋转中渲染（rotateAngle!=0 走旋转变换分支 B23）
    button->setRotatable(true);
    button->startRotate();
    ASSERT_EQ(button->m_rotateAngle, 54);
    QPixmap pixmap2(48, 48);
    EXPECT_NO_THROW(button->render(&pixmap2));
    // Assert: 两次渲染目标可用（B25）
    EXPECT_FALSE(pixmap.isNull());
    EXPECT_FALSE(pixmap2.isNull());
    button->stopRotate();
}

TEST_F(CommonIconButtonTest, CommonIconButton_RefreshIcon_ReappliesCurrentState)
{
    // Arrange: 打桩 fromTheme 记录请求；配置 Default 映射
    stub.set_lamda(static_cast<QIcon (*)(const QString &, const QIcon &)>(&QIcon::fromTheme),
                   [](const QString &name, const QIcon &fallback) {
                       Q_UNUSED(fallback)
                       requestedThemeNames.append(name);
                       return QIcon(":/icons/deepin/builtin/actions/dsm_pluginicon_light_20px.svg");
                   });
    // Act: 先设置字符串图标，再触发主题变化回调
    button->setIcon("status-icon", "status-fallback");
    requestedThemeNames.clear();
    EXPECT_NO_THROW(button->refreshIcon());
    // Assert: refreshIcon 按当前状态重查主题图标
    EXPECT_FALSE(button->m_icon.isNull());
    EXPECT_FALSE(requestedThemeNames.isEmpty());
}
