/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     gaoxiang <gaoxiang@uniontech.com>
*
* Maintainer: gaoxiang <gaoxiang@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef NET_INFO_H
#define DISKIO_INFO_H

#include <QSharedPointer>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

namespace core {
namespace system {

struct net_stat {
    unsigned long long rx_bytes; // received bytes
    unsigned long long tx_bytes; // transmitted bytes
    char iface[IF_NAMESIZE + 1]; // interface name
};

class NetInfo
{
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

public:
    explicit NetInfo();

    void update();

    /**
     * @brief recvBps   获取接收速度
     * @return
     */
    qreal recvBps();

    /**
     * @brief sentBps   获取发送速度
     * @return
     */
    qreal sentBps();

    /**
     * @brief totalRecvBytes    总接收流量
     * @return
     */
    qulonglong totalRecvBytes();

    /**
     * @brief totalSentBytes    总发送流量
     * @return
     */
    qulonglong totalSentBytes();

    /**
     * @brief resdNetInfo   读取网络信息
     */
    void resdNetInfo();

private:
    timeval timevalList[kStatCount] = {timeval{0, 0}, timeval{0, 0}};
    QSharedPointer<struct net_stat> m_netStat[kStatCount] {{}, {}};

    qreal m_recvBps = 0;             // 接收速度
    qreal m_sentBps = 0;             // 发送速度
    qulonglong m_totalRecvBytes = 0; // 接收总流量
    qulonglong m_totalSentBytes = 0; // 发送总流量
};

} // namespace system
} // namespace core

#endif // DISKIO_INFO_H
