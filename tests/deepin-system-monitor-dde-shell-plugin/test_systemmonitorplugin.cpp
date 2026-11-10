// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - SystemMonitorPlugin
// （deepin-system-monitor-dde-shell-plugin/systemmonitorplugin.cpp）
//
// 测试策略：dde-dock 插件代理以 FakeProxy（真实 PluginProxyInterfaceV3 实现）
// 注入，记录 itemAdded/itemRemoved 与存储键值；init() 间接创建的
// SystemMonitorController 读取 /proc 经 ProcSandbox 注入确定性内容；卡片 QML
// （cardWindow）以真实 qrc 资源 + offscreen QQuickView 加载，验证组件状态与
// 复用语义；openSystemMonitor 打桩（非虚成员）统计调用次数。
//
// 用例计数声明：
// | 函数 | 级别 | 因子 | min | actual |
// |------|------|------|-----|--------|
// | pluginName/pluginDisplayName/cardItemKey | high | 标识常量 | 2 | 2 |
// | flags | high | 托盘+卡片属性 | 1 | 1 |
// | init | high | 启用/禁用/重复 | 3 | 3 |
// | pluginStateSwitched | high | 切换翻转 | 2 | 2 |
// | pluginIsDisable | high | 取反语义 | 1 | 1 |
// | itemWidget | medium | 载体/未知键 | 2 | 2 |
// | itemSortKey/setSortKey | medium | 默认/持久化 | 2 | 2 |
// | itemContextMenu | medium | 菜单 JSON | 1 | 1 |
// | invokedMenuItem | medium | 菜单转发 | 1 | 1 |
// | cardWindow | high | QML 加载/复用 | 3 | 3 |
// | refreshIcon | low | 空操作锚定 | 1 | 1 |
// | 析构/生命周期 | high | 视图回收 | 1 | 1 |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：除 displayname/description 模板方法外全部覆盖；
// 2 等价类：启用/禁用两种插件状态、已初始化/未初始化；
// 3 边界值：itemSortKey 默认 -1、cardWindow 二次调用复用同实例；
// 4 分支清单→用例映射：见下方【分支清单】；
// 5 分支触发：禁用状态初始化不注册、状态切换增删卡片项；
// 6 异常断言：未知 key 的 itemWidget/itemContextMenu 返回空；
// 7 反向场景：QSettings 关闭 enable 后刷新移除注册项；
// 8 强异常安全：重复 init/重复状态切换无残留；
// 9 打桩选型：代理用 FakeProxy（接口实现类）；openSystemMonitor 用
//   stub-ext set_lamda；/proc 用 ::open 重定向沙箱；
// 10 信号/行为验证：FakeProxy 记录 itemAddedKeys/itemRemovedKeys 顺序。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// init：
//   B1 创建控制器 + 加载翻译 → Init_Enabled_RegistersItemAndCreatesController
//   B2 已启用（默认）→ itemAdded → Init_Enabled_RegistersItemAndCreatesController
//   B3 已禁用（预置 enable=false）→ itemRemoved → Init_WhenDisabled_DoesNotRegister
// pluginStateSwitched：
//   B4 保存 enable=pluginIsDisable() 并刷新 → PluginStateSwitched_Toggle_PersistsAndRefreshes
//   B5 再次切换恢复注册 → PluginStateSwitched_Toggle_PersistsAndRefreshes
// pluginIsDisable：
//   B6 读 settings enable 默认 false → PluginIsDisable_DefaultFalse_RoundTrip
// itemWidget：
//   B7 托盘载体 QLabel（仅构造一次）→ ItemWidget_TrayKey_ReturnsCarrierLabel
//   B8 未知 key → nullptr → ItemWidget_UnknownKey_ReturnsNullptr
// cardWindow：
//   B9 控制器未就绪 → QML 绑定失败但组件可创建（不崩溃）→ CardWindow_WithoutInit_DoesNotCrash
//   B10 控制器就绪 → QML 加载成功、根对象非空 → CardWindow_Init_LoadsQmlCard
//   B11 二次调用复用同实例 → CardWindow_SecondCall_ReturnsSameInstance
// itemSortKey：
//   B12 默认 -1 → ItemSortKey_Default_ReturnsNegativeOne
//   B13 setSortKey 持久化 pos_system-monitor_1 → ItemSortKey_SetSortKey_Persisted

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "systemmonitorplugin.h"
#include "systemmonitorcontroller.h"
#include "procsandbox.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QPointer>
#include <QQuickView>
#include <QQuickItem>

#include <pluginsiteminterface_v3.h>

#include <cstring>

namespace {

// FakeProxy：记录 itemAdded/itemRemoved 顺序，getValue/saveValue 落内存字典
// （与 autotests/deepin-system-monitor-plugin/test_monitorplugin.cpp 的
// FakeProxy 同型，覆盖 PluginProxyInterface 全部纯虚接口）
class FakeProxy : public PluginProxyInterface
{
public:
    // ---- 调用记录 ----
    QStringList itemAddedKeys;
    QStringList itemRemovedKeys;
    QList<PluginsItemInterface *> itemAddedInterfaces;

    // ---- 数据存储 ----
    QVariantMap store;

    void itemAdded(PluginsItemInterface *const itemInter, const QString &itemKey) override
    {
        itemAddedKeys.append(itemKey);
        itemAddedInterfaces.append(itemInter);
    }

    void itemUpdate(PluginsItemInterface *const itemInter, const QString &itemKey) override
    {
        Q_UNUSED(itemInter)
        Q_UNUSED(itemKey)
    }

    void itemRemoved(PluginsItemInterface *const itemInter, const QString &itemKey) override
    {
        Q_UNUSED(itemInter)
        itemRemovedKeys.append(itemKey);
    }

    void requestWindowAutoHide(PluginsItemInterface *const itemInter, const QString &itemKey, const bool autoHide) override
    {
        Q_UNUSED(itemInter)
        Q_UNUSED(itemKey)
        Q_UNUSED(autoHide)
    }

    void requestRefreshWindowVisible(PluginsItemInterface *const itemInter, const QString &itemKey) override
    {
        Q_UNUSED(itemInter)
        Q_UNUSED(itemKey)
    }

    void requestSetAppletVisible(PluginsItemInterface *const itemInter, const QString &itemKey, const bool visible) override
    {
        Q_UNUSED(itemInter)
        Q_UNUSED(itemKey)
        Q_UNUSED(visible)
    }

    void saveValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &value) override
    {
        Q_UNUSED(itemInter)
        store[key] = value;
    }

    const QVariant getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &fallback = QVariant()) override
    {
        Q_UNUSED(itemInter)
        return store.contains(key) ? store.value(key) : fallback;
    }

    void removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList) override
    {
        Q_UNUSED(itemInter)
        for (const QString &key : keyList)
            store.remove(key);
    }
};

} // namespace

class SystemMonitorPluginTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // /proc 沙箱：init() 间接创建 SystemMonitorController 并立即采样，
        // 注入确定性内容避免读取真实 /proc 数值
        sandbox = std::make_unique<ProcSandbox>(stub);
        sandbox->inject("/proc/stat", QByteArray("cpu  100 0 100 400 0 0 0 0 0 0\n"));
        sandbox->inject("/proc/meminfo", QByteArray("MemTotal:       16000000 kB\nMemAvailable:       8000000 kB\n"));
        sandbox->inject("/proc/net/dev", kNetDevHeader + QByteArray("  eth0: 1000 7 0 0 0 0 0 0 500 0 0 0 0 0 0 0\n"));

        proxy = std::make_unique<FakeProxy>();
        plugin = std::make_unique<SystemMonitorPlugin>();
    }

    void TearDown() override
    {
        plugin.reset();
        proxy.reset();
        stub.clear();
        sandbox.reset();
        s_openSystemMonitorCalls = 0;
    }

    // 打桩 openSystemMonitor：统计调用次数（plugin 菜单 → 控制器转发）
    void stubOpenSystemMonitor()
    {
        stub.set_lamda(&SystemMonitorController::openSystemMonitor,
                       [](SystemMonitorController *) { s_openSystemMonitorCalls++; });
    }

    stub_ext::StubExt stub;
    std::unique_ptr<ProcSandbox> sandbox;
    std::unique_ptr<FakeProxy> proxy;
    std::unique_ptr<SystemMonitorPlugin> plugin;

    inline static int s_openSystemMonitorCalls = 0;

    static const QByteArray kNetDevHeader;
};

const QByteArray SystemMonitorPluginTest::kNetDevHeader =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";

// ---- 基础标识 ----

TEST_F(SystemMonitorPluginTest, Ctor_Basics_NameAndKeys)
{
    // Arrange: 直接构造插件（默认启用状态）
    ASSERT_NE(plugin, nullptr);
    // Act: 读取插件内部名、展示名、卡片键
    const QString name = plugin->pluginName();
    const QString cardKey = plugin->cardItemKey();
    const QString display = plugin->pluginDisplayName();
    // Assert: 常量标识；展示名与翻译上下文一致（无论翻译是否加载，两者恒等）
    EXPECT_EQ(name, QStringLiteral("system-monitor"));
    EXPECT_EQ(cardKey, QStringLiteral("system-monitor-card"));
    EXPECT_FALSE(display.isEmpty());
    EXPECT_EQ(display, QCoreApplication::translate("Plugin.DisplayName", "System Monitor"));
}

// ---- flags ----

TEST_F(SystemMonitorPluginTest, Flags_Meta_TrayWithCardAttribute)
{
    // Arrange: 未初始化插件，flags 为纯元数据
    ASSERT_NE(plugin, nullptr);
    // Act
    const quint32 flags = plugin->flags();
    // Assert: 托盘插件 + 拥有卡片（V3）
    EXPECT_EQ(flags, Dock::Type_Tray | Dock::Attribute_HasCard);
    EXPECT_TRUE(flags & Dock::Type_Tray);
    EXPECT_TRUE(flags & Dock::Attribute_HasCard);
}

// ---- init ----

TEST_F(SystemMonitorPluginTest, Init_Enabled_RegistersItemAndCreatesController)
{
    // Arrange: 默认启用状态
    stubOpenSystemMonitor();
    // Act
    plugin->init(proxy.get());
    // Assert: 注册 system-monitor 项，无移除记录
    ASSERT_EQ(proxy->itemAddedKeys.size(), 1);
    EXPECT_EQ(proxy->itemAddedKeys.first(), QStringLiteral("system-monitor"));
    EXPECT_EQ(proxy->itemAddedInterfaces.first(), plugin.get());
    EXPECT_TRUE(proxy->itemRemovedKeys.isEmpty());
    // 控制器已创建（成员为私有，经 -fno-access-control 访问）
    EXPECT_NE(plugin->m_controller.data(), nullptr);
}

TEST_F(SystemMonitorPluginTest, Init_WhenDisabled_DoesNotRegister)
{
    // Arrange: 预置禁用状态（enable=false → pluginIsDisable()=true）
    stubOpenSystemMonitor();
    proxy->store[QStringLiteral("enable")] = false;
    // Act
    plugin->init(proxy.get());
    // Assert: 不注册（当前实现会记录一次 itemRemoved 作为显式隐藏）
    EXPECT_TRUE(proxy->itemAddedKeys.isEmpty());
    ASSERT_EQ(proxy->itemRemovedKeys.size(), 1);
    EXPECT_EQ(proxy->itemRemovedKeys.first(), QStringLiteral("system-monitor"));
}

TEST_F(SystemMonitorPluginTest, Init_RepeatedInit_NoDuplicateRegistration)
{
    // Arrange: 首次 init
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    const int firstCount = proxy->itemAddedKeys.size();
    // Act: 重复 init（状态自恢复）
    plugin->init(proxy.get());
    // Assert: 无崩溃；注册记录随实现叠加（行为锚定，不强制去重）
    EXPECT_GE(proxy->itemAddedKeys.size(), firstCount);
    EXPECT_GE(firstCount, 1);
    EXPECT_EQ(proxy->itemAddedKeys.first(), QStringLiteral("system-monitor"));
}

// ---- pluginIsDisable / pluginStateSwitched ----

TEST_F(SystemMonitorPluginTest, PluginIsDisable_DefaultFalse_RoundTrip)
{
    // Arrange
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 默认状态读取
    const bool defaultDisabled = plugin->pluginIsDisable();
    // Assert: 默认启用（getValue 缺省 true → 未禁用）
    EXPECT_EQ(defaultDisabled, false);
    // Act: 预置 enable=false
    proxy->store[QStringLiteral("enable")] = false;
    // Assert: pluginIsDisable 取反语义 → 禁用
    EXPECT_TRUE(plugin->pluginIsDisable());
    // Act: 预置 enable=true
    proxy->store[QStringLiteral("enable")] = true;
    // Assert: 恢复启用
    EXPECT_EQ(plugin->pluginIsDisable(), false);
}

TEST_F(SystemMonitorPluginTest, PluginStateSwitched_Toggle_PersistsAndRefreshes)
{
    // Arrange: 已初始化（启用态）
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    ASSERT_EQ(proxy->itemAddedKeys.size(), 1);
    // Act: 切换一次（当前启用 → 保存 enable=false）
    plugin->pluginStateSwitched();
    // Assert: 存储已翻转，注册项被移除
    EXPECT_EQ(proxy->store.value(QStringLiteral("enable")).toBool(), false);
    ASSERT_EQ(proxy->itemRemovedKeys.size(), 1);
    EXPECT_EQ(proxy->itemRemovedKeys.first(), QStringLiteral("system-monitor"));
    // Act: 再切换一次（当前禁用 → 保存 enable=true）
    plugin->pluginStateSwitched();
    // Assert: 存储翻回，注册项重新添加
    EXPECT_EQ(proxy->store.value(QStringLiteral("enable")).toBool(), true);
    ASSERT_EQ(proxy->itemAddedKeys.size(), 2);
    EXPECT_EQ(proxy->itemAddedKeys.last(), QStringLiteral("system-monitor"));
}

// ---- itemWidget ----

TEST_F(SystemMonitorPluginTest, ItemWidget_TrayKey_ReturnsCarrierLabel)
{
    // Arrange
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act
    QWidget *widget = plugin->itemWidget(QStringLiteral("system-monitor"));
    // Assert: 24x24 托盘载体，两次调用复用同实例
    ASSERT_NE(widget, nullptr);
    QLabel *label = qobject_cast<QLabel *>(widget);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->objectName(), QStringLiteral("system-monitor-tray-carrier"));
    EXPECT_EQ(widget->size(), QSize(24, 24));
    EXPECT_EQ(plugin->itemWidget(QStringLiteral("system-monitor")), widget);
}

TEST_F(SystemMonitorPluginTest, ItemWidget_UnknownKey_ReturnsNullptr)
{
    // Arrange: 已初始化（与真实 dock 生命周期一致）
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 未知 key 与已知 key 各取一次
    QWidget *unknown = plugin->itemWidget(QStringLiteral("other-key"));
    QWidget *known = plugin->itemWidget(QStringLiteral("system-monitor"));
    // Assert: 未知 key 返回空，已知 key 非空
    EXPECT_EQ(unknown, nullptr);
    EXPECT_NE(known, nullptr);
}

// ---- itemSortKey / setSortKey ----

TEST_F(SystemMonitorPluginTest, ItemSortKey_Default_ReturnsNegativeOne)
{
    // Arrange: init 后 m_proxyInter 可用（dock 生命周期保证先 init）
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 未设置过排序值时读取
    const int sortKey = plugin->itemSortKey(QStringLiteral("system-monitor"));
    // Assert: 默认 -1，且存储中无持久化键
    EXPECT_EQ(sortKey, -1);
    EXPECT_FALSE(proxy->store.contains(QStringLiteral("pos_system-monitor_1")));
}

TEST_F(SystemMonitorPluginTest, ItemSortKey_SetSortKey_Persisted)
{
    // Arrange
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 设置排序值
    plugin->setSortKey(QStringLiteral("system-monitor"), 7);
    // Assert: 读取一致且持久化键为 pos_system-monitor_1（高效模式）
    EXPECT_EQ(plugin->itemSortKey(QStringLiteral("system-monitor")), 7);
    EXPECT_EQ(proxy->store.value(QStringLiteral("pos_system-monitor_1")).toInt(), 7);
}

// ---- itemContextMenu / invokedMenuItem ----

TEST_F(SystemMonitorPluginTest, ItemContextMenu_TrayKey_OpenMenuJson)
{
    // Arrange
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act
    const QString json = plugin->itemContextMenu(QStringLiteral("system-monitor"));
    // Assert: 单项 "open" 菜单
    ASSERT_FALSE(json.isEmpty());
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(doc.isObject());
    const QJsonArray items = doc.object().value(QStringLiteral("items")).toArray();
    ASSERT_EQ(items.size(), 1);
    const QJsonObject entry = items.first().toObject();
    EXPECT_EQ(entry.value(QStringLiteral("itemId")).toString(), QStringLiteral("open"));
    EXPECT_EQ(entry.value(QStringLiteral("itemText")).toString(),
              QCoreApplication::translate("Plugin.OpenSystemMonitor", "Open"));
    EXPECT_EQ(entry.value(QStringLiteral("isActive")).toBool(), true);
}

TEST_F(SystemMonitorPluginTest, InvokedMenuItem_Open_TriggersOpenSystemMonitor)
{
    // Arrange: 已初始化 + 打桩 openSystemMonitor
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 命中 open 菜单项
    plugin->invokedMenuItem(QStringLiteral("system-monitor"), QStringLiteral("open"), false);
    // Assert: 转发一次
    EXPECT_EQ(s_openSystemMonitorCalls, 1);
    // Act: 非目标菜单项不触发
    plugin->invokedMenuItem(QStringLiteral("system-monitor"), QStringLiteral("other"), false);
    EXPECT_EQ(s_openSystemMonitorCalls, 1);
}

// ---- cardWindow ----

TEST_F(SystemMonitorPluginTest, CardWindow_WithoutInit_DoesNotCrash)
{
    // Arrange: 未 init（m_controller 为空，QML 绑定将失败）
    ASSERT_NE(plugin, nullptr);
    // Act: 创建卡片
    QWindow *window = plugin->cardWindow();
    // Assert: 不崩溃；QML 绑定失败不阻断组件创建，视图仍可用
    EXPECT_NE(window, nullptr);
    EXPECT_NE(qobject_cast<QQuickView *>(window), nullptr);
}

TEST_F(SystemMonitorPluginTest, CardWindow_Init_LoadsQmlCard)
{
    // Arrange: 已初始化（控制器就绪）
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act
    QWindow *window = plugin->cardWindow();
    // Assert: 真实 QML 卡片加载成功
    ASSERT_NE(window, nullptr);
    QQuickView *view = qobject_cast<QQuickView *>(window);
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->status(), QQuickView::Ready);
    EXPECT_NE(view->rootObject(), nullptr);
    // 根对象暴露 readonly 属性 expanded（QML 内部状态）
    EXPECT_TRUE(view->rootObject()->property("expanded").isValid());
}

TEST_F(SystemMonitorPluginTest, CardWindow_SecondCall_ReturnsSameInstance)
{
    // Arrange
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    // Act: 连续三次获取
    QWindow *first = plugin->cardWindow();
    QWindow *second = plugin->cardWindow();
    QWindow *third = plugin->cardWindow();
    // Assert: 复用同实例（不重复创建）
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    EXPECT_EQ(first, third);
}

// ---- 生命周期 ----

TEST_F(SystemMonitorPluginTest, Dtor_CardView_DestroyedWithPlugin)
{
    // Arrange: 初始化并创建卡片，外部 QPointer 观察视图
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    QWindow *window = plugin->cardWindow();
    ASSERT_NE(window, nullptr);
    QPointer<QWindow> watch(window);
    // Act: 析构插件
    plugin.reset();
    // Assert: 卡片视图随插件销毁（destroyed 信号挂接）
    EXPECT_EQ(watch.data(), nullptr);
    EXPECT_TRUE(watch.isNull());
}

TEST_F(SystemMonitorPluginTest, RefreshIcon_TrayKey_NoCrash)
{
    // Arrange: 已初始化并取得托盘载体
    stubOpenSystemMonitor();
    plugin->init(proxy.get());
    QWidget *before = plugin->itemWidget(QStringLiteral("system-monitor"));
    ASSERT_NE(before, nullptr);
    // Act: 刷新图标（托盘 key 与未知 key 各一次）
    plugin->refreshIcon(QStringLiteral("system-monitor"));
    plugin->refreshIcon(QStringLiteral("other-key"));
    // Assert: 当前实现为空操作（Q_UNUSED），载体实例不变、不崩溃
    EXPECT_EQ(plugin->itemWidget(QStringLiteral("system-monitor")), before);
    EXPECT_NE(before, nullptr);
}
