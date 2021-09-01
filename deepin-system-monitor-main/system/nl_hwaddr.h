/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
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

#ifndef NL_HWADDR_H
#define NL_HWADDR_H
#include <QByteArray>
#include <QString>
using namespace std;

class NLHWAddr
{
public:
    explicit NLHWAddr(const QByteArray &ifname);

public:
    QByteArray conn_type() const;
    bool is_virtual() const;
    int sa_family() const;
private:
    void initData();
    QByteArray m_conn_type;
    QByteArray m_ifname;
    int m_sa_family;
    bool  m_is_virtual;
};

inline bool NLHWAddr::is_virtual() const
{
    return m_is_virtual;
}

inline int NLHWAddr::sa_family() const
{
    return m_sa_family;
}

inline QByteArray NLHWAddr::conn_type() const
{
    return m_conn_type;
}


#endif // NL_HWADDR_H
