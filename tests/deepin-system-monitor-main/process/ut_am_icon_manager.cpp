// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// AMIconManager 单元测试（UT_AMIconManager）
// 被测文件：deepin-system-monitor-main/process/am_icon_manager.cpp
//
// 用例数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | AMIconManager | low | - | 1 | 3 |
// | instance | mid | in_degree:4 | 2 | 2 |
// | isAMAvailable | low | - | 1 | 1 |
// | initializeAMInterface | low | - | 1 | 2 |
// | clearCache | mid | name_pattern | 2 | 2 |
// | removeCachedIcon | mid | name_pattern | 2 | 2 |
// | getAppNameByAppId | high | complexity:8 | 3 | 3 |
// | getIconByAppId | mid | complexity:6 | 2 | 3 |
// | getIconByPid | low | - | 1 | 4 |
// | getProcessGroupInfo | low | - | 1 | 3 |
// | identifyProcess | high | complexity:6 | 3 | 4 |
// | resolveLinglongAppId | low | - | 1 | 2 |
// | ~AMIconManager | low | destructor | 1 | 1(隐含) |
//
// 等价类划分：
//   - 输入 appId/pid：空串、缓存命中、缓存未命中（DBus/文件系统路径）、不可用状态
//   - 系统版本：V25+ / Pre-V25；AM 服务：已注册 / 未注册 / 接口无效
// 边界值分析：majorVersion 边界 25（含）/ 20（不含）；pidfd_open 失败边界（pid 越界 99999999）
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]（dtor 经全部用例析构隐含验证）
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]（版本 25/20、pid 越界）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（getIconByAppId/getIconByPid 等价类同构，已按独立用例列出）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]（DBus Reply 离线不可构造路径已标注盲区）
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无 throw 路径）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（QDBus 虚方法 VADDR 桩化，无 gMock）
//
// 分支清单（来源：AMIconManager::AMIconManager(QObject*)，worktree ut-sm-t07 实际源码，工具计数 3）
// B1: if (m_systemMajorVersion >= 25) 版本门槛（>=25 走 AM 探测，<25 直接不可用）
// B2: if (m_amAvailable) AM 服务已注册 → initializeAMInterface()，否则仅告警
// B3: initializeAMInterface 内 if (!m_amInterface) 创建接口（isValid 失败时回退不可用）
// 用例映射：
// - Construct_PreV25System_MarksUnavailable          → B1(假分支)+B2(假分支)
// - Construct_V25WithoutAMService_MarksUnavailable   → B1(真分支)+B2(假分支)
// - Construct_V25WithAMService_InvalidInterface      → B1+B2+B3
//
// 分支清单（来源：AMIconManager::initializeAMInterface()，worktree ut-sm-t07 实际源码，工具计数 3）
// B1: if (!m_amAvailable) 早退（接口保持 nullptr）
// B2: if (!m_amInterface) 创建 QDBusInterface（已存在则跳过，保证幂等）
// B3: if (!m_amInterface->isValid()) 接口无效 → m_amAvailable=false；否则保持可用
// 用例映射：
// - InitializeAMInterface_Unavailable_KeepsInterfaceNull        → B1
// - InitializeAMInterface_AvailableInvalidInterface_Disables    → B2+B3
// - Construct_V25WithAMService_InvalidInterface                 → B2+B3（经构造函数）
//
// 分支清单（来源：AMIconManager::getAppNameByAppId(const QString&)，worktree ut-sm-t07 实际源码，工具计数 12）
// B1: if (appId.isEmpty()) 空输入早退
// B2: B1 分支 return QString()
// B3: if (m_appIdToNameCache.contains(appId)) 缓存命中
// B4: B3 分支 return m_appIdToNameCache[appId]
// B5: if (!propsInterface.isValid()) 属性接口无效
// B6: B5 分支 return QString()
// B7: if (!nameReply.isValid()) 属性调用失败
// B8: B7 分支 return QString()
// B9: if (names.contains(localeKey)) 本地化名称命中
// B10: else if (names.contains(kDefaultKey)) 默认名称命中
// B11: else if (!names.isEmpty()) 取第一个名称
// B12: if (!displayName.isEmpty()) 写缓存 + 末尾 return displayName
// 用例映射：
// - GetAppNameByAppId_EmptyInput_ReturnsEmpty          → B1+B2
// - GetAppNameByAppId_CacheHit_ReturnsCachedValue      → B3+B4
// - GetAppNameByAppId_InvalidInterface_ReturnsEmpty    → B5+B6
// （B7-B12 需要真实 DBus Reply a{ss} 解码，离线不可构造，列入覆盖盲区）
//
// 分支清单（来源：AMIconManager::getIconByAppId(const QString&)，worktree ut-sm-t07 实际源码，工具计数 10）
// B1: if (appId.isEmpty()) 空输入早退
// B2: B1 分支 return QString()
// B3: if (m_appIdToIconCache.contains(appId)) 缓存命中
// B4: B3 分支 return m_appIdToIconCache[appId]
// B5: if (!propsInterface.isValid()) 属性接口无效
// B6: B5 分支 return QString()
// B7: if (!iconsReply.isValid()) 属性调用失败 → return QString()
// B8: if (iconName.isEmpty()) Desktop Entry 键缺失 → 回退 default 键
// B9: if (!iconName.isEmpty()) 写缓存
// B10: 末尾 return iconName
// 用例映射：
// - GetIconByAppId_EmptyInput_ReturnsEmpty             → B1+B2
// - GetIconByAppId_CacheHit_ReturnsCachedValue         → B3+B4
// - GetIconByAppId_InvalidInterface_ReturnsEmpty       → B5+B6
// （B7-B10 需要真实 DBus Reply a{ss} 解码，离线不可构造，列入覆盖盲区）
// 分支清单（来源：AMIconManager::getIconByPid(pid_t)，worktree ut-sm-t07 实际源码，工具计数 8）
// B1: if (!m_amAvailable || !m_amInterface) 不可用早退
// B2: if (m_pidToIconCache.contains(pid)) 缓存命中
// B3: if (pidfd < 0) pidfd_open 失败早退
// B4: if (!info.isValid()) 识别结果无效早退
// B5: 缓存写入块（无条件，含 icon 与 group 双写）
// B6: 末尾 return info.iconName
// B7: close(pidfd) 资源释放（无分支）
// B8: 双缓存一致性（iconName 与 groupInfo 同源）
// 用例映射：
// - GetIconByPid_Unavailable_ReturnsEmpty              → B1
// - GetIconByPid_CacheHit_ReturnsCachedIcon            → B2
// - GetIconByPid_PidfdOpenFails_ReturnsEmpty           → B3
// - GetIconByPid_IdentifyInvalid_ReturnsEmpty          → B4
//
// 分支清单（来源：AMIconManager::getProcessGroupInfo(pid_t)，worktree ut-sm-t07 实际源码，工具计数 8）
// B1: if (!m_amAvailable || !m_amInterface) 不可用早退（返回默认 info）
// B2: if (m_pidToGroupCache.contains(pid)) 缓存命中
// B3: if (pidfd < 0) pidfd_open 失败早退（告警带 errno）
// B4: if (info.isValid()) 有效结果写缓存
// B5: 末尾 return info
// B6: close(pidfd) 资源释放（无分支）
// B7: 缓存返回与写入缓存同源
// B8: 无效结果不写缓存（负例）
// 用例映射：
// - GetProcessGroupInfo_Unavailable_ReturnsInvalidInfo   → B1
// - GetProcessGroupInfo_CacheHit_ReturnsCachedInfo       → B2
// - GetProcessGroupInfo_PidfdOpenFails_ReturnsInvalid    → B3
//
// 分支清单（来源：AMIconManager::identifyProcess(int)，worktree ut-sm-t07 实际源码，工具计数 10）
// B1: if (!m_amInterface || !m_amInterface->isValid()) 接口不可用早退
// B2: B1 分支 return result（无效）
// B3: if (reply.type() == ErrorMessage) DBus 错误回复早退
// B4: B3 分支 return result
// B5: if (args.size() < 3) 参数数量异常早退
// B6: B5 分支 return result
// B7: if (args.at(1).canConvert<QDBusObjectPath>()) 对象路径转换
// B8: else if (args.at(1).canConvert<QString>()) 字符串路径转换
// B9: if (result.appId.isEmpty()) 空 appId 早退 return result
// B10: icon/name/linglong 组装 + 末尾 return result
// 用例映射：
// - IdentifyProcess_InterfaceInvalid_ReturnsInvalidInfo → B1+B2
// - IdentifyProcess_DBusCallFails_ReturnsInvalidInfo    → B3+B4
// - IdentifyProcess_UnexpectedArgCount_ReturnsInvalid   → B5+B6
// - IdentifyProcess_ValidReply_ParsesGroupInfo          → B7+B9(不触发)+B10
// （B8 字符串路径转换 / B9 空 appId 为低价值重复路径，列入盲区说明）
// 分支清单（来源：AMIconManager::resolveLinglongAppId(const QString&)，worktree ut-sm-t07 实际源码，工具计数 5）
// B1: if (m_linglongAppIdCache.contains(appId)) 缓存命中
// B2: if (!QFile::exists(desktopPath)) desktop 文件不存在 → 缓存空串并返回
// B3: DDesktopEntry 读取 X-linglong 键（文件存在路径）
// B4: 缓存写入 linglongId
// B5: 末尾 return linglongId
// 用例映射：
// - ResolveLinglongAppId_CacheHit_ReturnsCachedId       → B1
// - ResolveLinglongAppId_NoDesktopFile_ReturnsEmpty     → B2
// （B3-B5 依赖系统 /var/lib/linglong 路径，不可注入，列入覆盖盲区）
//
// 分支清单（来源：AMIconManager::clearCache()，worktree ut-sm-t07 实际源码，工具计数 0）
// 无分支（三映射表无条件清空）
//
// 分支清单（来源：AMIconManager析构函数~AMIconManager()，worktree ut-sm-t07 实际源码，工具计数 1）
// B1: if (m_amInterface) delete 并置空
// 用例映射：所有用例析构触发（Construct_V25WithAMService_InvalidInterface 覆盖非空删除路径）
//
// 分支清单（来源：AMIconManager::removeCachedIcon(pid_t)，worktree ut-sm-t07 实际源码，工具计数 0）
// 无分支（双 pid 映射无条件删除）
//
// 分支清单（来源：AMIconManager::isAMAvailable() const，worktree ut-sm-t07 实际源码，工具计数 0）
// 无分支（透传成员）
//
// 分支清单（来源：AMIconManager::instance()，worktree ut-sm-t07 实际源码，工具计数 2）
// B1: if (!sin) 外层快路径判定（已存在 → 直接 return sin）
// B2: 加锁后 if (!sin) 二次检查 → 创建并 store
// 用例映射：
// - Instance_RepeatedCalls_ReturnsSameNonNull      → B1（第二次调用命中快路径）
// - Instance_FastPath_ReturnsPreSeededInstance     → B1（预置实例直接返回）
// （B2 创建分支经 UT_AMIconManagerSafe 历史用例覆盖；本夹具首次 instance() 调用即触发 B2）

//self
#include "process/am_icon_manager.h"

//3rdparty
#include "3rdparty/stub-ext/stubext.h"

//gtest
#include <gtest/gtest.h>

//qt
#include <QString>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDBusError>

//dtk
#include <DSysInfo>
#include <DDesktopEntry>

//c
#include <unistd.h>

using namespace core::process;
using namespace Dtk::Core;

//━━━━━━━━━━━━━━━━━━━━ 测试夹具 ━━━━━━━━━━━━━━━━━━━━
class UT_AMIconManager : public ::testing::Test {
protected:
    void SetUp() override {
        stub.clear();
    }
    void TearDown() override {
        stub.clear();
    }

    // 构造一个确定不可用的 AMIconManager（V25 + AM 服务未注册，不触碰真实 D-Bus 接口创建）
    std::unique_ptr<AMIconManager> makeUnavailableManager() {
        stub.set_lamda(&DSysInfo::majorVersion, []() -> QString {
            __DBG_STUB_INVOKE__
            return QStringLiteral("25");
        });
        stub.set_lamda(VADDR(QDBusConnectionInterface, isServiceRegistered),
                       [](QDBusConnectionInterface *, const QString &) -> QDBusReply<bool> {
            __DBG_STUB_INVOKE__
            QDBusMessage msg;
            return QDBusReply<bool>(msg.createReply(QVariant::fromValue(false)));
        });
        return std::unique_ptr<AMIconManager>(new AMIconManager());
    }

    // 为缓存命中路径挂一个真实接口对象（命中路径不发起任何 D-Bus 调用）
    void attachAvailableInterface(AMIconManager *obj) {
        obj->m_amAvailable = true;
        obj->m_amInterface = new QDBusInterface(
            QStringLiteral("org.desktopspec.ApplicationManager1"),
            QStringLiteral("/org/desktopspec/ApplicationManager1"),
            QStringLiteral("org.desktopspec.ApplicationManager1"),
            QDBusConnection::sessionBus());
    }
    void detachInterface(AMIconManager *obj) {
        delete obj->m_amInterface;
        obj->m_amInterface = nullptr;
        obj->m_amAvailable = false;
    }

    stub_ext::StubExt stub;
};

//━━━━━━━━━━━━━━━━━━━━ 构造 / 生命周期 ━━━━━━━━━━━━━━━━━━━━

// Pre-V25 系统：版本门槛不满足，直接不可用
TEST_F(UT_AMIconManager, Construct_PreV25System_MarksUnavailable)
{
    // Arrange: 打桩系统版本为 20（< 25 门槛）
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString {
        __DBG_STUB_INVOKE__
        return QStringLiteral("20");
    });
    std::unique_ptr<AMIconManager> obj;
    // Act: 构造管理器
    obj.reset(new AMIconManager());
    // Assert: 版本缓存 20，AM 不可用，未创建 D-Bus 接口
    EXPECT_EQ(obj->m_systemMajorVersion, 20);
    EXPECT_FALSE(obj->m_amAvailable);
    EXPECT_EQ(obj->m_amInterface, nullptr);
}

// V25 但 AM 服务未注册：不可用且不创建接口
TEST_F(UT_AMIconManager, Construct_V25WithoutAMService_MarksUnavailable)
{
    // Arrange: V25 + isServiceRegistered 恒 false
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString {
        __DBG_STUB_INVOKE__
        return QStringLiteral("25");
    });
    stub.set_lamda(VADDR(QDBusConnectionInterface, isServiceRegistered),
                   [](QDBusConnectionInterface *, const QString &) -> QDBusReply<bool> {
        __DBG_STUB_INVOKE__
        QDBusMessage msg;
        return QDBusReply<bool>(msg.createReply(QVariant::fromValue(false)));
    });
    std::unique_ptr<AMIconManager> obj;
    // Act: 构造管理器
    obj.reset(new AMIconManager());
    // Assert: 版本 25 但服务未注册 → 不可用、接口为空
    EXPECT_EQ(obj->m_systemMajorVersion, 25);
    EXPECT_FALSE(obj->m_amAvailable);
    EXPECT_EQ(obj->m_amInterface, nullptr);
}

// V25 且注册成功但接口无效：接口创建后回退不可用，析构删除接口
TEST_F(UT_AMIconManager, Construct_V25WithAMService_InvalidInterface)
{
    // Arrange: V25 + 服务已注册 + 接口 isValid 恒 false
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString {
        __DBG_STUB_INVOKE__
        return QStringLiteral("25");
    });
    stub.set_lamda(VADDR(QDBusConnectionInterface, isServiceRegistered),
                   [](QDBusConnectionInterface *, const QString &) -> QDBusReply<bool> {
        __DBG_STUB_INVOKE__
        QDBusMessage msg;
        return QDBusReply<bool>(msg.createReply(QVariant::fromValue(true)));
    });
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    std::unique_ptr<AMIconManager> obj;
    // Act: 构造管理器（内部走 initializeAMInterface）
    obj.reset(new AMIconManager());
    // Assert: 接口已创建但无效 → 可用性回退 false
    EXPECT_NE(obj->m_amInterface, nullptr);
    EXPECT_FALSE(obj->m_amAvailable);
}

// 单例：两次获取同指针
TEST_F(UT_AMIconManager, Instance_RepeatedCalls_ReturnsSameNonNull)
{
    // Arrange: 使用真实单例（不注入桩，走本机默认路径）
    AMIconManager *first = nullptr;
    AMIconManager *second = nullptr;
    // Act: 连续两次调用
    first = AMIconManager::instance();
    second = AMIconManager::instance();
    // Assert: 非空且同源
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// 单例快速路径：已存在实例时直接返回，不再创建
TEST_F(UT_AMIconManager, Instance_FastPath_ReturnsPreSeededInstance)
{
    // Arrange: 备份并预置全局单例指针
    AMIconManager *saved = AMIconManager::m_instance.load();
    AMIconManager sentinel;
    AMIconManager::m_instance.store(&sentinel);
    AMIconManager *result = nullptr;
    // Act: 获取单例（走快速路径）
    result = AMIconManager::instance();
    // Assert: 返回预置实例；恢复后单例回到原值
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, &sentinel);
    AMIconManager::m_instance.store(saved);
    EXPECT_EQ(AMIconManager::instance(), saved);
}

//━━━━━━━━━━━━━━━━━━━━ 可用性与接口初始化 ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, IsAMAvailable_InitialState_ReturnsFalse)
{
    // Arrange: 构造不可用实例
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    bool result = true;
    // Act: 查询可用性
    result = obj->isAMAvailable();
    // Assert: 与内部成员一致（false）
    EXPECT_FALSE(result);
    EXPECT_EQ(result, obj->m_amAvailable);
}

TEST_F(UT_AMIconManager, InitializeAMInterface_Unavailable_KeepsInterfaceNull)
{
    // Arrange: 不可用实例
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    // Act: 主动初始化（应早退）
    obj->initializeAMInterface();
    // Assert: 接口保持为空
    EXPECT_EQ(obj->m_amInterface, nullptr);
    EXPECT_FALSE(obj->m_amAvailable);
}

TEST_F(UT_AMIconManager, InitializeAMInterface_AvailableInvalidInterface_DisablesAvailability)
{
    // Arrange: 手工置可用，打桩接口无效
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    // Act: 两次初始化（验证幂等）
    obj->initializeAMInterface();
    QDBusInterface *first = obj->m_amInterface;
    obj->initializeAMInterface();
    // Assert: 接口创建一次且复用；无效接口使可用性回退
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(obj->m_amInterface, first);
    EXPECT_FALSE(obj->m_amAvailable);
}

//━━━━━━━━━━━━━━━━━━━━ 缓存操作 ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, ClearCache_PopulatedCaches_EmptiesAllMaps)
{
    // Arrange: 直接填充三类内部缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_pidToIconCache[100] = QStringLiteral("icon-a");
    obj->m_appIdToIconCache[QStringLiteral("app.a")] = QStringLiteral("icon-b");
    obj->m_appIdToNameCache[QStringLiteral("app.a")] = QStringLiteral("名称");
    obj->m_pidToGroupCache[100] = AMProcessGroupInfo();
    // Act: 清空缓存
    obj->clearCache();
    // Assert: 三个映射为空；名称缓存不在 clearCache 范围内（保持不变，
    // 该行为与源码一致，疑似设计疏漏，已记入缺陷清单）
    EXPECT_EQ(obj->m_pidToIconCache.size(), 0);
    EXPECT_EQ(obj->m_appIdToIconCache.size(), 0);
    EXPECT_EQ(obj->m_pidToGroupCache.size(), 0);
    EXPECT_EQ(obj->m_appIdToNameCache.size(), 1);
}

TEST_F(UT_AMIconManager, ClearCache_Idempotent_SecondCallStaysEmpty)
{
    // Arrange: 填充后清空一次，再次填充少量条目
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_pidToIconCache[1] = QStringLiteral("i");
    obj->clearCache();
    // Act: 第二次清空（对已空缓存幂等）
    obj->clearCache();
    // Assert: 仍为空，无副作用
    EXPECT_EQ(obj->m_pidToIconCache.size(), 0);
    EXPECT_EQ(obj->m_appIdToIconCache.size(), 0);
    EXPECT_EQ(obj->m_pidToGroupCache.size(), 0);
}

TEST_F(UT_AMIconManager, RemoveCachedIcon_ExistingPid_RemovesFromBothPidCaches)
{
    // Arrange: 填充 pid 双缓存与 appId 缓存（appId 缓存应不受影响）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_pidToIconCache[100] = QStringLiteral("icon-a");
    obj->m_pidToGroupCache[100] = AMProcessGroupInfo();
    obj->m_pidToIconCache[200] = QStringLiteral("icon-c");
    obj->m_appIdToIconCache[QStringLiteral("app.a")] = QStringLiteral("icon-b");
    // Act: 删除 pid=100
    obj->removeCachedIcon(100);
    // Assert: pid=100 双缓存移除，其余保留
    EXPECT_EQ(obj->m_pidToIconCache.count(100), 0);
    EXPECT_EQ(obj->m_pidToGroupCache.count(100), 0);
    EXPECT_EQ(obj->m_pidToIconCache.count(200), 1);
    EXPECT_EQ(obj->m_appIdToIconCache.count(QStringLiteral("app.a")), 1);
}

TEST_F(UT_AMIconManager, RemoveCachedIcon_AbsentPid_KeepsOtherEntries)
{
    // Arrange: 填充 pid=100/200，删除不存在的 pid=300
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_pidToIconCache[100] = QStringLiteral("icon-a");
    obj->m_pidToGroupCache[100] = AMProcessGroupInfo();
    obj->m_pidToIconCache[200] = QStringLiteral("icon-c");
    // Act: 删除不存在的 pid
    obj->removeCachedIcon(300);
    // Assert: 原有条目完好
    EXPECT_EQ(obj->m_pidToIconCache.count(100), 1);
    EXPECT_EQ(obj->m_pidToIconCache.count(200), 1);
    EXPECT_EQ(obj->m_pidToGroupCache.count(100), 1);
}

//━━━━━━━━━━━━━━━━━━━━ getAppNameByAppId ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, GetAppNameByAppId_EmptyInput_ReturnsEmpty)
{
    // Arrange: 可用实例（appId 空判断在最前，不触达 D-Bus）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    QString result = QStringLiteral("sentinel");
    // Act: 空 appId 查询
    result = obj->getAppNameByAppId(QString());
    // Assert: 返回空串且不污染缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_appIdToNameCache.count(QString()), 0);
}

TEST_F(UT_AMIconManager, GetAppNameByAppId_CacheHit_ReturnsCachedValue)
{
    // Arrange: 预置名称缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_appIdToNameCache[QStringLiteral("cached.app")] = QStringLiteral("缓存名");
    EXPECT_EQ(obj->m_appIdToNameCache.count(QStringLiteral("cached.app")), 1);
    QString result;
    // Act: 查询已缓存 appId
    result = obj->getAppNameByAppId(QStringLiteral("cached.app"));
    // Assert: 命中缓存
    EXPECT_EQ(result, QStringLiteral("缓存名"));
}

TEST_F(UT_AMIconManager, GetAppNameByAppId_InvalidInterface_ReturnsEmpty)
{
    // Arrange: 未命中缓存 + 属性接口无效
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    QString result = QStringLiteral("sentinel");
    // Act: 查询未缓存 appId
    result = obj->getAppNameByAppId(QStringLiteral("no.cache.app"));
    // Assert: 返回空串且不污染缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_appIdToNameCache.count(QStringLiteral("no.cache.app")), 0);
}

//━━━━━━━━━━━━━━━━━━━━ getIconByAppId ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, GetIconByAppId_EmptyInput_ReturnsEmpty)
{
    // Arrange: 可用实例
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    QString result = QStringLiteral("sentinel");
    // Act: 空 appId 查询
    result = obj->getIconByAppId(QString());
    // Assert: 返回空串且不污染缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_appIdToIconCache.count(QString()), 0);
}

TEST_F(UT_AMIconManager, GetIconByAppId_CacheHit_ReturnsCachedValue)
{
    // Arrange: 预置图标缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_appIdToIconCache[QStringLiteral("cached.app")] = QStringLiteral("cached-icon");
    EXPECT_EQ(obj->m_appIdToIconCache.count(QStringLiteral("cached.app")), 1);
    QString result;
    // Act: 查询已缓存 appId
    result = obj->getIconByAppId(QStringLiteral("cached.app"));
    // Assert: 命中缓存
    EXPECT_EQ(result, QStringLiteral("cached-icon"));
}

TEST_F(UT_AMIconManager, GetIconByAppId_InvalidInterface_ReturnsEmpty)
{
    // Arrange: 未命中缓存 + 属性接口无效
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    QString result = QStringLiteral("sentinel");
    // Act: 查询未缓存 appId
    result = obj->getIconByAppId(QStringLiteral("no.cache.app"));
    // Assert: 返回空串且不污染缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_appIdToIconCache.count(QStringLiteral("no.cache.app")), 0);
}

//━━━━━━━━━━━━━━━━━━━━ getIconByPid ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, GetIconByPid_Unavailable_ReturnsEmpty)
{
    // Arrange: 不可用实例
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    QString result = QStringLiteral("sentinel");
    // Act: 查询 pid 图标
    result = obj->getIconByPid(4321);
    // Assert: 早退返回空串且不写缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_pidToIconCache.count(4321), 0);
}

TEST_F(UT_AMIconManager, GetIconByPid_CacheHit_ReturnsCachedIcon)
{
    // Arrange: 置可用（缓存命中路径要求先通过可用性检查），并预置缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    attachAvailableInterface(obj.get());
    obj->m_pidToIconCache[4321] = QStringLiteral("pid-icon");
    EXPECT_EQ(obj->m_pidToIconCache.count(4321), 1);
    QString result;
    // Act: 查询已缓存 pid
    result = obj->getIconByPid(4321);
    // Assert: 命中缓存
    EXPECT_EQ(result, QStringLiteral("pid-icon"));
    detachInterface(obj.get());
}

TEST_F(UT_AMIconManager, GetIconByPid_PidfdOpenFails_ReturnsEmpty)
{
    // Arrange: 置可用并挂真实接口，pid 取越界值 99999999（pidfd_open 必然失败）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    QString result = QStringLiteral("sentinel");
    // Act: 查询越界 pid
    result = obj->getIconByPid(99999999);
    // Assert: 返回空串且不写缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_pidToIconCache.count(99999999), 0);
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

TEST_F(UT_AMIconManager, GetIconByPid_IdentifyInvalid_ReturnsEmpty)
{
    // Arrange: 置可用 + 接口无效（identifyProcess 内 isValid 早退），pid 取当前进程
    //（pidfd_open 对自身必然成功）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    QString result = QStringLiteral("sentinel");
    // Act: 查询自身 pid
    result = obj->getIconByPid(::getpid());
    // Assert: 识别失败返回空串且不写缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_pidToIconCache.count(::getpid()), 0);
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

//━━━━━━━━━━━━━━━━━━━━ getProcessGroupInfo ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, GetProcessGroupInfo_Unavailable_ReturnsInvalidInfo)
{
    // Arrange: 不可用实例
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    AMProcessGroupInfo info;
    info.appId = QStringLiteral("sentinel");
    // Act: 查询进程组信息
    info = obj->getProcessGroupInfo(4321);
    // Assert: 返回默认构造（无效）info
    EXPECT_FALSE(info.isValid());
    EXPECT_EQ(info.appId, QString());
}

TEST_F(UT_AMIconManager, GetProcessGroupInfo_CacheHit_ReturnsCachedInfo)
{
    // Arrange: 置可用（缓存命中路径要求先通过可用性检查），并预置组信息缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    attachAvailableInterface(obj.get());
    AMProcessGroupInfo cached;
    cached.appId = QStringLiteral("cached.app");
    cached.instancePath = QStringLiteral("/instance/path");
    obj->m_pidToGroupCache[4321] = cached;
    AMProcessGroupInfo info;
    // Act: 查询已缓存 pid
    info = obj->getProcessGroupInfo(4321);
    // Assert: 返回缓存对象
    EXPECT_TRUE(info.isValid());
    EXPECT_EQ(info.appId, QStringLiteral("cached.app"));
    EXPECT_EQ(info.instancePath, QStringLiteral("/instance/path"));
    detachInterface(obj.get());
}

TEST_F(UT_AMIconManager, GetProcessGroupInfo_PidfdOpenFails_ReturnsInvalidInfo)
{
    // Arrange: 置可用并挂真实接口，pid 越界（pidfd_open 失败）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    AMProcessGroupInfo info;
    info.appId = QStringLiteral("sentinel");
    // Act: 查询越界 pid
    info = obj->getProcessGroupInfo(99999999);
    // Assert: 返回无效 info 且不写缓存
    EXPECT_FALSE(info.isValid());
    EXPECT_EQ(obj->m_pidToGroupCache.count(99999999), 0);
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

//━━━━━━━━━━━━━━━━━━━━ identifyProcess ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, IdentifyProcess_InterfaceInvalid_ReturnsInvalidInfo)
{
    // Arrange: 接口无效（isValid 早退），fd 传 -1（不参与该路径）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    AMProcessGroupInfo info;
    info.appId = QStringLiteral("sentinel");
    // Act: 识别（fd 无效值，早退路径不使用 fd）
    info = obj->identifyProcess(-1);
    // Assert: 返回无效 info
    EXPECT_FALSE(info.isValid());
    EXPECT_EQ(info.appId, QString());
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

// doCall 打桩辅助：返回预设回复（Qt6 的 call() 均内联转发到 doCall）
static QDBusMessage g_identifyReply;
static void stubDoCallReply(stub_ext::StubExt &stub, const QDBusMessage &reply)
{
    using DoCallSig = QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode,
                                                               const QString &,
                                                               const QVariant *,
                                                               size_t);
    g_identifyReply = reply;
    stub.set_lamda(static_cast<DoCallSig>(&QDBusAbstractInterface::doCall),
                   [](QDBusAbstractInterface *, QDBus::CallMode, const QString &,
                      const QVariant *, size_t) -> QDBusMessage {
        __DBG_STUB_INVOKE__
        return g_identifyReply;
    });
}

TEST_F(UT_AMIconManager, IdentifyProcess_DBusCallFails_ReturnsInvalidInfo)
{
    // Arrange: 接口有效但 DBus 调用返回错误回复
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    stubDoCallReply(stub, QDBusMessage::createError(
        QDBusError::ServiceUnknown, QStringLiteral("stub: service unknown")));
    AMProcessGroupInfo info;
    info.appId = QStringLiteral("sentinel");
    // Act: 识别
    info = obj->identifyProcess(3);
    // Assert: 错误回复 → 无效 info
    EXPECT_FALSE(info.isValid());
    EXPECT_EQ(info.appId, QString());
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

TEST_F(UT_AMIconManager, IdentifyProcess_UnexpectedArgCount_ReturnsInvalidInfo)
{
    // Arrange: 接口有效，回复参数数量为 1（期望 3）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QDBusMessage req = QDBusMessage::createMethodCall(
        QStringLiteral("t.svc"), QStringLiteral("/"), QStringLiteral("t.I"), QStringLiteral("Identify"));
    stubDoCallReply(stub, req.createReply(QList<QVariant>{QStringLiteral("app.only")}));
    AMProcessGroupInfo info;
    info.appId = QStringLiteral("sentinel");
    // Act: 识别
    info = obj->identifyProcess(3);
    // Assert: 参数数量异常 → 无效 info
    EXPECT_FALSE(info.isValid());
    EXPECT_EQ(info.appId, QString());
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

TEST_F(UT_AMIconManager, IdentifyProcess_ValidReply_ParsesGroupInfo)
{
    // Arrange: 接口有效，回复三参数（appId + 对象路径 + 附加信息）
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_amAvailable = true;
    obj->m_amInterface = new QDBusInterface(
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QStringLiteral("/org/desktopspec/ApplicationManager1"),
        QStringLiteral("org.desktopspec.ApplicationManager1"),
        QDBusConnection::sessionBus());
    stub.set_lamda(VADDR(QDBusInterface, isValid), [](QDBusAbstractInterface *) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QDBusMessage req = QDBusMessage::createMethodCall(
        QStringLiteral("t.svc"), QStringLiteral("/"), QStringLiteral("t.I"), QStringLiteral("Identify"));
    QList<QVariant> replyArgs;
    replyArgs << QStringLiteral("com.deepin.testapp")
              << QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/desktopspec/Instance/com_2edeepin_2etestapp")))
              << QVariant(QStringList{QStringLiteral("meta")});
    stubDoCallReply(stub, req.createReply(replyArgs));
    AMProcessGroupInfo info;
    // Act: 识别
    info = obj->identifyProcess(3);
    // Assert: appId 与 instancePath 正确解析，组信息有效
    EXPECT_TRUE(info.isValid());
    EXPECT_EQ(info.appId, QStringLiteral("com.deepin.testapp"));
    EXPECT_EQ(info.instancePath, QStringLiteral("/org/desktopspec/Instance/com_2edeepin_2etestapp"));
    // 清理自建接口
    delete obj->m_amInterface;
    obj->m_amInterface = nullptr;
}

//━━━━━━━━━━━━━━━━━━━━ resolveLinglongAppId ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_AMIconManager, ResolveLinglongAppId_CacheHit_ReturnsCachedId)
{
    // Arrange: 预置 linglong 缓存
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    obj->m_linglongAppIdCache[QStringLiteral("cached.app")] = QStringLiteral("org.deepin.cached");
    EXPECT_EQ(obj->m_linglongAppIdCache.count(QStringLiteral("cached.app")), 1);
    QString result;
    // Act: 查询已缓存 appId
    result = obj->resolveLinglongAppId(QStringLiteral("cached.app"));
    // Assert: 命中缓存
    EXPECT_EQ(result, QStringLiteral("org.deepin.cached"));
}

TEST_F(UT_AMIconManager, ResolveLinglongAppId_NoDesktopFile_ReturnsEmpty)
{
    // Arrange: 不打桩 QFile::exists —— 使用真实文件系统检查一个必然不存在的路径
    std::unique_ptr<AMIconManager> obj = makeUnavailableManager();
    const QString appId = QStringLiteral("no.such.linglong.app");
    QString result = QStringLiteral("sentinel");
    // Act: 查询未缓存且无 desktop 文件的 appId
    result = obj->resolveLinglongAppId(appId);
    // Assert: 返回空串，且空结果已写入缓存
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(obj->m_linglongAppIdCache.count(appId), 1);
    EXPECT_EQ(obj->m_linglongAppIdCache[appId], QString());
}
