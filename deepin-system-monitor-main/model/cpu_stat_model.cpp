// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu_stat_model.h"
#include "ddlog.h"

using namespace DDLog;

// Model constructor
CPUStatModel::CPUStatModel(const TimePeriod &period, QObject *parent)
    : QAbstractTableModel(parent)
    , m_statSampleDB(new CPUStatSample(period))
    , m_usageSampleDB(new CPUUsageSample(period))
{
    qCDebug(app) << "CPUStatModel constructor";
}

CPUStatModel::~CPUStatModel()
{
    // qCDebug(app) << "CPUStatModel destructor";
}

int CPUStatModel::rowCount(const QModelIndex &) const
{
    qCDebug(app) << "CPUStatModel::rowCount";
    if (m_statSampleDB && m_usageSampleDB
            && m_statSampleDB->count() == m_usageSampleDB->count())
        return m_usageSampleDB->count();
    else
        return 0;
}

int CPUStatModel::columnCount(const QModelIndex &) const
{
    qCDebug(app) << "CPUStatModel::columnCount";
    return kStatPropMax;
}

QVariant CPUStatModel::data(const QModelIndex &index, int role) const
{
    qCDebug(app) << "CPUStatModel::data for index" << index << "role" << role;
    if (!index.isValid() || !m_usageSampleDB || !m_statSampleDB) {
        qCDebug(app) << "Invalid index or data";
        return {};
    }

    int row = index.row();
    if (row < 0 || row >= int(m_usageSampleDB->count())) {
        qCDebug(app) << "Row out of bounds";
        return {};
    }

    auto *stat = m_statSampleDB->sample(index.row());
    auto *usage = m_usageSampleDB->sample(index.row());
    if (!stat || !usage) {
        qCDebug(app) << "Stat or usage data is null";
        return {};
    }

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        qCDebug(app) << "Display/Accessible role";
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
