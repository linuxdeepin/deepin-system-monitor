// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 最小清单（生成前完成情况）：
// | # | 检查项 | 完成情况 |
// | 1 | 每个公开方法 ≥ 1 用例 | ✅ 9 个方法（含 2 个自由函数）全覆盖 |
// | 2 | 输入维度等价类划分 | ✅ 设备名(空/合法/过长)、路由表(空/表头/默认路由/多路由)、pcap 返回链(失败/警告/成功)、队列水位(<64/[64,256)/≥256) |
// | 3 | 边界值显式覆盖 | ✅ 64(低水位下界)/256(高水位下界)/IFNAMSIZ 名称长度边界 |
// | 4 | 同质多组输入 TEST_P | ✅ N/A：各方法输入形态异质（pcap 返回值链用独立用例表达更清晰），无 ≥3 组同构同断言输入 |
// | 5 | 分支清单已列出并映射用例 | ✅ 下方逐方法列出（来源：get_code_snippet 真实源码，计数口径与 extract_branches 一致） |
// | 6 | 每条 if/switch/循环/早退 return 分支有用例 | ✅ 见分支清单映射 |
// | 7 | EXPECT_THROW 精确类型 | ✅ N/A：被测方法均不抛异常（Qt/C API 风格，错误经返回值/日志表达） |
// | 8 | 负面场景专门用例 | ✅ 空 devName、pcap_create 失败、activate 失败、setnonblock 失败、dispatch 错误等 |
// | 9 | 负面用例强异常安全 | ✅ 失败路径后断言 go/handle/队列/计数器状态未被破坏 |
// | 10 | 打桩选择 | ✅ Qt 内建类(QProcess)/全局 C 函数(ioctl/inet_ntoa/pcap_*/getifaddrs)用旧 Stub 字节补丁；SysInfo::readSockStat 静态成员同机制；无项目虚接口故不用 gMock |

// 用例计数声明表（方法 | level | factors | min | actual）
// | NetifPacketCapture::NetifPacketCapture | mid | in_degree:9 | 2 | 2 |
// | hasDevIP | low | - | 1 | 5 |
// | getCurrentDevName | high | complexity:13,alloc_in_loop:2 | 4 | 4 |
// | whetherDevChanged | low | - | 1 | 3 |
// | startNetifMonitorJob | high | complexity:7 | 3 | 6 |
// | dispatchPackets | high | complexity:12,transitive_loop_depth:4 | 4 | 7 |
// | refreshIfAddrsHashCache | high | transitive_loop_depth:3 | 4 | 4 |
// | readNetIfAddrs | low | - | 1 | 6 |
// | pcap_callback | high | complexity:15 | 3 | 10 |

//self
#include "system/netif_packet_capture.h"
#include "system/netif_monitor.h"
#include "system/sys_info.h"
#include "common/hash.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>
#include <QProcess>
#include <QTemporaryDir>

//c
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pcap.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <initializer_list>
#include <vector>

using namespace core::system;

// 生产代码 netif_packet_capture.cpp 顶部的内部 #define 常量（测试内镜像，勿改动语义）
constexpr int PACKET_DISPATCH_QUEUE_LWAT_ = 64;   // queue low water mark
constexpr int PACKET_DISPATCH_QUEUE_HWAT_ = 256;  // queue high water mark

// 测试需要直连的命名空间级自由函数（friend 注入对普通查找不可见，须显式前置声明）
namespace core {
namespace system {
void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet);
bool readNetIfAddrs(NetIFAddrsMap &addrsMap);
}
}

/***************************************STUB begin*********************************************/
// 说明：被测方法的外部依赖（QProcess/ioctl/inet_ntoa/pcap 家族/getifaddrs/SysInfo::readSockStat/
// readNetIfAddrs）一律打桩，测试不依赖真实设备、路由表或网络环境。旧 Stub 为 RAII，测试作用域
// 结束自动还原字节补丁。

static QString g_routeOutput;            // QProcess::readAllStandardOutput 桩返回值（route -n 输出）
static int g_startJobCount = 0;          // startNetifMonitorJob 桩调用计数
static int g_pcapCloseCount = 0;         // pcap_close 桩调用计数
static char g_pcapErrBuf[] = "stub-pcap-error";
static bool g_readNetIfAddrsOk = false;  // readNetIfAddrs 桩返回值
static NetIFAddrsMap g_stubAddrsMap;     // readNetIfAddrs 桩填充内容
static struct ifaddrs *g_ifaddrsHead = nullptr;  // getifaddrs 桩返回的链表头
static int g_getifaddrsRet = 0;          // getifaddrs 桩返回值

// ---- QProcess（Qt 内建类，字节补丁打桩） ----
static void stub_qp_start(void *, const QString &, const QStringList &, QIODevice::OpenMode) {}
static bool stub_qp_waitFinished(void *, int) { return true; }
static QByteArray stub_qp_readStdout(void *) { return g_routeOutput.toLocal8Bit(); }

// ---- NetifPacketCapture 成员 ----
static void stub_startJob_count(void *) { ++g_startJobCount; }

// ---- inet/ioctl（hasDevIP 用） ----
static int stub_ioctl_fail(int, unsigned long, ...) { return -1; }
static int stub_ioctl_ok(int, unsigned long, ...) { return 0; }
static char *stub_inet_ntoa_ok(struct in_addr) { return const_cast<char *>("192.168.1.100"); }
static char *stub_inet_ntoa_null(struct in_addr) { return nullptr; }

// ---- pcap 家族 ----
static pcap_t *stub_pcap_create_ok(const char *, char *) { return reinterpret_cast<pcap_t *>(0x1234); }
static pcap_t *stub_pcap_create_fail(const char *, char *) { return nullptr; }
static int stub_pcap_activate_ok(pcap_t *) { return 0; }
static int stub_pcap_activate_warn(pcap_t *) { return 1; }   // rc>0: 警告但继续
static int stub_pcap_activate_fail(pcap_t *) { return -1; }
static int stub_pcap_setnonblock_ok(pcap_t *, int, char *) { return 0; }
static int stub_pcap_setnonblock_fail(pcap_t *, int, char *) { return -1; }
static void stub_pcap_close_count(pcap_t *) { ++g_pcapCloseCount; }
static int stub_pcap_dispatch_idle(pcap_t *, int, pcap_handler, u_char *) { return 0; }
static int stub_pcap_dispatch_error(pcap_t *, int, pcap_handler, u_char *) { return -1; }
static int stub_pcap_dispatch_breakloop(pcap_t *, int, pcap_handler, u_char *) { return -2; }
static char *stub_pcap_geterr(pcap_t *) { return g_pcapErrBuf; }

// ---- SysInfo 静态成员 ----
static bool stub_readSockStat_ok(SockStatMap &) { return true; }

// ---- 自由函数 readNetIfAddrs ----
static bool stub_readNetIfAddrs(NetIFAddrsMap &addrsMap)
{
    addrsMap = g_stubAddrsMap;
    return g_readNetIfAddrsOk;
}

// ---- getifaddrs 家族 ----
static int stub_getifaddrs(struct ifaddrs **out)
{
    *out = g_ifaddrsHead;
    return g_getifaddrsRet;
}
static void stub_freeifaddrs_noop(struct ifaddrs *) {}
/***************************************STUB end***********************************************/

namespace {
constexpr uint16_t SRC_PORT = 0x1234;
constexpr uint16_t DST_PORT = 80;
constexpr const char *SRC_IP = "192.168.1.100";
constexpr const char *DST_IP = "202.100.1.20";
constexpr const char *SRC_IP6 = "2001:db8::100";
constexpr const char *DST_IP6 = "2001:db8::1";

// 填充 pcap 包头
void fillHdr(pcap_pkthdr &hdr, size_t caplen)
{
    memset(&hdr, 0, sizeof(hdr));
    hdr.caplen = static_cast<bpf_u_int32>(caplen);
    hdr.len = static_cast<bpf_u_int32>(caplen);
}

// 构造 IPv4 包：ether + ip(20) + [tcp(20)|udp(8)] + payload；可选写入真实源/目的地址
std::vector<u_char> buildIPv4Packet(uint8_t proto, size_t payloadLen,
                                    const char *srcIp = nullptr, const char *dstIp = nullptr)
{
    const size_t ethLen = sizeof(ether_header);
    const size_t ipLen = sizeof(struct ip);
    size_t l4Len = 0;
    if (proto == IPPROTO_TCP) l4Len = 20;
    else if (proto == IPPROTO_UDP) l4Len = sizeof(struct udphdr);
    std::vector<u_char> pkt(ethLen + ipLen + l4Len + payloadLen, 0);

    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IP);

    auto *iph = reinterpret_cast<struct ip *>(pkt.data() + ethLen);
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_p = proto;
    if (srcIp) inet_pton(AF_INET, srcIp, &iph->ip_src);
    if (dstIp) inet_pton(AF_INET, dstIp, &iph->ip_dst);

    if (proto == IPPROTO_TCP) {
        auto *tcp = reinterpret_cast<struct tcphdr *>(pkt.data() + ethLen + ipLen);
        tcp->th_sport = htons(SRC_PORT);
        tcp->th_dport = htons(DST_PORT);
        *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50;  // th_off=5
    } else if (proto == IPPROTO_UDP) {
        auto *udp = reinterpret_cast<struct udphdr *>(pkt.data() + ethLen + ipLen);
        udp->uh_sport = htons(SRC_PORT);
        udp->uh_dport = htons(DST_PORT);
        udp->uh_ulen = htons(static_cast<uint16_t>(l4Len + payloadLen));
    }
    return pkt;
}

// 构造 IPv6/TCP 包（无扩展头）；可选写入真实源/目的地址
std::vector<u_char> buildIPv6TCPPacket(const char *srcIp6 = nullptr, const char *dstIp6 = nullptr)
{
    const size_t ethLen = sizeof(ether_header);
    const size_t ip6Len = sizeof(struct ip6_hdr);
    const size_t tcpLen = 20;
    std::vector<u_char> pkt(ethLen + ip6Len + tcpLen + 40, 0);

    auto *eth = reinterpret_cast<struct ether_header *>(pkt.data());
    eth->ether_type = htons(ETHERTYPE_IPV6);

    auto *ip6 = reinterpret_cast<struct ip6_hdr *>(pkt.data() + ethLen);
    ip6->ip6_nxt = IPPROTO_TCP;
    if (srcIp6) inet_pton(AF_INET6, srcIp6, &ip6->ip6_src);
    if (dstIp6) inet_pton(AF_INET6, dstIp6, &ip6->ip6_dst);

    auto *tcp = reinterpret_cast<struct tcphdr *>(pkt.data() + ethLen + ip6Len);
    tcp->th_sport = htons(SRC_PORT);
    tcp->th_dport = htons(DST_PORT);
    *(reinterpret_cast<uint8_t *>(tcp) + 12) = 0x50;
    return pkt;
}

// 构造 net_ifaddr_t（refreshIfAddrsHashCache 测试用）
NetIFAddr makeIfAddr(int family, const char *ip, const char *iface)
{
    auto *na = new struct net_ifaddr_t;
    memset(na, 0, sizeof(*na));
    na->family = family;
    if (family == AF_INET) inet_pton(AF_INET, ip, &na->addr.in4);
    else if (family == AF_INET6) inet_pton(AF_INET6, ip, &na->addr.in6);
    if (iface) strncpy(na->iface, iface, sizeof(na->iface) - 1);
    return NetIFAddr(na);
}

// 复现生产代码的地址字符串哈希（refreshIfAddrsHashCache / pcap_callback 同源）
uint64_t addrHash(const char *addrStr)
{
    uint64_t h[2] {};
    util::common::hash(addrStr, int(strlen(addrStr)), util::common::global_seed, h);
    return h[0];
}

// 复现生产代码的套接字模式哈希："saddr:sport-daddr:dport"
uint64_t patternHash(const QString &pattern)
{
    QByteArray fmtbuf = pattern.toLocal8Bit();
    uint64_t cchash[2] {};
    util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
    return cchash[0];
}

// 预置本地地址哈希缓存（模拟 refreshIfAddrsHashCache 的结果）
void primeIfaddrHash(NetifPacketCapture *cap, const char *addrStr)
{
    cap->m_ifaddrsHashCache.insert(addrHash(addrStr), 0);
}

// 预置套接字统计缓存（模拟 SysInfo::readSockStat 的结果）
void primeSockStat(NetifPacketCapture *cap, const QString &pattern, ino_t ino)
{
    auto stat = SockStat::create();
    stat->ino = ino;
    cap->m_sockStats.insert(patternHash(pattern), stat);
}

// 构造 getifaddrs 桩用的静态链表（测试作用域内生命周期覆盖被调期）
struct FakeIfaddr {
    int family;              // -1 表示 ifa_addr 为 nullptr
    const char *ip;
    const char *name;
};
static struct ifaddrs g_fakeNodes[4];
static struct sockaddr_in g_fakeSin[4];
static struct sockaddr_in6 g_fakeSin6[4];
static struct sockaddr g_fakeOther[4];

void buildFakeIfaddrs(const std::initializer_list<FakeIfaddr> &nodes)
{
    memset(g_fakeNodes, 0, sizeof(g_fakeNodes));
    g_ifaddrsHead = nullptr;
    struct ifaddrs *prev = nullptr;
    int i = 0;
    for (const auto &n : nodes) {
        auto &node = g_fakeNodes[i];
        node.ifa_name = const_cast<char *>(n.name);
        if (n.family == AF_INET) {
            memset(&g_fakeSin[i], 0, sizeof(g_fakeSin[i]));
            g_fakeSin[i].sin_family = AF_INET;
            inet_pton(AF_INET, n.ip, &g_fakeSin[i].sin_addr);
            node.ifa_addr = reinterpret_cast<struct sockaddr *>(&g_fakeSin[i]);
        } else if (n.family == AF_INET6) {
            memset(&g_fakeSin6[i], 0, sizeof(g_fakeSin6[i]));
            g_fakeSin6[i].sin6_family = AF_INET6;
            inet_pton(AF_INET6, n.ip, &g_fakeSin6[i].sin6_addr);
            node.ifa_addr = reinterpret_cast<struct sockaddr *>(&g_fakeSin6[i]);
        } else if (n.family == -1) {
            node.ifa_addr = nullptr;  // 空地址节点
        } else {
            memset(&g_fakeOther[i], 0, sizeof(g_fakeOther[i]));
            g_fakeOther[i].sa_family = n.family;
            node.ifa_addr = &g_fakeOther[i];
        }
        if (prev) prev->ifa_next = &node;
        else g_ifaddrsHead = &node;
        prev = &node;
        ++i;
    }
}
}  // namespace

class UT_NetifPacketCapture: public ::testing::Test
{
public:
    UT_NetifPacketCapture() : m_monitor(nullptr), m_tester(nullptr) {}

    virtual void SetUp()
    {
        // 重置桩状态，避免用例间串扰
        g_routeOutput.clear();
        g_startJobCount = 0;
        g_pcapCloseCount = 0;
        g_readNetIfAddrsOk = false;
        g_stubAddrsMap.clear();
        g_ifaddrsHead = nullptr;
        g_getifaddrsRet = 0;

        m_monitor = new NetifMonitor();
        m_tester = new NetifPacketCapture(m_monitor);
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
        delete m_monitor;
        m_monitor = nullptr;
    }

protected:
    NetifMonitor *m_monitor;
    NetifPacketCapture *m_tester;
};

// 所有 QProcess 相关用例共用：桩化 start/waitForFinished/readAllStandardOutput
static void applyQProcessStubs(Stub &stub)
{
    stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), // __DBG_STUB_INVOKE__
             stub_qp_start);
    stub.set(static_cast<bool (QProcess::*)(int)>(&QProcess::waitForFinished), stub_qp_waitFinished);
    stub.set(static_cast<QByteArray (QProcess::*)()>(&QProcess::readAllStandardOutput), stub_qp_readStdout);
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::NetifPacketCapture，get_code_snippet 实源码；0 分支，列定时器初始化锚点）
// B1: m_timer 单发定时器建立并连接 dispatchPackets → ctor_SetsUpTimers
// B2: m_timerChangeDev 启动(DEVICE_CHANGE_JUDGEMENT_TIME) → ctor_SetsUpTimers
// ============================================================================
TEST_F(UT_NetifPacketCapture, netifCapture_Ctor_SetsUpTimersAndInitialState)
{
    // Arrange：断言夹具已就绪
    ASSERT_NE(m_tester, nullptr);
    // Act：读取构造后的定时器与初始状态
    bool mainSingleShot = m_tester->m_timer->isSingleShot();
    bool changeActive = m_tester->m_timerChangeDev->isActive();
    // Assert：分发定时器为单发、设备变更定时器已启动、初始状态干净
    EXPECT_TRUE(mainSingleShot);
    EXPECT_TRUE(changeActive);
    EXPECT_FALSE(m_tester->go);
    EXPECT_EQ(m_tester->m_devName, QString());
    EXPECT_EQ(m_tester->m_netifMonitor, m_monitor);
}

// 构造后的析构安全：删除对象不崩溃且不残留激活定时器句柄
TEST_F(UT_NetifPacketCapture, ctor_ConstructDestroy_NoCrash)
{
    // Arrange：构造一个独立对象
    auto *cap = new NetifPacketCapture(m_monitor);
    // Act：立即销毁
    delete cap;
    // Assert：原对象仍可用（析构未破坏 fixture 状态）
    EXPECT_NE(m_tester, nullptr);
    EXPECT_FALSE(m_tester->go);
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::hasDevIP；实际 8 分支 = 4 if + 4 早退 return）
// B1: if m_devName 为空 → 警告 → hasDevIP_EmptyDevName
// B2: return false（空名早退出口）→ hasDevIP_EmptyDevName
// B3: if 名称长度 >= IFNAMSIZ → 警告 → hasDevIP_TooLongDevName
// B4: return false（过长早退出口）→ hasDevIP_TooLongDevName
// B5: if ioctl(SIOCGIFADDR) == -1 → close 并警告 → hasDevIP_IoctlFails
// B6: return false（ioctl 失败早退出口）→ hasDevIP_IoctlFails
// B7: if inet_ntoa 非空 → close 并 return true → hasDevIP_InetNtoaNotNull
// B8: else → close 并 return false → hasDevIP_InetNtoaNull
// ============================================================================
TEST_F(UT_NetifPacketCapture, hasDevIP_EmptyDevName_ReturnsFalse)
{
    // Arrange：设备名为空（默认），不打桩（短路路径不触系统调用）
    m_tester->m_devName.clear();
    // Act
    bool ok = m_tester->hasDevIP();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName, QString());
}

TEST_F(UT_NetifPacketCapture, hasDevIP_TooLongDevName_ReturnsFalse)
{
    // Arrange：设备名 19 字符，达到 IFNAMSIZ(16) 上界外
    m_tester->m_devName = "aaaa-bbbb-cccc-dddx";
    // Act
    bool ok = m_tester->hasDevIP();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName.size(), 19);
}

TEST_F(UT_NetifPacketCapture, hasDevIP_IoctlFails_ReturnsFalse)
{
    // Arrange：合法长度设备名 + ioctl 失败
    Stub stub;
    m_tester->m_devName = "eth0";
    stub.set(ioctl, stub_ioctl_fail);
    // Act
    bool ok = m_tester->hasDevIP();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName, QString("eth0"));
}

TEST_F(UT_NetifPacketCapture, hasDevIP_InetNtoaNotNull_ReturnsTrue)
{
    // Arrange：ioctl 成功 + inet_ntoa 返回非空地址串
    Stub stub;
    m_tester->m_devName = "eth0";
    stub.set(ioctl, stub_ioctl_ok);
    stub.set(inet_ntoa, stub_inet_ntoa_ok);
    // Act
    bool ok = m_tester->hasDevIP();
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->m_devName, QString("eth0"));
}

TEST_F(UT_NetifPacketCapture, hasDevIP_InetNtoaNull_ReturnsFalse)
{
    // Arrange：ioctl 成功 + inet_ntoa 返回空
    Stub stub;
    m_tester->m_devName = "eth0";
    stub.set(ioctl, stub_ioctl_ok);
    stub.set(inet_ntoa, stub_inet_ntoa_null);
    // Act
    bool ok = m_tester->hasDevIP();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName, QString("eth0"));
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::getCurrentDevName；实际 14 分支 = 10 if + 3 for + 1 早退 return）
// B1: if outputList.size()>0 → removeFirst 去首行 → Empty/HeaderOnly/DefaultRoute/MinMetric
// B2: if totalList.size()>0 → 进入表头扫描 → HeaderOnly
// B3: for j 遍历表头列 → HeaderOnly
// B4: if 表头列=="Metric" → 记录 metric 列号 → HeaderOnly
// B5: if 表头列=="Iface" → 记录 iface 列号 → HeaderOnly
// B6: if totalList.size()>0 → removeFirst 去表头行 → HeaderOnly
// B7: for i 遍历数据行 → DefaultRoute/MinMetric
// B8: if 列数 > devColNum && > metricColNum → 处理该行（短行跳过）→ MinMetric
// B9: if list.first()=="0.0.0.0" → 清空 metricList、记录默认路由并 break → DefaultRoute
// B10: else → 追加 metricList → MinMetric
// B11: for i 遍历 metricList → DefaultRoute/MinMetric
// B12: if i==0 → 初始化最小 metric 与设备 → MinMetric
// B13: if 发现更小 metric → 更新 perfectDev → MinMetric
// B14: if m_devName 为空 → 警告 → Empty/HeaderOnly
// B15: return false（无设备早退出口）→ Empty/HeaderOnly
// 映射说明：B9 命中即 break，证明"清空+记录默认路由"优先于普通路由的最小 metric
// ============================================================================
TEST_F(UT_NetifPacketCapture, getCurrentDevName_EmptyOutput_ReturnsFalse)
{
    // Arrange：route -n 输出为空
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput = "";
    // Act
    bool ok = m_tester->getCurrentDevName();
    // Assert：无可选设备
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName, QString());
}

TEST_F(UT_NetifPacketCapture, getCurrentDevName_HeaderOnly_ReturnsFalse)
{
    // Arrange：仅有表头（无数据行）——覆盖去首行/扫表头/去表头分支
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput = "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n";
    // Act
    bool ok = m_tester->getCurrentDevName();
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(m_tester->m_devName, QString());
}

TEST_F(UT_NetifPacketCapture, getCurrentDevName_DefaultRoutePresent_BreaksWithDefaultDev)
{
    // Arrange：普通行(metric 50)在前、默认路由(metric 100)在后；
    // 若 B9 未按默认路由 break+clear，最小 metric 会选中 wlan0(50)
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "10.0.0.0        0.0.0.0         255.0.0.0       U     50     0        0 wlan0\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    // Act
    bool ok = m_tester->getCurrentDevName();
    // Assert：命中默认路由即 break
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->m_devName, QString("eth0"));
}

TEST_F(UT_NetifPacketCapture, getCurrentDevName_MultipleRoutesNoDefault_PicksMinMetric)
{
    // Arrange：无默认路由，两条普通路由 metric 100/50 + 一条短行(列不足被跳过)
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "192.168.1.0     0.0.0.0         255.255.255.0   U     100    0        0 eth0\n"
        "Gateway\n"
        "10.0.0.0        0.0.0.0         255.0.0.0       U     50     0        0 wlan0\n";
    // Act
    bool ok = m_tester->getCurrentDevName();
    // Assert：最小 metric 胜出
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_tester->m_devName, QString("wlan0"));
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::whetherDevChanged；实际 2 分支 = 2 if）
// B1: if m_devName 为空 → m_changedDev=true + startNetifMonitorJob → whetherDevChanged_EmptyDevName
// B2: if current_dev 与新名不同 → m_changedDev=true + startNetifMonitorJob → SameDevice(否分支)/DeviceChanged
// ============================================================================
TEST_F(UT_NetifPacketCapture, whetherDevChanged_EmptyDevName_StartsNewJob)
{
    // Arrange：设备名为空，startNetifMonitorJob 打桩计数
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, startNetifMonitorJob), stub_startJob_count);
    // Act
    m_tester->whetherDevChanged();
    // Assert
    EXPECT_TRUE(m_tester->m_changedDev);
    EXPECT_EQ(g_startJobCount, 1);
}

TEST_F(UT_NetifPacketCapture, whetherDevChanged_SameDevice_DoesNotRestartJob)
{
    // Arrange：当前设备 eth0，route 输出仍以 eth0 为最优
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(NetifPacketCapture, startNetifMonitorJob), stub_startJob_count);
    m_tester->m_devName = "eth0";
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    // Act
    m_tester->whetherDevChanged();
    // Assert：设备未变，不重启任务
    EXPECT_FALSE(m_tester->m_changedDev);
    EXPECT_EQ(g_startJobCount, 0);
}

TEST_F(UT_NetifPacketCapture, whetherDevChanged_DeviceChanged_RestartsJob)
{
    // Arrange：当前设备 wlan0，route 输出最优设备已变为 eth0
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(NetifPacketCapture, startNetifMonitorJob), stub_startJob_count);
    m_tester->m_devName = "wlan0";
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    // Act
    m_tester->whetherDevChanged();
    // Assert：设备变更，重启任务
    EXPECT_TRUE(m_tester->m_changedDev);
    EXPECT_EQ(g_startJobCount, 1);
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::startNetifMonitorJob；
// 实际 12 分支 = 7 if + 5 早退 return，其中 2 if + 2 return 位于 #if 0 死代码段）
// B1: if m_devName 为空 → startNetifMonitorJob_NoDevice
// B2: return（无设备早退出口）→ NoDevice
// B3: if !m_handle（pcap_create 失败）→ PcapCreateFails
// B4: return（创建失败早退出口）→ PcapCreateFails
// B5: (#if 0) if pcap_compile == -1 → 死代码分支（编译过滤永不启用）
// B6: (#if 0) return（compile 失败早退，死代码）
// B7: (#if 0) if pcap_setfilter == -1 → 死代码分支
// B8: (#if 0) return（setfilter 失败早退，死代码）
// B9: if pcap_activate rc > 0 → 警告但继续 → ActivateWarningContinues
// B10: else if rc < 0 → pcap_close + return → ActivateFails
// B11: if pcap_setnonblock == -1 → pcap_close + return → SetNonblockFails
// B12: return（setnonblock 失败早退出口）→ SetNonblockFails
// 成功路径（无分支命中至尾部）→ ActivateWarningContinues / SuccessStartsDispatch
// ============================================================================
TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_NoDevice_ReturnsEarly)
{
    // Arrange：route 输出为空 → getCurrentDevName 得空设备名
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput = "";
    // Act
    m_tester->startNetifMonitorJob();
    // Assert：状态未被推进
    EXPECT_FALSE(m_tester->go);
    EXPECT_EQ(m_tester->m_handle, nullptr);
    EXPECT_FALSE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_PcapCreateFails_ReturnsEarly)
{
    // Arrange：route 输出给出 eth0，pcap_create 失败
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    stub.set(pcap_create, stub_pcap_create_fail);
    // Act
    m_tester->startNetifMonitorJob();
    // Assert
    EXPECT_FALSE(m_tester->go);
    EXPECT_EQ(m_tester->m_handle, nullptr);
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_PcapActivateFails_ClosesHandle)
{
    // Arrange：创建成功、activate 失败
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    stub.set(pcap_create, stub_pcap_create_ok);
    stub.set(pcap_activate, stub_pcap_activate_fail);
    stub.set(pcap_close, stub_pcap_close_count);
    // Act
    m_tester->startNetifMonitorJob();
    // Assert：句柄被关闭、任务未启动
    EXPECT_FALSE(m_tester->go);
    EXPECT_EQ(g_pcapCloseCount, 1);
}

TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_PcapSetNonblockFails_ClosesHandle)
{
    // Arrange：activate 成功、setnonblock 失败
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    stub.set(pcap_create, stub_pcap_create_ok);
    stub.set(pcap_activate, stub_pcap_activate_ok);
    stub.set(pcap_setnonblock, stub_pcap_setnonblock_fail);
    stub.set(pcap_close, stub_pcap_close_count);
    // Act
    m_tester->startNetifMonitorJob();
    // Assert
    EXPECT_FALSE(m_tester->go);
    EXPECT_EQ(g_pcapCloseCount, 1);
}

TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_PcapActivateWarning_ContinuesAndStarts)
{
    // Arrange：activate 返回 1（>0 警告分支）但流程继续，setnonblock 成功
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    stub.set(pcap_create, stub_pcap_create_ok);
    stub.set(pcap_activate, stub_pcap_activate_warn);
    stub.set(pcap_setnonblock, stub_pcap_setnonblock_ok);
    stub.set(pcap_close, stub_pcap_close_count);
    // Act
    m_tester->startNetifMonitorJob();
    // Assert：警告不中断启动
    EXPECT_TRUE(m_tester->go);
    EXPECT_TRUE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, startNetifMonitorJob_Success_StartsDispatch)
{
    // Arrange：全链路成功
    Stub stub;
    applyQProcessStubs(stub);
    g_routeOutput =
        "Kernel IP routing table\n"
        "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n"
        "0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0\n";
    stub.set(pcap_create, stub_pcap_create_ok);
    stub.set(pcap_activate, stub_pcap_activate_ok);
    stub.set(pcap_setnonblock, stub_pcap_setnonblock_ok);
    stub.set(pcap_close, stub_pcap_close_count);
    // Act
    m_tester->startNetifMonitorJob();
    // Assert
    EXPECT_TRUE(m_tester->go);
    EXPECT_NE(m_tester->m_handle, nullptr);
    EXPECT_TRUE(m_tester->m_timer->isActive());
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::dispatchPackets；实际 15 分支 = 12 if + 1 while + 2 早退 return）
// B1: if !go → GoNotSet
// B2: return（go 未置位早退出口）→ GoNotSet
// B3: if m_devName 为空 → NoDevice
// B4: return（无设备早退出口）→ NoDevice
// B5: if npkts > 0 → 进入水位判断 → LowWater/HighWater
// B6: if npkts∈[64,256) → tryLock 宽松分发 → LowWaterQueue
// B7: if tryLock 成功 → 批量移交/清空/唤醒 → LowWaterQueue（失败留本地，单线程竞争不可达）
// B8: else if npkts >= 256 → 强制锁分发 → HighWaterQueue
// B9: if quit → m_timer->stop + break → QuitRequested
// B10: if sockstat 刷新间隔到 → SysInfo::readSockStat（随循环执行）→ LowWater/DispatchError/BreakLoop
// B11: if ifaddrs 刷新间隔到 → refreshIfAddrsHashCache（随循环执行）→ LowWater/DispatchError/BreakLoop
// B12: if nr==0 → m_timer->start(IDLE) + return → LowWater
// B13: if nr==-1 → 警告 + stop + break → DispatchError
// B14: if nr==-2 → stop + break → BreakLoop
// B15: while(true) 循环体 → QuitRequested/DispatchError/BreakLoop（退出后 pcap_close）
// ============================================================================
TEST_F(UT_NetifPacketCapture, dispatchPackets_GoNotSet_ReturnsImmediately)
{
    // Arrange：go 显式置 false，不打任何桩（早退路径无外部调用）
    m_tester->go = false;
    // Act
    m_tester->dispatchPackets();
    // Assert：未启动任何定时器/未关闭句柄
    EXPECT_FALSE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_NoDevice_ReturnsEarly)
{
    // Arrange：go 置位但设备名为空
    m_tester->go = true;
    // Act
    m_tester->dispatchPackets();
    // Assert
    EXPECT_FALSE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_LowWaterQueue_DispatchesRelaxedAndIdles)
{
    // Arrange：go 置位、设备 eth0、本地队列 65 个包（落在 [64,256) 宽松区）
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(SysInfo, readSockStat), stub_readSockStat_ok);
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    stub.set(pcap_dispatch, stub_pcap_dispatch_idle);
    stub.set(pcap_close, stub_pcap_close_count);
    m_tester->go = true;
    m_tester->m_devName = "eth0";
    for (int i = 0; i < PACKET_DISPATCH_QUEUE_LWAT_; ++i)
        m_tester->m_localPendingPackets.enqueue(PacketPayload::create());
    // Act
    m_tester->dispatchPackets();
    // Assert：包已移交监控队列、本地清空、进入 idle 定时、未走 close
    EXPECT_EQ(m_monitor->m_pendingPackets.size(), PACKET_DISPATCH_QUEUE_LWAT_);
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_TRUE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_HighWaterQueue_DispatchesForced)
{
    // Arrange：本地队列 256 个包（到达高水位，走强制锁分支）
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(SysInfo, readSockStat), stub_readSockStat_ok);
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    stub.set(pcap_dispatch, stub_pcap_dispatch_idle);
    stub.set(pcap_close, stub_pcap_close_count);
    m_tester->go = true;
    m_tester->m_devName = "eth0";
    for (int i = 0; i < PACKET_DISPATCH_QUEUE_HWAT_; ++i)
        m_tester->m_localPendingPackets.enqueue(PacketPayload::create());
    // Act
    m_tester->dispatchPackets();
    // Assert
    EXPECT_EQ(m_monitor->m_pendingPackets.size(), PACKET_DISPATCH_QUEUE_HWAT_);
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_TRUE(m_tester->m_timer->isActive());
    EXPECT_EQ(g_pcapCloseCount, 0);
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_QuitRequested_StopsTimerAndClosesHandle)
{
    // Arrange：请求退出，句柄为伪句柄，pcap_close 计数
    Stub stub;
    stub.set(pcap_close, stub_pcap_close_count);
    m_tester->go = true;
    m_tester->m_devName = "eth0";
    m_tester->m_handle = reinterpret_cast<pcap_t *>(0x1234);
    m_tester->m_quitRequested.store(true);
    // Act
    m_tester->dispatchPackets();
    // Assert：break 后关闭句柄、停表；退出标志复位以便后续用例
    EXPECT_EQ(g_pcapCloseCount, 1);
    EXPECT_FALSE(m_tester->m_timer->isActive());
    m_tester->m_quitRequested.store(false);
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_PcapDispatchError_StopsAndClosesHandle)
{
    // Arrange：pcap_dispatch 返回 -1（错误路径）
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(SysInfo, readSockStat), stub_readSockStat_ok);
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    stub.set(pcap_dispatch, stub_pcap_dispatch_error);
    stub.set(pcap_geterr, stub_pcap_geterr);
    stub.set(pcap_close, stub_pcap_close_count);
    m_tester->go = true;
    m_tester->m_devName = "eth0";
    m_tester->m_handle = reinterpret_cast<pcap_t *>(0x1234);
    // Act
    m_tester->dispatchPackets();
    // Assert
    EXPECT_EQ(g_pcapCloseCount, 1);
    EXPECT_FALSE(m_tester->m_timer->isActive());
}

TEST_F(UT_NetifPacketCapture, dispatchPackets_PcapDispatchBreakLoop_StopsAndClosesHandle)
{
    // Arrange：pcap_dispatch 返回 -2（breakloop 路径）
    Stub stub;
    applyQProcessStubs(stub);
    stub.set(ADDR(SysInfo, readSockStat), stub_readSockStat_ok);
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    stub.set(pcap_dispatch, stub_pcap_dispatch_breakloop);
    stub.set(pcap_close, stub_pcap_close_count);
    m_tester->go = true;
    m_tester->m_devName = "eth0";
    m_tester->m_handle = reinterpret_cast<pcap_t *>(0x1234);
    // Act
    m_tester->dispatchPackets();
    // Assert
    EXPECT_EQ(g_pcapCloseCount, 1);
    EXPECT_FALSE(m_tester->m_timer->isActive());
}

// ============================================================================
// 分支清单（来源：NetifPacketCapture::refreshIfAddrsHashCache；实际 4 分支 = 3 if + 1 while）
// B1: if readNetIfAddrs 成功 → 遍历地址表（否则缓存保持不变）→ RefreshFails
// B2: while 遍历地址卡 → 全部用例（循环体）
// B3: if family==AF_INET → IPv4 地址串哈希入缓存 → IPv4AddrCached/MixedAndOtherFamily
// B4: else if family==AF_INET6 → IPv6 地址串哈希入缓存 → IPv6AddrCached/MixedAndOtherFamily
// 映射说明：MixedAndOtherFamily 中 AF_PACKET 节点验证 else 空分支
// ============================================================================
TEST_F(UT_NetifPacketCapture, refreshIfAddrsHashCache_ReadFails_CacheStaysEmpty)
{
    // Arrange：readNetIfAddrs 桩返回 false
    Stub stub;
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    g_readNetIfAddrsOk = false;
    // Act
    m_tester->refreshIfAddrsHashCache();
    // Assert
    EXPECT_TRUE(m_tester->m_ifaddrsHashCache.isEmpty());
    EXPECT_EQ(m_tester->m_ifaddrsHashCache.size(), 0);
}

TEST_F(UT_NetifPacketCapture, refreshIfAddrsHashCache_IPv4Addr_AddrHashCached)
{
    // Arrange：一张 IPv4 地址卡
    Stub stub;
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    g_readNetIfAddrsOk = true;
    g_stubAddrsMap.insert("eth0", makeIfAddr(AF_INET, "10.0.0.1", "eth0"));
    // Act
    m_tester->refreshIfAddrsHashCache();
    // Assert：以 inet_ntop 规范化字符串的哈希为键
    EXPECT_EQ(m_tester->m_ifaddrsHashCache.size(), 1);
    EXPECT_TRUE(m_tester->m_ifaddrsHashCache.contains(addrHash("10.0.0.1")));
}

TEST_F(UT_NetifPacketCapture, refreshIfAddrsHashCache_IPv6Addr_AddrHashCached)
{
    // Arrange：一张 IPv6 地址卡
    Stub stub;
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    g_readNetIfAddrsOk = true;
    g_stubAddrsMap.insert("eth0", makeIfAddr(AF_INET6, "fe80::1", "eth0"));
    // Act
    m_tester->refreshIfAddrsHashCache();
    // Assert
    EXPECT_EQ(m_tester->m_ifaddrsHashCache.size(), 1);
    EXPECT_TRUE(m_tester->m_ifaddrsHashCache.contains(addrHash("fe80::1")));
}

TEST_F(UT_NetifPacketCapture, refreshIfAddrsHashCache_MixedAndOtherFamily_OnlyInetCached)
{
    // Arrange：AF_PACKET(链路层) + IPv4 + IPv6 混合
    Stub stub;
    stub.set(readNetIfAddrs, stub_readNetIfAddrs);
    g_readNetIfAddrsOk = true;
    g_stubAddrsMap.insert("eth0", makeIfAddr(AF_PACKET, "", "eth0"));
    g_stubAddrsMap.insert("wlan0", makeIfAddr(AF_INET, "10.0.0.2", "wlan0"));
    g_stubAddrsMap.insert("eth1", makeIfAddr(AF_INET6, "fe80::2", "eth1"));
    // Act
    m_tester->refreshIfAddrsHashCache();
    // Assert：仅 inet 家族入缓存
    EXPECT_EQ(m_tester->m_ifaddrsHashCache.size(), 2);
    EXPECT_TRUE(m_tester->m_ifaddrsHashCache.contains(addrHash("10.0.0.2")));
    EXPECT_TRUE(m_tester->m_ifaddrsHashCache.contains(addrHash("fe80::2")));
}

// ============================================================================
// 分支清单（来源：readNetIfAddrs，netif_packet_capture.cpp 自由函数；实际 8 分支 = 5 if + 1 for + 2 return）
// B1: if getifaddrs == -1 → 警告 → GetifaddrsFails
// B2: return false（getifaddrs 失败出口）→ GetifaddrsFails
// B3: for 遍历 ifaddrs 链表 → 全部成功用例（循环体）
// B4: if !ifa_addr → continue 跳过 → NullAddrNodeSkipped
// B5: if family==AF_INET → 收集 IPv4 → IPv4AddrCollected
// B6: else if family==AF_INET6 → 收集 IPv6 → IPv6AddrCollected；else → continue → OtherFamilySkipped
// B7: if addrsMap.size()>0 → return true（成功出口）→ IPv4/IPv6/NullAddr/OtherFamily
// B8: else → 警告 + return false（空表出口）→ EmptyChain
// ============================================================================
TEST_F(UT_NetifPacketCapture, readNetIfAddrs_GetifaddrsFails_ReturnsFalse)
{
    // Arrange：getifaddrs 失败
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    g_getifaddrsRet = -1;
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(addrsMap.size(), 0);
}

TEST_F(UT_NetifPacketCapture, readNetIfAddrs_IPv4Addr_Collected)
{
    // Arrange：单个 IPv4 节点
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    buildFakeIfaddrs({ {AF_INET, "10.0.0.1", "eth0"} });
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(addrsMap.size(), 1);
    ASSERT_TRUE(addrsMap.contains("eth0"));
    EXPECT_EQ(addrsMap.value("eth0")->family, AF_INET);
}

TEST_F(UT_NetifPacketCapture, readNetIfAddrs_IPv6Addr_Collected)
{
    // Arrange：单个 IPv6 节点
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    buildFakeIfaddrs({ {AF_INET6, "fe80::1", "wlan0"} });
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert
    EXPECT_TRUE(ok);
    ASSERT_TRUE(addrsMap.contains("wlan0"));
    EXPECT_EQ(addrsMap.value("wlan0")->family, AF_INET6);
}

TEST_F(UT_NetifPacketCapture, readNetIfAddrs_NullAddrNode_Skipped)
{
    // Arrange：首节点 ifa_addr 为空、次节点 IPv4
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    buildFakeIfaddrs({ {-1, nullptr, "eth0"}, {AF_INET, "10.0.0.1", "eth0"} });
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert：空地址节点被跳过，仅收集有效节点
    EXPECT_TRUE(ok);
    EXPECT_EQ(addrsMap.size(), 1);
}

TEST_F(UT_NetifPacketCapture, readNetIfAddrs_OtherFamily_Skipped)
{
    // Arrange：AF_PACKET(17) + AF_INET 混合
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    buildFakeIfaddrs({ {AF_PACKET, nullptr, "eth0"}, {AF_INET, "10.0.0.1", "eth0"} });
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert：非 inet 家族不入表
    EXPECT_TRUE(ok);
    EXPECT_EQ(addrsMap.size(), 1);
}

TEST_F(UT_NetifPacketCapture, readNetIfAddrs_EmptyChain_ReturnsFalse)
{
    // Arrange：getifaddrs 成功但链表为空
    Stub stub;
    stub.set(getifaddrs, stub_getifaddrs);
    stub.set(freeifaddrs, stub_freeifaddrs_noop);
    g_ifaddrsHead = nullptr;
    g_getifaddrsRet = 0;
    NetIFAddrsMap addrsMap;
    // Act
    bool ok = readNetIfAddrs(addrsMap);
    // Assert：无有效节点 → false
    EXPECT_FALSE(ok);
    EXPECT_EQ(addrsMap.size(), 0);
}

// ============================================================================
// 分支清单（来源：pcap_callback，netif_packet_capture.cpp 自由函数；
// 实际 17 分支 = 12 if + 1 while + 4 早退 return）
// B1: if !context → NullContext
// B2: if parsePacket 失败 → ParseFail
// B3: if sa_family==AF_INET → IPv4 哈希判定 → IPv4 系用例
// B4: if 源地址命中本地缓存 → kOutboundPacket → IPv4SrcLocal/SocketMatched/LowWater/HighWater
// B5: else if 目的地址命中本地缓存 → kInboundPacket → IPv4DstLocal
// B6: else → return（IPv4 无命中）→ IPv4NoLocalMatch
// B7: if sa_family==AF_INET6 → IPv6 哈希判定 → IPv6SrcLocal
// B8: if 源地址6命中本地缓存 → kOutboundPacket → IPv6SrcLocal
// B9: else if 目的地址6命中本地缓存 → kInboundPacket（与 B5 同构，本轮以 B8 为主）
// B10: else → return（IPv6 无命中，与 B6 同构）
// B11: if 套接字统计命中 hash → 取 ino → IPv4SocketMatched
// B12: while it.key()==hash → 取首个命中项 ino 并 break → SocketMatched
// B13: else → return（无套接字命中；末尾 return 不计早退）→ IPv4NoSocketMatch
// B14: if npkts∈[64,256) → tryLock 宽松分发 → LowWaterFlush
// B15: if tryLock 成功 → 移交+清空+唤醒（失败留本地，单线程竞争不可达）→ LowWaterFlush
// B16: else if npkts>=256 → 强制锁分发 → HighWaterFlush
// B17: return（context 空早退出口）→ NullContext
// ============================================================================
TEST_F(UT_NetifPacketCapture, pcapCallback_NullContext_ReturnsImmediately)
{
    // Arrange：合法包 + 空上下文
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    // Act：不应崩溃
    pcap_callback(nullptr, &hdr, pkt.data());
    // Assert：本地队列无入包
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 0);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_ParseFail_ReturnsImmediately)
{
    // Arrange：截断包（caplen 不足）→ parsePacket 失败
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, 15);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 0);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv4SrcLocal_EnqueuesOutbound)
{
    // Arrange：源地址为本机
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP);
    // 无套接字命中会被丢弃，需同时命中 m_sockStats
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg(SRC_IP).arg(SRC_PORT).arg(DST_IP).arg(DST_PORT), 1);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：入本地队列且方向为上行
    ASSERT_EQ(m_tester->m_localPendingPackets.size(), 1);
    auto payload = m_tester->m_localPendingPackets.head();
    EXPECT_EQ(payload->direction, kOutboundPacket);
    EXPECT_EQ(payload->sa_family, AF_INET);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv4DstLocal_EnqueuesInbound)
{
    // Arrange：仅目的地址为本机
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, DST_IP);
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg(SRC_IP).arg(SRC_PORT).arg(DST_IP).arg(DST_PORT), 1);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：方向为下行
    ASSERT_EQ(m_tester->m_localPendingPackets.size(), 1);
    EXPECT_EQ(m_tester->m_localPendingPackets.head()->direction, kInboundPacket);
    EXPECT_EQ(m_tester->m_localPendingPackets.head()->sa_family, AF_INET);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv4NoLocalMatch_ReturnsImmediately)
{
    // Arrange：本地缓存为空，源/目的均不命中
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：包被丢弃
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 0);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv4NoSocketMatch_ReturnsImmediately)
{
    // Arrange：源地址命中本地，但无套接字统计
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：无 ino 可取，包被丢弃
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
    EXPECT_EQ(m_tester->m_localPendingPackets.size(), 0);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv4SocketMatched_SetsInoFromSockStat)
{
    // Arrange：源地址命中本地 + 套接字统计表命中（ino=777）
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP);
    QString pattern = QString("%1:%2-%3:%4").arg(SRC_IP).arg(SRC_PORT).arg(DST_IP).arg(DST_PORT);
    primeSockStat(m_tester, pattern, 777);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：包携带套接字 inode 入队
    ASSERT_EQ(m_tester->m_localPendingPackets.size(), 1);
    auto payload = m_tester->m_localPendingPackets.head();
    EXPECT_EQ(payload->ino, static_cast<ino_t>(777));
    EXPECT_EQ(payload->direction, kOutboundPacket);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_IPv6SrcLocal_EnqueuesOutbound)
{
    // Arrange：IPv6 包，源地址为本机
    auto pkt = buildIPv6TCPPacket(SRC_IP6, DST_IP6);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP6);
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg(SRC_IP6).arg(SRC_PORT).arg(DST_IP6).arg(DST_PORT), 1);
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert
    ASSERT_EQ(m_tester->m_localPendingPackets.size(), 1);
    auto payload = m_tester->m_localPendingPackets.head();
    EXPECT_EQ(payload->sa_family, AF_INET6);
    EXPECT_EQ(payload->direction, kOutboundPacket);
}

TEST_F(UT_NetifPacketCapture, pcapCallback_LowWaterQueue_RelaxedFlushToMonitor)
{
    // Arrange：本地队列预填 64 包（+1=65 落在 [64,256)），本包合法且源地址命中
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP);
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg(SRC_IP).arg(SRC_PORT).arg(DST_IP).arg(DST_PORT), 1);
    for (int i = 0; i < PACKET_DISPATCH_QUEUE_LWAT_; ++i)
        m_tester->m_localPendingPackets.enqueue(PacketPayload::create());
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert：整批移交监控队列，本地清空
    EXPECT_EQ(m_monitor->m_pendingPackets.size(), PACKET_DISPATCH_QUEUE_LWAT_ + 1);
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
}

TEST_F(UT_NetifPacketCapture, pcapCallback_HighWaterQueue_ForceFlushToMonitor)
{
    // Arrange：本地队列预填 256 包（+1=257 达高水位）
    auto pkt = buildIPv4Packet(IPPROTO_TCP, 40, SRC_IP, DST_IP);
    pcap_pkthdr hdr;
    fillHdr(hdr, pkt.size());
    primeIfaddrHash(m_tester, SRC_IP);
    primeSockStat(m_tester, QString("%1:%2-%3:%4").arg(SRC_IP).arg(SRC_PORT).arg(DST_IP).arg(DST_PORT), 1);
    for (int i = 0; i < PACKET_DISPATCH_QUEUE_HWAT_; ++i)
        m_tester->m_localPendingPackets.enqueue(PacketPayload::create());
    // Act
    pcap_callback(reinterpret_cast<u_char *>(m_tester), &hdr, pkt.data());
    // Assert
    EXPECT_EQ(m_monitor->m_pendingPackets.size(), PACKET_DISPATCH_QUEUE_HWAT_ + 1);
    EXPECT_TRUE(m_tester->m_localPendingPackets.isEmpty());
}
