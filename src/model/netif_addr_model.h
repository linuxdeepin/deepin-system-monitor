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
#ifndef NETIF_ADDR_MODEL_H
#define NETIF_ADDR_MODEL_H

#include "system/netif_info_db.h"
#include "system/netif.h"

#include <QAbstractTableModel>

using namespace core::system;

class NetifInfoModel;

class NetifAddrModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum AddrType {
        kINet4Addr,
        kINet6Addr,
        kOtherAddr,

        kAddrTypeMax
    };

    explicit NetifAddrModel(QObject *parent = nullptr);
    virtual ~NetifAddrModel() override = default;

    int inet4AddrCount() const;
    int inet6AddrCount() const;
    int otherAddrCount() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QList<INetAddr> m_addr4DB; // inet
    QList<INetAddr> m_addr6DB; // inet4
    QList<INetAddr> m_addrDB; // others

    friend class NetifInfoModel;
};

inline int NetifAddrModel::inet4AddrCount() const
{
    return m_addr4DB.size();
}

inline int NetifAddrModel::inet6AddrCount() const
{
    return m_addr6DB.size();
}

inline int NetifAddrModel::otherAddrCount() const
{
    return m_addrDB.size();
}

#endif // NETIF_ADDR_MODEL_H
