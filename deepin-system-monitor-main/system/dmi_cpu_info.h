// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEEPIN_SYSTEM_MONITOR_DMI_CPU_INFO_H
#define DEEPIN_SYSTEM_MONITOR_DMI_CPU_INFO_H

#include <QMap>
#include <QString>

namespace core {
namespace system {

struct DmiCpuInfo
{
    bool hasCpuFrequency {false};
    QString cpuMHz;
    QString cpuMaxMHz;
    QMap<QString, QString> cacheInfo;

    bool hasData() const
    {
        return hasCpuFrequency || !cacheInfo.isEmpty();
    }
};

} // namespace system
} // namespace core

#endif // DEEPIN_SYSTEM_MONITOR_DMI_CPU_INFO_H
