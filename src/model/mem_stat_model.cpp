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
#include "mem_stat_model.h"

#include "common/common.h"

using namespace common::format;

// Model constructor
MemStatModel::MemStatModel(const TimePeriod &period, QObject *parent)
    : QAbstractTableModel(parent)
    , m_stat(new MemStatSample(period))
{
}

MemStatModel::~MemStatModel()
{
}

int MemStatModel::rowCount(const QModelIndex &) const
{
    return m_stat->count();
}

QVariant MemStatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_stat)
        return {};

    int row = index.row();
    if (row < 0 || row >= int(m_stat->count()))
        return {};

    auto *sample = m_stat->sample(row);
    if (!sample)
        return {};

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (index.column()) {
        case kStatUsedMem: {
            return formatUnit(sample->stat->memUsed << 10, B, 2);
        }
        case kStatTotalMem: {
            return formatUnit(sample->stat->memTotal << 10, B, 1);
        }
        case kStatUsedSwap: {
            return formatUnit(sample->stat->swapUsed << 10, B, 2);
        }
        case kStatTotalSwap: {
            return formatUnit(sample->stat->swapTotal << 10, B, 1);
        }
        default:
            break;
        } // ::switch
    } else if (role == kValueRole) {
        switch (index.column()) {
        case kStatUsedMem: {
            return sample->stat->memUsed;
        }
        case kStatTotalMem: {
            return sample->stat->memTotal;
        }
        case kStatUsedSwap: {
            return sample->stat->swapUsed;
        }
        case kStatTotalSwap: {
            return sample->stat->swapTotal;
        }
        default:
            break;
        } // ::switch
    } // ::if

    return {};
}

int MemStatModel::columnCount(const QModelIndex &) const
{
    return kStatPropMax;
}
