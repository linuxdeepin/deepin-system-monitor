// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ddlog.h"
#include "perf.h"
#include <QDateTime>
#include <QDebug>

#include <sys/time.h>
using namespace DDLog;

DebugTimeManager *DebugTimeManager::s_Instance = nullptr;

DebugTimeManager::DebugTimeManager()
{
    qCDebug(app) << "DebugTimeManager constructor";
}

void DebugTimeManager::clear()
{
    qCDebug(app) << "DebugTimeManager clear";
    m_MapPoint.clear();
}

void DebugTimeManager::beginPointLinux(const QString &point, const QString &status)
{
    // qCDebug(app) << "DebugTimeManager beginPointLinux:" << point << "status:" << status;
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    PointInfo info;
    info.desc = status;
    info.time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    m_MapPoint.insert(point, info);
}
void DebugTimeManager::endPointLinux(const QString &point)
{
    // qCDebug(app) << "DebugTimeManager endPointLinux:" << point;
    if (m_MapPoint.find(point) != m_MapPoint.end()) {
        // qCDebug(app) << "point found, calculating time";
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        m_MapPoint[point].time = tv.tv_sec * 1000 + tv.tv_usec / 1000 - m_MapPoint[point].time;
        qCInfo(app) << QString("[GRABPOINT] %1 %2 time=%3ms").arg(point).arg(m_MapPoint[point].desc).arg(m_MapPoint[point].time);
    }
}
