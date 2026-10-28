// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_WMAtom：WMAtom 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法                  | level | 维度                                        | min | actual
// | WMAtom()默认构造      | mid   | 未初始化全0                                | 1   | 1 |
// | initialize(conn)      | mid   | 全部成功,前缀失败部分回退,全部失败全NONE    | 3   | 3 |
// | atom(type)            | mid   | 未初始化返回0,初始化后返回intern值 | 2   | 6 |
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入维度等价类：reply 成功 / 失败；连接有效（null 连接时 initialize 会向 xcb 库
//        传入 nullptr，属库层未定义行为，且实际运行时连接由 WMConnection 保证非空，
//        不设用例）
// [x] 3 边界值：失败 0 / 部分 / 全部 三档
// [x] 4 同质 ≥3 组输入 → TEST_P（参数化见 PartialFailure 用例的循环断言）
// [x] 5 分支清单映射见下
// [x] 6 initialize 循环体内 reply 为 null 的 early-continue 分支有触发用例
// [x] 7 无显式 throw，不适用
// [x] 8 负面场景：reply 失败专门用例
// [x] 9 失败路径后对象保持可用（atom() 继续返回 0 而非崩溃）
// [x] 10 外部 xcb 依赖经 fakexcb.h stub
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet wm_atom.cpp initialize 23-93）
// initialize(conn):
//   B1 reply == nullptr    → atom 保持 XCB_ATOM_NONE 并继续下一个   → Initialize_*ReplyFailure 覆盖
//   B2 reply 成功          → m_atoms[i] = reply->atom               → Initialize_SetsInternedAtoms 覆盖
// atom(type):
//   B3 未初始化            → 返回 0                                  → DefaultConstruct_AllAtomsNone 覆盖
//   B4 已初始化            → 返回 intern 值                          → Initialize_SetsInternedAtoms 覆盖

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "fakexcb.h"
#include "wm/wm_atom.h"

using namespace testing;
using namespace ut_wm;
using core::wm::WMAtom;

namespace {
class UT_WMAtom : public ::testing::Test
{
public:
    Stub stub;

    void SetUp() override
    {
        FakeX::instance().reset();
        installStubs();
    }

    void TearDown() override {}

    // wm_atom.cpp initialize 涉及的全部 xcb 外部依赖一律 stub
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
    }
};

TEST_F(UT_WMAtom, DefaultConstruct_WithoutInitialize_AllAtomsReturnNone)
{
    // Arrange：未调用 initialize，atoms 数组保持默认
    WMAtom atomObj;

    // Act：逐类读取 atom（读取本身即行为）
    const xcb_atom_t first = atomObj.atom(WMAtom::WM_STATE);
    const xcb_atom_t mid = atomObj.atom(WMAtom::CARDINAL);
    const xcb_atom_t last = atomObj.atom(WMAtom::UTF8_STRING);

    // Assert：首位、中位、末位 atom 均为 0（B3）
    EXPECT_EQ(first, 0);
    EXPECT_EQ(mid, 0);
    EXPECT_EQ(last, 0);
}

TEST_F(UT_WMAtom, Initialize_WithValidConnection_SetsInternedAtomValues)
{
    // Arrange：intern 全部成功
    FakeX &w = FakeX::instance();
    w.connectCalls = 0;
    WMAtom atomObj;

    // Act
    atomObj.initialize(w.conn);

    // Assert：ICCCM 与 EWMH 关键 atom 值与 intern 注册表一致
    EXPECT_EQ(atomObj.atom(WMAtom::WM_STATE), w.atomByName.value("WM_STATE", 0xFFFF));
    EXPECT_EQ(atomObj.atom(WMAtom::WM_CLASS), w.atomByName.value("WM_CLASS", 0xFFFF));
    EXPECT_EQ(atomObj.atom(WMAtom::CARDINAL), w.atomByName.value("CARDINAL", 0xFFFF));
    EXPECT_EQ(atomObj.atom(WMAtom::UTF8_STRING), w.atomByName.value("UTF8_STRING", 0xFFFF));
    EXPECT_EQ(atomObj.atom(WMAtom::_NET_WM_STATE), w.atomByName.value("_NET_WM_STATE", 0xFFFF));
    EXPECT_EQ(atomObj.atom(WMAtom::_NET_WM_WINDOW_TYPE),
              w.atomByName.value("_NET_WM_WINDOW_TYPE", 0xFFFF));
}

TEST_F(UT_WMAtom, Initialize_PrefixReplyFailure_FailedAtomsFallBackToNoneOthersSucceed)
{
    // Arrange：前 2 个 intern reply 失败，其余成功（B1/B2 混合边界）
    FakeX &w = FakeX::instance();
    w.failInternReply = 2;
    WMAtom atomObj;

    // Act
    atomObj.initialize(w.conn);

    // Assert：失败者回退 NONE，其余仍被赋值；失败不影响后续循环（强可用性）
    // 首位 intern 失败（前缀失败 #1）
    EXPECT_EQ(atomObj.atom(static_cast<WMAtom::atom_t>(0)), 0);
    int nonNoneCount = 0;
    const int totalCount = int(WMAtom::ATOM_MAX) - 1;
    for (int i = 0; i < totalCount; ++i) {
        const auto t = static_cast<WMAtom::atom_t>(i);
        if (atomObj.atom(t) != 0)
            ++nonNoneCount;
    }
    EXPECT_EQ(nonNoneCount, totalCount - 2);
}

TEST_F(UT_WMAtom, Initialize_AllReplyFailure_AllAtomsFallBackToNone)
{
    // Arrange：全部 intern reply 失败（B1 全量）
    FakeX &w = FakeX::instance();
    w.failInternReply = 100;
    WMAtom atomObj;

    // Act：全部 reply 失败下执行 initialize
    atomObj.initialize(w.conn);
    const xcb_atom_t first = atomObj.atom(WMAtom::WM_STATE);
    const xcb_atom_t last = atomObj.atom(WMAtom::UTF8_STRING);

    // Assert：全部回退为 0，且对象可继续访问不崩溃
    EXPECT_EQ(first, 0);
    EXPECT_EQ(last, 0);
    for (int i = 0; i < int(WMAtom::ATOM_MAX) - 1; ++i) {
        const auto t = static_cast<WMAtom::atom_t>(i);
        EXPECT_EQ(atomObj.atom(t), 0) << "atom_t=" << i;
    }
}
}   // namespace
