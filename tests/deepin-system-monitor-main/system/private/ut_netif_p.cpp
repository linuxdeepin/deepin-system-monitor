// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 最小清单（生成前完成情况）：
// | # | 检查项 | 完成情况 |
// | 1 | 每个公开方法 ≥ 1 用例 | ✅ 默认构造/拷贝构造全覆盖；私有字段经 ACCESS_PRIVATE_FIELD 断言 |
// | 2 | 输入维度等价类划分 | ✅ 默认对象/全字段填充对象/含无线 iw_info 对象/含地址列表对象 |
// | 3 | 边界值显式覆盖 | ✅ iw_info 深拷贝指针不等性、QSharedData 引用计数 1→2→1 |
// | 4 | 同质多组输入 TEST_P | ✅ N/A：拷贝语义用独立用例表达更准确（字段组不同、断言焦点不同） |
// | 5 | 分支清单已列出并映射用例 | ✅ 下方分支清单（来源：netif_p.h 实源码：2 构造 + 1 析构，无显式 if； getters 的 iw_info 空指针分支在 ut_netif.cpp/ut_netif_info_db.cpp 侧覆盖） |
// | 6 | 每条 if/循环/早退 return 分支有用例 | ✅ N/A：本类构造/拷贝均为线性初始化列表，无分支语句 |
// | 7 | EXPECT_THROW 精确类型 | ✅ N/A：无异常路径（unique_ptr 深拷贝 new 失败即 terminate，不可测） |
// | 8 | 负面场景专门用例 | ✅ 拷贝独立性（003）：修改副本不影响源对象 |
// | 9 | 负面用例强异常安全 | ✅ 全部通过局部对象/智能指针持有，无裸 new 跨断言存活 |
// | 10 | 打桩选择 | ✅ 无外部依赖（纯数据类），无需打桩；私有访问经 tests/addr_pri.h（friend 注入，非 #define private public） |

// 用例计数声明表（方法 | level | factors | min | actual）
// | NetifInfoPrivate::NetifInfoPrivate | low | - | 1 | 2 |
// | NetifInfoPrivate::NetifInfoPrivate(const&) | low | - | 1 | 3 |
// | NetifInfoPrivate::~NetifInfoPrivate | low | destructor | 1 | （全部用例析构路径间接覆盖） |

//self
#include "system/private/netif_p.h"
#include "system/netif.h"

//gtest
#include "addr_pri.h"
#include <gtest/gtest.h>

//qt/stl
#include <QSharedDataPointer>
#include <memory>

using namespace core::system;

/***************************************PRIVATE ACCESS begin*********************************************/
// 生成 access_private_field::NetifInfoPrivateXxx(obj) 私有成员访问器
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, int, index)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint8_t, carrier)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint8_t, oper_stat)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint8_t, link_mode)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, mtu)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, flags)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, arp_type)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, txqlen)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, carrier_changes)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, uint, speed)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QByteArray, ifname)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QByteArray, alias)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QByteArray, conn_type)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QByteArray, hw_addr)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QByteArray, hw_bcast)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QString, brand)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, qreal, recv_bps)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, qreal, sent_bps)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QList<INet4Addr>, addr4infolst)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, QList<INet6Addr>, addr6infolst)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, std::unique_ptr<iw_info_t>, iw_info)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, bool, isWireless)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_packets)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_bytes)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_errors)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_dropped)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_fifo)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, rx_frame)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_packets)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_bytes)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_errors)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_dropped)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_fifo)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, tx_carrier)
ACCESS_PRIVATE_FIELD(NetifInfoPrivate, unsigned long long, collisions)
// NetifInfo 的 QSharedDataPointer<NetifInfoPrivate> d，用于私有实现 ↔ 公开 getter 映射验证
ACCESS_PRIVATE_FIELD(NetifInfo, QSharedDataPointer<NetifInfoPrivate>, d)
/***************************************PRIVATE ACCESS end***********************************************/

namespace {
// 构造一个填充了全字段组的 NetifInfoPrivate（链路参数/无线扩展/统计计数/地址列表）
static void populatePrivate(NetifInfoPrivate &priv)
{
    access_private_field::NetifInfoPrivateindex(priv) = 3;
    access_private_field::NetifInfoPrivatecarrier(priv) = 1;
    access_private_field::NetifInfoPrivateoper_stat(priv) = 4;   // OPER_STATE_UP
    access_private_field::NetifInfoPrivatelink_mode(priv) = 1;
    access_private_field::NetifInfoPrivatemtu(priv) = 1500u;
    access_private_field::NetifInfoPrivateflags(priv) = 0x1u;
    access_private_field::NetifInfoPrivatearp_type(priv) = 1u;   // ARPHRD_ETHER
    access_private_field::NetifInfoPrivatetxqlen(priv) = 1000u;
    access_private_field::NetifInfoPrivatecarrier_changes(priv) = 2u;
    access_private_field::NetifInfoPrivatespeed(priv) = 10000u;
    access_private_field::NetifInfoPrivateifname(priv) = "uteth0";
    access_private_field::NetifInfoPrivatealias(priv) = "utalias";
    access_private_field::NetifInfoPrivateconn_type(priv) = "ether";
    access_private_field::NetifInfoPrivatehw_addr(priv) = "aa:bb:cc:dd:ee:ff";
    access_private_field::NetifInfoPrivatehw_bcast(priv) = "ff:ff:ff:ff:ff:ff";
    access_private_field::NetifInfoPrivatebrand(priv) = "UT Vendor";
    access_private_field::NetifInfoPrivaterecv_bps(priv) = 1024.5;
    access_private_field::NetifInfoPrivatesent_bps(priv) = 2048.25;
    access_private_field::NetifInfoPrivateisWireless(priv) = true;
    access_private_field::NetifInfoPrivateiw_info(priv)->essid = "ut-ssid";
    // iw_quality.qual/level/noise 在 linux/wireless.h 中均为 __u8（无符号），
    // 取值需在 0..255 范围内才能保证回读一致
    access_private_field::NetifInfoPrivateiw_info(priv)->qual.qual = 70;
    access_private_field::NetifInfoPrivateiw_info(priv)->qual.level = 200;
    access_private_field::NetifInfoPrivateiw_info(priv)->qual.noise = 150;
    access_private_field::NetifInfoPrivateaddr4infolst(priv).append(std::make_shared<inet_addr4_t>());
    access_private_field::NetifInfoPrivateaddr6infolst(priv).append(std::make_shared<inet_addr6_t>());
    access_private_field::NetifInfoPrivaterx_packets(priv) = 11ull;
    access_private_field::NetifInfoPrivaterx_bytes(priv) = 12345ull;
    access_private_field::NetifInfoPrivaterx_errors(priv) = 1ull;
    access_private_field::NetifInfoPrivaterx_dropped(priv) = 2ull;
    access_private_field::NetifInfoPrivaterx_fifo(priv) = 3ull;
    access_private_field::NetifInfoPrivaterx_frame(priv) = 4ull;
    access_private_field::NetifInfoPrivatetx_packets(priv) = 22ull;
    access_private_field::NetifInfoPrivatetx_bytes(priv) = 54321ull;
    access_private_field::NetifInfoPrivatetx_errors(priv) = 5ull;
    access_private_field::NetifInfoPrivatetx_dropped(priv) = 6ull;
    access_private_field::NetifInfoPrivatetx_fifo(priv) = 7ull;
    access_private_field::NetifInfoPrivatetx_carrier(priv) = 8ull;
    access_private_field::NetifInfoPrivatecollisions(priv) = 9ull;
}
} // namespace

class UT_NetifInfoPrivate: public ::testing::Test
{
public:
    UT_NetifInfoPrivate() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifInfoPrivate();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifInfoPrivate *m_tester;
};

// ============================================================================
// 分支清单（来源：netif_p.h 实源码，类为纯数据 QSharedData 私有实现）
// 默认构造：初始化列表线性赋值，无分支语句 → 001 覆盖全字段零值/空值/iw_info 默认分配
// 拷贝构造：初始化列表线性拷贝，iw_info 为 unique_ptr 深拷贝（new）→ 002/003/004 覆盖
//   - 002: 深拷贝字段值相等性
//   - 003: 拷贝独立性（iw_info 指针不等 + 副本修改不回写源）
//   - 004: 空源对象拷贝后 iw_info 仍被分配（unique_ptr(new) 对空源同样成立）
// 析构：unique_ptr 自动释放 → 全部用例析构路径间接覆盖
// QSharedData 引用计数：QSharedDataPointer 拷贝/复位 → 005 覆盖（NetifInfo 隐式共享的根基）
// NetifInfo 公开 getter ↔ 私有字段映射（netif.h inline 实现）→ 006/007 覆盖
// ============================================================================

// 默认构造：全部数值字段零值、字符串空、地址列表空、iw_info 默认分配、isWireless 假
TEST_F(UT_NetifInfoPrivate, initTest_DefaultCtor_ZeroInitializesAllMembers_001)
{
    // Arrange：SetUp 已创建默认对象
    ASSERT_NE(m_tester, nullptr);

    // Act：读取全部私有字段

    // Assert：链路参数零值
    EXPECT_EQ(access_private_field::NetifInfoPrivateindex(*m_tester), 0);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecarrier(*m_tester), 0);
    EXPECT_EQ(access_private_field::NetifInfoPrivateoper_stat(*m_tester), 0);
    EXPECT_EQ(access_private_field::NetifInfoPrivatelink_mode(*m_tester), 0);
    EXPECT_EQ(access_private_field::NetifInfoPrivatemtu(*m_tester), 0u);
    EXPECT_EQ(access_private_field::NetifInfoPrivateflags(*m_tester), 0u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatearp_type(*m_tester), 0u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetxqlen(*m_tester), 0u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecarrier_changes(*m_tester), 0u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatespeed(*m_tester), 0u);
    // Assert：字符串空、地址列表空
    EXPECT_TRUE(access_private_field::NetifInfoPrivateifname(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivatealias(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivateconn_type(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivatehw_addr(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivatehw_bcast(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivatebrand(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivateaddr4infolst(*m_tester).isEmpty());
    EXPECT_TRUE(access_private_field::NetifInfoPrivateaddr6infolst(*m_tester).isEmpty());
    // Assert：速率/无线/统计零值；iw_info 默认分配非空
    EXPECT_DOUBLE_EQ(access_private_field::NetifInfoPrivaterecv_bps(*m_tester), 0);
    EXPECT_DOUBLE_EQ(access_private_field::NetifInfoPrivatesent_bps(*m_tester), 0);
    EXPECT_FALSE(access_private_field::NetifInfoPrivateisWireless(*m_tester));
    EXPECT_NE(access_private_field::NetifInfoPrivateiw_info(*m_tester).get(), nullptr);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_packets(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_bytes(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_errors(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_dropped(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_fifo(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_frame(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_packets(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_bytes(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_errors(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_dropped(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_fifo(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_carrier(*m_tester), 0ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecollisions(*m_tester), 0ull);
}

// 拷贝构造：全字段组深拷贝，值逐一相等
TEST_F(UT_NetifInfoPrivate, test_CopyCtor_DeepCopiesAllFields_002)
{
    // Arrange：源对象填充全字段组
    populatePrivate(*m_tester);

    // Act
    NetifInfoPrivate copy(*m_tester);

    // Assert：链路参数
    EXPECT_EQ(access_private_field::NetifInfoPrivateindex(copy), 3);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecarrier(copy), 1);
    EXPECT_EQ(access_private_field::NetifInfoPrivateoper_stat(copy), 4);
    EXPECT_EQ(access_private_field::NetifInfoPrivatelink_mode(copy), 1);
    EXPECT_EQ(access_private_field::NetifInfoPrivatemtu(copy), 1500u);
    EXPECT_EQ(access_private_field::NetifInfoPrivateflags(copy), 0x1u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatearp_type(copy), 1u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetxqlen(copy), 1000u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecarrier_changes(copy), 2u);
    EXPECT_EQ(access_private_field::NetifInfoPrivatespeed(copy), 10000u);
    // Assert：字符串/品牌/速率
    EXPECT_EQ(access_private_field::NetifInfoPrivateifname(copy), QByteArray("uteth0"));
    EXPECT_EQ(access_private_field::NetifInfoPrivatealias(copy), QByteArray("utalias"));
    EXPECT_EQ(access_private_field::NetifInfoPrivateconn_type(copy), QByteArray("ether"));
    EXPECT_EQ(access_private_field::NetifInfoPrivatehw_addr(copy), QByteArray("aa:bb:cc:dd:ee:ff"));
    EXPECT_EQ(access_private_field::NetifInfoPrivatehw_bcast(copy), QByteArray("ff:ff:ff:ff:ff:ff"));
    EXPECT_EQ(access_private_field::NetifInfoPrivatebrand(copy), QString("UT Vendor"));
    EXPECT_DOUBLE_EQ(access_private_field::NetifInfoPrivaterecv_bps(copy), 1024.5);
    EXPECT_DOUBLE_EQ(access_private_field::NetifInfoPrivatesent_bps(copy), 2048.25);
    // Assert：无线扩展深拷贝内容
    // 拷贝构造初始化列表包含 isWireless{other.isWireless}（netif_p.h），
    // 源对象 populatePrivate 置 true，拷贝后应原值传递
    EXPECT_TRUE(access_private_field::NetifInfoPrivateisWireless(copy));
    ASSERT_NE(access_private_field::NetifInfoPrivateiw_info(copy).get(), nullptr);
    EXPECT_EQ(access_private_field::NetifInfoPrivateiw_info(copy)->essid, QByteArray("ut-ssid"));
    EXPECT_EQ(access_private_field::NetifInfoPrivateiw_info(copy)->qual.qual, 70);
    EXPECT_EQ(access_private_field::NetifInfoPrivateiw_info(copy)->qual.level, 200);
    EXPECT_EQ(access_private_field::NetifInfoPrivateiw_info(copy)->qual.noise, 150);
    // Assert：地址列表
    EXPECT_EQ(access_private_field::NetifInfoPrivateaddr4infolst(copy).size(), 1);
    EXPECT_EQ(access_private_field::NetifInfoPrivateaddr6infolst(copy).size(), 1);
    // Assert：统计计数
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_packets(copy), 11ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_bytes(copy), 12345ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_errors(copy), 1ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_dropped(copy), 2ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_fifo(copy), 3ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_frame(copy), 4ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_packets(copy), 22ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_bytes(copy), 54321ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_errors(copy), 5ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_dropped(copy), 6ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_fifo(copy), 7ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatetx_carrier(copy), 8ull);
    EXPECT_EQ(access_private_field::NetifInfoPrivatecollisions(copy), 9ull);
}

// 拷贝独立性：iw_info 指针不等（unique_ptr 深拷贝），修改副本不影响源
TEST_F(UT_NetifInfoPrivate, test_CopyCtor_IwInfoDeepCopied_IndependentOfSource_003)
{
    // Arrange：源对象填充无线信息
    populatePrivate(*m_tester);
    const iw_info_t *srcIw = access_private_field::NetifInfoPrivateiw_info(*m_tester).get();

    // Act
    NetifInfoPrivate copy(*m_tester);
    iw_info_t *copyIw = access_private_field::NetifInfoPrivateiw_info(copy).get();

    // Assert： iw_info 为独立分配（深拷贝），非共享指针
    EXPECT_NE(srcIw, copyIw);
    // Assert： 修改副本 essid 后源对象不变
    copyIw->essid = "modified";
    EXPECT_EQ(access_private_field::NetifInfoPrivateiw_info(*m_tester)->essid, QByteArray("ut-ssid"));
    // Assert： 修改副本数值字段不影响源
    access_private_field::NetifInfoPrivaterx_bytes(copy) = 999ull;
    EXPECT_EQ(access_private_field::NetifInfoPrivaterx_bytes(*m_tester), 12345ull);
}

// 空源对象拷贝：拷贝后的 iw_info 仍被默认分配（unique_ptr(new) 不依赖源状态）
TEST_F(UT_NetifInfoPrivate, test_CopyCtor_FromDefault_AllocatesOwnIwInfo_004)
{
    // Arrange：源对象为默认构造（SetUp 已创建）
    ASSERT_NE(access_private_field::NetifInfoPrivateiw_info(*m_tester).get(), nullptr);

    // Act
    NetifInfoPrivate copy(*m_tester);

    // Assert：副本 iw_info 非空且与源独立
    EXPECT_NE(access_private_field::NetifInfoPrivateiw_info(copy).get(), nullptr);
    EXPECT_NE(access_private_field::NetifInfoPrivateiw_info(copy).get(),
              access_private_field::NetifInfoPrivateiw_info(*m_tester).get());
}

// QSharedData 引用计数：QSharedDataPointer 拷贝共享实例并使 ref 1→2，复位后回到 1
// （NetifInfo 隐式共享语义的根基，NetifInfoPrivate 继承 QSharedData）
TEST_F(UT_NetifInfoPrivate, test_QSharedDataPointer_RefCountSharedInstance_005)
{
    // Arrange：QSharedDataPointer 持有 SetUp 对象（接管所有权）
    QSharedDataPointer<NetifInfoPrivate> p(m_tester);
    m_tester = nullptr;   // 所有权移交，TearDown 不再删除
    EXPECT_EQ(p.constData()->ref.loadRelaxed(), 1);

    // Act：拷贝 QSharedDataPointer → 共享同一实例
    QSharedDataPointer<NetifInfoPrivate> p2(p);

    // Assert：引用计数增至 2（注意：ref 必须经 constData() 读取，
    // 非 const operator-> 会触发 detach 导致读到克隆对象的计数）
    EXPECT_EQ(p.constData()->ref.loadRelaxed(), 2);
    EXPECT_EQ(p.constData(), p2.constData());

    // Act：复位副本
    p2 = nullptr;

    // Assert：引用计数回落到 1，原对象仍有效
    EXPECT_EQ(p.constData()->ref.loadRelaxed(), 1);
    EXPECT_NE(p.constData(), static_cast<const NetifInfoPrivate *>(nullptr));
}

// NetifInfo 公开 getter ↔ 私有字段映射：将填充的私有实现挂到 NetifInfo::d 上，
// 公开 getter 逐一回读（覆盖 netif.h 中 index/ifname/mtu/carrier/rxPackets 等 inline 映射）
TEST_F(UT_NetifInfoPrivate, test_NetifInfo_Getters_MapPrivateFields_006)
{
    // Arrange：填充私有实现并挂到 NetifInfo
    populatePrivate(*m_tester);
    NetifInfo info;
    access_private_field::NetifInfod(info) = QSharedDataPointer<NetifInfoPrivate>(m_tester);
    m_tester = nullptr;   // 所有权移交 NetifInfo，TearDown 不再删除

    // Act：逐一调用公开 getter 回读

    // Assert：链路参数映射（后续各段同为 getter 回读断言）
    EXPECT_EQ(info.index(), 3);
    EXPECT_EQ(info.ifname(), QByteArray("uteth0"));
    EXPECT_EQ(info.alias(), QByteArray("utalias"));
    EXPECT_EQ(info.mtu(), 1500u);
    EXPECT_EQ(info.flags(), 0x1u);
    EXPECT_EQ(info.arpType(), 1u);
    EXPECT_EQ(info.txqlen(), 1000u);
    EXPECT_EQ(info.operState(), 4);
    EXPECT_EQ(info.linkMode(), 1);
    EXPECT_EQ(info.carrier(), 1);
    EXPECT_EQ(info.carrierChanges(), 2u);
    EXPECT_EQ(info.speed(), 10000u);
    EXPECT_EQ(info.connectionType(), QByteArray("ether"));
    EXPECT_EQ(info.linkAddress(), QByteArray("aa:bb:cc:dd:ee:ff"));
    EXPECT_EQ(info.linkBroadcast(), QByteArray("ff:ff:ff:ff:ff:ff"));
    EXPECT_EQ(info.brand(), QString("UT Vendor"));
    // Act & Assert：无线扩展映射
    EXPECT_TRUE(info.isWireless());
    EXPECT_EQ(info.essid(), QByteArray("ut-ssid"));
    EXPECT_EQ(info.linkQuality(), 70);
    EXPECT_EQ(info.signalLevel(), 200);
    EXPECT_EQ(info.noiseLevel(), 150);
    // Act & Assert：统计计数映射
    EXPECT_EQ(info.rxPackets(), 11ull);
    EXPECT_EQ(info.rxBytes(), 12345ull);
    EXPECT_EQ(info.rxErrors(), 1ull);
    EXPECT_EQ(info.rxDropped(), 2ull);
    EXPECT_EQ(info.rxFIFO(), 3ull);
    EXPECT_EQ(info.rxFrame(), 4ull);
    EXPECT_EQ(info.txPackets(), 22ull);
    EXPECT_EQ(info.txBytes(), 54321ull);
    EXPECT_EQ(info.txErrors(), 5ull);
    EXPECT_EQ(info.txDropped(), 6ull);
    EXPECT_EQ(info.txFIFO(), 7ull);
    EXPECT_EQ(info.txCarrier(), 8ull);
    EXPECT_EQ(info.collisions(), 9ull);
    // Act & Assert：速率字段与地址列表映射
    EXPECT_DOUBLE_EQ(info.recv_bps(), 1024.5);
    EXPECT_DOUBLE_EQ(info.sent_bps(), 2048.25);
    EXPECT_EQ(info.addr4InfoList().size(), 1);
    EXPECT_EQ(info.addr6InfoList().size(), 1);
}

// NetifInfo::set_recv_bps/set_sent_bps 写透到私有字段（公开写入接口唯一路径）
TEST_F(UT_NetifInfoPrivate, test_NetifInfo_SetRecvSentBps_WriteThroughToPrivate_007)
{
    // Arrange：默认私有实现挂到 NetifInfo
    NetifInfo info;
    QSharedDataPointer<NetifInfoPrivate> &dref = access_private_field::NetifInfod(info);
    EXPECT_NE(dref->iw_info.get(), nullptr);

    // Act
    info.set_recv_bps(12.5);
    info.set_sent_bps(3.25);

    // Assert：写入透传到私有实现
    EXPECT_DOUBLE_EQ(dref->recv_bps, 12.5);
    EXPECT_DOUBLE_EQ(dref->sent_bps, 3.25);
}
