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
#ifndef NETIF_H
#define NETIF_H

#include <QString>

#include <netlink/route/link.h>
#include <netlink/netlink.h>

#include <memory>
#include <unordered_map>

#include <linux/if_link.h>
#include <linux/wireless.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#ifndef IFALIASZ
#define IFALIASZ 256
#endif

#define PRODUCT_NAME_MAX 256
#define VENDOR_NAME_MAX 128
#define PCI_SLOT_MAX 16
#define USB_PORT_MAX 16

enum link_type_t {
    kEthernet,
    k80211Wireless
};

enum bus_type_t {
    kPCIBusType,
    kUSBBusType,
    /*
    kCPUBusType,
    kSCSIBusType
    kMemoryBusType
    kHIDBusType
    kPnPBusType
    kPCIExpressBusType
    kACPIBusType
    */
};

//struct iw_info_t {
//    struct iw_point         essid;
//    struct iw_quality       qual;
//    struct iw_freq          freq;
//    struct iw_statistics    stat;
//};

struct netif_info_t {
    char name[IFNAMSIZ]; // netlink: link name
    int ifindex; // netlink: link index
    unsigned int type; // netlink: hardware type
    unsigned int flags; // netlink: link flags
    //    unsigned int    metric;                 // netlink:
    unsigned int mtu; // netlink: maximum transmission unit
    uint8_t status; // netlink: operational status
    uint32_t group; // netlink: group identifier
    unsigned int tx_queue_len; // netlink: transmission queue length
    unsigned int carrier_changes; // /sys/class/net/{if}/carrier_changes
    unsigned int carrier_up_count; // /sys/class/net/{if}/carrier_up_count
    unsigned int carrier_down_count; // /sys/class/net/{if}/carrier_down_count
    char hwaddr[32]; // netlink: link address

    struct device_info_t {
        int dev_type;
        int dev_class;
        int product_id;
        int vendor_id;
        char product_name[PRODUCT_NAME_MAX];
        char vendor_name[VENDOR_NAME_MAX];

        enum bus_type_t bus_type; // /sys/bus
        union {
            char pci_slot[PCI_SLOT_MAX]; // /sys/bus/pci/devices
            char usb_port[USB_PORT_MAX]; // /sys/bus/usb/devices
        } bus;
    } device;

    struct rtnl_link_stats stat; // statistics

    struct link_conf_t {
        int has_ip : 1;
        int has_ipx_bb : 1; // legacy(TBD)
        int has_ipx_sn : 1; // legacy(TBD)
        int has_ipx_e3 : 1; // legacy(TBD)
        int has_ipx_e2 : 1; // legacy(TBD)

        union {
            struct {
                struct sockaddr ipaddr;
                struct sockaddr ipaddr_dst;
                struct sockaddr ipaddr_broadcast;
                struct sockaddr ipaddr_netmask;
            } ip; // IP network address

            struct {
                struct sockaddr ipxaddr_bb;
                struct sockaddr ipxaddr_sn;
                struct sockaddr ipxaddr_e3;
                struct sockaddr ipxaddr_e2;
            } ipx; // legacy(TBD): IPX network address
        } addr;
    } conf;

    struct link_info_t {
        enum link_type_t link_type;
        union {
            struct {
                char essid[16];
                struct iw_freq freq;
                struct iw_quality qual; //link quality & signal level & noise level
                struct iw_statistics stats;
            } wifi;
        } aux;
    } link;
};

using NetifInfo = std::shared_ptr<struct netif_info_t>;
using NetifInfoMap = std::unordered_map<int, NetifInfo>; // index:netif_info

#endif // NETIF_H
