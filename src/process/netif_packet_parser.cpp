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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "netif_packet_parser.h"

#include <QDebug>

#include <netinet/in.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define IP6_NEXT_HEADER_HBH         0
#define IP6_NEXT_HEADER_TCP         6
#define IP6_NEXT_HEADER_UDP         17
#define IP6_NEXT_HEADER_EIP6        41
#define IP6_NEXT_HEADER_ROUTING     43
#define IP6_NEXT_HEADER_FRAGMENT    44
#define IP6_NEXT_HEADER_RRSVP       46
#define IP6_NEXT_HEADER_ESP         50  // skip (TODO: ipsec packets)
#define IP6_NEXT_HEADER_AUTH        51  // skip (TODO: ipsec packets)
#define IP6_NEXT_HEADER_ICMP6       58  // skip (TODO: icmpv6 packets)
#define IP6_NEXT_HEADER_NNH         59
#define IP6_NEXT_HEADER_DESOPT      60

bool NetifPacketParser::parsePacket(const pcap_pkthdr *pkt_hdr,
                                    const u_char *packet,
                                    PacketPayload &payload)
{
    if (!payload) {
        payload = QSharedPointer<struct packet_payload_t>::create();
    }
    payload->ts = pkt_hdr->ts;

    const u_char *hdr = packet;

    // parse hdr&packet
    auto *eth_hdr = reinterpret_cast<const struct ether_header *>(packet);
    auto type = ntohs(eth_hdr->ether_type);
    auto eth_hdr_len = sizeof(struct ether_header);
    uint proto {};
    ulong ip_hdr_len {};

    if (type == ETHERTYPE_IP) {
        auto *ip_hdr = reinterpret_cast<const struct ip *>(packet + eth_hdr_len);
        // ip header length
        ip_hdr_len = ulong((ip_hdr->ip_hl & 0x0f) * 4);
        // ip payload
        hdr = packet + eth_hdr_len + ip_hdr_len;
        // protocol
        proto = ip_hdr->ip_p;

        // truncated packets
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len) {
            return false;
        }

        payload->sa_family = AF_INET;
        payload->proto = proto;
        payload->sin4_addr = ip_hdr->ip_src;
        payload->din4_addr = ip_hdr->ip_dst;

    } else if (type == ETHERTYPE_IPV6) {
        auto *ip6_hdr = reinterpret_cast<const struct ip6_hdr *>(packet + eth_hdr_len);

        // next header field in ip6 header
        uint8_t nhtype = ip6_hdr->ip6_nxt;
        // find upper-layer protocol header (the last extension header)
        hdr = packet + eth_hdr_len + sizeof(struct ip6_hdr);
        bool stop {false};
        while (!stop) {
            switch (nhtype) {
            case  IP6_NEXT_HEADER_HBH: {
                // Hop-by-Hop Options Header
                auto *hbh = reinterpret_cast<const struct ip6_hbh *>(hdr);
                hdr = hdr + hbh->ip6h_len * 8 + 8;
                nhtype = hbh->ip6h_nxt;
                break;
            }
            case IP6_NEXT_HEADER_TCP: { // UPPER-LAYER protocol header
                // TCP
                proto = IPPROTO_TCP;
                stop = true;
                break;
            }
            case IP6_NEXT_HEADER_UDP: { // UPPER-LAYER protocol header
                // UDP
                proto = IPPROTO_UDP;
                stop = true;
                break;
            }
            case IP6_NEXT_HEADER_EIP6: {
                // Encapsulated IPv6 Header: IPv6 Tunneling
                return false;
            }
            case IP6_NEXT_HEADER_ROUTING: {
                // Routing Header
                auto *rh = reinterpret_cast<const struct ip6_rthdr *>(hdr);
                hdr = hdr + rh->ip6r_len * 8 + 8;
                nhtype = rh->ip6r_nxt;
                break;
            }
            case IP6_NEXT_HEADER_FRAGMENT: {
                // Fragment Header
                auto *fh = reinterpret_cast<const struct ip6_frag *>(hdr);
                hdr = hdr + sizeof(struct ip6_frag);
                nhtype = fh->ip6f_nxt;
                break;
            }
            case IP6_NEXT_HEADER_RRSVP: {
                // Resource ReSerVation Protocol
                return false;
            }
            case IP6_NEXT_HEADER_ESP: {
                // Encapsulating Security Payload: ignored for now
                return false;
            }
            case IP6_NEXT_HEADER_AUTH: {
                // Authentication Header: ignored for now
                return false;
            }
            case IP6_NEXT_HEADER_ICMP6: { // UPPER-LAYER protocol header
                // ICMPv6: ignored for now
                return false;
            }
            case IP6_NEXT_HEADER_NNH: {
                // No next header: empty payload
                return false;
            }
            case IP6_NEXT_HEADER_DESOPT: {
                // Destination Options Header
                auto *dh = reinterpret_cast<const struct ip6_dest *>(hdr);
                hdr = hdr + dh->ip6d_len * 8 + 8;
                nhtype = dh->ip6d_nxt;
                break;
            }
            default:
                // unknown next header type found, stop parsing
                return false;
            } // !switch
        } // !while

        payload->sa_family = AF_INET6;
        payload->proto = proto;
        payload->sin6_addr = ip6_hdr->ip6_src;
        payload->din6_addr = ip6_hdr->ip6_dst;

    } else {
        // ignore non ip4 & ip6 packets
        return false;
    }

    if (proto == IPPROTO_TCP) {
        auto *tcp_hdr = reinterpret_cast<const struct tcphdr *>(hdr);
        auto tcp_hdr_len = ulong(((tcp_hdr->th_off & 0xf0) >> 4) * 4);
        // no payload data
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len + tcp_hdr_len) {
            return false;
        }
        payload->payload = pkt_hdr->caplen - eth_hdr_len - ip_hdr_len - tcp_hdr_len;
        payload->s_port = ntohs(tcp_hdr->th_sport);
        payload->d_port = ntohs(tcp_hdr->th_dport);

    } else if (proto == IPPROTO_UDP) {
        auto *udp_hdr = reinterpret_cast<const struct udphdr *>(hdr);
        auto udp_len = ulong(ntohs(udp_hdr->uh_ulen));
        auto udp_hdr_len = sizeof(struct udphdr);
        ulong ulen {};
        if (udp_len == 0) {
            // rfc#2675:
            // the length field is set to zero if the length of the UDP header plus
            // UDP data is greater than 65,535
            ulen = udp_hdr_len;
        } else if (udp_len <= udp_hdr_len) {
            ulen = udp_len;
        } else {
            ulen = udp_hdr_len;
        }
        // truncated, no payload data
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len + ulen) {
            return false;
        }
        payload->payload = pkt_hdr->caplen - eth_hdr_len - ip_hdr_len - ulen;
        payload->s_port = ntohs(udp_hdr->uh_sport);
        payload->d_port = ntohs(udp_hdr->uh_dport);

    } else {
        // unexpected case, unknown proto type
        return false;
    }

    return true;
}
