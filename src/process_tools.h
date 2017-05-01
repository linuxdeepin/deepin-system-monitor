/*
 * Copyright (C) 2017 Lily Rivers (VioletDarkKitty)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef PROCESSTOOLS
#define PROCESSTOOLS
#include <QString>
#include <proc/readproc.h>
#include <QIcon>
#include <unordered_map>

namespace processTools {
QString getProcessName(proc_t* p);
double calculateCPUPercentage(const proc_t* before, const proc_t* after, const unsigned long long &cpuTime);
QString getProcessCmdline(pid_t pid);
QString getProcessStartDate(unsigned long long start_time);
QString getProcessStatus(proc_t* p);
unsigned long long getTotalCpuTime();
QIcon getProcessIconFromName(QString procName, std::unordered_map<QString, QIcon> &processIconMapCache);
}

#endif // PROCESSTOOLS

