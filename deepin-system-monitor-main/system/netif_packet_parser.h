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

#include "packet.h"

namespace core {
namespace system {

class NetifPacketParser
{
public:
    static bool parsePacket(const struct pcap_pkthdr *pkt_hdr,
                            const u_char *packet,
                            PacketPayload &payload);


private:
    NetifPacketParser() = default;
};

} // namespace system
} // namespace core

#endif // NETIF_PACKET_PARSER_H
