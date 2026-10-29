// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif.h"
#include "system/wireless.h"
#include "system/nl_link.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>
#include <QProcess>
#include <QSharedDataPointer>

//system
#include <linux/sockios.h>
#include <linux/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netlink/route/link.h>
#include <netlink/addr.h>
#include <cstdarg>
#include <cstring>

using namespace core::system;

/***************************************STUB begin*********************************************/

// wireless 桩：隔离真实无线设备查询
static bool stub_is_wireless_true(void *obj)
{
    Q_UNUSED(obj);
    return true;
}

static bool stub_is_wireless_false(void *obj)
{
    Q_UNUSED(obj);
    return false;
}

static QByteArray stub_essid(void *obj)
{
    Q_UNUSED(obj);
    return QByteArray("ut-essid");
}

static uint8_t stub_link_quality(void *obj)
{
    Q_UNUSED(obj);
    return 70;
}

static uint8_t stub_signal_level(void *obj)
{
    Q_UNUSED(obj);
    return 55;
}

static uint8_t stub_noise_level(void *obj)
{
    Q_UNUSED(obj);
    return 10;
}

// QProcess 桩：避免真实调用外部 iw 进程
static void stub_process_start(void *obj, const QString &program, const QStringList &args)
{
    Q_UNUSED(obj);
    Q_UNUSED(program);
    Q_UNUSED(args);
}

// socket/ioctl 桩：注入 SIOCETHTOOL(SIOCETHROTOOL 拼写保持与内核一致) 与 SIOCGIFHWADDR 结果
static int g_stub_socket_fd = 99;
static int g_stub_ioctl_ret = -1;
static int g_stub_ioctl_request = 0;   // 需要注入数据的请求码
static int g_stub_family = 0;          // SIOCGIFHWADDR 注入的 sa_family
static unsigned char g_stub_mac[6] = {0};
static int g_stub_ethtool_speed = 0;   // SIOCETHTOOL 注入的速率

static int stub_socket(int domain, int type, int protocol)
{
    Q_UNUSED(domain);
    Q_UNUSED(type);
    Q_UNUSED(protocol);
    return g_stub_socket_fd;
}

static int stub_ioctl_inject(int fd, unsigned long request, ...)
{
    Q_UNUSED(fd);
    if (request != static_cast<unsigned long>(g_stub_ioctl_request)) {
        return -1;
    }
    va_list ap;
    va_start(ap, request);
    struct ifreq *ifr = va_arg(ap, struct ifreq *);
    va_end(ap);
    if (!ifr) {
        return -1;
    }
    if (request == SIOCGIFHWADDR) {
        ifr->ifr_hwaddr.sa_family = g_stub_family;
        memcpy(ifr->ifr_hwaddr.sa_data, g_stub_mac, sizeof(g_stub_mac));
    } else if (request == SIOCETHTOOL && ifr->ifr_data) {
        struct ethtool_cmd *ecmd = reinterpret_cast<struct ethtool_cmd *>(ifr->ifr_data);
        ecmd->speed = static_cast<decltype(ecmd->speed)>(g_stub_ethtool_speed);
    }
    return g_stub_ioctl_ret;
}

/***************************************STUB end**********************************************/

class UT_NetifInfo: public ::testing::Test
{
public:
    UT_NetifInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifInfo *m_tester;
};

// 构造：默认状态下链接参数与统计均为空/零值
TEST_F(UT_NetifInfo, NetifInfo_Construction_DefaultsAreZero)
{
    // Arrange
    // SetUp 中已构造 NetifInfo

    // Act
    // 逐一读取默认状态

    // Assert
    EXPECT_EQ(m_tester->index(), 0);
    EXPECT_TRUE(m_tester->ifname().isEmpty());
    EXPECT_TRUE(m_tester->alias().isEmpty());
    EXPECT_EQ(m_tester->mtu(), 0u);
    EXPECT_EQ(m_tester->carrier(), 0);
    EXPECT_EQ(m_tester->speed(), 0u);
    EXPECT_TRUE(m_tester->connectionType().isEmpty());
    EXPECT_FALSE(m_tester->isWireless());
    EXPECT_TRUE(m_tester->addr4InfoList().isEmpty());
    EXPECT_TRUE(m_tester->addr6InfoList().isEmpty());
    EXPECT_EQ(m_tester->recv_bps(), 0.0);
    EXPECT_EQ(m_tester->sent_bps(), 0.0);
    EXPECT_EQ(m_tester->rxPackets(), 0ull);
    EXPECT_EQ(m_tester->txBytes(), 0ull);
}

// 拷贝构造：QSharedData 写时复制——写前共享、写后分离
TEST_F(UT_NetifInfo, NetifInfo_CopyConstructor_CopiesOnWrite)
{
    // Arrange
    m_tester->set_recv_bps(5.0);

    // Act
    NetifInfo copy(*m_tester);

    // Assert
    // 拷贝后共享同一份数据
    EXPECT_EQ(copy.recv_bps(), 5.0);
    // 对副本写入触发写时复制，不影响原对象
    copy.set_recv_bps(7.0);
    EXPECT_EQ(copy.recv_bps(), 7.0);
    EXPECT_EQ(m_tester->recv_bps(), 5.0);
}

// 赋值运算符：自赋值直接返回自身引用
TEST_F(UT_NetifInfo, AssignOperator_SelfAssignment_ReturnsSelfReference)
{
    // Arrange
    m_tester->set_sent_bps(3.0);

    // Act
    NetifInfo &ref = m_tester->operator=(*m_tester);

    // Assert
    EXPECT_EQ(&ref, m_tester);
    EXPECT_EQ(m_tester->sent_bps(), 3.0);
}

// 赋值运算符：异对象赋值共享同一份数据
TEST_F(UT_NetifInfo, AssignOperator_DifferentObject_SharesData)
{
    // Arrange
    m_tester->set_recv_bps(9.0);
    NetifInfo other;

    // Act
    other = *m_tester;

    // Assert
    EXPECT_EQ(other.recv_bps(), 9.0);
    EXPECT_EQ(other.sent_bps(), 0.0);
}

// 速率设置：recv/sent bps 往返一致
TEST_F(UT_NetifInfo, SetRecvBpsAndSentBps_RoundTrip_ReturnsSameValues)
{
    // Arrange
    const qreal kRecv = 125.5;
    const qreal kSent = 62.25;

    // Act
    m_tester->set_recv_bps(kRecv);
    m_tester->set_sent_bps(kSent);

    // Assert
    EXPECT_DOUBLE_EQ(m_tester->recv_bps(), kRecv);
    EXPECT_DOUBLE_EQ(m_tester->sent_bps(), kSent);
}

// 链接信息更新：真实 rtnl_link 内存对象驱动全字段填充
TEST_F(UT_NetifInfo, UpdateLinkInfo_FakeEthLink_PopulatesAllLinkFields)
{
    // Arrange
    // 用 libnl 内存对象构造假以太网链路（不触真实网络）
    struct rtnl_link *link = rtnl_link_alloc();
    ASSERT_NE(link, nullptr);
    rtnl_link_set_name(link, "utfake0");
    rtnl_link_set_ifindex(link, 42);
    rtnl_link_set_mtu(link, 1500);
    rtnl_link_set_txqlen(link, 1000);
    rtnl_link_set_carrier(link, 1);
    rtnl_link_set_operstate(link, 6 /*IF_OPER_UP*/);
    rtnl_link_set_linkmode(link, 0 /*IF_LINK_MODE_DEFAULT*/);
    rtnl_link_set_flags(link, IFF_UP);
    rtnl_link_set_arptype(link, ARPHRD_ETHER);
    rtnl_link_set_ifalias(link, "utalias");
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    struct nl_addr *hw = nl_addr_build(AF_LLC, mac, sizeof(mac));
    rtnl_link_set_addr(link, hw);
    nl_addr_put(hw);
    const unsigned char bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct nl_addr *bc = nl_addr_build(AF_LLC, bcast, sizeof(bcast));
    rtnl_link_set_broadcast(link, bc);
    nl_addr_put(bc);
    rtnl_link_set_stat(link, RTNL_LINK_RX_BYTES, 1234);
    rtnl_link_set_stat(link, RTNL_LINK_RX_PACKETS, 11);
    rtnl_link_set_stat(link, RTNL_LINK_TX_BYTES, 5678);
    rtnl_link_set_stat(link, RTNL_LINK_TX_PACKETS, 13);
    // socket/ioctl 全部失败：wireless / ethtool / hwaddr 子流程走失败分支
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl_inject);
    g_stub_socket_fd = -1;
    g_stub_ioctl_ret = -1;
    g_stub_ioctl_request = 0;

    // Act
    NLLink nlLink(link, false);
    m_tester->updateLinkInfo(&nlLink);

    // Assert
    EXPECT_EQ(m_tester->ifname(), QByteArray("utfake0"));
    EXPECT_EQ(m_tester->index(), 42);
    EXPECT_EQ(m_tester->mtu(), 1500u);
    EXPECT_EQ(m_tester->txqlen(), 1000u);
    EXPECT_EQ(m_tester->carrier(), 1);
    EXPECT_EQ(m_tester->operState(), 6u);
    EXPECT_EQ(m_tester->linkMode(), 0u);
    EXPECT_EQ(m_tester->flags(), static_cast<uint>(IFF_UP));
    EXPECT_EQ(m_tester->arpType(), static_cast<uint>(ARPHRD_ETHER));
    EXPECT_EQ(m_tester->alias(), QByteArray("utalias"));
    // libnl 无 carrier_changes 的内存 setter，默认应为 0
    EXPECT_EQ(m_tester->carrierChanges(), 0u);
    EXPECT_EQ(m_tester->linkAddress(), QByteArray("00:11:22:33:44:55"));
    EXPECT_EQ(m_tester->linkBroadcast(), QByteArray("ff:ff:ff:ff:ff:ff"));
    EXPECT_EQ(m_tester->rxBytes(), 1234ull);
    EXPECT_EQ(m_tester->rxPackets(), 11ull);
    EXPECT_EQ(m_tester->txBytes(), 5678ull);
    EXPECT_EQ(m_tester->txPackets(), 13ull);
    // ioctl 失败分支：非无线、连接类型与速率为空/零
    EXPECT_FALSE(m_tester->isWireless());
    EXPECT_TRUE(m_tester->connectionType().isEmpty());
    EXPECT_EQ(m_tester->speed(), 0u);
    // fake link 已由 hold=false 的 NLLink 析构时 rtnl_link_put 释放，不再重复释放
}

// 链接信息更新：空指针链接直接返回，状态不变
TEST_F(UT_NetifInfo, UpdateLinkInfo_NullLink_KeepsStateUnchanged)
{
    // Arrange
    m_tester->set_recv_bps(4.0);

    // Act
    m_tester->updateLinkInfo(nullptr);

    // Assert
    EXPECT_TRUE(m_tester->ifname().isEmpty());
    EXPECT_EQ(m_tester->index(), 0);
    EXPECT_EQ(m_tester->recv_bps(), 4.0);
}

// 品牌速率更新：SIOCETHTOOL 成功时写入链路速率
TEST_F(UT_NetifInfo, UpdateBrandInfo_EthtoolOk_SetsSpeed)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl_inject);
    g_stub_socket_fd = 99;
    g_stub_ioctl_request = SIOCETHTOOL;
    g_stub_ioctl_ret = 0;
    g_stub_ethtool_speed = 1000;

    // Act
    m_tester->updateBrandInfo();

    // Assert
    EXPECT_EQ(m_tester->d->speed, 1000u);
    EXPECT_EQ(m_tester->speed(), 1000u);
}

// 品牌速率更新：ioctl 失败时保持速率不变
TEST_F(UT_NetifInfo, UpdateBrandInfo_EthtoolFails_KeepsSpeed)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl_inject);
    g_stub_socket_fd = -1;
    g_stub_ioctl_request = SIOCETHTOOL;
    g_stub_ioctl_ret = -1;
    g_stub_ethtool_speed = 0;

    // Act
    m_tester->updateBrandInfo();

    // Assert
    EXPECT_EQ(m_tester->d->speed, 0u);
    EXPECT_EQ(m_tester->speed(), 0u);
}

// 硬件地址更新：SIOCGIFHWADDR 成功时按类型映射连接类型
TEST_F(UT_NetifInfo, UpdateHWAddr_IoctlSucceeds_SetsConnectionType)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl_inject);
    g_stub_socket_fd = 99;
    g_stub_ioctl_request = SIOCGIFHWADDR;
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_ETHER;
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x66};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester->updateHWAddr("utfake0");

    // Assert
    EXPECT_EQ(m_tester->d->conn_type, QByteArray("Ethernet"));
    EXPECT_EQ(m_tester->connectionType(), QByteArray("Ethernet"));
}

// 无线信息更新：无线设备填充 essid 与信号质量字段
TEST_F(UT_NetifInfo, UpdateWirelessInfo_WirelessDevice_PopulatesWirelessFields)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(wireless, is_wireless), stub_is_wireless_true);
    stub.set(ADDR(wireless, essid), stub_essid);
    stub.set(ADDR(wireless, link_quality), stub_link_quality);
    stub.set(ADDR(wireless, signal_levle), stub_signal_level);
    stub.set(ADDR(wireless, noise_level), stub_noise_level);
    // 阻断外部 iw 进程调用：getWirelessSpeed 拿不到输出返回 -1，速率保持不变
    // Qt5 的 QProcess 启动重载成员指针含第 3 参 OpenMode
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
                 ADDR(QProcess, start), stub_process_start);

    // Act
    m_tester->updateWirelessInfo();

    // Assert
    EXPECT_TRUE(m_tester->isWireless());
    EXPECT_EQ(m_tester->essid(), QByteArray("ut-essid"));
    EXPECT_EQ(m_tester->linkQuality(), 70);
    EXPECT_EQ(m_tester->signalLevel(), 55);
    EXPECT_EQ(m_tester->noiseLevel(), 10);
    EXPECT_EQ(m_tester->speed(), 0u);
}

// 无线信息更新：非无线设备标记为非无线
TEST_F(UT_NetifInfo, UpdateWirelessInfo_NonWirelessDevice_MarksNotWireless)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(wireless, is_wireless), stub_is_wireless_false);

    // Act
    m_tester->updateWirelessInfo();

    // Assert
    EXPECT_FALSE(m_tester->isWireless());
    EXPECT_TRUE(m_tester->essid().isEmpty());
}

// 无线速率：外部工具无输出时返回 -1
TEST_F(UT_NetifInfo, GetWirelessSpeed_NoBitrateOutput_ReturnsMinusOne)
{
    // Arrange
    Stub stub;
    stub.set((void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode))
                 ADDR(QProcess, start), stub_process_start);

    // Act
    double speed = m_tester->getWirelessSpeed("utfake0");

    // Act
    double speedEmptyName = m_tester->getWirelessSpeed(QString());

    // Assert
    EXPECT_DOUBLE_EQ(speed, -1.0);
    EXPECT_DOUBLE_EQ(speedEmptyName, -1.0);
}

// 接收统计：各 getter 与 d 私有字段一一对应
TEST_F(UT_NetifInfo, RxGetters_PopulatedFields_ReturnMatchingValues)
{
    // Arrange
    m_tester->d->rx_packets = 1;
    m_tester->d->rx_bytes = 2;
    m_tester->d->rx_errors = 3;
    m_tester->d->rx_dropped = 4;
    m_tester->d->rx_fifo = 5;
    m_tester->d->rx_frame = 6;

    // Act
    const quint64 kPackets = m_tester->rxPackets();
    const quint64 kBytes = m_tester->rxBytes();
    const quint64 kErrors = m_tester->rxErrors();
    const quint64 kDropped = m_tester->rxDropped();
    const quint64 kFifo = m_tester->rxFIFO();
    const quint64 kFrame = m_tester->rxFrame();

    // Assert
    EXPECT_EQ(kPackets, 1ull);
    EXPECT_EQ(kBytes, 2ull);
    EXPECT_EQ(kErrors, 3ull);
    EXPECT_EQ(kDropped, 4ull);
    EXPECT_EQ(kFifo, 5ull);
    EXPECT_EQ(kFrame, 6ull);
}

// 发送统计：各 getter 与 d 私有字段一一对应
TEST_F(UT_NetifInfo, TxGetters_PopulatedFields_ReturnMatchingValues)
{
    // Arrange
    m_tester->d->tx_packets = 1;
    m_tester->d->tx_bytes = 2;
    m_tester->d->tx_errors = 3;
    m_tester->d->tx_dropped = 4;
    m_tester->d->tx_fifo = 5;
    m_tester->d->tx_carrier = 6;
    m_tester->d->collisions = 7;

    // Act
    const quint64 kPackets = m_tester->txPackets();
    const quint64 kBytes = m_tester->txBytes();
    const quint64 kErrors = m_tester->txErrors();
    const quint64 kDropped = m_tester->txDropped();
    const quint64 kFifo = m_tester->txFIFO();
    const quint64 kCarrier = m_tester->txCarrier();
    const quint64 kCollisions = m_tester->collisions();

    // Assert
    EXPECT_EQ(kPackets, 1ull);
    EXPECT_EQ(kBytes, 2ull);
    EXPECT_EQ(kErrors, 3ull);
    EXPECT_EQ(kDropped, 4ull);
    EXPECT_EQ(kFifo, 5ull);
    EXPECT_EQ(kCarrier, 6ull);
    EXPECT_EQ(kCollisions, 7ull);
}

// IPv4 地址列表：更新后原样返回
TEST_F(UT_NetifInfo, UpdateV4AddrInfo_SingleAddress_ListRoundTrip)
{
    // Arrange
    auto addr4 = std::make_shared<struct inet_addr4_t>();
    addr4->family = AF_INET;
    addr4->addr = QByteArray("192.168.1.10");
    addr4->mask = QByteArray("255.255.255.0");
    addr4->bcast = QByteArray("192.168.1.255");
    QList<INet4Addr> addrList {addr4};

    // Act
    m_tester->updateAddr4Info(addrList);

    // Assert
    QList<INet4Addr> result = m_tester->addr4InfoList();
    EXPECT_EQ(result.size(), 1);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.first()->addr, QByteArray("192.168.1.10"));
    EXPECT_EQ(result.first()->mask, QByteArray("255.255.255.0"));
    EXPECT_EQ(result.first()->bcast, QByteArray("192.168.1.255"));
}

// IPv6 地址列表：更新后原样返回
TEST_F(UT_NetifInfo, UpdateV6AddrInfo_SingleAddress_ListRoundTrip)
{
    // Arrange
    auto addr6 = std::make_shared<struct inet_addr6_t>();
    addr6->family = AF_INET6;
    addr6->addr = QByteArray("fe80::1");
    addr6->prefixlen = 64;
    addr6->scope = 2;
    QList<INet6Addr> addrList {addr6};

    // Act
    m_tester->updateAddr6Info(addrList);

    // Assert
    QList<INet6Addr> result = m_tester->addr6InfoList();
    EXPECT_EQ(result.size(), 1);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.first()->addr, QByteArray("fe80::1"));
    EXPECT_EQ(result.first()->prefixlen, 64);
    EXPECT_EQ(result.first()->scope, 2);
}
