// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netif_addr_model.h"
#include "ddlog.h"

using namespace DDLog;

NetifAddrModel::NetifAddrModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_addr4DB {}
    , m_addr6DB {}
    , m_addrDB {}
{
    qCDebug(app) << "NetifAddrModel constructor";
}

int NetifAddrModel::rowCount(const QModelIndex &) const
{
    int count = m_addrDB.size() + m_addr4DB.size() + m_addr6DB.size();
    // qCDebug(app) << "NetifAddrModel rowCount:" << count;
    return count;
}

int NetifAddrModel::columnCount(const QModelIndex &) const
{
    // qCDebug(app) << "NetifAddrModel columnCount:" << kAddrTypeMax;
    return kAddrTypeMax;
}

QVariant NetifAddrModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qCDebug(app) << "NetifAddrModel::data invalid index";
        return {};
    }
    // qCDebug(app) << "NetifAddrModel::data for index" << index << "role" << role;

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
