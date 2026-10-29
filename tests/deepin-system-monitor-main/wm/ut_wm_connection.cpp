// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_WMConnection：WMConnection 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法                        | level | 维度                                 | min | actual
// | WMConnection(display)       | mid   | display空/非空,X错误,screen越界       | 3   | 4 |
// | ~WMConnection/XDisconnector | mid   | 有效连接断开一次,错误与空连接不断开   | 2   | 3 |
// | xcb_connection/rootWindow/atom | mid | 由构造与析构用例断言                 | 3   | 8 |
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 每个输入维度按等价类划分，每类 ≥ 1 用例（display 空/非空；X 成功/错误；screen 在界/越界）
// [x] 3 每个等价类的边界值显式覆盖（screenCount==screenNumber-1 越界边界）
// [x] 4 同质多组输入用 TEST_P 参数化（无 ≥3 组同质断言输入，不适用）
// [x] 5 分支清单已列出并映射到用例名（见下）
// [x] 6 每条 if/switch/throw/early-return 分支有触发用例（xcb_connect 空返回为库契约不可达，已注明）
// [x] 7 异常路径用 EXPECT_THROW 精确匹配（本类无显式 throw，不适用）
// [x] 8 负面场景（空/越界/类型不符）有专门用例（X 错误、screen 越界）
// [x] 9 负面用例验证强异常安全（错误路径后连接指针为空、atom 为 0）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（外部 xcb 依赖经 fakexcb.h stub）
//
// ============================== 分支清单 ==============================
// （来源：MCP get_code_snippet wm_connection.cpp / wm_connection.h）
// WMConnection(display):
//   B1  display 为空        → xcb_connect(nullptr)          → Construction_DefaultDisplay_...
//   B2  display 非空        → xcb_connect(name)             → Construction_NamedDisplay_...
//   B3  conn 非空且 has_error != 0 → reset + return（atoms 不初始化）→ Construction_XServerError_...
//   B4  screenCount < screenNumber → 越界告警 return（root 不确定）→ Construction_ScreenOutOfRange_...
//   B5  正常路径            → initialize + setup + roots 迭代 → B1/B2 覆盖
// XDisconnector::operator():
//   B6  conn 为 null        → 不 disconnect                  → XServerError（析构空连接）覆盖
//   B7  conn 有效           → xcb_disconnect 恰好一次        → Destruction_ValidConnection_...
//   B8  has_error != 0      → 不 disconnect                  → XServerError（显式 reset）覆盖
//
// 说明：xcb_connect 依据库契约永不返回 nullptr（失败返回错误态连接），
// 对应分支不可达，不设用例；rootWindow() 在越界路径下读取未初始化成员（源码现状），
// 值不确定，该用例不断言其返回值。

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "fakexcb.h"
#include "wm/wm_connection.h"
#include "wm/wm_atom.h"

#include <QByteArray>

using namespace testing;
using namespace ut_wm;
using core::wm::WMAtom;
using core::wm::WMConnection;
using core::wm::WMWId;

namespace {
class UT_WMConnection : public ::testing::Test
{
public:
    Stub stub;

    void SetUp() override
    {
        FakeX::instance().reset();
        installStubs();
    }

    void TearDown() override {}

    // wm_connection.cpp 构造路径涉及的全部 xcb 外部依赖一律 stub
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

TEST_F(UT_WMConnection, Construction_DefaultDisplay_InitializesConnectionAtomsAndRoot)
{
    // Arrange：X 连接成功，1 个 screen，root = 0x42
    FakeX::instance().root = 0x42;
    FakeX::instance().screenStorage[0].root = 0x42;

    // Act
    WMConnection conn;

    // Assert：连接指针、root、WM_STATE atom 与 intern 表一致、screen 编号
    EXPECT_NE(conn.xcb_connection(), nullptr);
    EXPECT_EQ(conn.rootWindow(), static_cast<WMWId>(0x42));
    EXPECT_EQ(conn.atom(WMAtom::WM_STATE),
              FakeX::instance().atomByName.value("WM_STATE", 0xFFFF));
    EXPECT_EQ(conn.m_screenNumber, 0);
    EXPECT_EQ(FakeX::instance().connectCalls, 1);
}

TEST_F(UT_WMConnection, Construction_NamedDisplay_PassesDisplayNameToXcbConnect)
{
    // Arrange：display 非空（":99"）与空（默认）两种边界（B1/B2 对照）
    FakeX::instance().screenNumber = 0;

    // Act：先构造命名 display 连接，再构造默认 display 连接
    WMConnection named(":99");
    WMConnection def;

    // Assert：display 按调用序传递（非空→":99"，空→nullptr 记录为空串）
    EXPECT_EQ(FakeX::instance().displayNames,
              QVector<QByteArray>({ QByteArray(":99"), QByteArray() }));
    EXPECT_EQ(FakeX::instance().connectCalls, 2);
    EXPECT_EQ(def.rootWindow(), static_cast<WMWId>(0x1));
}

TEST_F(UT_WMConnection, Construction_XServerError_ConnectionCleared_NoAtomsInitialized)
{
    // Arrange：xcb_connection_has_error 返回非 0（B3）
    FakeX::instance().connectError = 3;

    // Act
    WMConnection conn;

    // Assert：连接清空、atom 未初始化为 0、错误路径不断开（B6/B8）
    EXPECT_EQ(conn.xcb_connection(), nullptr);
    EXPECT_EQ(conn.atom(WMAtom::WM_STATE), 0);
    EXPECT_EQ(FakeX::instance().disconnectCount, 0);
}

TEST_F(UT_WMConnection, Construction_ScreenOutOfRange_ConnectionKeptAtomsInitialized)
{
    // Arrange：screenNumber=2 > screenCount=1（B4 越界边界）
    FakeX::instance().screenNumber = 2;
    FakeX::instance().screenCount = 1;

    // Act
    WMConnection conn;

    // Assert：连接保留、atom 已初始化（initialize 在 setup 检查之前）；
    // rootWindow() 读取未初始化成员（源码现状），值不确定，不作断言。
    EXPECT_NE(conn.xcb_connection(), nullptr);
    EXPECT_EQ(conn.m_screenNumber, 2);
    EXPECT_EQ(conn.atom(WMAtom::WM_STATE),
              FakeX::instance().atomByName.value("WM_STATE", 0xFFFF));
    EXPECT_EQ(FakeX::instance().disconnectCount, 0);
}

TEST_F(UT_WMConnection, Destruction_ValidConnection_DisconnectsExactlyOnce)
{
    // Arrange：成功连接（B7）
    FakeX::instance().root = 0x77;
    FakeX::instance().screenStorage[0].root = 0x77;

    // Act：作用域内构造，离开作用域触发析构
    {
        WMConnection conn;
        EXPECT_EQ(conn.rootWindow(), static_cast<WMWId>(0x77));
        EXPECT_EQ(FakeX::instance().disconnectCount, 0);
    }

    // Assert：作用域结束析构，xcb_disconnect 恰好调用一次
    EXPECT_EQ(FakeX::instance().disconnectCount, 1);
}
}   // namespace
