// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wireless.h"
#include "ddlog.h"
#include<sys/socket.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include <linux/wireless.h>

using namespace DDLog;
namespace core{
namespace system{

wireless::wireless()
    : m_bwireless(false)
    , m_link_quality(0)
    , m_signal_levle(0)
    , m_noise_level(0)
{
    qCDebug(app) << "wireless object created with default constructor";
}

wireless::wireless(QByteArray ifname)
    : m_bwireless(false)
    , m_ifname(ifname)
    , m_link_quality(0)
    , m_signal_levle(0)
    , m_noise_level(0)
{
    qCDebug(app) << "wireless object created for interface:" << ifname;
    read_wireless_info();
}

wireless::~wireless()
{
    qCDebug(app) << "wireless object destroyed";
}

QByteArray wireless::essid()
{
       return m_essid;
}

unsigned char wireless::link_quality()
{
    return m_link_quality;
}

unsigned char wireless::signal_levle()
{
    return m_signal_levle;
}

unsigned char wireless::noise_level()
{
    return  m_noise_level;
}

bool wireless::is_wireless()
{
    return m_bwireless;
}

bool wireless::read_wireless_info()
{
    qCDebug(app) << "Reading wireless info for interface:" << m_ifname;
    if(m_ifname.isNull()){
        qCWarning(app) << "Interface name is null, cannot read wireless info";
        m_bwireless =false;
        return false;
    }
    struct iwreq wrq;
    struct iw_statistics stats;
    char buffer[256];
    memset(&wrq,0,sizeof(wrq));
    memset(&stats,0,sizeof(stats));
    memset(buffer, 0, 256);
    strcpy(wrq.ifr_name,m_ifname.data());
    wrq.u.data.pointer = &stats;
    wrq.u.data.length=sizeof(iw_statistics);
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
          qCWarning(app) << "Failed to create socket for wireless info:" << strerror(errno);
          m_bwireless = false;
          close(sock);
          return false;
     }

    if (ioctl(sock, SIOCGIWSTATS, &wrq) == -1) {
        qCWarning(app) << "ioctl(SIOCGIWSTATS) failed for" << m_ifname << ":" << strerror(errno);
        m_bwireless = false;
        close(sock);
        return false;
    }

    wrq.u.essid.pointer = buffer;//如果不写这行可能会错误
    wrq.u.essid.length = 256;
    if (ioctl(sock, SIOCGIWESSID, &wrq) == -1) {
        qCWarning(app) << "ioctl(SIOCGIWESSID) failed for" << m_ifname << ":" << strerror(errno);
        m_bwireless = false;
        close(sock);
        return false;
    }

    if(wrq.u.essid.flags != 0){
      m_essid = buffer;
    }
    m_link_quality = stats.qual.qual;
    m_signal_levle = stats.qual.level;
    m_noise_level = stats.qual.noise;
    m_bwireless = true;
    qCDebug(app) << "Successfully read wireless info for" << m_ifname << "ESSID:" << m_essid << "Link Quality:" << m_link_quality << "Signal Level:" << m_signal_levle;
    close(sock);
    return true;
}

}
}
