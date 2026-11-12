// Copyright (C) 2019 ~ 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/netif_addr_model.h"

//dependence
#include "system/netif.h"

//gtest
#include <gtest/gtest.h>

//qt
#include <QModelIndex>
#include <QVariant>

// private member access：三张地址库为 private（友元为 NetifInfoModel），
// 测试经 addr_pri 注入真实地址对象，避免为测试开放友元
#include "addr_pri.h"

ACCESS_PRIVATE_FIELD(NetifAddrModel, QList<core::system::INetAddr>, m_addr4DB)
ACCESS_PRIVATE_FIELD(NetifAddrModel, QList<core::system::INetAddr>, m_addr6DB)
ACCESS_PRIVATE_FIELD(NetifAddrModel, QList<core::system::INetAddr>, m_addrDB)

using namespace core::system;
using core::system::INetAddr;

// 分支清单（源自 netif_addr_model.cpp / netif_addr_model.h）:
// 1. ctor: 三张地址库空初始化
// 2. rowCount: m_addrDB + m_addr4DB + m_addr6DB 三库求和
// 3. columnCount: 恒返回 kAddrTypeMax(3)
// 4. data B1: !index.isValid() -> 返回空 QVariant
// 5. data B2: role 非 DisplayRole/AccessibleTextRole -> 返回空 QVariant
// 6. data B3: column==kINet4Addr 且 row<m_addr4DB.size() 且指针非空 -> addr4->addr
// 7. data B4: column==kINet4Addr 且 row 越界 -> break 返回空
// 8. data B5: column==kINet6Addr 且 row<m_addr6DB.size() -> addr6->addr
// 9. data B6: column==kOtherAddr 且 row<m_addrDB.size() -> oaddr->addr
// 10. data B7: column 超出枚举 -> default 返回空
//     （防御性死分支：QAbstractTableModel::index 对 column>=columnCount(=3)
//      一律返回无效索引，经公开 API 无法携带越界列抵达 switch，故不设用例）

namespace {

// 注入器：向指定库追加一个真实地址对象（store 为基类指针，data() 内
// reinterpret_cast 还原派生类型，故对象必须按对应派生类型构造）
static void inject4(NetifAddrModel *model, const QByteArray &addr)
{
    INetAddr a {std::make_shared<inet_addr4_t>()};
    reinterpret_cast<inet_addr4_t *>(a.get())->addr = addr;
    reinterpret_cast<inet_addr4_t *>(a.get())->mask = QByteArray("255.255.255.0");
    access_private_field::NetifAddrModelm_addr4DB(*model).append(a);
}

static void inject6(NetifAddrModel *model, const QByteArray &addr)
{
    INetAddr a {std::make_shared<inet_addr6_t>()};
    reinterpret_cast<inet_addr6_t *>(a.get())->addr = addr;
    reinterpret_cast<inet_addr6_t *>(a.get())->prefixlen = 64;
    access_private_field::NetifAddrModelm_addr6DB(*model).append(a);
}

static void injectOther(NetifAddrModel *model, const QByteArray &addr)
{
    INetAddr a {std::make_shared<inet_addr_t>()};
    a->addr = addr;
    access_private_field::NetifAddrModelm_addrDB(*model).append(a);
}

}  // namespace

// 用例台账（DECL）
// | 方法 | 级别 | 因子 | min | actual |
// | ctor | low | 空库初始状态 | 1 | 1 |
// | rowCount | low | 三库求和/空库 | 2 | 2 |
// | columnCount | low | 恒等于 kAddrTypeMax | 1 | 1 |
// | data | high | 无效索引/非法role/三类列命中/越界/未知列 | 7 | 7 |
// | inet4AddrCount/inet6AddrCount/otherAddrCount | low | 分库计数 | 1 | 1 |

class UT_NetifAddrModel : public ::testing::Test
{
public:
    UT_NetifAddrModel() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        m_tester = new NetifAddrModel();
        ASSERT_NE(m_tester, nullptr);
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
    }

protected:
    NetifAddrModel *m_tester;
};

// ───────────────────────── ctor / columnCount ─────────────────────────
TEST_F(UT_NetifAddrModel, Ctor_EmptyDbs_StartsEmpty)
{
    // Arrange: 新建模型，不经任何注入
    NetifAddrModel model;

    // Act: 读取空模型的全套状态
    const int rows = model.rowCount();
    const int columns = model.columnCount();
    const int count4 = model.inet4AddrCount();
    const int count6 = model.inet6AddrCount();
    const int countOther = model.otherAddrCount();

    // Assert: 行数为 0，三个分库计数均为 0，列数为类型上限
    EXPECT_EQ(rows, 0);
    EXPECT_EQ(columns, NetifAddrModel::kAddrTypeMax);
    EXPECT_EQ(count4, 0);
    EXPECT_EQ(count6, 0);
    EXPECT_EQ(countOther, 0);
}

TEST_F(UT_NetifAddrModel, ColumnCount_WithInjectedRows_ReturnsThreeTypes)
{
    // Arrange: 注入混合数据，列数不应受行数影响
    inject4(m_tester, "10.0.0.1");
    inject6(m_tester, "fe80::1");

    // Act
    const int columns = m_tester->columnCount();

    // Assert: 恒为地址类型枚举上限 3
    EXPECT_EQ(columns, NetifAddrModel::kAddrTypeMax);
    EXPECT_EQ(columns, 3);
}

// ───────────────────────── rowCount（三库求和）─────────────────────────
TEST_F(UT_NetifAddrModel, RowCount_MixedDbs_ReturnsTotalSum)
{
    // Arrange: 4/6/other 三库分别注入 2/1/1 条
    inject4(m_tester, "10.0.0.1");
    inject4(m_tester, "10.0.0.2");
    inject6(m_tester, "fe80::1");
    injectOther(m_tester, "unix");

    // Act
    const int rows = m_tester->rowCount();

    // Assert: 行数 = 三库之和
    EXPECT_EQ(rows, 4);
    EXPECT_EQ(m_tester->inet4AddrCount(), 2);
    EXPECT_EQ(m_tester->inet6AddrCount(), 1);
    EXPECT_EQ(m_tester->otherAddrCount(), 1);
}

// ───────────────────────── data 分支矩阵 ─────────────────────────
TEST_F(UT_NetifAddrModel, Data_InvalidIndex_ReturnsEmptyVariant)
{
    // Arrange: 无效 QModelIndex（B1）
    const QModelIndex invalid;

    // Act: 同一无效索引分别以两种展示角色取值
    const QVariant display = m_tester->data(invalid, Qt::DisplayRole);
    const QVariant accessible = m_tester->data(invalid, Qt::AccessibleTextRole);

    // Assert: 两种角色均返回空 QVariant
    EXPECT_EQ(display, QVariant());
    EXPECT_EQ(accessible, QVariant());
}

TEST_F(UT_NetifAddrModel, Data_UnsupportedRole_ReturnsEmptyVariant)
{
    // Arrange: 有效索引 + 非 Display/AccessibleText 角色（B2）
    inject4(m_tester, "10.0.0.1");
    const QModelIndex idx = m_tester->index(0, NetifAddrModel::kINet4Addr);
    ASSERT_TRUE(idx.isValid());

    // Act: 同一索引分别以不受支持的角色与 DisplayRole 取值
    const QVariant tooltip = m_tester->data(idx, Qt::ToolTipRole);
    const QVariant display = m_tester->data(idx, Qt::DisplayRole);

    // Assert: 不受支持的角色为空，DisplayRole 正常返回（证明分支由 role 决定）
    EXPECT_EQ(tooltip, QVariant());
    EXPECT_NE(display, QVariant());
}

TEST_F(UT_NetifAddrModel, Data_Inet4Column_InRange_ReturnsAddrString)
{
    // Arrange: 4 库注入一条，命中 B3
    inject4(m_tester, "192.168.1.10");
    const QModelIndex idx = m_tester->index(0, NetifAddrModel::kINet4Addr);
    ASSERT_TRUE(idx.isValid());

    // Act: DisplayRole 与 AccessibleTextRole 均走同一展示分支
    const QVariant display = m_tester->data(idx, Qt::DisplayRole);
    const QVariant accessible = m_tester->data(idx, Qt::AccessibleTextRole);

    // Assert: 返回注入的地址字符串
    EXPECT_EQ(display.toString(), QString("192.168.1.10"));
    EXPECT_EQ(accessible.toString(), QString("192.168.1.10"));
}

TEST_F(UT_NetifAddrModel, Data_Inet4Column_OutOfRange_ReturnsEmptyVariant)
{
    // Arrange: 4 库仅 1 条，行 1 越界（B4）
    inject4(m_tester, "10.0.0.1");
    inject6(m_tester, "fe80::1");  // 保证总行数足够使 index() 有效
    const QModelIndex idx = m_tester->index(1, NetifAddrModel::kINet4Addr);
    ASSERT_TRUE(idx.isValid());

    // Act: 同一列分别取越界行与命中行
    const QVariant outOfRange = m_tester->data(idx, Qt::DisplayRole);
    const QVariant inRange = m_tester->data(m_tester->index(0, NetifAddrModel::kINet4Addr),
                                            Qt::DisplayRole);

    // Assert: 4 库越界为空，命中行正常返回（证明空值源于越界而非列分发）
    EXPECT_EQ(outOfRange, QVariant());
    EXPECT_EQ(inRange.toString(), QString("10.0.0.1"));
}

TEST_F(UT_NetifAddrModel, Data_Inet6Column_InRange_ReturnsAddrString)
{
    // Arrange: 6 库注入一条，命中 B5
    inject6(m_tester, "fe80::abcd");
    const QModelIndex idx = m_tester->index(0, NetifAddrModel::kINet6Addr);
    ASSERT_TRUE(idx.isValid());

    // Act: DisplayRole 与 AccessibleTextRole 同走展示分支
    const QVariant display = m_tester->data(idx, Qt::DisplayRole);
    const QVariant accessible = m_tester->data(idx, Qt::AccessibleTextRole);

    // Assert: 返回注入的 IPv6 地址
    EXPECT_EQ(display.toString(), QString("fe80::abcd"));
    EXPECT_EQ(accessible.toString(), QString("fe80::abcd"));
}

TEST_F(UT_NetifAddrModel, Data_OtherColumn_InRange_ReturnsAddrString)
{
    // Arrange: other 库注入一条，命中 B6
    injectOther(m_tester, "packet");
    const QModelIndex idx = m_tester->index(0, NetifAddrModel::kOtherAddr);
    ASSERT_TRUE(idx.isValid());

    // Act: DisplayRole 与 AccessibleTextRole 同走展示分支
    const QVariant display = m_tester->data(idx, Qt::DisplayRole);
    const QVariant accessible = m_tester->data(idx, Qt::AccessibleTextRole);

    // Assert: 返回注入的非 IP 地址
    EXPECT_EQ(display.toString(), QString("packet"));
    EXPECT_EQ(accessible.toString(), QString("packet"));
}
