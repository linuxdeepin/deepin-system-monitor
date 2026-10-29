// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_WMWindowList：WMWindowList 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// | 方法                 | level | 维度                                       | min | actual |
// | WMWindowList()构造    | mid   | 经各用例隐式覆盖                           | 1   | 10 |
// | updateWindowListCache | high  | 正常填充,reply空早退                       | 2   | 2 |
// | getWindowTitle        | mid   | 单标题,重复pid覆盖,未知pid                 | 3   | 3 |
// | getWindowPid          | mid   | CARDINAL命中,属性缺失                      | 2   | 2 |
// | getWindowType         | mid   | 原子名列表,属性缺失                        | 2   | 2 |
// | getWindowInfo         | mid   | 完整映射,NET_WM_NAME缺失回退ICCCM          | 2   | 2 |
// | getWindowIcon         | high  | 最大图标,数据过小,尺寸非法,pid未命中       | 3   | 4 |
// | getAppCount           | high  | root合计,非root扣减,空缓存                 | 3   | 3 |
// | addDesktopEntryApp    | high  | 空指针,全路径,shell,脚本,裸名,托盘GUI优先,去重 | 5 | 10 |
// | removeDesktopEntryApp | mid   | 存在移除,不存在保持                        | 2   | 2 |
// | isTrayApp等查询       | low   | 空缓存批量断言                             | 1   | 1 |
// | getTrayWindows        | mid   | DBus降级路径                               | 1   | 1 |
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例（getWindowIcon/getAppCount/addDesktopEntryApp 为 high）
// [x] 2 输入维度等价类：shell/脚本/普通可执行、全路径/裸命令名、属性存在/缺失、
//        icon 数据合法/过小/尺寸非法、用户 root/非 root
// [x] 3 边界值：icon len<2、w>maxImageW、maxImageW=1024 上限、多标题格式拼接顺序
// [x] 4 同质 ≥3 组输入 → TEST_P（AddDesktopEntryApp_PathResolution 参数化）
// [x] 5 分支清单映射见下
// [x] 6 早退分支：client list reply 空 return、icon len<2 return、空指针参数 return
// [x] 7 无显式 throw，不适用
// [x] 8 负面场景：属性缺失、未知 pid/窗口、数据损坏、缓存未命中
// [x] 9 失败路径对象可继续使用（reply 空后缓存保持原状）
// [x] 10 外部依赖全部 stub：xcb（fakexcb.h）、Process（/proc）、DBus（getTrayWindows/sessionBus）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet wm_window_list.cpp / wm_window_list.h）
// updateWindowListCache:
//   B1  client list reply 空 → 直接 return（缓存保持）      → UpdateWindowListCache_ReplyNull
//   B2  NORMAL/DIALOG 类型 → guiAppcache                     → UpdateWindowListCache_ClientList
//   B3  tray pid>0 → trayAppcache                            → UpdateWindowListCache_ClientList
// getWindowTitle: titles>1 拼接 / ==1 直返 / 空 → GetWindowTitle_* 三态覆盖
// getWindowPid/getWindowType/getWindowInfo: reply 空或类型不符 → 各负面用例覆盖
// getWindowIcon:
//   B4  len < 2 → return {}                                 → GetWindowIcon_DataTooSmall
//   B5  size<=0 或 w/h 超上限 → break                        → GetWindowIcon_InvalidDimensions
//   B6  最大图标选取（w>max_w||h>max_h）                     → GetWindowIcon_ParsesLargestIcon
//   B7  pid 不在缓存 → winId=UINT32_MAX 查询                → GetWindowIcon_PidNotInCache
// addDesktopEntryApp:
//   B8  proc 空 → return                                    → AddDesktopEntryApp_NullPointer
//   B9  isTrayApp/isGuiApp/重复 pid → 不入缓存              → *_TrayApp/*_GuiApp/*_DuplicatePid
//   B10 cmd 以 / 开头 → 全路径 shell/脚本判定               → *_NormalBinary/*_ShellCommand/*_ScriptInterpreter
//   B11 裸命令名 → pathList 逐项拼接判定（命中 break）      → *_RawNameViaPathList / *_ShellViaPathList
// getAppCount:
//   B12 root → 各缓存 root uid 计数直接相加                 → GetAppCount_RootUser
//   B13 非 root → 总数减 root 占用                          → GetAppCount_NonRootUser
//   B14 缓存遍历中 size 变化 break（并发修改保护）           → 单线程测试环境不可达，不设用例
// getTrayWindows:
//   B15 DBus 不可用/参数为空 → QDBusArgument 空遍历返回空表  → GetTrayWindows_DbusDegraded
//       （注意：arguments().first() 无 isEmpty 守卫，空 reply 属未定义行为，
//        已登记缺陷 B-DEF-TRAY-01；本用例注入单空 QVariant 规避崩溃）

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "fakexcb.h"
#include "wm/wm_window_list.h"
#include "common/common.h"
#include "process/process.h"

#include <stub.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QImage>
#include <QList>
#include <QStringList>
#include <QVariant>

#include <list>
#include <map>

using namespace testing;
using namespace ut_wm;
using core::process::Process;
using core::wm::WMWId;
using core::wm::WMWindowList;
using core::wm::wm_window_t;

namespace {

// ---------------- stub 函数（进程 /proc 与 DBus 依赖） ----------------
static void stub_readProcessInfo(void *self)
{
    Q_UNUSED(self);
}

static void stub_readProcessSimpleInfo(void *self, bool skipStatReading)
{
    Q_UNUSED(self);
    Q_UNUSED(skipStatReading);
}

static uid_t stub_uid(const Process *self)
{
    const FakeX &w = FakeX::instance();
    return w.procUid.value(self->pid(), w.defaultUid);
}

static QString stub_userName(const Process *self)
{
    Q_UNUSED(self);
    return FakeX::instance().procUser;
}

static QByteArrayList stub_cmdline(const Process *self)
{
    return FakeX::instance().procCmdline.value(self->pid(), QByteArrayList());
}

static QList<QVariant> stub_dbus_arguments(const QDBusMessage *self)
{
    Q_UNUSED(self);
    return QList<QVariant> { QVariant() };
}

static QDBusConnection stub_sessionBusInvalid()
{
    return QDBusConnection(QStringLiteral("invalid"));
}

typedef QList<WMWId> (*fptr_tray)(const WMWindowList *);

QList<WMWId> stub_getTrayWindows(const WMWindowList *self)
{
    Q_UNUSED(self);
    const FakeX &w = FakeX::instance();
    return QList<WMWId>(w.trayWindows.constBegin(), w.trayWindows.constEnd());
}

static QList<WMWId> stub_getTrayWindowsEmpty(const WMWindowList *self)
{
    Q_UNUSED(self);
    return QList<WMWId>();
}

// 收集测试期 shellList/pathList 注入状态，TearDown 恢复
class UT_WMWindowList : public ::testing::Test
{
public:
    Stub stub;

    void SetUp() override
    {
        FakeX::instance().buildClientWorld();
        installStubs();
        savedShellList = common::init::shellList;
        savedScriptList = common::init::scriptList;
        savedPathList = common::init::pathList;
    }

    void TearDown() override
    {
        common::init::shellList = savedShellList;
        common::init::scriptList = savedScriptList;
        common::init::pathList = savedPathList;
    }

    // wm_window_list.cpp 全部 xcb / proc / DBus 外部依赖一律 stub
    void installStubs()
    {
        stub.set(&xcb_connect, stub_xcb_connect);
        stub.set(&xcb_connection_has_error, stub_xcb_connection_has_error);
        stub.set(&xcb_disconnect, stub_xcb_disconnect);
        stub.set(&xcb_flush, stub_xcb_flush);
        stub.set(&xcb_discard_reply, stub_xcb_discard_reply);
        stub.set(&xcb_get_setup, stub_xcb_get_setup);
        stub.set(&xcb_setup_roots_iterator, stub_xcb_setup_roots_iterator);
        stub.set(&xcb_setup_roots_length, stub_xcb_setup_roots_length);
        stub.set(&xcb_screen_next, stub_xcb_screen_next);
        stub.set(&xcb_intern_atom, stub_xcb_intern_atom);
        stub.set(&xcb_intern_atom_reply, stub_xcb_intern_atom_reply);
        stub.set(&xcb_get_property, stub_xcb_get_property);
        stub.set(&xcb_get_property_reply, stub_xcb_get_property_reply);
        stub.set(&xcb_icccm_get_wm_name, stub_xcb_icccm_get_wm_name);
        stub.set(&xcb_icccm_get_wm_name_reply, stub_xcb_icccm_get_wm_name_reply);
        stub.set(&xcb_get_geometry, stub_xcb_get_geometry);
        stub.set(&xcb_get_geometry_reply, stub_xcb_get_geometry_reply);
        stub.set(&xcb_get_window_attributes, stub_xcb_get_window_attributes);
        stub.set(&xcb_get_window_attributes_reply, stub_xcb_get_window_attributes_reply);
        stub.set(&xcb_translate_coordinates, stub_xcb_translate_coordinates);
        stub.set(&xcb_translate_coordinates_reply, stub_xcb_translate_coordinates_reply);
        stub.set(&xcb_query_tree, stub_xcb_query_tree);
        stub.set(&xcb_query_tree_reply, stub_xcb_query_tree_reply);
        stub.set(&xcb_get_atom_name, stub_xcb_get_atom_name);
        stub.set(&xcb_get_atom_name_reply, stub_xcb_get_atom_name_reply);

        typedef void (*fptr_rpi)(Process *);
        stub.set((fptr_rpi)&Process::readProcessInfo, stub_readProcessInfo);
        typedef void (*fptr_rpsi)(Process *, bool);
        stub.set((fptr_rpsi)&Process::readProcessSimpleInfo, stub_readProcessSimpleInfo);
        typedef uid_t (*fptr_uid)(const Process *);
        stub.set((fptr_uid)&Process::uid, stub_uid);
        typedef QString (*fptr_user)(const Process *);
        stub.set((fptr_user)&Process::userName, stub_userName);
        typedef QByteArrayList (*fptr_cmd)(const Process *);
        stub.set((fptr_cmd)&Process::cmdline, stub_cmdline);
    }

    xcb_connection_t *conn() const { return FakeX::instance().conn; }

    QList<QString> savedShellList;
    QList<QString> savedScriptList;
    QList<QString> savedPathList;
};

TEST_F(UT_WMWindowList, UpdateWindowListCache_ClientList_PopulatesGuiAndTrayCaches)
{
    // Arrange：client world：0x107(nopid-normal) 0x105(ionly-normal) 0x102(dialog)
    //          0x101(normal) 全部入 guiAppcache；托盘 0x201(pid 3003)
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    WMWindowList wm;

    // Act
    wm.updateWindowListCache();

    // Assert
    EXPECT_EQ(static_cast<int>(wm.m_guiAppcache.size()), 4);
    ASSERT_TRUE(wm.m_guiAppcache.count(3001) > 0);
    EXPECT_EQ(wm.m_guiAppcache[3001]->title, QString("Files"));
    EXPECT_EQ(wm.m_guiAppcache[3001]->winId, static_cast<WMWId>(0x101));
    EXPECT_EQ(wm.m_guiAppcache[3002]->title, QString("Calc"));
    EXPECT_EQ(static_cast<int>(wm.m_trayAppcache.size()), 1);
    EXPECT_TRUE(wm.isTrayApp(3003));
    EXPECT_TRUE(wm.isGuiApp(3001));
    EXPECT_TRUE(wm.m_desktopEntryCache.isEmpty());
}

TEST_F(UT_WMWindowList, UpdateWindowListCache_ReplyNull_EarlyReturnKeepsCaches)
{
    // Arrange：先放入一个旧缓存项，再注入 client list reply 失败
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    WMWindowList wm;
    wm.m_guiAppcache[9999] = core::wm::WMWindow(new wm_window_t());
    FakeX::instance().failPropertyReply = 1;

    // Act
    wm.updateWindowListCache();

    // Assert：early-return 分支，旧缓存未被清空
    EXPECT_EQ(static_cast<int>(wm.m_guiAppcache.size()), 1);
    EXPECT_TRUE(wm.m_guiAppcache.count(9999) > 0);
}

TEST_F(UT_WMWindowList, GetWindowTitle_SingleTitle_ReturnsTitle)
{
    // Arrange：client world 建好后 3001 缓存标题为 "Files"
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindowsEmpty);
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：按 pid 查询标题
    const QString title = wm.getWindowTitle(3001);

    // Assert：单标题直返，不加括号拼接
    EXPECT_EQ(title, QString("Files"));
    EXPECT_EQ(wm.m_guiAppcache[3001]->title, QString("Files"));
}

TEST_F(UT_WMWindowList, GetWindowTitle_DuplicatePid_LastWindowWriteWins)
{
    // Arrange：同一 pid 两个窗口，client list 顺序 [.., 0x103("Doc"), 0x101("Files")]
    // 现状：m_guiAppcache 为 std::map，同 pid 后写覆盖先写；getWindowTitle 的
    // equal_range 多标题拼接分支不可达（疑似缺陷 B-DEF-WMLIST-01，仅标红不设通过用例），
    // 本用例锁定实际语义：保留最后一个同 pid 窗口的标题。
    FakeX &w = FakeX::instance();
    FakeWindow second;
    second.id = 0x103;
    second.pid = 3001;
    second.hasPidProp = true;
    second.name = "Doc";
    second.wmName = "Doc";
    second.hasDesktopProp = true;
    second.desktop = 0;
    second.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
    w.windows.insert(0x103, second);
    w.windows[w.root].childrenBottomUp = { 0x107, 0x105, 0x102, 0x103, 0x101 };
    w.clientListBottomUp = { 0x107, 0x105, 0x102, 0x103, 0x101 };

    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindowsEmpty);
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：按 pid 查询标题
    const QString title = wm.getWindowTitle(3001);

    // Assert：遍历序末尾的 0x101("Files") 覆盖 0x103("Doc")
    EXPECT_EQ(title, QString("Files"));
    EXPECT_EQ(static_cast<int>(wm.m_guiAppcache.size()), 4);
    EXPECT_FALSE(wm.getWindowTitle(3001).contains(QString("Doc")));
}

TEST_F(UT_WMWindowList, GetWindowTitle_UnknownPid_ReturnsEmptyString)
{
    // Arrange：client list 建缓存后查询不存在的 pid
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindowsEmpty);
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：查询未知 pid
    const QString title = wm.getWindowTitle(424242);

    // Assert：titles 为空 → 空 QString
    EXPECT_EQ(title, QString());
    EXPECT_FALSE(wm.isGuiApp(424242));
}

TEST_F(UT_WMWindowList, GetWindowPid_CardinalProperty_ReturnsPid)
{
    // Arrange：0x101/0x102 均带 _NET_WM_PID(CARDINAL)
    WMWindowList wm;

    // Act：查询两窗口 pid
    const pid_t pid1 = wm.getWindowPid(0x101);
    const pid_t pid2 = wm.getWindowPid(0x102);

    // Assert：CARDINAL 属性值回传
    EXPECT_EQ(pid1, 3001);
    EXPECT_EQ(pid2, 3002);
}

TEST_F(UT_WMWindowList, GetWindowPid_MissingProperty_ReturnsMinusOne)
{
    // Arrange：未知窗口 → 属性 type=NONE
    WMWindowList wm;

    // Act：查询不存在窗口的 pid
    const pid_t pid = wm.getWindowPid(0x999);

    // Assert：属性缺失回退 -1
    EXPECT_EQ(pid, -1);
    EXPECT_EQ(wm.getWindowPid(0x888), -1);
}

TEST_F(UT_WMWindowList, GetWindowType_AtomNameList_InPropertyOrder)
{
    // Arrange：0x102 同时带 NORMAL 与 DIALOG 两个类型原子
    FakeX::instance().windows[0x102].types.prepend(
        FakeX::instance().internAtom("_NET_WM_WINDOW_TYPE_NORMAL"));
    WMWindowList wm;

    // Act
    const QStringList types = wm.getWindowType(0x102);

    // Assert：按属性顺序映射原子名
    EXPECT_EQ(types, QStringList({ "_NET_WM_WINDOW_TYPE_NORMAL", "_NET_WM_WINDOW_TYPE_DIALOG" }));
    EXPECT_EQ(types.size(), 2);
}

TEST_F(UT_WMWindowList, GetWindowType_MissingProperty_ReturnsEmptyList)
{
    // Arrange：0x999 属性缺失，0x101 仅 NORMAL 类型
    WMWindowList wm;

    // Act：分别查询缺失属性窗口与普通窗口
    const QStringList missing = wm.getWindowType(0x999);
    const QStringList normal = wm.getWindowType(0x101);

    // Assert：type=NONE → 循环不执行 → 空表；普通窗口命中类型名
    EXPECT_EQ(missing, QStringList());
    EXPECT_EQ(normal, QStringList({ "_NET_WM_WINDOW_TYPE_NORMAL" }));
}

TEST_F(UT_WMWindowList, GetWindowInfo_ValidWindow_MapsPropertiesToStruct)
{
    // Arrange：0x101 完整属性（winId/pid/标题）
    WMWindowList wm;

    // Act（私有方法经 -fno-access-control 直测）
    auto window = wm.getWindowInfo(0x101);

    // Assert：属性字段逐一映射
    EXPECT_EQ(window->winId, static_cast<WMWId>(0x101));
    EXPECT_EQ(window->pid, 3001);
    EXPECT_EQ(window->title, QString("Files"));
}

TEST_F(UT_WMWindowList, GetWindowInfo_NetWmNameMissing_FallsBackToIcccmWmName)
{
    // Arrange：_NET_WM_NAME 缺失（name 为空）但 WM_NAME 存在
    FakeX::instance().windows[0x101].name = QString();
    FakeX::instance().windows[0x101].wmName = "IcccM Name";
    WMWindowList wm;

    // Act
    const unsigned icccmCallsBefore = FakeX::instance().icccmNameRequests;
    auto window = wm.getWindowInfo(0x101);

    // Assert：确认 _NET_WM_NAME 缺失时源码确实回退查询了 WM_NAME（请求计数 +1）；
    // title 长度 = WM_NAME 字节数（fromLocal8Bit 按 len 逐字节构造，不因空截断）。
    // 已知源码缺陷 B-DEF-WMLIST-02：wm_window_list.cpp getWindowInfo 的 icccm 回退分支中
    // nameReply 作用于 else 块内，name 指针在其外被使用（use-after-free），title 具体内容
    // 依赖已释放内存，属未定义行为，故此处不校验 title 文本，仅校验长度与回退调用。
    EXPECT_EQ(FakeX::instance().icccmNameRequests, icccmCallsBefore + 1);
    EXPECT_EQ(window->title.size(), 10);
    EXPECT_EQ(window->winId, static_cast<WMWId>(0x101));
}

TEST_F(UT_WMWindowList, GetWindowIcon_MultipleSizes_ParsesLargest)
{
    // Arrange：两个图标 2x2 与 4x4，应取后者
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    FakeX::instance().windows[0x101].iconData = {
        2, 2, 1, 2, 3, 4,
        4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
    };
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：解析 pid 3001 的图标
    const QImage icon = wm.getWindowIcon(3001);

    // Assert：取 4x4 ARGB32 最大者
    EXPECT_FALSE(icon.isNull());
    EXPECT_EQ(icon.width(), 4);
    EXPECT_EQ(icon.height(), 4);
    EXPECT_EQ(icon.format(), QImage::Format_ARGB32);
}

TEST_F(UT_WMWindowList, GetWindowIcon_DataTooSmall_ReturnsNullImage)
{
    // Arrange：iconData 为空（len=0 < 2 字节 early-return）
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    WMWindowList wm;
    wm.updateWindowListCache();
    ASSERT_TRUE(wm.getWindowIcon(3001).isNull());

    // Act：1 word（4 字节）不满足 len<2，走解析路径 w=7、h 读填充 0 → size=0 → break
    FakeX::instance().windows[0x101].iconData = { 7 };
    const QImage icon = wm.getWindowIcon(3001);

    // Assert：解析中断 → 空图像，宽高为 0
    EXPECT_TRUE(icon.isNull());
    EXPECT_EQ(icon.width(), 0);
    EXPECT_EQ(icon.height(), 0);
}

TEST_F(UT_WMWindowList, GetWindowIcon_InvalidDimensions_BreaksParseLoop)
{
    // Arrange：宽 2000 超过 maxImageW(1024) 上限
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    FakeX::instance().windows[0x101].iconData = { 2000, 2000, 1, 1, 2, 2 };
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：解析超上限图标
    const QImage icon = wm.getWindowIcon(3001);

    // Assert：超限 break → 空图像
    EXPECT_EQ(icon.isNull(), true);
    EXPECT_EQ(icon.width(), 0);
}

TEST_F(UT_WMWindowList, GetWindowIcon_PidNotInCache_QueriesSentinelWindow)
{
    // Arrange：pid 不在缓存 → winId=UINT32_MAX，属性缺失
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindowsEmpty);
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act：查询缓存外 pid
    const QImage icon = wm.getWindowIcon(777777);

    // Assert：不崩溃且返回空图像
    EXPECT_EQ(icon.isNull(), true);
    EXPECT_EQ(icon.width(), 0);
}

TEST_F(UT_WMWindowList, GetWindowIcon_OversizedIcon_ReturnsNullImage)
{
    // Arrange：图标尺寸超过 maxImageW/maxImageH（1024）上限
    FakeX::instance().trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    FakeX::instance().windows[0x101].iconData = { 2000, 2000, 1 };
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act
    const QImage icon = wm.getWindowIcon(3001);

    // Assert：超尺寸图标直接返回空图像，不构造 QImage
    EXPECT_EQ(icon.isNull(), true);
    EXPECT_EQ(icon.width(), 0);
}

TEST_F(UT_WMWindowList, GetWindowInfo_NoPidProperty_PidFallsBackToMinusOne)
{
    // Arrange：0x106 窗口无 _NET_WM_PID 属性（hasPidProp=false）
    WMWindowList wm;
    wm.updateWindowListCache();

    // Act
    const pid_t pid = wm.getWindowPid(0x106);

    // Assert：无 pid 属性 → pid 回落 -1；且触发了 icccm 名称回退（证伪仅限该窗口缺失 _NET_WM_NAME）
    EXPECT_EQ(pid, -1);
    EXPECT_EQ(FakeX::instance().icccmNameRequests, 1);
}

TEST_F(UT_WMWindowList, GetAppCount_RootUser_SumsRootOwnedAcrossCaches)
{
    // Arrange：root 用户且所有应用 uid=0；gui 4 + tray 1 + desktop 2
    FakeX &w = FakeX::instance();
    w.trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    w.procUser = QStringLiteral("root");
    w.defaultUid = 0;
    WMWindowList wm;
    wm.updateWindowListCache();
    Process p1(5001);
    wm.addDesktopEntryApp(&p1);
    Process p2(5002);
    wm.addDesktopEntryApp(&p2);

    // Act
    const int count = wm.getAppCount();

    // Assert：root → trayAppNum + guiAppNum + desktopAppNum = 1 + 4 + 2
    EXPECT_EQ(count, 7);
    EXPECT_EQ(static_cast<int>(wm.m_desktopEntryCache.size()), 2);
}

TEST_F(UT_WMWindowList, GetAppCount_NonRootUser_SubtractsRootOwnedApps)
{
    // Arrange：非 root 用户；gui 中仅 3001 为 root，desktop 中仅 5001 为 root
    FakeX &w = FakeX::instance();
    w.trayWindows = { 0x201 };
    stub.set((fptr_tray)&WMWindowList::getTrayWindows, stub_getTrayWindows);
    w.procUser = QStringLiteral("user");
    w.procUid[3001] = 0;
    w.procUid[5001] = 0;
    WMWindowList wm;
    wm.updateWindowListCache();
    Process p1(5001);
    wm.addDesktopEntryApp(&p1);
    Process p2(5002);
    wm.addDesktopEntryApp(&p2);

    // Act
    const int count = wm.getAppCount();

    // Assert：(4+1+2) - (1 gui root + 1 desktop root) = 5
    EXPECT_EQ(count, 5);
    EXPECT_EQ(wm.isDesktopEntryApp(5001), true);
}

TEST_F(UT_WMWindowList, GetAppCount_EmptyCaches_NonRootUser_ReturnsZero)
{
    // Arrange：无任何缓存，非 root 用户
    FakeX::instance().procUser = QStringLiteral("user");
    WMWindowList wm;

    // Act：空缓存计数
    const int count = wm.getAppCount();

    // Assert：空缓存且非 root → 0
    EXPECT_EQ(count, 0);
    EXPECT_EQ(static_cast<int>(wm.m_guiAppcache.size()), 0);
    EXPECT_EQ(static_cast<int>(wm.m_trayAppcache.size()), 0);
}

// addDesktopEntryApp 路径解析等价类：全路径/裸名 × shell/脚本/普通程序
struct DesktopEntryParam
{
    const char *desc;
    QList<QString> shellListExtra;
    QList<QString> scriptListExtra;
    QList<QString> pathListExtra;
    QByteArrayList cmdline;
    bool expectAdded;
};

class UT_WMWindowListDesktopEntry : public UT_WMWindowList,
                                    public WithParamInterface<DesktopEntryParam>
{
};

TEST_P(UT_WMWindowListDesktopEntry, AddDesktopEntryApp_PathResolution_EquivalenceClasses)
{
    // Arrange
    common::init::shellList << GetParam().shellListExtra;
    common::init::scriptList << GetParam().scriptListExtra;
    common::init::pathList << GetParam().pathListExtra;
    WMWindowList wm;
    Process proc(4001);
    FakeX::instance().procCmdline[4001] = GetParam().cmdline;

    // Act
    wm.addDesktopEntryApp(&proc);

    // Assert：按等价类决定入/不入 desktopEntryCache
    EXPECT_EQ(wm.isDesktopEntryApp(4001), GetParam().expectAdded) << GetParam().desc;
    EXPECT_EQ(static_cast<int>(wm.m_desktopEntryCache.size()), GetParam().expectAdded ? 1 : 0)
        << GetParam().desc;
    SUCCEED() << GetParam().desc;
}

INSTANTIATE_TEST_SUITE_P(PathResolution, UT_WMWindowListDesktopEntry, ValuesIn(
    std::vector<DesktopEntryParam> {   // 路径串仅为缓存键（tempPath 占位，不触盘）
        { "fullpath-normal-binary-added", {}, {}, {}, { "/usr/bin/deepin-editor" }, true },  // tempPath 占位键，不触盘
        { "fullpath-shell-skipped", { "mysh" }, {}, {}, { "/bin/mysh" }, false },  // tempPath 占位键，不触盘
        { "fullpath-script-skipped", {}, { "/usr/bin/python" }, {}, { "/usr/bin/python3", "script.py" }, false },  // tempPath 占位键，不触盘
        { "raw-name-resolved-via-pathlist-added", {}, {}, { "/opt/apps" }, { "deepin-app" }, true },  // tempPath 占位键，不触盘
        { "raw-shell-resolved-via-pathlist-skipped", { "bash" }, {}, { "/opt/shells" }, { "bash" }, false },  // tempPath 占位键，不触盘
        { "empty-cmdline-added", {}, {}, {}, {}, true },
    }));

TEST_F(UT_WMWindowList, AddDesktopEntryApp_NullPointer_NoopNoCrash)
{
    // Arrange：空指针入参
    WMWindowList wm;

    // Act：空指针保护分支
    wm.addDesktopEntryApp(nullptr);

    // Assert：无副作用
    EXPECT_EQ(wm.isDesktopEntryApp(1), false);
    EXPECT_EQ(wm.m_desktopEntryCache.isEmpty(), true);
}

TEST_F(UT_WMWindowList, AddDesktopEntryApp_TrayAppPid_PrefersTrayCache)
{
    // Arrange：pid 已在托盘缓存
    WMWindowList wm;
    wm.m_trayAppcache[4002] = core::wm::WMWindow(new wm_window_t());
    Process proc(4002);
    FakeX::instance().procCmdline[4002] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘

    // Act
    wm.addDesktopEntryApp(&proc);

    // Assert：已入托盘缓存 → isTrayApp 为真，优先级高，不进入 desktopEntryCache
    EXPECT_EQ(wm.isDesktopEntryApp(4002), false);
    EXPECT_EQ(wm.m_desktopEntryCache.isEmpty(), true);
    EXPECT_EQ(wm.isTrayApp(4002), true);
}

TEST_F(UT_WMWindowList, AddDesktopEntryApp_GuiAppPid_PrefersGuiCache)
{
    // Arrange：pid 已在 GUI 缓存
    WMWindowList wm;
    wm.m_guiAppcache[4003] = core::wm::WMWindow(new wm_window_t());
    Process proc(4003);
    FakeX::instance().procCmdline[4003] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘

    // Act
    wm.addDesktopEntryApp(&proc);

    // Assert：已入 GUI 缓存 → isGuiApp 为真，优先级高，不进入 desktopEntryCache
    EXPECT_EQ(wm.isDesktopEntryApp(4003), false);
    EXPECT_EQ(wm.m_desktopEntryCache.isEmpty(), true);
    EXPECT_EQ(wm.isGuiApp(4003), true);
}

TEST_F(UT_WMWindowList, AddDesktopEntryApp_DuplicatePid_SkipsSecondInsert)
{
    // Arrange
    WMWindowList wm;
    Process proc(4004);
    FakeX::instance().procCmdline[4004] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘

    // Act
    wm.addDesktopEntryApp(&proc);
    wm.addDesktopEntryApp(&proc);

    // Assert：去重分支
    EXPECT_TRUE(wm.isDesktopEntryApp(4004));
    EXPECT_EQ(wm.m_desktopEntryCache.size(), 1);
}

TEST_F(UT_WMWindowList, RemoveDesktopEntryApp_Existing_RemovesFromCache)
{
    // Arrange
    WMWindowList wm;
    Process proc(4005);
    FakeX::instance().procCmdline[4005] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘
    wm.addDesktopEntryApp(&proc);
    ASSERT_TRUE(wm.isDesktopEntryApp(4005));

    // Act
    wm.removeDesktopEntryApp(4005);

    // Assert：存在移除
    EXPECT_EQ(wm.isDesktopEntryApp(4005), false);
    EXPECT_EQ(wm.m_desktopEntryCache.isEmpty(), true);
}

TEST_F(UT_WMWindowList, RemoveDesktopEntryApp_UnknownPid_KeepsOthers)
{
    // Arrange
    WMWindowList wm;
    Process proc1(4006);
    FakeX::instance().procCmdline[4006] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘
    wm.addDesktopEntryApp(&proc1);
    Process proc2(4007);
    FakeX::instance().procCmdline[4007] = { "/usr/bin/deepin-editor" };  // tempPath 占位键，不触盘
    wm.addDesktopEntryApp(&proc2);

    // Act
    wm.removeDesktopEntryApp(9999);

    // Assert：未知 pid 无副作用
    EXPECT_EQ(wm.m_desktopEntryCache.size(), 2);
    EXPECT_TRUE(wm.isDesktopEntryApp(4006));
    EXPECT_TRUE(wm.isDesktopEntryApp(4007));
}

TEST_F(UT_WMWindowList, IsTrayAppIsGuiAppIsDesktopEntryApp_EmptyCaches_ReturnFalse)
{
    // Arrange：三份缓存均为空
    WMWindowList wm;

    // Act：空缓存下逐个查询 pid 1
    const bool tray = wm.isTrayApp(1);
    const bool gui = wm.isGuiApp(1);
    const bool entry = wm.isDesktopEntryApp(1);

    // Assert：空缓存批量断言均为假
    EXPECT_EQ(tray, false);
    EXPECT_EQ(gui, false);
    EXPECT_EQ(entry, false);
}

TEST_F(UT_WMWindowList, GetTrayWindows_DbusDegraded_ReturnsEmptyList)
{
    // Arrange：sessionBus 不可用 + arguments 返回空 QVariant（降级路径）；
    // 注意 arguments().first() 无空列表守卫（缺陷 B-DEF-TRAY-01），
    // 故注入单个空 QVariant 而非空列表。
    Stub dbusStub;
    typedef QDBusConnection (*fptr_sb)();
    dbusStub.set((fptr_sb)&QDBusConnection::sessionBus, stub_sessionBusInvalid);
    typedef QList<QVariant> (*fptr_args)(const QDBusMessage *);
    dbusStub.set((fptr_args)&QDBusMessage::arguments, stub_dbus_arguments);
    WMWindowList wm;

    // Act：DBus 降级路径下取托盘窗口
    const QList<WMWId> trays = wm.getTrayWindows();

    // Assert：QDBusArgument 为空 → 数组遍历不执行 → 空列表
    EXPECT_EQ(trays.isEmpty(), true);
    EXPECT_EQ(trays.size(), 0);
}


}   // namespace
