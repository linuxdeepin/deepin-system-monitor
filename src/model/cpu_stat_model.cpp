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
#include "cpu_stat_model.h"

// Model constructor
CPUStatModel::CPUStatModel(const TimePeriod &period, QObject *parent)
    : QAbstractTableModel(parent)
    , m_statSampleDB(new CPUStatSample(period))
    , m_usageSampleDB(new CPUUsageSample(period))
{
}

CPUStatModel::~CPUStatModel()
{
}

int CPUStatModel::rowCount(const QModelIndex &) const
{
    if (m_statSampleDB && m_usageSampleDB
            && m_statSampleDB->count() == m_usageSampleDB->count())
        return m_usageSampleDB->count();
    else
        return 0;
}

int CPUStatModel::columnCount(const QModelIndex &) const
{
    return kStatPropMax;
}

QVariant CPUStatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_usageSampleDB || !m_statSampleDB)
        return {};

    int row = index.row();
    if (row < 0 || row >= int(m_usageSampleDB->count()))
        return {};

    auto *stat = m_statSampleDB->sample(index.row());
    auto *usage = m_usageSampleDB->sample(index.row());
    if (!stat || !usage)
        return {};

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (index.column()) {
        case kStatUserTime: {
            return stat->stat->user;
        }
        case kStatNiceTime: {
            return stat->stat->nice;
        }
        case kStatSystemTime: {
            return stat->stat->sys;
        }
        case kStatIdleTime: {
            return stat->stat->idle;
        }
        case kStatIOWaitTime: {
            return stat->stat->iowait;
        }
        case kStatHardIRQ: {
            return stat->stat->hardirq;
        }
        case kStatSoftIRQ: {
            return stat->stat->softirq;
        }
        case kStatStealTime: {
            return stat->stat->steal;
        }
        case kStatGuestTime: {
            return stat->stat->guest;
        }
        case kStatGuestNice: {
            return stat->stat->guest_nice;
        }
        case kStatTotalUsage: {
            return usage->stat->total;
        }
        case kStatIdleUsage: {
            return usage->stat->idle;
        }
        default:
            break;
        } // ::switch
    } // ::if

    return {};
}
