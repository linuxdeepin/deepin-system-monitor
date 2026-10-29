// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    uint8_t link_quality();
    uint8_t signal_levle();
    uint8_t noise_level();

    bool is_wireless();
protected:
    bool read_wireless_info();


private:
    bool m_bwireless;
    QByteArray m_ifname;

    QByteArray m_essid;
    uint8_t m_link_quality;
    uint8_t m_signal_level;
    uint8_t m_noise_level;
};


}
}



#endif // WIRELESS_H
