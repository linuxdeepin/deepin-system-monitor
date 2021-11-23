/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      baohaifeng <baohaifeng@uniontech.com>
* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//self
#include "system/netif_packet_parser.h"
#include <net/ethernet.h>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

/***************************************STUB begin*********************************************/

unsigned short int stub_ntohs_IP()
{
    return ETHERTYPE_IP;
}

unsigned short int stub_ntohs_IPV6()
{
    return ETHERTYPE_IPV6;
}

/***************************************STUB end**********************************************/


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

TEST_F(UT_NetifPacketParser, test_parsePacket_01)
{
//    pcap_pkthdr hdr;
//    memset(&hdr,0,sizeof(pcap_pkthdr));
//    const u_char packet[64] = {0};
//    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

//    Stub stub;
//    stub.set(ntohs, stub_ntohs_IP);
//    EXPECT_EQ(m_tester->parsePacket(&hdr, packet, payload), false);
}

TEST_F(UT_NetifPacketParser, test_parsePacket_02)
{
//    pcap_pkthdr hdr;
//    memset(&hdr,0,sizeof(hdr));
//    const u_char packet[64] = {0};
//    PacketPayload payload = QSharedPointer<struct packet_payload_t>::create();

//    Stub stub;
//    stub.set(ntohs, stub_ntohs_IPV6);
//    EXPECT_EQ(m_tester->parsePacket(&hdr, packet, payload), false);
}
