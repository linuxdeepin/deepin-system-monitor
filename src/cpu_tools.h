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
#ifndef CPUTOOLS_H
#define CPUTOOLS_H

namespace cpuTools {
typedef struct cpuStruct {
    long long unsigned idle, nonIdle;
} cpuStruct;
std::vector<cpuStruct> getCpuTimes();
std::vector<double> calculateCpuPercentages(std::vector<cpuStruct> now, std::vector<cpuStruct> prev);
}

#endif // CPUTOOLS_H
