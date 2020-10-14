/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
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

#ifndef NETIF_PACKET_PARSER_H
#define NETIF_PACKET_PARSER_H

#include <QQueue>
#include <QSharedPointer>
#include <QList>

#include <pcap.h>

#include <time.h>
#include <netinet/in.h>

/**
 * @brief Packet direction enum
 */
enum packet_direction {
    kInboundPacket, // inbound packet
    kOutboundPacket, // outbound packet
};

/**
 * @brief Packet payload struct
 */
struct packet_payload_t {
    struct timeval ts; // timestamp of the packet when captured
    packet_direction direction; // packet direction
    //    QList<ino_t>        inoList;
    ino_t ino; // inode of the packet
    int sa_family; // address family
    uint proto; // protocol
    union {
        in_addr in4;
        in6_addr in6;
    } s_addr; // source address
#define sin4_addr s_addr.in4
#define sin6_addr s_addr.in6
    uint16_t s_port; // source port
    union {
        in_addr in4;
        in6_addr in6;
    } d_addr; // destination address
#define din4_addr d_addr.in4
#define din6_addr d_addr.in6
    uint16_t d_port; // destination port
    qulonglong payload; // packet payload
};

using PacketPayload      = QSharedPointer<struct packet_payload_t>;
using PacketPayloadQueue = QQueue<PacketPayload>;

/**
 * @brief Packet parser
 */
class NetifPacketParser
{
public:
    /**
     * @brief Parse network packet
     * @param pkt_hdr Packet header
     * @param packet Packet
     * @param payload Packet payload
     * @return true: parse success; false: parse failure
     */
    static bool parsePacket(const struct pcap_pkthdr *pkt_hdr,
                            const u_char *packet,
                            PacketPayload &payload);
private:
    // default constructor
    NetifPacketParser() = default;
};

#endif // NETIF_PACKET_PARSER_H
