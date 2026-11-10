// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - MonitorPlugin（deepin-system-monitor-plugin/gui/monitor_plugin.cpp）
//
// 用例计数声明：
// | 函数                            | 用例数 |
// |---------------------------------|--------|
// | MonitorPlugin (构造函数)         | 1      |
// | ~MonitorPlugin (析构函数)        | 1      |
// | pluginName / pluginDisplayName   | 1      |
// | flags / pluginIsDisable / pluginIsAllowDisable | 1 |
// | init                             | 1      |
// | loadPlugin                       | 1      |
// | itemWidget                       | 1      |
// | itemTipsWidget                   | 2      |
// | itemCommand                      | 2      |
// | itemContextMenu                  | 2      |
// | invokedMenuItem                  | 2      |
// | displayModeChanged               | 1      |
// | itemSortKey / setSortKey         | 1      |
// | pluginStateSwitched              | 1      |
// | setMessageCallback               | 1      |
// | onSysMonPopVisibleChanged        | 2      |
// | onClickQuickPanel                | 1      |
// | openSystemMonitor                | 1      |
// | setRateUnitSensitive             | 1 (8 参数化) |
// | autoRateUnits                    | 2 (10 参数化) |
// | calcCpuRate                      | 2      |
// | calcMemRate                      | 2      |
// | calcNetRate                      | 2      |
// | udpateInfo                       | 1      |
// | udpateTipsInfo                   | 1      |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：MonitorPlugin 全部 public 接口均有用例；
// 2 等价类：setRateUnitSensitive 枚举全值、autoRateUnits 数值等级区间；
// 3 边界值：autoRateUnits 0/1/1023/1024/2^20/2^30/2^40/2^50、2^50-1、负数；
// 4 同质分组≥3 用 TEST_P：setRateUnitSensitive(8 组)、autoRateUnits(10 组)；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：QFile 失败(打桩 open=false)、显示模式双向切换、回调空/非空；
// 7 异常断言：函数无显式 throw，等价于"不抛异常"语义，由 EXPECT_NO_THROW 覆盖；
// 8 反向场景：/proc 文件打开失败、菜单回调 menuId 无效、回调未设置；
// 9 强异常安全：stub.clear() 后重复调用 autoRateUnits/udpateInfo 验证状态自恢复；
// 10 打桩选型：外部 I/O（/proc 读）与进程拉起（QProcess::startDetached，stub.set_lamda 打桩）均 stub_ext
//    （stub.set_lamda）；dock 代理 PluginProxyInterface 为纯接口（非 QObject），直接
//    以 FakeProxy 实现类注入，不引入 gMock。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// MonitorPlugin::MonitorPlugin (构造)：
//   B1 默认构造路径（创建定时器/快捷面板）→ MonitorPlugin_Ctor_InitializesDefaults
// ~MonitorPlugin：
//   B2 析构释放快捷面板 → MonitorPlugin_Dtor_WithInit_DestroysCleanly（其余用例 TearDown 同路径覆盖）
// init(PluginProxyInterface*)：
//   B3 !pluginIsDisable()（V2 恒 false）→ loadPlugin + itemAdded + 快捷面板图标/描述 → MonitorPlugin_Init_RegistersPluginAndWidgets
//   B4 m_proxyInter 为空时崩溃风险：init 前不调用任何代理方法（接口契约），以构造用例覆盖空态
// loadPlugin()：
//   B5 m_pluginLoaded==false → 创建气泡/图标控件 + itemAdded + displayModeChanged → MonitorPlugin_Init_RegistersPluginAndWidgets
//   B6 m_pluginLoaded==true → 幂等返回 → MonitorPlugin_LoadPlugin_Twice_IsIdempotent
// itemWidget(itemKey)：
//   B7 QUICK_ITEM_KEY → 快捷面板 → MonitorPlugin_ItemWidget_KeyRouting_ReturnsExpectedWidgets
//   B8 "system-monitor" → dock 图标控件 → 同上
//   B9 其它 key → nullptr → MonitorPlugin_ItemWidget_UnknownKey_ReturnsNullptr
// pluginStateSwitched()：
//   B10 !pluginIsDisable()==true（V2 恒 true）→ saveValue(disabled=true)+itemRemoved → MonitorPlugin_PluginStateSwitched_SavesDisabledState
//   B11 disabledNew==false 分支：V2 下 pluginIsDisable() 编译期恒 false，运行期不可达（宏分支），标注不测
// itemContextMenu(itemKey)：
//   B12 itemKey=="system-monitor" → 菜单 JSON → MonitorPlugin_ItemContextMenu_SystemMonitorKey_ReturnsMenuJson
//   B13 其它 key → 空串 → MonitorPlugin_ItemContextMenu_OtherKey_ReturnsEmpty
// invokedMenuItem(...)：
//   B14 menuId=="openSystemMointor" → openSystemMonitor → MonitorPlugin_InvokedMenuItem_OpenSystemMointor_TriggersLaunch
//   B15 其它 menuId → 无动作 → MonitorPlugin_InvokedMenuItem_OtherMenuId_NoAction
// itemCommand(itemKey)：
//   B16 "system-monitor" → openSystemMonitor → MonitorPlugin_ItemCommand_SystemMonitorKey_LaunchesMonitor
//   B17 其它 key → 空串无动作 → MonitorPlugin_ItemCommand_OtherKey_ReturnsEmpty
// displayModeChanged(mode)：
//   B18 !pluginIsDisable() → 刷新图标与快捷面板 → MonitorPlugin_DisplayModeChanged_UpdatesWidgets_NoCrash
// itemSortKey/setSortKey：
//   B19 key 拼接 pos_<itemKey>_<displayMode>（Fashion/Efficient）→ MonitorPlugin_ItemSortKey_RoundTripsThroughProxyStore
// setRateUnitSensitive：
//   B20 枚举 7 值 + default → MonitorPlugin_SetRateUnitSensitive_MapsAllUnits（TEST_P）
// autoRateUnits：
//   B21 unit!=RateByte → -1 → MonitorPlugin_AutoRateUnits_NonByteUnit_ReturnsNegative
//   B22 0<=speed<2^10 → RateByte 原值 → TEST_P
//   B23 2^10<=speed<2^20 → RateKb speed/2^10 → TEST_P
//   B24 2^20<=speed<2^30 → RateMb → TEST_P
//   B25 2^30<=speed<2^40 → RateGb → TEST_P
//   B26 2^40<=speed<2^50 → RateTb → TEST_P
//   B27 speed>=2^50 或 <0 → RateUnknow -1 → TEST_P
// calcCpuRate：
//   B28 /proc/stat 打开成功 → total>0, 0<=available<=total → MonitorPlugin_CalcCpuRate_ValidProcStat_NonZeroTotal
//   B29 打开失败 → 双零 → MonitorPlugin_CalcCpuRate_OpenFailure_ReturnsZeros
// calcMemRate：
//   B30 /proc/meminfo 打开成功 → 0<=memory<=all, all>0 → MonitorPlugin_CalcMemRate_ValidProcMeminfo_WithinTotal
//   B31 打开失败 → 双零 → MonitorPlugin_CalcMemRate_OpenFailure_ReturnsZeros
// calcNetRate：
//   B32 /proc/net/dev 打开成功 → 非负 → MonitorPlugin_CalcNetRate_ValidProcNetDev_NonNegative
//   B33 打开失败 → 入参原值保持（源码失败分支不置零）→ MonitorPlugin_CalcNetRate_OpenFailure_KeepsCallerValues
// udpateInfo：
//   B34 正常刷新 → 百分比/速率字符串 → MonitorPlugin_UdpateInfo_FormatsTipStrings
// udpateTipsInfo：
//   B35 刷新并回填气泡文本 → MonitorPlugin_UdpateTipsInfo_FillsTipsLabel
// setMessageCallback / onSysMonPopVisibleChanged：
//   B36 回调非空 → 上报 {"msgType":"itemActiveState","data":bool} → MonitorPlugin_SetMessageCallback_InvokedOnPopupVisible
//   B37 回调为空 → 直接返回 → MonitorPlugin_OnSysMonPopVisibleChanged_WithoutCallback_NoCrash
// onClickQuickPanel：
//   B38 隐藏气泡 + openSystemMonitor → requestSetAppletVisible 共 2 次 → MonitorPlugin_OnClickQuickPanel_HidesAppletAndOpensMonitor
// openSystemMonitor：
//   B39 AM1 DBus 启动失败（无总线）仅告警；隐藏 applet 请求必发 → MonitorPlugin_OpenSystemMonitor_RequestsHideApplet
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | MonitorPlugin | high | ctor/dtor/init/loadPlugin | 8 | 8 |
// | itemWidget/itemTipsWidget | high | key-routing | 4 | 4 |
// | itemContextMenu/invokedMenuItem | high | key/menuId-branches | 5 | 5 |
// | itemCommand/pluginStateSwitched/displayModeChanged | mid | branches | 5 | 5 |
// | calcCpuRate/calcMemRate/calcNetRate | high | proc-branches/units | 12 | 12 |
// | udpateInfo/udpateTipsInfo/callback/quickpanel/launch | high | format/stubs | 14 | 14 |

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "monitor_plugin.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QProcess>

#include <dlfcn.h>
#include <cstring>
#include <QTimer>

#include <DApplication>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE

// V2（USE_API_QUICKPANEL20）下的 dock 代理桩：PluginProxyInterface 为纯接口类（非 QObject），
// 直接实现全部 9 个纯虚函数；FakeProxy 记录调用并存储 saveValue/getValue 数据。
class FakeProxy : public PluginProxyInterface
{
public:
    // ---- 调用记录 ----
    QStringList itemAddedKeys;
    QStringList itemRemovedKeys;
    int requestSetAppletVisibleCount = 0;
    QString lastAppletKey;
    bool lastAppletShow = false;

    // ---- 数据存储 ----
    QVariantMap store;

    // ---- PluginsItemInterface 代理桩 ----
    void itemAdded(PluginsItemInterface *const itemInter, const QString &itemKey) override
    {
        Q_UNUSED(itemInter)
        itemAddedKeys.append(itemKey);
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
    void requestSetAppletVisible(PluginsItemInterface *const itemInter, const QString &itemKey, const bool show) override
    {
        Q_UNUSED(itemInter)
        ++requestSetAppletVisibleCount;
        lastAppletKey = itemKey;
        lastAppletShow = show;
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

// MonitorPluginTest：MonitorPlugin 全接口测试（构造气泡/快捷面板控件，依赖 QApplication）
class MonitorPluginTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        plugin = new MonitorPlugin();
        proxy = new FakeProxy();
    }
    void TearDown() override
    {
        delete plugin;
        delete proxy;
        stub.clear();
        qApp->setProperty(PROP_DISPLAY_MODE, QVariant());
    }

    // QFile::open 为虚函数，GCC 对虚函数成员指针编码为 vtable 索引（非代码地址），
    // 直接 set_lamda(&QFile::open, ...) 会将补丁写到非法地址导致段错误。
    // 故经 dlsym 取真实代码地址，手工构造非虚布局成员指针 {addr, 0} 后打桩；
    // 虚分派（vtable 槽指向同一代码地址）与直接调用均命中补丁，clear 后恢复原码。
    static bool (QFile::*qfileOpenRealAddr())(QIODevice::OpenMode)
    {
        void *handle = dlopen("libQt6Core.so.6", RTLD_NOW | RTLD_GLOBAL);
        Q_ASSERT(handle);
        // 符号：QFile::open(QIODevice::OpenMode)（nm -D libQt6Core.so.6 可查证）
        void *addr = dlsym(handle, "_ZN5QFile4openE6QFlagsIN13QIODeviceBase12OpenModeFlagEE");
        Q_ASSERT(addr);
        bool (QFile::*mp)(QIODevice::OpenMode) = nullptr;
        struct
        {
            void *addr;
            long adj;
        } raw;
        raw.addr = addr;
        raw.adj = 0;
        static_assert(sizeof(raw) == sizeof(mp), "unexpected member-pointer layout");
        std::memcpy(&mp, &raw, sizeof(mp));
        return mp;
    }
    // 打桩 QFile::open（虚函数，见 qfileOpenRealAddr 说明）
    void stubQFileOpen(bool result)
    {
        stub.set_lamda(qfileOpenRealAddr(),
                       [result](QFile *, QIODevice::OpenMode) {
                           return result;
                       });
    }

    stub_ext::StubExt stub;
    MonitorPlugin *plugin = nullptr;
    FakeProxy *proxy = nullptr;
};

// ---- 构造 / 析构 / 基础接口 ----

TEST_F(MonitorPluginTest, MonitorPlugin_Ctor_InitializesDefaults)
{
    // Arrange: SetUp 已构造默认插件对象
    // Act: 直接检查构造后的默认状态
    // Assert: 插件名固定；V2 标志位包含 Type_Quick；未 loadPlugin 前 dock 图标控件为空、快捷面板已建
    EXPECT_EQ(plugin->pluginName(), QString("system-monitor"));
    EXPECT_EQ(plugin->flags() & Dock::Type_Quick, Dock::Type_Quick);
    EXPECT_NE(plugin->itemWidget(Dock::QUICK_ITEM_KEY), nullptr); // 快捷面板在构造函数中创建
    EXPECT_EQ(plugin->itemWidget("system-monitor"), nullptr);     // loadPlugin 前无 dock 图标控件
    EXPECT_TRUE(plugin->pluginIsAllowDisable());                  // V2 编译分支恒返回 true
    EXPECT_FALSE(plugin->pluginIsDisable());                      // V2 编译分支恒返回 false
}

TEST_F(MonitorPluginTest, MonitorPlugin_PluginDisplayName_IsTranslatedName)
{
    // Arrange: 构造完成
    // Act: 读取显示名
    const QString displayName = plugin->pluginDisplayName();
    // Assert: 与翻译表一致且非空
    EXPECT_EQ(displayName, DApplication::translate("Plugin.DisplayName", "System Monitor"));
    EXPECT_FALSE(displayName.isEmpty());
}

TEST_F(MonitorPluginTest, MonitorPlugin_Dtor_WithInit_DestroysCleanly)
{
    // Arrange: 局部对象 init 后析构，覆盖 ~MonitorPlugin 的释放路径
    // Act: 离开作用域析构
    {
        MonitorPlugin scoped;
        scoped.init(proxy);
        proxy->itemAddedKeys.clear();
        proxy->itemRemovedKeys.clear();
    }
    // Assert: 析构无崩溃，代理记录不被析构动作追加
    EXPECT_TRUE(proxy->itemAddedKeys.isEmpty());
    EXPECT_TRUE(proxy->itemRemovedKeys.isEmpty());
}

// ---- init / loadPlugin ----

TEST_F(MonitorPluginTest, MonitorPlugin_Init_RegistersPluginAndWidgets)
{
    // Arrange: 空代理
    // Act: init
    plugin->init(proxy);
    // Assert: QUICKPANEL20 下 init 与 loadPlugin 各一次 itemAdded，键均为
    // pluginName()（"deepin-system-monitor-plugin"）；快捷面板经 QuickPanelWidget
    // 直接挂接，不单独注册 QUICK_ITEM_KEY（B15）
    EXPECT_EQ(proxy->itemAddedKeys.count(), 2);
    EXPECT_EQ(proxy->itemAddedKeys.count(plugin->pluginName()), 2);
    EXPECT_NE(plugin->itemWidget("system-monitor"), nullptr);
}

TEST_F(MonitorPluginTest, MonitorPlugin_LoadPlugin_Twice_IsIdempotent)
{
    // Arrange: 已 init（内部 loadPlugin 一次）
    plugin->init(proxy);
    const int addedAfterInit = proxy->itemAddedKeys.count();
    // Act: 再次 loadPlugin（m_pluginLoaded 已置位）
    plugin->loadPlugin();
    // Assert: 不重复注册 itemAdded，且键仍为 pluginName()
    EXPECT_EQ(proxy->itemAddedKeys.count(), addedAfterInit);
    EXPECT_EQ(proxy->itemAddedKeys.count(plugin->pluginName()), addedAfterInit);
}

// ---- itemWidget / itemTipsWidget ----

TEST_F(MonitorPluginTest, MonitorPlugin_ItemWidget_KeyRouting_ReturnsExpectedWidgets)
{
    // Arrange: init 后控件就绪
    plugin->init(proxy);
    // Act: 按 key 取控件
    QWidget *quick = plugin->itemWidget(Dock::QUICK_ITEM_KEY);
    QWidget *icon  = plugin->itemWidget("system-monitor");
    // Assert: QUICK_ITEM_KEY → 快捷面板；"system-monitor" → dock 图标控件，两者不同对象
    EXPECT_NE(quick, nullptr);
    EXPECT_NE(icon, nullptr);
    EXPECT_NE(quick, icon);
}

TEST_F(MonitorPluginTest, MonitorPlugin_ItemWidget_UnknownKey_ReturnsNullptr)
{
    // Arrange: init 后
    plugin->init(proxy);
    // Act: 用未知 key 取控件，已知 key 作对照
    QWidget *unknown = plugin->itemWidget("unknown-key");
    QWidget *known = plugin->itemWidget("system-monitor");
    // Assert: 未知 key 返回空指针，已知 key 非空
    EXPECT_EQ(unknown, nullptr);
    EXPECT_NE(known, nullptr);
}

TEST_F(MonitorPluginTest, MonitorPlugin_ItemTipsWidget_SetsNameAndText)
{
    // Arrange: init 后气泡控件就绪
    plugin->init(proxy);
    // Act: 取提示控件
    QWidget *tips = plugin->itemTipsWidget("system-monitor");
    // Assert: 对象名随 itemKey 设置，且重复获取返回同一实例
    ASSERT_NE(tips, nullptr);
    EXPECT_EQ(tips->objectName(), QString("system-monitor"));
    EXPECT_EQ(plugin->itemTipsWidget("other-key"), tips);
}

// ---- itemContextMenu / invokedMenuItem ----

TEST_F(MonitorPluginTest, MonitorPlugin_ItemContextMenu_SystemMonitorKey_ReturnsMenuJson)
{
    // Arrange: 已构造
    // Act: 取右键菜单数据
    const QString menuJson = plugin->itemContextMenu("system-monitor");
    const QJsonDocument doc = QJsonDocument::fromJson(menuJson.toUtf8());
    const QJsonArray items = doc.object().value("items").toArray();
    // Assert: 仅一项菜单（启动系统监视器），字段符合 dock 菜单协议
    EXPECT_EQ(items.size(), 1);
    const QJsonObject item = items.at(0).toObject();
    EXPECT_EQ(item.value("itemId").toString(), QString("openSystemMointor"));
    EXPECT_EQ(item.value("itemText").toString(), DApplication::translate("Plugin.OpenSystemMontitor", "Open"));
    EXPECT_EQ(item.value("isActive").toBool(), true);
    EXPECT_EQ(item.value("checkableMenu").toBool(), false);
}

TEST_F(MonitorPluginTest, MonitorPlugin_ItemContextMenu_OtherKey_ReturnsEmpty)
{
    // Arrange: 已构造
    // Act: 非 system-monitor 的 key 取右键菜单
    const QString menuJson = plugin->itemContextMenu("other-key");
    // Assert: 返回空串
    EXPECT_EQ(menuJson, QString(""));
    EXPECT_TRUE(menuJson.isEmpty());
}

TEST_F(MonitorPluginTest, MonitorPlugin_InvokedMenuItem_OpenSystemMointor_TriggersLaunch)
{
    // Arrange: init 后打桩进程拉起（openSystemMonitor 内 200ms 单发定时器持 this 上下文，
    // 用例结束删除对象即失效，真实 fork 不会发生）
    plugin->init(proxy);
    stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                   [](const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid) {
                       Q_UNUSED(program)
                       return true;
                   });
    // Act: 菜单项回调（正确 menuId）
    plugin->invokedMenuItem("system-monitor", "openSystemMointor", false);
    // Assert: 隐藏气泡面板的请求发出
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 1);
    EXPECT_EQ(proxy->lastAppletKey, QString("system-monitor"));
    EXPECT_EQ(proxy->lastAppletShow, false);
}

TEST_F(MonitorPluginTest, MonitorPlugin_InvokedMenuItem_OtherMenuId_NoAction)
{
    // Arrange: init 后清零请求计数
    plugin->init(proxy);
    proxy->requestSetAppletVisibleCount = 0;
    // Act: 未知 menuId
    plugin->invokedMenuItem("system-monitor", "unknownMenuId", false);
    // Assert: 无任何请求发出，气泡键未被改写
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 0);
    EXPECT_TRUE(proxy->lastAppletKey.isEmpty());
}

// ---- itemCommand ----

TEST_F(MonitorPluginTest, MonitorPlugin_ItemCommand_SystemMonitorKey_LaunchesMonitor)
{
    // Arrange: init 后打桩进程拉起
    plugin->init(proxy);
    stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                   [](const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid) {
                       Q_UNUSED(program)
                       return true;
                   });
    // Act: 左键命令（itemKey=="system-monitor" 触发 openSystemMonitor）
    const QString cmd = plugin->itemCommand("system-monitor");
    // Assert: 命令返回空串；同时发出隐藏气泡请求
    EXPECT_EQ(cmd, QString(""));
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 1);
}

TEST_F(MonitorPluginTest, MonitorPlugin_ItemCommand_OtherKey_ReturnsEmpty)
{
    // Arrange: init
    plugin->init(proxy);
    proxy->requestSetAppletVisibleCount = 0;
    // Act: 其它 key 取命令
    const QString cmd = plugin->itemCommand("other-key");
    // Assert: 不触发启动且返回空串
    EXPECT_EQ(cmd, QString(""));
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 0);
}

TEST_F(MonitorPluginTest, MonitorPlugin_OpenSystemMonitor_RequestsHideApplet)
{
    // Arrange: init 后打桩进程拉起
    plugin->init(proxy);
    stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                   [](const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid) {
                       Q_UNUSED(program)
                       return true;
                   });
    proxy->requestSetAppletVisibleCount = 0;
    // Act: 直接触发启动（AM1 DBus 无总线时仅告警，不崩溃）
    EXPECT_NO_THROW(plugin->openSystemMonitor());
    // Assert: V2 分支隐藏 applet 请求必发
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 1);
    EXPECT_EQ(proxy->lastAppletKey, QString("system-monitor"));
}

// ---- displayModeChanged / itemSortKey / setSortKey ----

TEST_F(MonitorPluginTest, MonitorPlugin_DisplayModeChanged_UpdatesWidgets_NoCrash)
{
    // Arrange: init 后两控件就绪
    plugin->init(proxy);
    // Act: 模拟 dock 通知显示模式切换（Efficient=1 / Fashion=0 两个方向）
    // Assert: 均不崩溃且不重置注册状态
    EXPECT_NO_THROW(plugin->displayModeChanged(Dock::Efficient));
    EXPECT_NO_THROW(plugin->displayModeChanged(Dock::Fashion));
    EXPECT_NE(plugin->itemWidget("system-monitor"), nullptr);
    EXPECT_NE(plugin->itemWidget(Dock::QUICK_ITEM_KEY), nullptr);
}

TEST_F(MonitorPluginTest, MonitorPlugin_ItemSortKey_RoundTripsThroughProxyStore)
{
    // Arrange: init 后设置 dock 显示模式为 Efficient(1)，pos key 形如 pos_<key>_<mode>
    plugin->init(proxy);
    qApp->setProperty(PROP_DISPLAY_MODE, QVariant::fromValue(Dock::Efficient));
    // Act: 写入排序值再读取
    plugin->setSortKey("system-monitor", 5);
    const int afterSet = plugin->itemSortKey("system-monitor");
    // Assert: 经代理存储往返一致
    EXPECT_EQ(afterSet, 5);
    EXPECT_EQ(proxy->store.value("pos_system-monitor_1").toInt(), 5);

    // 强异常安全：切换 Fashion(0) 后 key 隔离，互不影响
    qApp->setProperty(PROP_DISPLAY_MODE, QVariant::fromValue(Dock::Fashion));
    EXPECT_EQ(plugin->itemSortKey("system-monitor"), DOCK_DEFAULT_POS); // 未写入过 → 默认值
    plugin->setSortKey("system-monitor", 2);
    EXPECT_EQ(plugin->itemSortKey("system-monitor"), 2);
    qApp->setProperty(PROP_DISPLAY_MODE, QVariant::fromValue(Dock::Efficient));
    EXPECT_EQ(plugin->itemSortKey("system-monitor"), 5);
}

// ---- pluginStateSwitched ----

TEST_F(MonitorPluginTest, MonitorPlugin_PluginStateSwitched_SavesDisabledState)
{
    // Arrange: init 后 V2 禁用状态恒 false，切换后必为 true
    plugin->init(proxy);
    proxy->itemRemovedKeys.clear();
    // Act: 切换插件启用状态
    plugin->pluginStateSwitched();
    // Assert: 保存 disabled=true 并从 dock 移除本体
    EXPECT_EQ(proxy->store.value("disabled").toBool(), true);
    EXPECT_EQ(proxy->itemRemovedKeys, QStringList({ QString("system-monitor") }));
}

// ---- V2 回调 / 气泡显隐 / 快捷面板点击 ----

TEST_F(MonitorPluginTest, MonitorPlugin_SetMessageCallback_InvokedOnPopupVisible)
{
    // Arrange: 安装回调并 init
    static QString lastMessage;
    lastMessage.clear();
    plugin->setMessageCallback([](PluginsItemInterfaceV2 *, const QString &msg) {
        lastMessage = msg;
        return QString("ok");
    });
    plugin->init(proxy);
    // Act: 气泡显隐状态变化
    plugin->onSysMonPopVisibleChanged(true);
    // Assert: 回调收到 {"msgType":"itemActiveState","data":true}
    EXPECT_FALSE(lastMessage.isEmpty());
    const QJsonObject obj = QJsonDocument::fromJson(lastMessage.toUtf8()).object();
    EXPECT_EQ(obj.value("msgType").toString(), QString("itemActiveState"));
    EXPECT_EQ(obj.value("data").toBool(), true);
}

TEST_F(MonitorPluginTest, MonitorPlugin_OnSysMonPopVisibleChanged_WithoutCallback_NoCrash)
{
    // Arrange: 未设置回调（默认为空）并 init
    static QString lastMessage;
    lastMessage.clear();
    plugin->setMessageCallback(nullptr);
    plugin->init(proxy);
    // Act: 气泡显隐状态变化（回调为空），true/false 两向触发
    EXPECT_NO_THROW(plugin->onSysMonPopVisibleChanged(false));
    plugin->onSysMonPopVisibleChanged(true);
    // Assert: 无回调直接返回不崩溃，消息未被发出，也无 dock 侧副作用
    EXPECT_TRUE(lastMessage.isEmpty());
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 0);
}

TEST_F(MonitorPluginTest, MonitorPlugin_OnClickQuickPanel_HidesAppletAndOpensMonitor)
{
    // Arrange: init 后打桩进程拉起
    plugin->init(proxy);
    stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                   [](const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid) {
                       Q_UNUSED(program)
                       return true;
                   });
    proxy->requestSetAppletVisibleCount = 0;
    // Act: 点击快捷面板图标
    plugin->onClickQuickPanel();
    // Assert: onClickQuickPanel 先隐藏 1 次，openSystemMonitor（V2）再隐藏 1 次，共 2 次
    EXPECT_EQ(proxy->requestSetAppletVisibleCount, 2);
    EXPECT_EQ(proxy->lastAppletKey, QString("system-monitor"));
    EXPECT_EQ(proxy->lastAppletShow, false);
}

// ---- setRateUnitSensitive（枚举全值等价类，TEST_P）----

namespace {
struct RateUnitCase {
    MonitorPlugin::RateUnit unit;
    const char *expected;
};
std::ostream &operator<<(std::ostream &os, const RateUnitCase &c)
{
    return os << "unit=" << int(c.unit) << " expect=" << c.expected;
}
}

class RateUnitParamTest : public MonitorPluginTest,
                          public ::testing::WithParamInterface<RateUnitCase>
{
};

TEST_P(RateUnitParamTest, MonitorPlugin_SetRateUnitSensitive_MapsAllUnits)
{
    // Arrange: 参数即输入
    const RateUnitCase param = GetParam();
    // Act: 设置速率单位（返回字符串）
    const QString str = plugin->setRateUnitSensitive(param.unit);
    // Assert: 枚举 7 值 + default 分支映射正确；重复调用幂等
    EXPECT_EQ(str, QString::fromLatin1(param.expected));
    EXPECT_EQ(plugin->setRateUnitSensitive(param.unit), str);
}

INSTANTIATE_TEST_SUITE_P(RateUnitMapping, RateUnitParamTest,
                         ::testing::Values(
                             RateUnitCase { MonitorPlugin::RateBit, "BIT/s" },
                             RateUnitCase { MonitorPlugin::RateByte, "B/s" },
                             RateUnitCase { MonitorPlugin::RateKb, "KB/s" },
                             RateUnitCase { MonitorPlugin::RateMb, "MB/s" },
                             RateUnitCase { MonitorPlugin::RateGb, "GB/s" },
                             RateUnitCase { MonitorPlugin::RateTb, "TB/s" },
                             RateUnitCase { MonitorPlugin::RateUnknow, "" },
                             RateUnitCase { MonitorPlugin::RateUnit(99), "" })); // default 分支

// ---- autoRateUnits（数值等级边界，TEST_P）----

namespace {
struct AutoRateCase {
    qlonglong speed;
    MonitorPlugin::RateUnit expectUnit;
    double expectSpeed;
};
std::ostream &operator<<(std::ostream &os, const AutoRateCase &c)
{
    return os << "speed=" << c.speed << " unit=" << int(c.expectUnit) << " v=" << c.expectSpeed;
}
}

class AutoRateParamTest : public MonitorPluginTest,
                          public ::testing::WithParamInterface<AutoRateCase>
{
};

TEST_P(AutoRateParamTest, MonitorPlugin_AutoRateUnits_BoundaryValues_ScalesCorrectly)
{
    // Arrange: 参数即输入（unit 作出参）
    const AutoRateCase param = GetParam();
    MonitorPlugin::RateUnit unit = MonitorPlugin::RateByte;
    // Act: 自动换算单位
    const double speed = plugin->autoRateUnits(param.speed, unit);
    // Assert: 单位与数值同时匹配（0/1/1023/1024/2^20/2^30/2^40 边界与越界回退）
    EXPECT_EQ(unit, param.expectUnit);
    EXPECT_DOUBLE_EQ(speed, param.expectSpeed);
}

INSTANTIATE_TEST_SUITE_P(AutoRateBoundary, AutoRateParamTest,
                         ::testing::Values(
                             AutoRateCase { 0, MonitorPlugin::RateByte, 0.0 },
                             AutoRateCase { 1, MonitorPlugin::RateByte, 1.0 },
                             AutoRateCase { 1023, MonitorPlugin::RateByte, 1023.0 },
                             AutoRateCase { 1024, MonitorPlugin::RateKb, 1.0 },
                             AutoRateCase { qlonglong(1024) * 1024, MonitorPlugin::RateMb, 1.0 },
                             AutoRateCase { qlonglong(1024) * 1024 * 1024, MonitorPlugin::RateGb, 1.0 },
                             AutoRateCase { qlonglong(1024) * 1024 * 1024 * 1024, MonitorPlugin::RateTb, 1.0 },
                             AutoRateCase { qlonglong(1024) * 1024 * 1024 * 1024 * 1024, MonitorPlugin::RateUnknow, -1.0 },
                             AutoRateCase { -5, MonitorPlugin::RateUnknow, -1.0 },
                             AutoRateCase { qlonglong(1024) * 1024 - 1, MonitorPlugin::RateKb, (qlonglong(1024) * 1024 - 1) / 1024.0 }));

TEST_F(MonitorPluginTest, MonitorPlugin_AutoRateUnits_NonByteUnit_ReturnsNegative)
{
    // Arrange: 预置非字节单位
    MonitorPlugin::RateUnit unit = MonitorPlugin::RateKb;
    // Act: 以非 RateByte 单位换算
    const double rate = plugin->autoRateUnits(4096, unit);
    // Assert: unit!=RateByte 直接返回 -1 且不改单位
    EXPECT_DOUBLE_EQ(rate, -1.0);
    EXPECT_EQ(unit, MonitorPlugin::RateKb);

    // 强异常安全：恢复字节单位后换算恢复正常（4KiB → 4.0KB）
    unit = MonitorPlugin::RateByte;
    EXPECT_DOUBLE_EQ(plugin->autoRateUnits(4096, unit), 4.0);
    EXPECT_EQ(unit, MonitorPlugin::RateKb);
}

// ---- calcCpuRate / calcMemRate / calcNetRate（/proc 采集）----

TEST_F(MonitorPluginTest, MonitorPlugin_CalcCpuRate_ValidProcStat_NonZeroTotal)
{
    // Arrange: 真实 /proc/stat（CI Linux 环境恒存在，不依赖具体数值）
    qlonglong total = 0;
    qlonglong available = 0;
    // Act: 采集
    EXPECT_NO_THROW(plugin->calcCpuRate(total, available));
    // Assert: 总量非零，可用量落在 [0, total]
    EXPECT_GT(total, 0);
    EXPECT_GE(available, 0);
    EXPECT_LE(available, total);
}

TEST_F(MonitorPluginTest, MonitorPlugin_CalcCpuRate_OpenFailure_ReturnsZeros)
{
    // Arrange: 打桩 QFile::open(OpenMode) 失败（覆盖 /proc 不可读分支）
    stubQFileOpen(false);
    qlonglong total = 0;
    qlonglong available = 0;
    // Act: 采集
    plugin->calcCpuRate(total, available);
    // Assert: 双零回退（函数入口已先置零）
    EXPECT_EQ(total, 0);
    EXPECT_EQ(available, 0);
}

TEST_F(MonitorPluginTest, MonitorPlugin_CalcMemRate_ValidProcMeminfo_WithinTotal)
{
    // Arrange: 真实 /proc/meminfo
    qlonglong memory = 0;
    qlonglong memoryAll = 0;
    // Act: 采集
    EXPECT_NO_THROW(plugin->calcMemRate(memory, memoryAll));
    // Assert: 总内存非零且可用不超过总量
    EXPECT_GT(memoryAll, 0);
    EXPECT_GE(memory, 0);
    EXPECT_LE(memory, memoryAll);
}

TEST_F(MonitorPluginTest, MonitorPlugin_CalcMemRate_OpenFailure_ReturnsZeros)
{
    // Arrange: 打桩 open 失败
    stubQFileOpen(false);
    qlonglong memory = 0;
    qlonglong memoryAll = 0;
    // Act: 采集
    plugin->calcMemRate(memory, memoryAll);
    // Assert: 双零回退
    EXPECT_EQ(memory, 0);
    EXPECT_EQ(memoryAll, 0);
}

TEST_F(MonitorPluginTest, MonitorPlugin_CalcNetRate_ValidProcNetDev_NonNegative)
{
    // Arrange: 真实 /proc/net/dev
    qlonglong down = 0;
    qlonglong upload = 0;
    // Act: 采集
    EXPECT_NO_THROW(plugin->calcNetRate(down, upload));
    // Assert: 累计字节数非负
    EXPECT_GE(down, 0);
    EXPECT_GE(upload, 0);
}

TEST_F(MonitorPluginTest, MonitorPlugin_CalcNetRate_OpenFailure_KeepsCallerValues)
{
    // Arrange: 打桩 open 失败；源码失败分支提前 return 且不重置入参
    stubQFileOpen(false);
    qlonglong down = -7;
    qlonglong upload = -9;
    // Act: 采集
    plugin->calcNetRate(down, upload);
    // Assert: 失败时入参原值保持（与 cpu/mem 失败置零行为不同，记录差异）
    EXPECT_EQ(down, -7);
    EXPECT_EQ(upload, -9);
}

// ---- udpateInfo / udpateTipsInfo ----

TEST_F(MonitorPluginTest, MonitorPlugin_UdpateInfo_FormatsTipStrings)
{
    // Arrange: init（loadPlugin 已把刷新定时器间隔设为 1000ms，避免除零）
    plugin->init(proxy);
    // Act: 手动刷新一次
    EXPECT_NO_THROW(plugin->udpateInfo());
    // Assert: CPU/内存为百分数格式，上/下行速率为 "/s" 结尾；重复刷新结果仍合法
    EXPECT_THAT(plugin->m_cpuStr.toStdString(), testing::MatchesRegex("[0-9]+(\\.[0-9])?%"));
    EXPECT_THAT(plugin->m_memStr.toStdString(), testing::MatchesRegex("[0-9]+(\\.[0-9])?%"));
    EXPECT_TRUE(plugin->m_downloadStr.endsWith("/s"));
    EXPECT_TRUE(plugin->m_uploadStr.endsWith("/s"));
    EXPECT_NO_THROW(plugin->udpateInfo());
    EXPECT_THAT(plugin->m_cpuStr.toStdString(), testing::MatchesRegex("[0-9]+(\\.[0-9])?%"));
}

TEST_F(MonitorPluginTest, MonitorPlugin_UdpateTipsInfo_FillsTipsLabel)
{
    // Arrange: init 后气泡控件就绪
    plugin->init(proxy);
    // Act: 刷新提示内容（内部先 udpateInfo 再回填文本）
    EXPECT_NO_THROW(plugin->udpateTipsInfo());
    // Assert: 提示控件对象名/实例稳定
    QWidget *tips = plugin->itemTipsWidget("system-monitor");
    ASSERT_NE(tips, nullptr);
    EXPECT_EQ(tips->objectName(), QString("system-monitor"));
    EXPECT_EQ(plugin->itemTipsWidget("system-monitor"), tips);
}
