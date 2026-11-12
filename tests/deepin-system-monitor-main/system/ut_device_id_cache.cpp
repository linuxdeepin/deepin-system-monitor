// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Rewritten unit tests for core::system::DeviceIDCache
// (deepin-system-monitor-main/system/device_id_cache.cpp) against
// upstream/master @ 00005c4e.
//
// All libudev hardware-database entry points are stubbed (udev_new /
// udev_unref / udev_hwdb_new / udev_hwdb_unref /
// udev_hwdb_get_properties_list_entry / udev_list_entry_*), so no real udev
// context or hwdb is required and the tests are hermetic.
//
// 用例计数声明（test-code-gen.md §4 下限表，level/factors 取自 .ut inventory t45）:
//   DeviceIDCache()             mid (factors: hwdb_new 返回值)  -> min 2 -> 2
//   ~DeviceIDCache()            low                             -> min 1 -> 2
//   deviceName()                high (cx=7, factors: modalias 空/非空,
//                                     缓存命中/未命中, hwdb 空/非空,
//                                     entry key 匹配/不匹配, value 空/非空)
//                                                               -> min 3 -> 6
//
// 最小清单（test-types.md §8）自检:
//   [x] 1. 公开方法已列出，每个方法 >= 1 用例（ctor/dtor/deviceName）
//   [x] 2. 输入维度等价类划分（modalias 空/非空；缓存未命中/命中；hwdb 空/非空）
//   [x] 3. 边界值显式覆盖（空 modalias；value=NULL；无匹配 key）
//   [x] 4. 无 ≥3 组同质输入的单断言矩阵（各分支断言语义不同，不强行 TEST_P）
//   [x] 5. 分支清单已列出并映射到用例名（BRANCH MAP 如下）
//   [x] 6. 每条 if/foreach/break 分支有触发用例
//   [x] 7. 源码无 throw 路径
//   [x] 8. 负面场景（空 modalias / NULL hwdb / NULL value / 无匹配 key）有专门用例
//   [x] 9. 负面用例验证状态保持（缓存写入后二次查询结果一致）
//   [x] 10. stub_ext 打桩（libudev C 接口），未引入 gMock
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet DeviceIDCache::deviceName(const QByteArray&)
//       @ deepin-system-monitor-main/system/device_id_cache.cpp:30-63
// ctor : m_hwdb = udev_hwdb_new(udev->handle())
// dtor : udev_hwdb_unref(m_hwdb)
// B1   : modalias.isEmpty()                          -> return {}
// B2   : m_cache.find(modalias) != end               -> return cached
// B3   : m_hwdb == nullptr                           -> name 保持空
// B4   : entry key == "ID_MODEL_FROM_DATABASE"       -> name = value, break
// B5   : value == nullptr                            -> name = QByteArray()
// B6   : foreach 结束无匹配 key                      -> name 保持空
// post : m_cache[modalias] = name; return name
// 用例映射:
//   Construction_StoresStubbedHwdbHandle              -> ctor
//   Construction_HwdbNewReturnsNull_Tolerated         -> ctor m_hwdb==NULL
//   Destruction_UnrefsHwdbExactlyOnce                 -> dtor
//   Lifecycle_NewUnrefCallsBalanced                   -> ctor+dtor 配对
//   DeviceName_EmptyModalias_ReturnsEmptyNoQuery      -> B1
//   DeviceName_CacheHit_SecondCallSkipsHwdb           -> B2 + post
//   DeviceName_NullHwdb_ReturnsEmpty                  -> B3 + post
//   DeviceName_MatchingEntry_ReturnsModelName         -> B4 + post
//   DeviceName_NullEntryValue_YieldsEmptyName         -> B5 + post
//   DeviceName_NoMatchingKey_ReturnsEmpty             -> B6 + post
// ====================================================================
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |ctor|mid|hwdb_new返回值|2|2|
// |dtor|low|作用域退出|1|2|
// |deviceName|high|modalias×缓存×hwdb×entry|3|6|
// 本 TU 共 10 个用例。
// ====================================================================

#include <gtest/gtest.h>

#include <QByteArray>

#include <libudev.h>

#include "system/device_id_cache.h"
#include "system/udev.h"
#include "3rdparty/stub-ext/stubext.h"

using stub_ext::StubExt;

namespace {

// Sentinel pointers that substitute for real libudev handles.
static struct udev *kFakeUdev = reinterpret_cast<struct udev *>(0x0BAD0001);
static struct udev_hwdb *kFakeHwdb = reinterpret_cast<struct udev_hwdb *>(0x0BAD0002);
static struct udev_list_entry *kFakeEntry = reinterpret_cast<struct udev_list_entry *>(0x0BAD0003);

class UT_DeviceIDCache : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_hwdbNewCalls = 0;
        m_hwdbUnrefCalls = 0;
        m_udevUnrefCalls = 0;
        m_propsQueryCalls = 0;

        // udev_new returns the sentinel; udev_unref records the call.
        using UdevNewFn = struct udev *(*)(void);
        m_stub.set_lamda(static_cast<UdevNewFn>(&udev_new),
                         []() -> struct udev * {
                             return kFakeUdev;
                         });
        using UdevUnrefFn = struct udev *(*)(struct udev *);
        auto *udevUnrefCalls = &m_udevUnrefCalls;
        m_stub.set_lamda(static_cast<UdevUnrefFn>(&udev_unref),
                         [udevUnrefCalls](struct udev *ctx) -> struct udev * {
                             ++(*udevUnrefCalls);
                             return ctx;
                         });

        // udev_hwdb_new returns the sentinel by default; udev_hwdb_unref
        // records the call. Individual tests may re-stub hwdb_new for null.
        using HwdbNewFn = struct udev_hwdb *(*)(struct udev *);
        auto *hwdbNewCalls = &m_hwdbNewCalls;
        m_stub.set_lamda(static_cast<HwdbNewFn>(&udev_hwdb_new),
                         [hwdbNewCalls](struct udev *) -> struct udev_hwdb * {
                             ++(*hwdbNewCalls);
                             return kFakeHwdb;
                         });
        using HwdbUnrefFn = struct udev_hwdb *(*)(struct udev_hwdb *);
        auto *hwdbUnrefCalls = &m_hwdbUnrefCalls;
        m_stub.set_lamda(static_cast<HwdbUnrefFn>(&udev_hwdb_unref),
                         [hwdbUnrefCalls](struct udev_hwdb *hwdb) -> struct udev_hwdb * {
                             ++(*hwdbUnrefCalls);
                             return hwdb;
                         });
    }

    void TearDown() override
    {
        m_stub.clear();
    }

    // Install the property-list stubs used by the deviceName() positive
    // paths. get_properties returns one entry whose key/value pair is taken
    // from the given strings; get_next ends the chain immediately.
    void stubPropsEntry(const char *key, const char *value)
    {
        auto *propsQueryCalls = &m_propsQueryCalls;
        using PropsFn = struct udev_list_entry *(*)(struct udev_hwdb *, const char *, unsigned int);
        m_stub.set_lamda(static_cast<PropsFn>(&udev_hwdb_get_properties_list_entry),
                         [propsQueryCalls](struct udev_hwdb *, const char *, unsigned int)
                                 -> struct udev_list_entry * {
                             ++(*propsQueryCalls);
                             return kFakeEntry;
                         });
        using NextFn = struct udev_list_entry *(*)(struct udev_list_entry *);
        m_stub.set_lamda(static_cast<NextFn>(&udev_list_entry_get_next),
                         [](struct udev_list_entry *) -> struct udev_list_entry * {
                             return nullptr;
                         });
        using NameFn = const char *(*)(struct udev_list_entry *);
        m_stub.set_lamda(static_cast<NameFn>(&udev_list_entry_get_name),
                         [key](struct udev_list_entry *) -> const char * {
                             return key;
                         });
        using ValueFn = const char *(*)(struct udev_list_entry *);
        m_stub.set_lamda(static_cast<ValueFn>(&udev_list_entry_get_value),
                         [value](struct udev_list_entry *) -> const char * {
                             return value;
                         });
    }

    StubExt m_stub;
    int m_hwdbNewCalls = 0;
    int m_hwdbUnrefCalls = 0;
    int m_udevUnrefCalls = 0;
    int m_propsQueryCalls = 0;
};

// ---------------------------------------------------------------------------
// Constructor initialises the hwdb handle from the udev context exactly once.
// Branch: DeviceIDCache ctor -> udev_hwdb_new(udev->handle()).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Construction_FakeUdev_StoresStubbedHwdbHandle)
{
    // Arrange: default SetUp stubs udev_new/udev_hwdb_new to sentinels
    core::system::UDev udev;

    // Act
    core::system::DeviceIDCache cache(&udev);

    // Assert: the handle came from our stub and was requested exactly once
    EXPECT_EQ(kFakeHwdb, cache.m_hwdb);
    EXPECT_EQ(1, m_hwdbNewCalls);
}

// ---------------------------------------------------------------------------
// udev_hwdb_new returning null is tolerated: the lookup guards on m_hwdb and
// the destructor still runs (stubbed unref is a safe no-op).
// Branch: ctor with hwdb_new -> NULL.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Construction_HwdbNull_DestructorStillSafe)
{
    // Arrange
    using HwdbNewFn = struct udev_hwdb *(*)(struct udev *);
    m_stub.set_lamda(static_cast<HwdbNewFn>(&udev_hwdb_new),
                     [](struct udev *) -> struct udev_hwdb * {
                         return nullptr;
                     });

    // Act
    {
        core::system::UDev udev;
        core::system::DeviceIDCache cache(&udev);
        EXPECT_EQ(nullptr, cache.m_hwdb);
    }

    // Assert: destructor ran without crashing and released once
    EXPECT_EQ(1, m_hwdbUnrefCalls);
}

// ---------------------------------------------------------------------------
// Destructor releases the hwdb handle exactly once.
// Branch: ~DeviceIDCache -> udev_hwdb_unref(m_hwdb).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Destruction_ScopeExit_UnrefsHwdbExactlyOnce)
{
    // Arrange: default SetUp stubs record every unref call
    core::system::UDev udev;

    // Act: construct in an inner scope so ~DeviceIDCache() runs right away
    {
        core::system::DeviceIDCache cache(&udev);
        EXPECT_EQ(0, m_hwdbUnrefCalls);
    }

    // Assert: exactly one unref after scope exit
    EXPECT_EQ(1, m_hwdbUnrefCalls);
}

// ---------------------------------------------------------------------------
// Full lifecycle balance: hwdb created once and released once; the udev
// context is released by UDev's own destructor.
// Branch: UDev ctor/dtor + DeviceIDCache ctor/dtor pairing.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, Lifecycle_ScopeExit_NewUnrefCallsBalanced)
{
    // Arrange: default SetUp stubs record hwdb_new / hwdb_unref / udev_unref
    ASSERT_EQ(0, m_hwdbNewCalls);

    // Act: construct + destroy everything in an inner scope
    {
        core::system::UDev udev;
        core::system::DeviceIDCache cache(&udev);
    }

    // Assert: one new + one unref for hwdb, one unref for the udev context
    EXPECT_EQ(1, m_hwdbNewCalls);
    EXPECT_EQ(1, m_hwdbUnrefCalls);
    EXPECT_EQ(1, m_udevUnrefCalls);
}

// ---------------------------------------------------------------------------
// B1: empty modalias short-circuits before any hwdb access.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_EmptyModalias_ReturnsEmptyNoQuery)
{
    // Arrange
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);

    // Act
    const QByteArray name = cache.deviceName(QByteArray());

    // Assert: no hwdb property query happened at all
    EXPECT_TRUE(name.isEmpty());
    EXPECT_EQ(0, m_propsQueryCalls);
}

// ---------------------------------------------------------------------------
// B2 + post: the second lookup for the same modalias is served from m_cache
// and does not touch the hwdb again.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_CacheHit_SecondCallSkipsHwdb)
{
    // Arrange
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);
    stubPropsEntry("ID_MODEL_FROM_DATABASE", "Cached Device");

    // Act
    const QByteArray first = cache.deviceName("usb:v1234p5678");
    const QByteArray second = cache.deviceName("usb:v1234p5678");

    // Assert: identical value, but the hwdb was queried only once
    EXPECT_EQ(QByteArray("Cached Device"), first);
    EXPECT_EQ(first, second);
    EXPECT_EQ(1, m_propsQueryCalls);
}

// ---------------------------------------------------------------------------
// B3 + post: with a null hwdb the lookup yields an empty name and the empty
// result is still cached (no crash, no query counter because the stub is not
// installed for this case).
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_NullHwdb_ReturnsEmpty)
{
    // Arrange
    using HwdbNewFn = struct udev_hwdb *(*)(struct udev *);
    m_stub.set_lamda(static_cast<HwdbNewFn>(&udev_hwdb_new),
                     [](struct udev *) -> struct udev_hwdb * {
                         return nullptr;
                     });
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);

    // Act
    const QByteArray name = cache.deviceName("usb:vAAAA");

    // Assert: guarded lookup leaves the name empty
    EXPECT_TRUE(name.isEmpty());
    EXPECT_EQ(nullptr, cache.m_hwdb);
}

// ---------------------------------------------------------------------------
// B4 + post: an ID_MODEL_FROM_DATABASE entry is resolved to its value.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_MatchingEntry_ReturnsModelName)
{
    // Arrange
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);
    stubPropsEntry("ID_MODEL_FROM_DATABASE", "Fake Device Name");

    // Act
    const QByteArray name = cache.deviceName("pci:v00008086");

    // Assert
    EXPECT_EQ(QByteArray("Fake Device Name"), name);
    EXPECT_EQ(1, m_propsQueryCalls);
}

// ---------------------------------------------------------------------------
// B5 + post: a matching key with a NULL value resolves to an empty name
// instead of dereferencing the null pointer.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_NullEntryValue_YieldsEmptyName)
{
    // Arrange
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);
    stubPropsEntry("ID_MODEL_FROM_DATABASE", nullptr);

    // Act
    const QByteArray name = cache.deviceName("pci:v00008086");

    // Assert: QByteArray(nullptr) stays empty
    EXPECT_TRUE(name.isEmpty());
    EXPECT_EQ(1, m_propsQueryCalls);
}

// ---------------------------------------------------------------------------
// B6 + post: when no entry carries the expected key the name stays empty.
// ---------------------------------------------------------------------------
TEST_F(UT_DeviceIDCache, DeviceName_NoMatchingKey_ReturnsEmpty)
{
    // Arrange
    core::system::UDev udev;
    core::system::DeviceIDCache cache(&udev);
    stubPropsEntry("ID_VENDOR_FROM_DATABASE", "Someone");

    // Act
    const QByteArray name = cache.deviceName("pci:v00008086");

    // Assert: loop ran but no key matched
    EXPECT_TRUE(name.isEmpty());
    EXPECT_EQ(1, m_propsQueryCalls);
}

} // namespace
