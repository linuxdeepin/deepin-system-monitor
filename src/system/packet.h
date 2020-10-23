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
#ifndef PACKET_H
#define PACKET_H

#include <QQueue>
#include <QSharedPointer>

#include <memory>

#include <pcap.h>
#include <time.h>
#include <netinet/in.h>

enum packet_direction {
    kInboundPacket,
    kOutboundPacket,
};

struct packet_payload_t {
    struct timeval      ts;
    packet_direction    direction;
//    QList<ino_t>        inoList;
    ino_t               ino;
    int                 sa_family;
    uint                proto;
    union {
        in_addr in4;
        in6_addr in6;
    } s_addr;
#define sin4_addr s_addr.in4
#define sin6_addr s_addr.in6
    uint16_t            s_port;
    union {
        in_addr in4;
        in6_addr in6;
    } d_addr;
#define din4_addr d_addr.in4
#define din6_addr d_addr.in6
    uint16_t            d_port;
    unsigned long long payload;
};

using PacketPayload      = QSharedPointer<struct packet_payload_t>;
using PacketPayloadQueue = QQueue<PacketPayload>;

#endif // PACKET_H
