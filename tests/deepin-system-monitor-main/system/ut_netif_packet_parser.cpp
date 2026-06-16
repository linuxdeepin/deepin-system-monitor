// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_packet_parser.h"
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
//gtest
#include "stub.h"
#include <gtest/gtest.h>

#include <vector>

using namespace core::system;

/***************************************STUB begin*********************************************/
// parsePacket 是纯解析函数，构造真实网络字节包即可覆盖全部分支，无需打桩。
/***************************************STUB end**********************************************/

namespace {
// 包构造辅助：在堆上分配并对齐安全的缓冲，按真实网络协议布局填充各层头部。
// 注意：ether_type / 端口均按网络字节序(htons)写入，函数内部用 ntohs 读取，故不再全局桩化
// ntohs（旧测试那样桩化会破坏端口号解析）。

constexpr uint16_t SRC_PORT = 0x1234;
constexpr uint16_t DST_PORT = 80;

// 填充 pcap 包头
void fillHdr(pcap_pkthdr &hdr, size_t caplen)
{
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = static_cast<bpf_u_int32>(caplen);
    hdr.len = static_cast<bpf_u_int32>(caplen);
}

// 构造一个 IPv4 包：ether + ip(20) + [tcp(20) | udp(8)] + payload
std::vector<u_char> buildIPv4Packet(uint8_t proto, size_t payloadLen, bool truncated = false)
{
    const size_t ethLen = sizeof(ether_header);   // 14
    const size_t ipLen = sizeof(struct ip);       // 20
    size_t l4Len = 0;
    if (proto == IPPROTO_TCP) l4Len = 20;
    else if (proto == IPPROTO_UDP) l4Len = sizeof(struct udphdr);
    size_t total = ethLen + ipLen + l4Len + payloadLen;
    if (truncated) total = ethLen + 1; // 故意截断到 IP 头之前
    std::vector<u_char> pkt(total, 0);

    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IP);

    if (total >= ethLen + ipLen) {
        auto *iph = reinterpret_cast<struct ip *>(pkt.data() + ethLen);
        iph->ip_hl = 5;   // 20 字节
        iph->ip_v = 4;
        iph->ip_p = proto;
    }
    if (proto == IPPROTO_TCP && total >= ethLen + ipLen + l4Len) {
        auto *tcp = reinterpret_cast<struct tcphdr *>(pkt.data() + ethLen + ipLen);
        tcp->th_sport = htons(SRC_PORT);
        tcp->th_dport = htons(DST_PORT);
        // 数据偏移字段：标准 TCP 头 20 字节 => doff=5
        // 平台为小端，byte[12] 高 4 位即 th_off
        *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50;
    } else if (proto == IPPROTO_UDP && total >= ethLen + ipLen + l4Len) {
        auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + ethLen + ipLen);
        udp->uh_sport = htons(SRC_PORT);
        udp->uh_dport = htons(DST_PORT);
        udp->uh_ulen = htons(static_cast<uint16_t>(l4Len + payloadLen));
    }
    return pkt;
}

// 构造 IPv6 包：ether + ip6_hdr(40) + 可选扩展头链 + [tcp|udp]
// nxtSeq 描述从 ip6_nxt 开始的下一头类型链（最后一个为传输层 6=TCP/17=UDP 或需早退的类型）
std::vector<u_char> buildIPv6Packet(const std::vector<uint8_t> &nxtSeq, size_t payloadLen)
{
    const size_t ethLen = sizeof(ether_header);
    const size_t ip6Len = sizeof(struct ip6_hdr);
    // 计算扩展头占用（仅支持本测试用到的几种）
    size_t extLen = 0;
    for (size_t i = 0; i + 1 < nxtSeq.size(); ++i) {
        uint8_t t = nxtSeq[i];
        if (t == 0 /*HBH*/ || t == 43 /*ROUTING*/ || t == 60 /*DEST*/) extLen += 8; // len=0 => 8 字节
        else if (t == 44 /*FRAGMENT*/) extLen += sizeof(struct ip6_frag);
    }
    uint8_t last = nxtSeq.back();
    size_t l4Len = 0;
    if (last == 6) l4Len = 20;
    else if (last == 17) l4Len = sizeof(struct udphdr);

    size_t total = ethLen + ip6Len + extLen + l4Len + payloadLen;
    std::vector<u_char> pkt(total, 0);

    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IPV6);

    auto *ip6 = reinterpret_cast<struct ip6_hdr *>(pkt.data() + ethLen);
    ip6->ip6_nxt = nxtSeq.front();

    u_char *p = pkt.data() + ethLen + ip6Len;
    for (size_t i = 0; i + 1 < nxtSeq.size(); ++i) {
        uint8_t t = nxtSeq[i];
        uint8_t next = nxtSeq[i + 1];
        if (t == 0) { auto *h = reinterpret_cast<struct ip6_hbh *>(p); h->ip6h_len = 0; h->ip6h_nxt = next; p += 8; }
        else if (t == 43) { auto *h = reinterpret_cast<struct ip6_rthdr *>(p); h->ip6r_len = 0; h->ip6r_nxt = next; p += 8; }
        else if (t == 60) { auto *h = reinterpret_cast<struct ip6_dest *>(p); h->ip6d_len = 0; h->ip6d_nxt = next; p += 8; }
        else if (t == 44) { auto *h = reinterpret_cast<struct ip6_frag *>(p); h->ip6f_nxt = next; p += sizeof(struct ip6_frag); }
    }
    if (last == 6) {
        auto *tcp = reinterpret_cast<struct tcphdr *>(p);
        tcp->th_sport = htons(SRC_PORT);
        tcp->th_dport = htons(DST_PORT);
        *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50;
    } else if (last == 17) {
        auto *udp = reinterpret_cast<struct udphdr *>(p);
        udp->uh_sport = htons(SRC_PORT);
        udp->uh_dport = htons(DST_PORT);
        udp->uh_ulen = htons(static_cast<uint16_t>(l4Len + payloadLen));
    }
    return pkt;
}
} // namespace

class UT_NetifPacketParser: public ::testing::Test
{
public:
    UT_NetifPacketParser() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifPacketParser();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifPacketParser *m_tester;
};

TEST_F(UT_NetifPacketParser, initTest)
{
}

// IPv4/TCP 正常路径：payload 为空时函数内部懒创建，断言族/协议/端口
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCP_NullPayloadLazyCreate_001)
{
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = nullptr;

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv4/TCP 正常路径：已有 payload 直接复用
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCP_ReusePayload_002)
{
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_GT(payload->payload, 0u);
}

// IPv4/UDP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDP_Success_003)
{
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv4 截断包（caplen <= eth+ip） => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4Truncated_ReturnsFalse_004)
{
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, 15); // 小于 eth(14)+ip(20)
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// 非 IPv4/IPv6 包（ARP）=> false
TEST_F(UT_NetifPacketParser, parsePacket_NonIP_ReturnsFalse_005)
{
    const size_t ethLen = sizeof(ether_header);
    std::vector<u_char> pkt(ethLen + 20, 0);
    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_ARP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv4 + 未知传输层协议（如 255）=> IP 解析后 proto 非 TCP/UDP => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UnknownProto_ReturnsFalse_006)
{
    auto pkt = buildIPv4Packet(255, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6/TCP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv6TCP_Success_007)
{
    auto pkt = buildIPv6Packet({6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6/UDP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv6UDP_Success_008)
{
    auto pkt = buildIPv6Packet({17 /*UDP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
}

// IPv6 + Hop-by-Hop 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6HBHThenTCP_Success_009)
{
    auto pkt = buildIPv6Packet({0 /*HBH*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
}

// IPv6 + Fragment 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6FragThenTCP_Success_010)
{
    auto pkt = buildIPv6Packet({44 /*FRAGMENT*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
}

// IPv6 + 封装 IPv6(41) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6EIP6_ReturnsFalse_011)
{
    auto pkt = buildIPv6Packet({41 /*EIP6*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6 + 无下一头(59) => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6NoNextHeader_ReturnsFalse_012)
{
    auto pkt = buildIPv6Packet({59 /*NNH*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6 + 未知 next header(200) => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6UnknownNxt_ReturnsFalse_013)
{
    auto pkt = buildIPv6Packet({200 /*unknown*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv4/TCP 在传输层截断（caplen 仅够到 IP 头） => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCPTruncatedAtTransport_ReturnsFalse_014)
{
    const size_t ethLen = sizeof(ether_header);
    const size_t ipLen = sizeof(struct ip);
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, ethLen + ipLen); // 不含任何 TCP 字节
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv4/UDP uh_ulen=0 分支（RFC2675 jumbogram）=> 不应崩溃，正常返回
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPZeroLength_NoCrash_015)
{
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    // 覆盖 uh_ulen = 0 分支
    auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + sizeof(ether_header) + sizeof(struct ip));
    udp->uh_ulen = htons(0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // uh_ulen=0 => ulen=udp_hdr_len；包体足够，应成功解析
    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
}

// IPv6 + Routing 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6RoutingThenTCP_Success_016)
{
    auto pkt = buildIPv6Packet({43 /*ROUTING*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
}

// IPv6 + Destination Options 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6DestThenTCP_Success_017)
{
    auto pkt = buildIPv6Packet({60 /*DEST*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
}

// IPv6 + RRSVP(46) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6RRSVP_ReturnsFalse_018)
{
    auto pkt = buildIPv6Packet({46 /*RRSVP*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6 + ESP(50) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6ESP_ReturnsFalse_019)
{
    auto pkt = buildIPv6Packet({50 /*ESP*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6 + AUTH(51) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6AUTH_ReturnsFalse_020)
{
    auto pkt = buildIPv6Packet({51 /*AUTH*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv6 + ICMPv6(58) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6ICMP6_ReturnsFalse_021)
{
    auto pkt = buildIPv6Packet({58 /*ICMP6*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv4/UDP 在传输层截断（caplen 仅到 IP 头）=> false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPTruncatedAtTransport_ReturnsFalse_022)
{
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, sizeof(ether_header) + sizeof(struct ip)); // 不含 UDP 字节
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_FALSE(m_tester->parsePacket(&hdr, pkt.data(), payload));
}

// IPv4/UDP uh_ulen 非零但 <= UDP 头长 => ulen=udp_len 分支，包体足够应成功
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPSmallLength_Success_023)
{
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + sizeof(ether_header) + sizeof(struct ip));
    udp->uh_ulen = htons(4); // 0 < 4 <= 8 => ulen=4
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    EXPECT_TRUE(m_tester->parsePacket(&hdr, pkt.data(), payload));
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
}
