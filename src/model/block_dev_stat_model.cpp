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
#include "block_dev_stat_model.h"

#include "common/common.h"

using namespace common::core;
using namespace common::format;

// model constructor
BlockDevStatModel::BlockDevStatModel(const TimePeriod &period, QObject *parent)
    : QAbstractTableModel(parent)
    , m_ioSampleDB(new IOSample(period))
    , m_iopsSampleDB(new IOPSSample(period))
{
}

int BlockDevStatModel::rowCount(const QModelIndex &) const
{
    if (m_iopsSampleDB && m_ioSampleDB && m_iopsSampleDB->count() == m_ioSampleDB->count())
        return m_iopsSampleDB->count();
    else
        return 0;
}

int BlockDevStatModel::columnCount(const QModelIndex &) const
{
    return kStatPropMax;
}

QVariant BlockDevStatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_ioSampleDB.get() || !m_iopsSampleDB.get())
        return {};

    if (index.row() < 0 || index.row() >= int(m_iopsSampleDB->count()))
        return {};

    auto *io = m_ioSampleDB->sample(index.row());
    auto *iops = m_iopsSampleDB->sample(index.row());

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (index.column()) {
        case kStatTotalRead: {
            if (io)
                return formatUnit(io->data.inBytes);

            break;
        }
        case kStatTotalWrite: {
            if (io)
                return formatUnit(io->data.outBytes);

            break;
        }
        case kStatReadSpeed: {
            if (iops)
                return formatUnit(iops->data.inBps, B, 1, true);

            break;
        }
        case kStatWriteSpeed: {
            if (iops)
                return formatUnit(iops->data.outBps, B, 1, true);

            break;
        }
        default:
            break;
        } // ::switch
    } // ::if(displayRole || accessibleRole)
    else if (role == kValueRole) {
        switch (index.column()) {
        case kStatTotalRead: {
            if (io)
                return io->data.inBytes;

            break;
        }
        case kStatTotalWrite: {
            if (io)
                return io->data.outBytes;

            break;
        }
        case kStatReadSpeed: {
            if (iops)
                return iops->data.inBps;

            break;
        }
        case kStatWriteSpeed: {
            if (iops)
                return iops->data.outBps;

            break;
        }
        default:
            break;
        } // ::switch
    } // ::if(valueRole

    return {};
}
