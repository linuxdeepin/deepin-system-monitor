// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_packet_capture.h"
#include "system/netif_monitor.h"
#include "system/packet.h"
#include "common/hash.h"
#include <net/if.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <pcap.h>
#include <sys/socket.h>
#include "system/netif_packet_parser.h"
#include <pcap/pcap.h>
#include <vector>
#include <QString>

//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include <QDebug>

using namespace core::system;

// pcap_callback 仅以 friend 形式声明于 NetifPacketCapture，friend 注入对普通查找/ADL 不可见，
// 需在命名空间作用域前置声明后才能在测试中直接调用。
namespace core {
namespace system {
void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet);
}
}

/***************************************STUB begin*********************************************/

size_t stub_strlen()
{
    return IFNAMSIZ+1;
}

void stub_whetherDevChanged()
{
    return;
}

int stub_ioctl (int num, unsigned long int __request, ...)
{
    return -1;
}

char* stub_inet_ntoa(struct in_addr __in)
{
    qInfo()<<"stub_inet_ntoa";
    return nullptr;
}

int stub_pcap_findalldevs(pcap_if_t **, char *)
{
    return 1;
}

int stub_socket (int __domain, int __type, int __protocol)
{
    return -1;
}

bool stub_getCurrentDevName_true()
{
    return true;
}

bool stub_getCurrentDevName_false()
{
    return false;
}

bool stub_hasDevIP_false()
{
    return false;
}

bool stub_hasDevIP_true()
{
    return true;
}

bool stub_parsePacket_false(const pcap_pkthdr *pkt_hdr,
                                    const u_char *packet,
                                    PacketPayload &payload)
{
    return false;
}

pcap_t	*stub_pcap_create(const char *, char *)
{
    return nullptr;
}

pcap_t	*stub_pcap_create_false(const char *, char *)
{
    return nullptr;
}

int	stub_localPendingPackets_64()
{
    return 65;
}
int	stub_localPendingPackets_255()
{
    return 255;
}

/***************************************STUB end**********************************************/

namespace {
// 构造 IPv4/TCP 包：ether + ip + tcp + payload。src/dst 默认 0.0.0.0
std::vector<u_char> makeIPv4TCPPacket(uint16_t sport, uint16_t dport)
{
    const size_t ethLen = sizeof(struct ether_header);
    const size_t ipLen = sizeof(struct ip);
    const size_t tcpLen = 20;
    const size_t payloadLen = 20;
    std::vector<u_char> pkt(ethLen + ipLen + tcpLen + payloadLen, 0);
    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IP);
    auto *iph = reinterpret_cast<struct ip *>(pkt.data() + ethLen);
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_p = IPPROTO_TCP;
    auto *tcp = reinterpret_cast<struct tcphdr *>(pkt.data() + ethLen + ipLen);
    tcp->th_sport = htons(sport);
    tcp->th_dport = htons(dport);
    *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50; // doff=5
    return pkt;
}

// 构造 IPv6/TCP 包：ether + ip6 + tcp + payload。src/dst 默认 ::
std::vector<u_char> makeIPv6TCPPacket(uint16_t sport, uint16_t dport)
{
    const size_t ethLen = sizeof(struct ether_header);
    const size_t ip6Len = sizeof(struct ip6_hdr);
    const size_t tcpLen = 20;
    const size_t payloadLen = 20;
    std::vector<u_char> pkt(ethLen + ip6Len + tcpLen + payloadLen, 0);
    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IPV6);
    auto *ip6 = reinterpret_cast<struct ip6_hdr *>(pkt.data() + ethLen);
    ip6->ip6_nxt = 6; // TCP
    auto *tcp = reinterpret_cast<struct tcphdr *>(pkt.data() + ethLen + ip6Len);
    tcp->th_sport = htons(sport);
    tcp->th_dport = htons(dport);
    *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50;
    return pkt;
}

// 复刻生产代码的哈希计算，向 m_ifaddrsHashCache 插入 src 地址哈希（触发 outbound）
void primeIfaddrHash(NetifPacketCapture *cap, const char *addrStr)
{
    uint64_t hpair[2] {};
    util::common::hash(addrStr, int(strlen(addrStr)), util::common::global_seed, hpair);
    cap->m_ifaddrsHashCache.insert(hpair[0], 0);
}

// 复刻生产代码 pattern 哈希，向 m_sockStats 插入对应 socket inode
void primeSockStat(NetifPacketCapture *cap, const QString &pattern, ino_t ino)
{
    QByteArray fmtbuf = pattern.toLocal8Bit();
    uint64_t cchash[2] {};
    util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
    auto stat = QSharedPointer<struct sock_stat_t>::create();
    stat->ino = ino;
    cap->m_sockStats.insert(cchash[0], stat);
}
} // namespace

class UT_NetifPacketCapture: public ::testing::Test
{
public:
    UT_NetifPacketCapture() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        NetifMonitor *netif = new NetifMonitor;;
        m_tester = new NetifPacketCapture(netif);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifPacketCapture *m_tester;
};

TEST_F(UT_NetifPacketCapture, initTest)
{
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_01)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    m_tester->m_devName = nullptr;
    bool ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_02)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    m_tester->m_devName = "123456789";
    auto ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_03)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    auto ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_04)
{
    Stub stub;
    stub.set(ioctl, stub_ioctl);
    auto ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_05)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);

    stub.set(inet_ntoa, stub_inet_ntoa);
    auto ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_hasDevIP_06)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);

    stub.set(inet_ntoa, stub_inet_ntoa);
    m_tester->m_devName = "12345678901234567";
    auto ok = m_tester->hasDevIP();
    EXPECT_EQ(ok, false);
}

TEST_F(UT_NetifPacketCapture, test_getCurrentDevName_01)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    stub.set(socket, stub_socket);
    auto ok = m_tester->getCurrentDevName();
}

TEST_F(UT_NetifPacketCapture, test_getCurrentDevName_02)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    stub.set(pcap_findalldevs, stub_pcap_findalldevs);
    auto ok = m_tester->getCurrentDevName();
}

TEST_F(UT_NetifPacketCapture, test_getCurrentDevName_03)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    stub.set(pcap_findalldevs, stub_pcap_findalldevs);
    auto ok = m_tester->getCurrentDevName();
}

//TEST_F(UT_NetifPacketCapture, test_getCurrentDevName_06)
//{
//    Stub stub;
//    auto ok = m_tester->getCurrentDevName();
//    EXPECT_EQ(ok, true);
//}

TEST_F(UT_NetifPacketCapture, test_whetherDevChanged_01)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    m_tester->m_devName = nullptr;
    m_tester->whetherDevChanged();
}

TEST_F(UT_NetifPacketCapture, test_whetherDevChanged_02)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, hasDevIP), stub_hasDevIP_false);
    stub.set(ADDR(NetifPacketCapture, getCurrentDevName), stub_getCurrentDevName_false);
    m_tester->whetherDevChanged();
    EXPECT_TRUE(m_tester->m_changedDev);
}

TEST_F(UT_NetifPacketCapture, test_whetherDevChanged_03)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, hasDevIP), stub_hasDevIP_false);
    stub.set(ADDR(NetifPacketCapture, getCurrentDevName), stub_getCurrentDevName_true);
    m_tester->whetherDevChanged();
    EXPECT_TRUE(m_tester->m_changedDev);
}

TEST_F(UT_NetifPacketCapture, test_whetherDevChanged_04)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, hasDevIP), stub_hasDevIP_true);
    m_tester->whetherDevChanged();
    EXPECT_TRUE(m_tester->m_changedDev);
}

TEST_F(UT_NetifPacketCapture, test_startNetifMonitorJob_01)
{
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, whetherDevChanged), stub_whetherDevChanged);
    m_tester->m_devName = nullptr;
    m_tester->startNetifMonitorJob();
}


TEST_F(UT_NetifPacketCapture, test_startNetifMonitorJob_02)
{
    Stub stub;
    stub.set(pcap_create, stub_pcap_create);
    m_tester->startNetifMonitorJob();
}

TEST_F(UT_NetifPacketCapture, test_startNetifMonitorJob_03)
{
    Stub stub;
    m_tester->startNetifMonitorJob();
}

TEST_F(UT_NetifPacketCapture, test_startNetifMonitorJob_04)
{
    Stub stub;
    stub.set(pcap_create, stub_pcap_create);
    m_tester->startNetifMonitorJob();
}

TEST_F(UT_NetifPacketCapture, test_dispatchPackets_01)
{
    m_tester->go = true;
    m_tester->m_devName = nullptr;
    m_tester->dispatchPackets();
}

TEST_F(UT_NetifPacketCapture, test_dispatchPackets_02)
{
    m_tester->go = true;
    m_tester->m_devName = nullptr;
    m_tester->dispatchPackets();
}

TEST_F(UT_NetifPacketCapture, test_dispatchPackets_03)
{
    m_tester->go = true;
    m_tester->m_devName = nullptr;
    Stub stub;
    stub.set(ADDR(PacketPayloadQueue, size), stub_localPendingPackets_64);
    m_tester->dispatchPackets();
}
TEST_F(UT_NetifPacketCapture, test_dispatchPackets_04)
{
    m_tester->go = true;
    Stub stub;
    stub.set(ADDR(PacketPayloadQueue, size), stub_localPendingPackets_64);
    m_tester->dispatchPackets();
}

TEST_F(UT_NetifPacketCapture, test_dispatchPackets_05)
{
    m_tester->go = true;
    Stub stub;
    stub.set(ADDR(PacketPayloadQueue, size), stub_localPendingPackets_255);
    m_tester->dispatchPackets();
}


TEST_F(UT_NetifPacketCapture, test_dispatchPackets_06)
{
    m_tester->go = false;
    m_tester->dispatchPackets();
}

TEST_F(UT_NetifPacketCapture, test_refreshIfAddrsHashCache)
{
    m_tester->refreshIfAddrsHashCache();
}

// ============ pcap_callback（libpcap 抓包回调，free 函数）分支覆盖 ============

// context 为空 => 直接返回
TEST_F(UT_NetifPacketCapture, pcap_callback_NullContext_ReturnsEarly)
{
    auto pkt = makeIPv4TCPPacket(0x1234, 80);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    pcap_callback(nullptr, &hdr, pkt.data()); // 不崩溃即通过
}

// 包解析失败（非 IP）=> parsePacket false => 返回
TEST_F(UT_NetifPacketCapture, pcap_callback_ParseFail_ReturnsEarly)
{
    std::vector<u_char> pkt(sizeof(struct ether_header) + 10, 0);
    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_ARP); // 非 IP，parsePacket 返回 false
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
}

// 地址不匹配本地（ifaddrsHashCache 空）=> 返回
TEST_F(UT_NetifPacketCapture, pcap_callback_NoLocalAddrMatch_ReturnsEarly)
{
    auto pkt = makeIPv4TCPPacket(0x1234, 80);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
}

// 匹配本地地址但无对应 socket（sockStats 空）=> 返回
TEST_F(UT_NetifPacketCapture, pcap_callback_NoMatchingSocket_ReturnsEarly)
{
    auto pkt = makeIPv4TCPPacket(0x1234, 80);
    primeIfaddrHash(m_tester, "0.0.0.0"); // 匹配本地（outbound）
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
}

// IPv4 完整成功路径：匹配本地(outbound) + 匹配 socket(inode) => 入队
TEST_F(UT_NetifPacketCapture, pcap_callback_IPv4_Success_EnqueuesOutbound)
{
    const uint16_t sport = 0x1234, dport = 80;
    auto pkt = makeIPv4TCPPacket(sport, dport);
    primeIfaddrHash(m_tester, "0.0.0.0");
    QString pattern = QString("%1:%2-%3:%4").arg("0.0.0.0").arg(sport).arg("0.0.0.0").arg(dport);
    primeSockStat(m_tester, pattern, 9999);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();

    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // 入队（< LWAT，不触发 dispatch，留在 localPendingPackets）
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 1);
    EXPECT_EQ(m_tester->m_localPendingPackets.head()->ino, 9999);
    EXPECT_EQ(m_tester->m_localPendingPackets.head()->direction, kOutboundPacket);
}

// IPv6 完整成功路径
TEST_F(UT_NetifPacketCapture, pcap_callback_IPv6_Success_EnqueuesOutbound)
{
    const uint16_t sport = 0x1234, dport = 80;
    auto pkt = makeIPv6TCPPacket(sport, dport);
    primeIfaddrHash(m_tester, "::"); // IPv6 src 全 0 => inet_ntop => "::"
    QString pattern = QString("%1:%2-%3:%4").arg("::").arg(sport).arg("::").arg(dport);
    primeSockStat(m_tester, pattern, 7777);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();

    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 1);
    EXPECT_EQ(m_tester->m_localPendingPackets.head()->ino, 7777);
}

// dispatch 低水位分支（npkts>=LWAT 且 <HWAT）：桩 size()=65 => tryLock 路径
TEST_F(UT_NetifPacketCapture, pcap_callback_DispatchLowWater)
{
    auto pkt = makeIPv4TCPPacket(0x1234, 80);
    primeIfaddrHash(m_tester, "0.0.0.0");
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg("0.0.0.0").arg(0x1234).arg("0.0.0.0").arg(80), 1);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    Stub stub;
    stub.set(ADDR(PacketPayloadQueue, size), stub_localPendingPackets_64);
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data()); // 不崩溃即通过
}

// dispatch 高水位分支（npkts>=HWAT）：桩 size()=255 => lock 路径
TEST_F(UT_NetifPacketCapture, pcap_callback_DispatchHighWater)
{
    auto pkt = makeIPv4TCPPacket(0x1234, 80);
    primeIfaddrHash(m_tester, "0.0.0.0");
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg("0.0.0.0").arg(0x1234).arg("0.0.0.0").arg(80), 1);
    pcap_pkthdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = pkt.size();
    Stub stub;
    stub.set(ADDR(PacketPayloadQueue, size), stub_localPendingPackets_255);
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data()); // 不崩溃即通过
}
