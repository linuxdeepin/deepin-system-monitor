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
#include "wireless.h"
#include<sys/socket.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include <linux/wireless.h>
namespace core{
namespace system{

wireless::wireless()
{
}

wireless::wireless(QByteArray ifname)
    : m_bwireless(false)
    , m_ifname(ifname)
    , m_link_quality(0)
    , m_signal_levle(0)
    , m_noise_level(0)
{

    read_wireless_info();
}

wireless::~wireless()
{

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
    if(m_ifname.isNull()){
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
          m_bwireless = false;
          close(sock);
          return false;
     }

    if (ioctl(sock, SIOCGIWSTATS, &wrq) == -1) {
        m_bwireless = false;
        close(sock);
        return false;
    }

    wrq.u.essid.pointer = buffer;//如果不写这行可能会错误
    wrq.u.essid.length = 256;
    if (ioctl(sock, SIOCGIWESSID, &wrq) == -1) {
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
    close(sock);
    return true;
}

}
}
