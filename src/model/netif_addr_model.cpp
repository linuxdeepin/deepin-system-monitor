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
#include "netif_addr_model.h"

NetifAddrModel::NetifAddrModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_addr4DB {}
    , m_addr6DB {}
    , m_addrDB {}
{
}

int NetifAddrModel::rowCount(const QModelIndex &) const
{
    return m_addrDB.size() + m_addr4DB.size() + m_addr6DB.size();
}

int NetifAddrModel::columnCount(const QModelIndex &) const
{
    return kAddrTypeMax;
}

QVariant NetifAddrModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (index.column()) {
        case kINet4Addr: {
            if (index.row() >= m_addr4DB.size())
                break;

            auto *addr4 = reinterpret_cast<struct inet_addr4_t *>(m_addr4DB[index.row()].get());
            if (addr4)
                return addr4->addr;

            break;
        }
        case kINet6Addr: {
            if (index.row() >= m_addr6DB.size())
                break;

            auto *addr6 = reinterpret_cast<struct inet_addr6_t *>(m_addr6DB[index.row()].get());
            if (addr6)
                return addr6->addr;

            break;
        }
        case kOtherAddr: {
            if (index.row() >= m_addrDB.size())
                break;

            auto *oaddr = m_addrDB[index.row()].get();
            if (oaddr)
                return oaddr->addr;

            break;
        }
        default:
            break;
        } // ::switch
    } // ::if

    return {};
}
