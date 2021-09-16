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
#ifndef WIRELESS_H
#define WIRELESS_H
#include <QByteArray>
#include <netlink/route/link.h>
namespace core {
namespace system {

class wireless
{
public:
    wireless();
    explicit wireless(QByteArray ifname);
    ~wireless();
    // 服务器别号
    QByteArray essid();
    unsigned char link_quality();
    unsigned char signal_levle();
    unsigned char noise_level();

    bool is_wireless();
protected:
    bool read_wireless_info();


private:
    bool m_bwireless;
    QByteArray m_ifname;

    QByteArray m_essid;
    unsigned char m_link_quality;
    unsigned char m_signal_levle;
    unsigned char m_noise_level;
};


}
}



#endif // WIRELESS_H
