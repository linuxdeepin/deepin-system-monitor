// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nl_hwaddr.h"
#include "ddlog.h"

#include <net/if.h>
#include <unistd.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <QObject>

using namespace DDLog;

static int maclen(unsigned family = ARPHRD_ETHER)
{
    qCDebug(app) << "Getting mac length for family" << family;
    switch (family) {
    case ARPHRD_INFINIBAND:
        return 20;
    case ARPHRD_ETHER:
        return 6;
    default:
        return 14;
    }
}
static string getmac(const unsigned char *mac, unsigned family = ARPHRD_ETHER)
{
    char buff[5];
    string result = "";
    bool valid = false;

    qCDebug(app) << "Getting mac string for family" << family;

    for (int i = 0; i < maclen(family); i++) {
        snprintf(buff, sizeof(buff), "%02x", mac[i]);

        valid |= (mac[i] != 0);

        if (i == 0)
            result = string(buff);
        else
            result += ":" + string(buff);
    }

    if (valid) {
        qCDebug(app) << "Valid mac found:" << result.c_str();
        return result;
    } else {
        qCDebug(app) << "Invalid or zero mac, returning empty string";
        return "";
    }
}

string uppercase(const string &s)
{
    string result;

    for (unsigned int i = 0; i < s.length(); i++)
        result += static_cast<char>(toupper(s[i]));

    return result;
}


static bool isVirtual(const string &MAC)
{
    if (MAC.length() < 8) {
        qCDebug(app) << "MAC address too short to be virtual:" << MAC.c_str();
        return false;
    }

    string manufacturer = uppercase(MAC.substr(0, 8));
    qCDebug(app) << "Checking for virtual MAC, manufacturer:" << manufacturer.c_str();

    if ((manufacturer == "00:05:69") ||
            (manufacturer == "00:0C:29") || (manufacturer == "00:50:56")) {
        qCDebug(app) << "VMware MAC detected";
        return true;    // VMware
    }
    if (manufacturer == "00:1C:42") {
        qCDebug(app) << "Parallels MAC detected";
        return true;    // Parallels
    }
    if (manufacturer == "0A:00:27") {
        qCDebug(app) << "VirtualBox MAC detected";
        return true;    // VirtualBox
    }

    qCDebug(app) << "MAC is not from a known virtual adapter";
    return false;
}

static QString hwname(int t)
{
    qCDebug(app) << "Getting hardware name for type" << t;
    switch (t) {
    case ARPHRD_ETHER:
        return "Ethernet";
    case ARPHRD_SLIP:
        return "SLIP";
    case ARPHRD_LOOPBACK:
        return "loopback";
    case ARPHRD_FDDI:
        return "FDDI";
    case ARPHRD_IEEE1394:
        return "IEEE1394";
    case ARPHRD_IRDA:
        return "IRDA";
    case ARPHRD_PPP:
        return "PPP";
    case ARPHRD_X25:
        return "X25";
    case ARPHRD_TUNNEL:
        return "IPtunnel";
    case ARPHRD_DLCI:
        return "Framerelay.DLCI";
    case ARPHRD_FRAD:
        return "Framerelay.AD";
    case ARPHRD_TUNNEL6:
        return "IP6tunnel";
    case ARPHRD_SIT:
        return "IP6inIP4";
    default:
        qCDebug(app) << "Unknown hardware type:" << t;
        return "";
    }
}

NLHWAddr::NLHWAddr(const QByteArray &ifname)
{
    qCDebug(app) << "NLHWAddr constructor for interface:" << ifname;
    m_conn_type = "";
    m_ifname = ifname;
    m_is_virtual = false;
    initData();
}


void NLHWAddr::initData()
{
    qCDebug(app) << "Initializing NLHWAddr data for" << m_ifname;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    //设备名称长度检查
    if (m_ifname.size() < IFNAMSIZ) {
        strcpy(ifr.ifr_name, m_ifname);
    }
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        qCDebug(app) << "ioctl SIOCGIFHWADDR successful for" << m_ifname;
        string hwaddr = getmac(reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data), ifr.ifr_hwaddr.sa_family);
        m_sa_family = ifr.ifr_hwaddr.sa_family;
        if (m_sa_family >= 256) {
            qCDebug(app) << "sa_family > 256, marking as virtual";
            m_is_virtual = true;
        }
        m_conn_type = hwname(m_sa_family).toUtf8();
        m_is_virtual = isVirtual(hwaddr);
    }
    close(fd);
}


