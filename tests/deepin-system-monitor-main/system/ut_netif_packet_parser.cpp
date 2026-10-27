// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 最小清单（生成前完成情况）：
// | # | 检查项 | 完成情况 |
// | 1 | 每个公开方法 ≥ 1 用例 | ✅ parsePacket 单方法类，25 用例 |
// | 2 | 输入维度等价类划分 | ✅ 以太类型(IP/IPv6/ARP)、传输层(TCP/UDP/未知)、扩展头链(0/43/44/60 组合)、uh_ulen(0/≤8/正常) |
// | 3 | 边界值显式覆盖 | ✅ caplen 恰等于阈值(004/014/022/024/025)、uh_ulen=0 与 =4 边界(015/023)、零负载扩展头包 |
// | 4 | 同质多组输入 TEST_P | ✅ N/A：各组输入的头部布局与断言焦点不同（扩展头类型各异、TCP/UDP 断言不同），独立用例表达更准确 |
// | 5 | 分支清单已列出并映射用例 | ✅ 下方分支清单（来源：get_code_snippet 实源码，口径与 extract_branches 一致：10 if + 13 case/default + 1 while + 12 return = 36） |
// | 6 | 每条 if/switch/循环/早退 return 分支有用例 | ✅ 见分支清单映射 |
// | 7 | EXPECT_THROW 精确类型 | ✅ N/A：纯解析函数无异常路径，畸形输入经返回值表达 |
// | 8 | 负面场景专门用例 | ✅ 截断包(004/014/022/024/025)、ARP(005)、未知协议(006/013)、忽略型扩展头(011/012/018-021) |
// | 9 | 负面用例强异常安全 | ✅ 失败路径后断言 payload 状态字段（sa_family/proto/端口）保持/符合预期，无越界读 |
// | 10 | 打桩选择 | ✅ 纯解析函数，真实网络字节包构造即可覆盖全部分支，无需打桩（helper 按 htons 写入，内部 ntohs 读取） |

// 用例计数声明表（方法 | level | factors | min | actual）
// | parsePacket | high | complexity:25 | 3 | 25 |

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

// ============================================================================
// 分支清单（来源：NetifPacketParser::parsePacket，get_code_snippet 实源码；
// 实际 36 分支 = 10 if + 12 case + 1 default + 1 while + 12 早退 return）
// B1: if !payload → 懒创建 → 001
// B2: if type==ETHERTYPE_IP → IPv4 系用例
// B3: if caplen <= eth+ip → IPv4 截断 → 004
// B4: return false（IPv4 截断早退出口）→ 004
// B5: else if type==ETHERTYPE_IPV6 → IPv6 系用例
// B6: while !stop 遍历扩展头链 → 009/010/016/017（多跳）/其余单跳用例
// B7: case IP6_NEXT_HEADER_HBH(0) → 009
// B8: case IP6_NEXT_HEADER_TCP(6) → 007/009/010/016/017/024
// B9: case IP6_NEXT_HEADER_UDP(17) → 008/025
// B10: case IP6_NEXT_HEADER_EIP6(41) → 011
// B11: return false（EIP6 早退出口）→ 011
// B12: case IP6_NEXT_HEADER_ROUTING(43) → 016
// B13: case IP6_NEXT_HEADER_FRAGMENT(44) → 010
// B14: case IP6_NEXT_HEADER_RRSVP(46) → 018
// B15: return false（RRSVP 早退出口）→ 018
// B16: case IP6_NEXT_HEADER_ESP(50) → 019
// B17: return false（ESP 早退出口）→ 019
// B18: case IP6_NEXT_HEADER_AUTH(51) → 020
// B19: return false（AUTH 早退出口）→ 020
// B20: case IP6_NEXT_HEADER_ICMP6(58) → 021
// B21: return false（ICMP6 早退出口）→ 021
// B22: case IP6_NEXT_HEADER_NNH(59) → 012
// B23: return false（NNH 早退出口）→ 012
// B24: case IP6_NEXT_HEADER_DESOPT(60) → 017
// B25: default（未知 next header）→ 013
// B26: return false（unknown nxt 早退出口）→ 013
// B27: else（非 IPv4/IPv6 以太类型）→ 005
// B28: return false（非 IP 早退出口）→ 005
// B29: if proto==IPPROTO_TCP → 001/002/007/009/010/016/017/024
// B30: if caplen <= eth+ip+tcp_hdr_len → TCP 截断 → 014（IPv4）/024（IPv6）
// B31: return false（TCP 截断早退出口）→ 014/024
// B32: else if proto==IPPROTO_UDP → 003/008/015/022/023/025
// B33: if udp_len==0 → RFC2675 jumbogram → 015
// B34: else if udp_len<=udp_hdr_len → ulen=udp_len → 023
// B35: if caplen <= eth+ip+ulen → UDP 截断 → 022（IPv4）/025（IPv6）
// B36: return false（UDP 截断早退出口）→ 022/025
// B37: else（未知传输层协议）→ 006
// B38: return false（未知 proto 早退出口）→ 006
// 说明：v6 扩展头早退用例（011-021/012/013）在 switch 内 return，位于 sa_family 赋值之前，
// 故断言 payload->sa_family==0；v4 早退（004/005）亦先于 sa_family 赋值。
// ============================================================================

// IPv4/TCP 正常路径：payload 为空时函数内部懒创建，断言族/协议/端口
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCP_NullPayloadLazyCreate_001)
{
    // Arrange：构造完整 IPv4/TCP 包，payload 置空
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = nullptr;

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：懒创建成功且各字段正确
    EXPECT_TRUE(ok);
    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv4/TCP 正常路径：已有 payload 直接复用
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCP_ReusePayload_002)
{
    // Arrange：预创建 payload
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：复用对象且字段正确
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_GT(payload->payload, 0u);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv4/UDP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDP_Success_003)
{
    // Arrange
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv4 截断包（caplen <= eth+ip） => false，且早退于族赋值前
TEST_F(UT_NetifPacketParser, parsePacket_IPv4Truncated_ReturnsFalse_004)
{
    // Arrange：caplen=15，小于 eth(14)+ip(20)
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, 15);
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：失败且字段未被赋值
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
    EXPECT_EQ(payload->proto, 0);
}

// 非 IPv4/IPv6 包（ARP）=> false
TEST_F(UT_NetifPacketParser, parsePacket_NonIP_ReturnsFalse_005)
{
    // Arrange：ARP 以太类型
    const size_t ethLen = sizeof(ether_header);
    std::vector<u_char> pkt(ethLen + 20, 0);
    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_ARP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
    EXPECT_EQ(payload->proto, 0);
}

// IPv4 + 未知传输层协议（如 255）=> IP 解析后 proto 非 TCP/UDP => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UnknownProto_ReturnsFalse_006)
{
    // Arrange
    auto pkt = buildIPv4Packet(255, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：族与协议已记录（早退于传输层分支）
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET);
    EXPECT_EQ(payload->proto, 255);
}

// IPv6/TCP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv6TCP_Success_007)
{
    // Arrange
    auto pkt = buildIPv6Packet({6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv6/UDP 正常路径
TEST_F(UT_NetifPacketParser, parsePacket_IPv6UDP_Success_008)
{
    // Arrange
    auto pkt = buildIPv6Packet({17 /*UDP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
    EXPECT_EQ(payload->d_port, DST_PORT);
}

// IPv6 + Hop-by-Hop 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6HBHThenTCP_Success_009)
{
    // Arrange：扩展头链 HBH(0) → TCP(6)
    auto pkt = buildIPv6Packet({0 /*HBH*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：穿透扩展头后解析 TCP
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6 + Fragment 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6FragThenTCP_Success_010)
{
    // Arrange：扩展头链 Fragment(44) → TCP(6)
    auto pkt = buildIPv6Packet({44 /*FRAGMENT*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6 + 封装 IPv6(41) => 忽略返回 false（早退于族赋值前）
TEST_F(UT_NetifPacketParser, parsePacket_IPv6EIP6_ReturnsFalse_011)
{
    // Arrange
    auto pkt = buildIPv6Packet({41 /*EIP6*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv6 + 无下一头(59) => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6NoNextHeader_ReturnsFalse_012)
{
    // Arrange
    auto pkt = buildIPv6Packet({59 /*NNH*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv6 + 未知 next header(200) => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6UnknownNxt_ReturnsFalse_013)
{
    // Arrange
    auto pkt = buildIPv6Packet({200 /*unknown*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv4/TCP 在传输层截断（caplen 仅够到 IP 头） => false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4TCPTruncatedAtTransport_ReturnsFalse_014)
{
    // Arrange：caplen = eth+ip = 34，恰为截断阈值
    const size_t ethLen = sizeof(ether_header);
    const size_t ipLen = sizeof(struct ip);
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, ethLen + ipLen); // 不含任何 TCP 字节
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：IPv4 截断分支在族赋值之前返回，sa_family 保持 0
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv4/UDP uh_ulen=0 分支（RFC2675 jumbogram）=> 不应崩溃，正常返回
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPZeroLength_NoCrash_015)
{
    // Arrange：uh_ulen 置 0
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + sizeof(ether_header) + sizeof(struct ip));
    udp->uh_ulen = htons(0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：ulen 取 udp_hdr_len(8)，负载 = 82-14-20-8 = 40
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
    EXPECT_EQ(payload->payload, 40u);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6 + Routing 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6RoutingThenTCP_Success_016)
{
    // Arrange：扩展头链 Routing(43) → TCP(6)
    auto pkt = buildIPv6Packet({43 /*ROUTING*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6 + Destination Options 扩展头 + TCP
TEST_F(UT_NetifPacketParser, parsePacket_IPv6DestThenTCP_Success_017)
{
    // Arrange：扩展头链 DestOpts(60) → TCP(6)
    auto pkt = buildIPv6Packet({60 /*DEST*/, 6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
    EXPECT_EQ(payload->s_port, SRC_PORT);
}

// IPv6 + RRSVP(46) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6RRSVP_ReturnsFalse_018)
{
    // Arrange
    auto pkt = buildIPv6Packet({46 /*RRSVP*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv6 + ESP(50) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6ESP_ReturnsFalse_019)
{
    // Arrange
    auto pkt = buildIPv6Packet({50 /*ESP*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv6 + AUTH(51) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6AUTH_ReturnsFalse_020)
{
    // Arrange
    auto pkt = buildIPv6Packet({51 /*AUTH*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv6 + ICMPv6(58) => 忽略返回 false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6ICMP6_ReturnsFalse_021)
{
    // Arrange
    auto pkt = buildIPv6Packet({58 /*ICMP6*/}, 0);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
}

// IPv4/UDP 在传输层截断（caplen 仅到 IP 头）=> false
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPTruncatedAtTransport_ReturnsFalse_022)
{
    // Arrange：caplen = eth+ip，不含 UDP 字节（ulen=8 阈值之下）
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, sizeof(ether_header) + sizeof(struct ip));
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：IPv4 截断分支在族赋值之前返回，sa_family/proto 保持 0
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, 0);
    EXPECT_EQ(payload->proto, 0);
}

// IPv4/UDP uh_ulen 非零但 <= UDP 头长 => ulen=udp_len 分支，包体足够应成功
TEST_F(UT_NetifPacketParser, parsePacket_IPv4UDPSmallLength_Success_023)
{
    // Arrange：uh_ulen=4（0 < 4 <= 8）
    auto pkt = buildIPv4Packet(IPPROTO_UDP, 40);
    auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + sizeof(ether_header) + sizeof(struct ip));
    udp->uh_ulen = htons(4);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：负载 = 82-14-20-4 = 44
    EXPECT_TRUE(ok);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
    EXPECT_EQ(payload->payload, 44u);
}

// IPv6/TCP 传输层截断（caplen 恰为以太头长：v6 路径 ip_hdr_len=0，th_off 位域使
// tcp_hdr_len=0，截断阈值为 eth+0+0）=> false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6TCPTruncatedAtTransport_ReturnsFalse_024)
{
    // Arrange：完整 114 字节包，caplen 压到 14
    auto pkt = buildIPv6Packet({6 /*TCP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, sizeof(ether_header));
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert：v6 族在传输层截断判定前已赋值
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_TCP);
}

// IPv6/UDP 传输层截断（ulen=udp_hdr_len=8，阈值 eth+0+8）=> false
TEST_F(UT_NetifPacketParser, parsePacket_IPv6UDPTruncatedAtTransport_ReturnsFalse_025)
{
    // Arrange：完整包，caplen 压到 22（恰为阈值）
    auto pkt = buildIPv6Packet({17 /*UDP*/}, 40);
    pcap_pkthdr hdr;
    fillHdr(hdr, sizeof(ether_header) + sizeof(struct udphdr));
    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

    // Act
    bool ok = m_tester->parsePacket(&hdr, pkt.data(), payload);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->proto, IPPROTO_UDP);
}
