// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/nl_hwaddr.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>

//system
#include <net/if.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdarg>
#include <string>

// 注：nl_hwaddr.h 中 NLHWAddr 位于全局命名空间（非 core::system）；
// uppercase 仅在 nl_hwaddr.cpp 中定义且头文件未声明，此处补声明以链接到实现
std::string uppercase(const std::string &s);

/***************************************STUB begin*********************************************/

// ioctl/socket 桩：注入 SIOCGIFHWADDR 结果（sa_family + MAC），隔离真实网卡环境。
// ioctl 第三参在 x86-64 SysV ABI 下经 va_arg 取回，即调用方传入的 struct ifreq *。
static int g_stub_socket_fd = 99;          // socket() 桩返回的假 fd
static int g_stub_ioctl_ret = -1;          // ioctl() 桩返回值
static int g_stub_family = 0;              // 注入的 sa_family
static unsigned char g_stub_mac[6] = {0};  // 注入的 MAC 地址

static int stub_socket(int domain, int type, int protocol)
{
    Q_UNUSED(domain);
    Q_UNUSED(type);
    Q_UNUSED(protocol);
    return g_stub_socket_fd;
}

static int stub_ioctl(int fd, unsigned long request, ...)
{
    Q_UNUSED(fd);
    if (request != SIOCGIFHWADDR || g_stub_ioctl_ret != 0) {
        return g_stub_ioctl_ret;
    }
    va_list ap;
    va_start(ap, request);
    struct ifreq *ifr = va_arg(ap, struct ifreq *);
    va_end(ap);
    if (!ifr) {
        return -1;
    }
    ifr->ifr_hwaddr.sa_family = g_stub_family;
    memcpy(ifr->ifr_hwaddr.sa_data, g_stub_mac, sizeof(g_stub_mac));
    return 0;
}

/***************************************STUB end**********************************************/

class UT_NLHWAddr: public ::testing::Test
{
public:
    UT_NLHWAddr() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 默认注入：socket 返回假 fd，ioctl 失败，用例按需覆盖注入参数
        g_stub_socket_fd = 99;
        g_stub_ioctl_ret = -1;
        g_stub_family = 0;
        memset(g_stub_mac, 0, sizeof(g_stub_mac));
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NLHWAddr *m_tester;
};

// 构造失败路径：ioctl 失败时保持默认状态（连接类型为空、非虚拟网卡）
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataIoctlFails_KeepsDefaultState)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = -1;

    // Act
    m_tester = new NLHWAddr("utfake0");

    // Assert
    EXPECT_TRUE(m_tester->conn_type().isEmpty());
    EXPECT_FALSE(m_tester->is_virtual());
}

// 正常路径：以太网类型注入后返回 Ethernet 且非虚拟
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataEtherFamily_ReturnsEthernetType)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_ETHER;
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester = new NLHWAddr("utfake0");

    // Assert
    EXPECT_EQ(m_tester->conn_type(), QByteArray("Ethernet"));
    EXPECT_EQ(m_tester->sa_family(), ARPHRD_ETHER);
    EXPECT_FALSE(m_tester->is_virtual());
}

// 类型映射：回环设备映射为 loopback
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataLoopbackFamily_ReturnsLoopbackType)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_LOOPBACK;
    const unsigned char mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester = new NLHWAddr("lo");

    // Assert
    EXPECT_EQ(m_tester->conn_type(), QByteArray("loopback"));
    EXPECT_EQ(m_tester->sa_family(), ARPHRD_LOOPBACK);
}

// 类型映射：PPP 设备映射为 PPP
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataPppFamily_ReturnsPppType)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_PPP;
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x56};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester = new NLHWAddr("ppp0");

    // Assert
    EXPECT_EQ(m_tester->conn_type(), QByteArray("PPP"));
    EXPECT_EQ(m_tester->sa_family(), ARPHRD_PPP);
}

// 类型映射：未知硬件类型返回空连接类型
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataUnknownFamily_ReturnsEmptyType)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = 0x7FF0;  // hwname 未覆盖的硬件类型

    // Act
    m_tester = new NLHWAddr("utfake0");

    // Assert
    EXPECT_TRUE(m_tester->conn_type().isEmpty());
    EXPECT_EQ(m_tester->sa_family(), 0x7FF0);
}

// 虚拟网卡识别：VMware OUI 段 MAC 标记为虚拟
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataVmwareMac_MarksVirtual)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_ETHER;
    const unsigned char mac[6] = {0x00, 0x05, 0x69, 0xAA, 0xBB, 0xCC};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester = new NLHWAddr("utfake0");

    // Assert
    EXPECT_EQ(m_tester->conn_type(), QByteArray("Ethernet"));
    EXPECT_TRUE(m_tester->is_virtual());
}

// 虚拟网卡识别：普通物理 MAC 不标记虚拟
TEST_F(UT_NLHWAddr, NLHWAddr_InitDataNormalMac_NotVirtual)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = ARPHRD_ETHER;
    const unsigned char mac[6] = {0x52, 0x54, 0x01, 0x12, 0x34, 0x56};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    m_tester = new NLHWAddr("utfake0");

    // Assert
    EXPECT_EQ(m_tester->conn_type(), QByteArray("Ethernet"));
    EXPECT_FALSE(m_tester->is_virtual());
}

// 边界：接口名超过 IFNAMSIZ 时不做名字拷贝，ioctl 失败保持默认
TEST_F(UT_NLHWAddr, NLHWAddr_LongIfname_KeepsDefaultState)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = -1;

    // Act
    m_tester = new NLHWAddr("verylonginterfacename12345");

    // Assert
    EXPECT_TRUE(m_tester->conn_type().isEmpty());
    EXPECT_FALSE(m_tester->is_virtual());
}

// 边界：空接口名时 ioctl 失败保持默认
TEST_F(UT_NLHWAddr, NLHWAddr_EmptyIfname_KeepsDefaultState)
{
    // Arrange
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = -1;

    // Act
    m_tester = new NLHWAddr(QByteArray());

    // Assert
    EXPECT_TRUE(m_tester->conn_type().isEmpty());
    EXPECT_FALSE(m_tester->is_virtual());
}

// 自由函数 uppercase：混合大小写转大写
TEST_F(UT_NLHWAddr, Uppercase_MixedCaseString_ReturnsUpperCase)
{
    // Arrange
    const std::string input = "aBc9xY";

    // Act
    std::string result = ::uppercase(input);

    // Assert
    EXPECT_EQ(result, std::string("ABC9XY"));
    EXPECT_NE(result, input);
}

// 自由函数 uppercase：空串原样返回
TEST_F(UT_NLHWAddr, Uppercase_EmptyString_ReturnsEmpty)
{
    // Arrange
    const std::string input;

    // Act
    std::string result = ::uppercase(input);

    // Assert
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.size(), 0u);
}

/***************************************hwname 参数化测试 begin********************************/

// hwname 硬件类型 → 名称映射表（覆盖全部 switch 分支与 default 分支）
struct HwnameCase {
    int family;
    const char *name;
};

class UT_HwnameParam: public ::testing::TestWithParam<HwnameCase>
{
};

// 经 NLHWAddr 公开接口间接驱动 hwname：注入指定 family 后校验映射名称
TEST_P(UT_HwnameParam, Hwname_MappedFamily_ReturnsExpectedName)
{
    // Arrange
    const HwnameCase param = GetParam();
    Stub stub;
    stub.set(socket, stub_socket);
    stub.set(ioctl, stub_ioctl);
    g_stub_ioctl_ret = 0;
    g_stub_family = param.family;
    const unsigned char mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x99};
    memcpy(g_stub_mac, mac, sizeof(mac));

    // Act
    NLHWAddr addr("utfake0");

    // Assert
    EXPECT_EQ(addr.conn_type(), QByteArray(param.name));
    EXPECT_EQ(addr.sa_family(), param.family);
}

INSTANTIATE_TEST_SUITE_P(HwnameFamilyMapping, UT_HwnameParam,
                         ::testing::Values(
                             HwnameCase {ARPHRD_ETHER, "Ethernet"},
                             HwnameCase {ARPHRD_SLIP, "SLIP"},
                             HwnameCase {ARPHRD_LOOPBACK, "loopback"},
                             HwnameCase {ARPHRD_FDDI, "FDDI"},
                             HwnameCase {ARPHRD_IEEE1394, "IEEE1394"},
                             HwnameCase {ARPHRD_IRDA, "IRDA"},
                             HwnameCase {ARPHRD_PPP, "PPP"},
                             HwnameCase {ARPHRD_X25, "X25"},
                             HwnameCase {ARPHRD_TUNNEL, "IPtunnel"},
                             HwnameCase {ARPHRD_DLCI, "Framerelay.DLCI"},
                             HwnameCase {ARPHRD_FRAD, "Framerelay.AD"},
                             HwnameCase {ARPHRD_TUNNEL6, "IP6tunnel"},
                             HwnameCase {ARPHRD_SIT, "IP6inIP4"},
                             HwnameCase {0x7F00, ""}));

/***************************************hwname 参数化测试 end**********************************/
