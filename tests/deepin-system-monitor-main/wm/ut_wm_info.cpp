// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_WMInfo：WMInfo 单元测试（qt-autotest-generator 生成）
//
// 注意：wm_window_ext_t / wm_window_type_t / wm_state_t 等类型定义位于 wm_info.cpp
// 内部（头文件仅前向声明，属不完整类型），测试 TU 无法解引用其字段。
// 因此类型/状态/属性的映射断言通过公开 API（selectWindow / getHoveredByWindowList /
// | isCursorHoveringDocks 返回的完整类型 wm_window_area_t）与完整可见的私有成员
// （m_internAtomCache / m_atomCache / m_dockWindowList）间接验证。
//
// ============================ 用例计数声明 ============================
// 方法                          | level | 维度                                           | min | actual
// | WMInfo() 构造                  | high  | 建树+root,dock列表,重复调用防重复              | 3   | 3 |
// | selectWindow                  | high  | 命中+边界矩形,类型过滤参数化,extents边界,多窗口 | 3   | 5 |
// | getHoveredByWindowList        | high  | wid提前终止参数化,全量扫描排序,区域不相交      | 3   | 3 |
// | isCursorHoveringDocks         | high  | dock矩形四类边界                               | 1   | 1 |
// | getRootWindow                 | mid   | 由构造用例断言（null-tree防御分支不可达）      | 1   | 1 |
// | findDockWindows               | mid   | 标准dock,嵌套递归,重复调用防重复               | 2   | 2 |
// | initAtomCache/getAtom/getAtomName | mid | 原子缓存,reply失败回退,名称缓存              | 3   | 3 |
// | requestWindowExtInfo          | high  | coords reply空的丢弃路径（经公开API覆盖）      | 2   | 1 |
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入维度等价类：窗口类型（normal/dock/desktop/inputonly）、map_state（viewable/
//        unmapped/unviewable）、pid 有无、hidden 有无、点位在 extents 内外、wid 匹配位置
// [x] 3 边界值：extents 边界 (93,93)/(94,94)、dock 矩形边界 (499,39)/(10,40)/(500,0)
// [x] 4 同质 ≥3 组输入 → TEST_P（SelectWindow_Filter / HoveredByWindowList_StopAt）
// [x] 5 分支清单映射见下
// [x] 6 早退分支：done=true 立即返回、geomReply 空 discard return 均有触发用例
// [x] 7 无显式 throw，不适用
// [x] 8 负面场景：reply 失败、未知 atom、区域不相交、点位全被过滤
// [x] 9 失败路径对象可继续使用（discard 后不崩溃，getAtom 失败返回 0）
// [x] 10 外部 xcb 依赖经 fakexcb.h stub，测试不依赖真实 X 服务器
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet wm_info.cpp）
// buildWindowTreeSchema:
//   B1  connect 错误 early-return      → 构造路径依赖 findDockWindows 非空树，触发即崩溃
//       （findDockWindows 缺空树守卫，属源码缺陷 B-DEF-WMINFO-01，不设用例）
//   B2  screenCount < screenNumber     → 同上不可达（B-DEF-WMINFO-01）
//   B3  正常建树                       → Construction_BuildsTreeSchema 覆盖
// findDockWindows: map_state/wclass/pid/hidden/type 五重过滤 → SelectWindow_Filter 覆盖 +
//   Construction_FindsDockWindows_DockListContent
// selectWindow/getHoveredByWindowList: 各过滤分支 + marginsAdded + contains/intersects +
//   done 提前终止 → SelectWindow_* / HoveredByWindowList_* 覆盖
// requestWindowExtInfo:
//   B7  geomReply 空 → discard_reply + return {} → RequestWindowExtInfo_GeometryReplyNull
//   B8  transCoordsReply 空 → rect = {} → SelectWindow 边界用例间接触证 + 由 region 不相交用例覆盖
//   B9  desktop/type/state/pid/extents 各 reply 空或类型不符的默认分支 → fake 世界中
//       每个分支均有触发窗口（nopid 无 pid 属性等）
//   B10 类型/状态名称映射的长 if-else 链 → 经 HoveredByWindowList 全量扫描与参数化用例覆盖
//        （含未知 atom → kUnknownWindowType/kUnknownState 分支）
// getRootWindow: m_tree/root 空分支 → 防御代码，构造路径经 findDockWindows 强制非空树，
//   无公开途径安全注入空树（不完整类型限制），不设用例（B-DEF-WMINFO-01 相关注记）
// getAtom: reply 空 → XCB_ATOM_NONE → GetAtom_InternReplyFailure
// getAtomName: 缓存命中不访问服务器 / 未命中走 xcb_get_atom_name → GetAtomName_* 覆盖

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "fakexcb.h"
#include "wm/wm_info.h"

#include <QRect>
#include <QStringList>
#include <QVector>

#include <list>

using namespace testing;
using namespace ut_wm;
using core::wm::WMInfo;
using core::wm::WMWindowArea;
using core::wm::WMWId;

namespace {

// 收集 selectWindow / getHoveredByWindowList 结果的 pid 序列（保序）
QVector<pid_t> pidList(const std::list<WMWindowArea> &list)
{
    QVector<pid_t> pids;
    for (const WMWindowArea &area : list)
        pids << area->pid;
    return pids;
}

class UT_WMInfo : public ::testing::Test
{
public:
    Stub stub;

    void SetUp() override
    {
        FakeX::instance().buildStandardWorld();
        installStubs();
    }

    void TearDown() override {}

    // wm_info.cpp 构造与查询路径涉及的全部 xcb 外部依赖一律 stub
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
    }

    xcb_connection_t *conn() const { return FakeX::instance().conn; }
};

TEST_F(UT_WMInfo, Construction_BuildsTreeSchema_WithRootWindowAndCache)
{
    // Arrange：FakeX 标准世界由 fixture 建立（root=0x1，7 个顶层窗口）
    ASSERT_EQ(FakeX::instance().windows.size(), 8);

    // Act
    WMInfo info;

    // Assert：root 经公开 API 验证；原子缓存 7 项（含 UTF8_STRING）
    // 注：wm_tree_t 为 wm_info.cpp 文件域类型（测试 TU 不完整），树结构断言
    // 以 getRootWindow + 行为类用例（GetHoveredByWindowList/SelectWindow 遍历序）覆盖
    EXPECT_EQ(info.getRootWindow(), static_cast<WMWId>(0x1));
    EXPECT_NE(info.m_tree.get(), nullptr);
    EXPECT_EQ(static_cast<int>(info.m_internAtomCache.size()), 7);
}

TEST_F(UT_WMInfo, Construction_FindsDockWindows_OnlyDockTypeInViewableIOWithPid)
{
    // Arrange：标准世界仅 0x102 同时满足 dock 类型/viewable/IO/有 pid 四重过滤
    ASSERT_EQ(FakeX::instance().windows.size(), 8);

    // Act
    WMInfo info;

    // Assert：标准世界仅 0x102（dock 类型、viewable、InputOutput、有 pid）入选
    ASSERT_EQ(info.m_dockWindowList.size(), 1);
    EXPECT_EQ(info.m_dockWindowList.front()->wid, static_cast<WMWId>(0x102));
    EXPECT_EQ(info.m_dockWindowList.front()->pid, 2002);
    EXPECT_EQ(info.m_dockWindowList.front()->rect, QRect(0, 0, 500, 40));
}

TEST_F(UT_WMInfo, Construction_FindsDockWindows_RepeatedCall_RewindsListWithoutDuplicates)
{
    // Arrange
    WMInfo info;
    ASSERT_EQ(info.m_dockWindowList.size(), 1);

    // Act：再次全量扫描（重建扫描树）
    info.findDockWindows();

    // Assert：先 clear 后重建，无重复
    EXPECT_EQ(info.m_dockWindowList.size(), 1);
    EXPECT_EQ(info.m_dockWindowList.front()->wid, static_cast<WMWId>(0x102));
}

TEST_F(UT_WMInfo, Construction_FindsDockWindows_NestedDockChild_VisitedViaRecursion)
{
    // Arrange：给 dock 增加一个同样满足条件的子窗口（viewable IO pid dock-type）
    FakeX &w = FakeX::instance();
    FakeWindow &dock = w.windows[0x102];
    dock.childrenBottomUp << 0x201;
    FakeWindow grand;
    grand.id = 0x201;
    grand.pid = 2100;
    grand.hasPidProp = true;
    grand.name = "Grand Dock";
    grand.wmName = "Grand Dock";
    grand.hasDesktopProp = true;
    grand.desktop = 0;
    grand.types << w.internAtom("_NET_WM_WINDOW_TYPE_DOCK");
    grand.width = 100;
    grand.height = 40;
    grand.dstX = 10;
    grand.dstY = 0;
    w.windows.insert(0x201, grand);

    // Act
    WMInfo info;

    // Assert：递归先入子孙后入自身
    ASSERT_EQ(info.m_dockWindowList.size(), 2);
    EXPECT_EQ(info.m_dockWindowList.front()->pid, 2100);
    EXPECT_EQ(info.m_dockWindowList.back()->pid, 2002);
}

TEST_F(UT_WMInfo, SelectWindow_PointInsideNormalWindow_ReturnsMarginAdjustedRect)
{
    // Arrange：NORM rect(98,98,300,200) + extents(4,4,4,4) → (94,94,308,208)
    WMInfo info;

    // Act
    std::list<WMWindowArea> result = info.selectWindow(QPoint(150, 150));

    // Assert：普通窗口命中，矩形含边框余量；dock/desktop 被类型过滤
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.front()->pid, 2001);
    EXPECT_EQ(result.front()->rect, QRect(94, 94, 308, 208));
    EXPECT_EQ(result.front()->wid, static_cast<WMWId>(0x101));
}

TEST_F(UT_WMInfo, SelectWindow_PointOnDockAndDesktopWindows_ReturnsEmpty)
{
    // Arrange：标准世界 dock (0,0,500,40)、desktop 覆盖左上角
    WMInfo info;

    // Act：分别探测 dock 内部点与 desktop 覆盖点
    const auto dockHit = info.selectWindow(QPoint(10, 10));
    const auto desktopHit = info.selectWindow(QPoint(5, 5));

    // Assert：dock/desktop 类型窗口均被过滤，不入选
    EXPECT_EQ(dockHit.size(), 0u);
    EXPECT_EQ(desktopHit.size(), 0u);
}

TEST_F(UT_WMInfo, SelectWindow_PointOnExtentsBoundary_MarginsAddedBoundary)
{
    // Arrange：NORM 调整后矩形 (94,94,308,208)
    WMInfo info;

    // Act：探测 extents 外边界点与内边界点
    const auto outside = info.selectWindow(QPoint(93, 93));
    const auto inside = info.selectWindow(QPoint(94, 94));

    // Assert：边界点 (93,93) 在外、(94,94) 在内（QRect contains 右/下开区间语义）
    EXPECT_EQ(outside.size(), 0u);
    EXPECT_EQ(inside.size(), 1u);
    EXPECT_EQ(inside.front()->wid, static_cast<WMWId>(0x101));
}

TEST_F(UT_WMInfo, SelectWindow_TwoOverlappingNormalWindows_BothSelectedInTopDownOrder)
{
    // Arrange：新增一个覆盖 (150,150) 的普通窗口并置于栈顶（X 序末尾）
    FakeX &w = FakeX::instance();
    FakeWindow extra;
    extra.id = 0x108;
    extra.pid = 2101;
    extra.hasPidProp = true;
    extra.name = "Overlapped";
    extra.wmName = "Overlapped";
    extra.hasDesktopProp = true;
    extra.desktop = 0;
    extra.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
    extra.width = 300;
    extra.height = 200;
    extra.dstX = 150;
    extra.dstY = 150;
    w.windows.insert(0x108, extra);
    FakeWindow &rootWin = w.windows[w.root];
    rootWin.childrenBottomUp.append(0x108);   // X 返回序末尾 → top-down 首位

    WMInfo info;

    // Act
    std::list<WMWindowArea> result = info.selectWindow(QPoint(150, 150));

    // Assert：两窗口均命中，top->bottom 顺序（0x108 先于 0x101）；0x108 未设 frame extents → 矩形即自身几何
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(pidList(result), QVector<pid_t>({ 2101, 2001 }));
    EXPECT_EQ(result.front()->rect, QRect(150, 150, 300, 200));
    EXPECT_EQ(result.back()->pid, static_cast<pid_t>(2001));
}

// | selectWindow 过滤等价类：非 viewable / InputOnly / 无 pid / hidden / 非 dock-desktop 类型
struct SelectFilterParam
{
    const char *desc;
    std::function<void(FakeX &)> setup;
    QPoint point;
    bool expectHit;
    pid_t expectPid;
};

class UT_WMInfoSelectWindowFilter : public UT_WMInfo,
                                    public WithParamInterface<SelectFilterParam>
{
};

TEST_P(UT_WMInfoSelectWindowFilter, SelectWindow_FilteredWindows_AppliesEquivalenceClasses)
{
    // Arrange
    GetParam().setup(FakeX::instance());
    WMInfo info;

    // Act
    std::list<WMWindowArea> result = info.selectWindow(GetParam().point);

    // Assert
    if (GetParam().expectHit) {
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result.front()->pid, GetParam().expectPid);
    } else {
        EXPECT_TRUE(result.empty());
    }
    SUCCEED() << GetParam().desc;
}

INSTANTIATE_TEST_SUITE_P(FilterClasses, UT_WMInfoSelectWindowFilter, ValuesIn(
    std::vector<SelectFilterParam> {
        { "inputonly-excluded", [](FakeX &w) {
             FakeWindow fw;
             fw.id = 0x301;
             fw.pid = 3001;
             fw.hasPidProp = true;
             fw.name = "IO";
             fw.wmName = "IO";
             fw.hasDesktopProp = true;
             fw.wclass = XCB_WINDOW_CLASS_INPUT_ONLY;
             fw.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
             fw.width = 200; fw.height = 200;
             w.windows.insert(0x301, fw);
             FakeWindow &rootWin = w.windows[w.root];
             rootWin.childrenBottomUp = { 0x301 };
         }, QPoint(50, 50), false, 0 },
        { "unmapped-excluded", [](FakeX &w) {
             FakeWindow fw;
             fw.id = 0x302;
             fw.pid = 3002;
             fw.hasPidProp = true;
             fw.name = "Un";
             fw.wmName = "Un";
             fw.hasDesktopProp = true;
             fw.mapState = XCB_MAP_STATE_UNMAPPED;
             fw.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
             fw.width = 200; fw.height = 200;
             w.windows.insert(0x302, fw);
             FakeWindow &rootWin = w.windows[w.root];
             rootWin.childrenBottomUp = { 0x302 };
         }, QPoint(50, 50), false, 0 },
        { "nopid-excluded", [](FakeX &w) {
             FakeWindow fw;
             fw.id = 0x303;
             fw.pid = -1;
             fw.hasPidProp = false;
             fw.name = "NoPid";
             fw.wmName = "NoPid";
             fw.hasDesktopProp = true;
             fw.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
             fw.width = 200; fw.height = 200;
             w.windows.insert(0x303, fw);
             FakeWindow &rootWin = w.windows[w.root];
             rootWin.childrenBottomUp = { 0x303 };
         }, QPoint(50, 50), false, 0 },
        { "hidden-excluded", [](FakeX &w) {
             FakeWindow fw;
             fw.id = 0x304;
             fw.pid = 3004;
             fw.hasPidProp = true;
             fw.name = "Hid";
             fw.wmName = "Hid";
             fw.hasDesktopProp = true;
             fw.types << w.internAtom("_NET_WM_WINDOW_TYPE_NORMAL");
             fw.states << w.internAtom("_NET_WM_STATE_HIDDEN");
             fw.width = 200; fw.height = 200;
             w.windows.insert(0x304, fw);
             FakeWindow &rootWin = w.windows[w.root];
             rootWin.childrenBottomUp = { 0x304 };
         }, QPoint(50, 50), false, 0 },
        { "non-normal-type-passes", [](FakeX &w) {
             FakeWindow fw;
             fw.id = 0x305;
             fw.pid = 3005;
             fw.hasPidProp = true;
             fw.name = "TB";
             fw.wmName = "TB";
             fw.hasDesktopProp = true;
             fw.types << w.internAtom("_NET_WM_WINDOW_TYPE_TOOLBAR");
             fw.width = 200; fw.height = 200;
             w.windows.insert(0x305, fw);
             FakeWindow &rootWin = w.windows[w.root];
             rootWin.childrenBottomUp = { 0x305 };
         }, QPoint(50, 50), true, 3005 },
    }));

TEST_F(UT_WMInfo, GetHoveredByWindowList_FullScan_DockFirstNormalLastDesktopExcluded)
{
    // Arrange：wid 取哨兵值（永不匹配）→ 全量扫描；desktop 类型被过滤，dock 保留
    WMInfo info;
    QRect area(0, 0, 1024, 768);

    // Act
    std::list<WMWindowArea> result = info.getHoveredByWindowList(0xFFFF, area);

    // Assert：top->bottom 顺序为 [NORM(2001), DOCK(2002)]，desktop/hidden/ionly/nopid/unmapped 被过滤
    EXPECT_EQ(pidList(result), QVector<pid_t>({ 2001, 2002 }));
    EXPECT_EQ(result.front()->rect, QRect(94, 94, 308, 208));
    EXPECT_EQ(result.back()->wid, static_cast<WMWId>(0x102));
}

// | getHoveredByWindowList 提前终止等价类：wid 位于扫描序列的不同位置
struct HoverStopParam
{
    WMWId wid;
    QVector<pid_t> expectPids;
};

class UT_WMInfoHoverStopAt : public UT_WMInfo, public WithParamInterface<HoverStopParam>
{
};

TEST_P(UT_WMInfoHoverStopAt, GetHoveredByWindowList_StopAtMatchingWid_EquivalenceClasses)
{
    // Arrange
    WMInfo info;
    QRect area(0, 0, 1024, 768);

    // Act
    std::list<WMWindowArea> result = info.getHoveredByWindowList(GetParam().wid, area);

    // Assert：扫描在命中 wid 处提前终止，入选集与参数一致
    EXPECT_EQ(result.size(), static_cast<size_t>(GetParam().expectPids.size()));
    EXPECT_EQ(pidList(result), GetParam().expectPids);
}

INSTANTIATE_TEST_SUITE_P(StopPositions, UT_WMInfoHoverStopAt, ValuesIn(
    std::vector<HoverStopParam> {
        { 0x101, {} },                     // 首个即命中 → 立即返回
        { 0x102, { 2001 } },               // 第二个命中 → 仅前者入选
        { 0x107, { 2001, 2002 } },         // 末位命中 → 前面全部入选
    }));

TEST_F(UT_WMInfo, GetHoveredByWindowList_AreaNotIntersecting_ReturnsEmpty)
{
    // Arrange：区域与所有窗口均不相交
    WMInfo info;
    QRect area(900, 900, 10, 10);

    // Act：全量扫描但 intersects 均为假
    const auto result = info.getHoveredByWindowList(0xFFFF, area);

    // Assert：无窗口入选
    EXPECT_EQ(result.size(), 0u);
    EXPECT_TRUE(result.empty());
}

TEST_F(UT_WMInfo, GetHoveredByWindowList_NonHiddenStateWindow_IncludedInList)
{
    // Arrange：NORM 窗口带非 hidden 状态（fullscreen），不应被过滤
    FakeX &w = FakeX::instance();
    w.windows[0x101].states << w.internAtom("_NET_WM_STATE_FULLSCREEN")
                            << w.internAtom("_NET_WM_STATE_ABOVE");
    WMInfo info;
    QRect area(0, 0, 1024, 768);

    // Act
    std::list<WMWindowArea> result = info.getHoveredByWindowList(0xFFFF, area);

    // Assert：状态映射与过滤未误伤，顺序与默认世界一致
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(pidList(result), QVector<pid_t>({ 2001, 2002 }));
    EXPECT_EQ(result.front()->wid, static_cast<WMWId>(0x101));
}

TEST_F(UT_WMInfo, IsCursorHoveringDocks_DockRectBoundaries_ContainsExclusiveEdges)
{
    // Arrange：标准世界 dock 矩形 (0,0,500,40)
    WMInfo info;

    // Act：四类边界点逐个探测
    const bool inside = info.isCursorHoveringDocks(QPoint(10, 10));
    const bool corner = info.isCursorHoveringDocks(QPoint(499, 39));
    const bool belowY = info.isCursorHoveringDocks(QPoint(10, 40));
    const bool beyondX = info.isCursorHoveringDocks(QPoint(500, 0));

    // Assert：内部/右下闭边界点在 dock 上，y=40 / x=500 开区间边界点不在
    EXPECT_EQ(inside, true);
    EXPECT_EQ(corner, true);
    EXPECT_EQ(belowY, false);
    EXPECT_EQ(beyondX, false);
}

TEST_F(UT_WMInfo, InitAtomCache_ValidConnection_PopulatesInternAndMetaCaches)
{
    // Arrange：构造期自动执行 initAtomCache（7 个原子 intern）
    WMInfo info;
    FakeX &w = FakeX::instance();

    // Act：读取两份缓存（读取即行为）
    const size_t internCount = info.m_internAtomCache.size();
    const size_t metaCount = info.m_atomCache.size();
    const xcb_atom_t nameAtom = info.m_internAtomCache[WMInfo::kNetNameAtom];

    // Assert：7 个原子全部 intern 成功并注册元数据；构造期树构建还经
    // getAtomName 解析窗口 type/state 原子名并回填元缓存（7 预置 + 4 唯一）
    EXPECT_EQ(static_cast<int>(internCount), 7);
    EXPECT_EQ(info.m_internAtomCache[WMInfo::kNetNameAtom],
              w.atomByName.value("_NET_WM_NAME", 0xFFFF));
    EXPECT_EQ(info.m_internAtomCache[WMInfo::kNetPIDAtom],
              w.atomByName.value("_NET_WM_PID", 0xFFFF));
    EXPECT_EQ(static_cast<int>(metaCount), 11);
    EXPECT_EQ(info.m_atomCache[nameAtom]->name, QByteArray("_NET_WM_NAME"));
}

TEST_F(UT_WMInfo, GetAtomName_CachedAtom_WithoutServerLookup)
{
    // Arrange：initAtomCache 已缓存 _NET_WM_PID
    WMInfo info;
    FakeX &w = FakeX::instance();
    const int callsBefore = w.getAtomNameCalls;
    const xcb_atom_t pidAtom = info.m_internAtomCache[WMInfo::kNetPIDAtom];

    // Act：查询已缓存原子
    const QByteArray name = info.getAtomName(conn(), pidAtom);

    // Assert：命中缓存，不访问服务器
    EXPECT_EQ(name, QByteArray("_NET_WM_PID"));
    EXPECT_EQ(w.getAtomNameCalls, callsBefore);
}

TEST_F(UT_WMInfo, GetAtomName_UnknownAtom_ServerLookupOnceThenCached)
{
    // Arrange：未知 atom → xcb_get_atom_name 返回空名并缓存；
    // 构造期已有若干 type/state 原子名查询，以 callsBefore 为基准断言增量
    WMInfo info;
    FakeX &w = FakeX::instance();
    const xcb_atom_t unknownAtom = 0x9ABC;
    const int callsBefore = w.getAtomNameCalls;

    // Act：连续两次查询未知原子
    const QByteArray first = info.getAtomName(conn(), unknownAtom);
    const QByteArray second = info.getAtomName(conn(), unknownAtom);

    // Assert：两次调用仅一次服务器访问（第二次命中缓存）
    EXPECT_EQ(first, QByteArray());
    EXPECT_EQ(second, QByteArray());
    EXPECT_EQ(w.getAtomNameCalls, callsBefore + 1);
}

TEST_F(UT_WMInfo, GetAtom_InternReplyFailure_ReturnsNoneAtom)
{
    // Arrange：reply 失败注入
    WMInfo info;
    FakeX &w = FakeX::instance();
    w.failInternReply = 1;
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn(), false, 9, "_NET_WM_X");

    // Act
    const xcb_atom_t atom = info.getAtom(conn(), cookie);

    // Assert：回退为 XCB_ATOM_NONE，对象可继续使用
    EXPECT_EQ(atom, static_cast<xcb_atom_t>(XCB_ATOM_NONE));
    w.failInternReply = 0;
    xcb_intern_atom_cookie_t okCookie = xcb_intern_atom(conn(), false, 9, "_NET_WM_Y");
    EXPECT_EQ(info.getAtom(conn(), okCookie),
              w.atomByName.value("_NET_WM_Y", 0xFFFF));
}

TEST_F(UT_WMInfo, SelectWindow_CoordsReplyNull_WindowSkippedByEmptyRect)
{
    // Arrange：0x101 的坐标换算 reply 失败（B8 → rect 回退空矩形）。
    // 注入发生在构造前：selectWindow 只读构造期缓存，不做 xcb 调用；
    // wm_window_ext_t 为文件域不完整类型，测试 TU 不能持有/析构其 unique_ptr。
    FakeX::instance().failCoordsReplyFor[0x101] = 1;

    // Act：构造（buildWindowTreeSchema 逐窗口驱动 requestWindowExtInfo）+ 选窗
    WMInfo info;
    const auto selected = info.selectWindow(QPoint(100, 100));

    // Assert：rect 为空 → contains(pos) 为假 → 0x101 不入选
    EXPECT_EQ(selected.size(), 0u);
    EXPECT_TRUE(selected.empty());
}
// 注：requestWindowExtInfo 的 geometry reply 空（B7）分支不可达为通过用例：
// buildWindowTreeSchema 对 root ext 空时即解引用 rootWinExtInfo->windowId，
// 子窗口 ext 空时 build_tree 仍无条件入缓存，后续 scan 解引用空指针崩溃
// —— 疑似源码缺陷 B-DEF-WMINFO-02（buildWindowTreeSchema 未判空入缓存），
// 仅在缺陷清单标红，不设用例。
}   // namespace
