// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_packet_parser.h"
#include "ddlog.h"

#include <netinet/in.h>
#include <cstring>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define IP6_NEXT_HEADER_HBH 0
#define IP6_NEXT_HEADER_TCP 6
#define IP6_NEXT_HEADER_UDP 17
#define IP6_NEXT_HEADER_EIP6 41
#define IP6_NEXT_HEADER_ROUTING 43
#define IP6_NEXT_HEADER_FRAGMENT 44
#define IP6_NEXT_HEADER_RRSVP 46
#define IP6_NEXT_HEADER_ESP 50 // skip (TODO: ipsec packets)
#define IP6_NEXT_HEADER_AUTH 51 // skip (TODO: ipsec packets)
#define IP6_NEXT_HEADER_ICMP6 58 // skip (TODO: icmpv6 packets)
#define IP6_NEXT_HEADER_NNH 59
#define IP6_NEXT_HEADER_DESOPT 60

using namespace DDLog;

namespace core {
namespace system {

bool NetifPacketParser::parsePacket(const pcap_pkthdr *pkt_hdr,
                                    const u_char *packet,
                                    PacketPayload &payload)
{
    // qCDebug(app) << "Parsing packet, captured length:" << pkt_hdr->caplen;
    if (!payload) {
        payload = QSharedPointer<struct packet_payload_t>::create();
    }
    payload->ts = pkt_hdr->ts;
    const u_char *hdr = packet;
    // parse hdr&packet
    if (pkt_hdr->caplen < sizeof(struct ether_header))
        return false;
    struct ether_header eth_hdr;
    memcpy(&eth_hdr, packet, sizeof(eth_hdr));
    auto type = ntohs(eth_hdr.ether_type);
    auto eth_hdr_len = sizeof(struct ether_header);
    uint proto {};
    ulong ip_hdr_len {};

    if (type == ETHERTYPE_IP) {
        qCDebug(app) << "Packet is ETHERTYPE_IP (IPv4)";
        if (pkt_hdr->caplen < eth_hdr_len + sizeof(struct ip))
            return false;
        struct ip ip_hdr;
        memcpy(&ip_hdr, packet + eth_hdr_len, sizeof(ip_hdr));
        // ip header length
        ip_hdr_len = ulong((ip_hdr.ip_hl & 0x0f) * 4);
        // ip payload
        // protocol
        proto = ip_hdr.ip_p;

        // truncated packets
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len) {
            qCWarning(app) << "Truncated IPv4 packet, captured length" << pkt_hdr->caplen << "is less than or equal to header length" << eth_hdr_len + ip_hdr_len;
            return false;
        }

        hdr = packet + eth_hdr_len + ip_hdr_len;
        payload->sa_family = AF_INET;
        payload->proto = proto;
        payload->s_addr.in4 = ip_hdr.ip_src;
        payload->d_addr.in4 = ip_hdr.ip_dst;

    } else if (type == ETHERTYPE_IPV6) {
        qCDebug(app) << "Packet is ETHERTYPE_IPV6";
        if (pkt_hdr->caplen < eth_hdr_len + sizeof(struct ip6_hdr))
            return false;
        struct ip6_hdr ip6_hdr;
        memcpy(&ip6_hdr, packet + eth_hdr_len, sizeof(ip6_hdr));

        // next header field in ip6 header
        uint8_t nhtype = ip6_hdr.ip6_nxt;
        // find upper-layer protocol header (the last extension header)
        hdr = packet + eth_hdr_len + sizeof(struct ip6_hdr);
        bool stop {false};
        while (!stop) {
            qCDebug(app) << "Parsing IPv6 next header, type:" << nhtype;
            switch (nhtype) {
            case  IP6_NEXT_HEADER_HBH: {
                // Hop-by-Hop Options Header
                if (size_t(packet + pkt_hdr->caplen - hdr) < sizeof(struct ip6_hbh))
                    return false;
                struct ip6_hbh hbh;
                memcpy(&hbh, hdr, sizeof(hbh));
                const size_t headerLength = hbh.ip6h_len * 8 + 8;
                if (headerLength > size_t(packet + pkt_hdr->caplen - hdr))
                    return false;
                hdr += headerLength;
                nhtype = hbh.ip6h_nxt;
                qCDebug(app) << "Parsed Hop-by-Hop header, next header type:" << nhtype;
                break;
            }
            case IP6_NEXT_HEADER_TCP: { // UPPER-LAYER protocol header
                // TCP
                proto = IPPROTO_TCP;
                stop = true;
                qCDebug(app) << "Found TCP header in IPv6";
                break;
            }
            case IP6_NEXT_HEADER_UDP: { // UPPER-LAYER protocol header
                // UDP
                proto = IPPROTO_UDP;
                stop = true;
                qCDebug(app) << "Found UDP header in IPv6";
                break;
            }
            case IP6_NEXT_HEADER_EIP6: {
                // Encapsulated IPv6 Header: IPv6 Tunneling
                qCWarning(app) << "Ignoring encapsulated IPv6 header";
                return false;
            }
            case IP6_NEXT_HEADER_ROUTING: {
                // Routing Header
                if (size_t(packet + pkt_hdr->caplen - hdr) < sizeof(struct ip6_rthdr))
                    return false;
                struct ip6_rthdr rh;
                memcpy(&rh, hdr, sizeof(rh));
                const size_t headerLength = rh.ip6r_len * 8 + 8;
                if (headerLength > size_t(packet + pkt_hdr->caplen - hdr))
                    return false;
                hdr += headerLength;
                nhtype = rh.ip6r_nxt;
                qCDebug(app) << "Parsed Routing header, next header type:" << nhtype;
                break;
            }
            case IP6_NEXT_HEADER_FRAGMENT: {
                // Fragment Header
                if (size_t(packet + pkt_hdr->caplen - hdr) < sizeof(struct ip6_frag))
                    return false;
                struct ip6_frag fh;
                memcpy(&fh, hdr, sizeof(fh));
                const size_t headerLength = sizeof(struct ip6_frag);
                if (headerLength > size_t(packet + pkt_hdr->caplen - hdr))
                    return false;
                hdr += headerLength;
                nhtype = fh.ip6f_nxt;
                qCDebug(app) << "Parsed Fragment header, next header type:" << nhtype;
                break;
            }
            case IP6_NEXT_HEADER_RRSVP: {
                // Resource ReSerVation Protocol
                qCWarning(app) << "Ignoring RRSVP header";
                return false;
            }
            case IP6_NEXT_HEADER_ESP: {
                // Encapsulating Security Payload: ignored for now
                qCWarning(app) << "Ignoring ESP header";
                return false;
            }
            case IP6_NEXT_HEADER_AUTH: {
                // Authentication Header: ignored for now
                qCWarning(app) << "Ignoring Auth header";
                return false;
            }
            case IP6_NEXT_HEADER_ICMP6: { // UPPER-LAYER protocol header
                // ICMPv6: ignored for now
                qCWarning(app) << "Ignoring ICMPv6 header";
                return false;
            }
            case IP6_NEXT_HEADER_NNH: {
                // No next header: empty payload
                qCWarning(app) << "Ignoring packet with no next header";
                return false;
            }
            case IP6_NEXT_HEADER_DESOPT: {
                // Destination Options Header
                if (size_t(packet + pkt_hdr->caplen - hdr) < sizeof(struct ip6_dest))
                    return false;
                struct ip6_dest dh;
                memcpy(&dh, hdr, sizeof(dh));
                const size_t headerLength = dh.ip6d_len * 8 + 8;
                if (headerLength > size_t(packet + pkt_hdr->caplen - hdr))
                    return false;
                hdr += headerLength;
                nhtype = dh.ip6d_nxt;
                qCDebug(app) << "Parsed Destination Options header, next header type:" << nhtype;
                break;
            }
            default:
                // unknown next header type found, stop parsing
                qCWarning(app) << "Unknown IPv6 next header type:" << nhtype;
                return false;
            } // !switch
        } // !while

        payload->sa_family = AF_INET6;
        payload->proto = proto;
        payload->s_addr.in6 = ip6_hdr.ip6_src;
        payload->d_addr.in6 = ip6_hdr.ip6_dst;

    } else {
        // ignore non ip4 & ip6 packets
        qCDebug(app) << "Ignoring non-IPv4/IPv6 packet, ethertype:" << type;
        return false;
    }

    if (proto == IPPROTO_TCP) {
        qCDebug(app) << "Parsing TCP header";
        if (pkt_hdr->caplen < ulong(hdr - packet) + sizeof(struct tcphdr))
            return false;
        struct tcphdr tcp_hdr;
        memcpy(&tcp_hdr, hdr, sizeof(tcp_hdr));
        auto tcp_hdr_len = ulong(((tcp_hdr.th_off & 0xf0) >> 4) * 4);
        // no payload data
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len + tcp_hdr_len) {
            qCWarning(app) << "Truncated TCP packet, captured length" << pkt_hdr->caplen << "is less than or equal to header length" << eth_hdr_len + ip_hdr_len + tcp_hdr_len;
            return false;
        }
        payload->payload = pkt_hdr->caplen - eth_hdr_len - ip_hdr_len - tcp_hdr_len;
        payload->s_port = ntohs(tcp_hdr.th_sport);
        payload->d_port = ntohs(tcp_hdr.th_dport);
        qCDebug(app) << "Parsed TCP packet: src port" << payload->s_port << "dst port" << payload->d_port << "payload size" << payload->payload;

    } else if (proto == IPPROTO_UDP) {
        qCDebug(app) << "Parsing UDP header";
        if (pkt_hdr->caplen < ulong(hdr - packet) + sizeof(struct udphdr))
            return false;
        struct udphdr udp_hdr;
        memcpy(&udp_hdr, hdr, sizeof(udp_hdr));
        auto udp_len = ulong(ntohs(udp_hdr.uh_ulen));
        auto udp_hdr_len = sizeof(struct udphdr);
        ulong ulen {};
        if (udp_len == 0) {
            // rfc#2675:
            // the length field is set to zero if the length of the UDP header plus
            // UDP data is greater than 65,535
            ulen = udp_hdr_len;
            qCDebug(app) << "UDP length is 0, using header length:" << ulen;
        } else if (udp_len <= udp_hdr_len) {
            ulen = udp_len;
        } else {
            ulen = udp_hdr_len;
        }
        // truncated, no payload data
        if (pkt_hdr->caplen <= eth_hdr_len + ip_hdr_len + ulen) {
            qCWarning(app) << "Truncated UDP packet, captured length" << pkt_hdr->caplen << "is less than or equal to header length" << eth_hdr_len + ip_hdr_len + ulen;
            return false;
        }
        payload->payload = pkt_hdr->caplen - eth_hdr_len - ip_hdr_len - ulen;
        payload->s_port = ntohs(udp_hdr.uh_sport);
        payload->d_port = ntohs(udp_hdr.uh_dport);
        qCDebug(app) << "Parsed UDP packet: src port" << payload->s_port << "dst port" << payload->d_port << "payload size" << payload->payload;

    } else {
        // unexpected case, unknown proto type
        qCWarning(app) << "Unknown transport protocol:" << proto;
        return false;
    }

    // qCInfo(app) << "Successfully parsed packet.";
    return true;
}

} // namespace system
} // namespace core
