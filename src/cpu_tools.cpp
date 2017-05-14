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
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "cpu_tools.h"

namespace cpuTools {

    std::vector<cpuStruct> getCpuTimes()
    {
        std::vector<cpuStruct> times;

        // adapted from https://github.com/scaidermern/top-processes/blob/master/top_proc.c#L54
        FILE* file = fopen("/proc/stat", "r");
        if (file == NULL) {
            perror("Could not open stat file");
            return times;
        }

        char buffer[1024];
        memset(buffer,1,1024); // initialise the buffer with known data but not 0 (null) so that the next while loop still works
        // skip the first line
        while(buffer[0] != '\n' && buffer != NULL) {
            buffer[0] = (char)fgetc(file);
        }

        while(buffer != NULL) {
            unsigned long long user = 0, nice = 0, system = 0, idle = 0;
            // added between Linux 2.5.41 and 2.6.33, see man proc(5)
            unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guestnice = 0;

            char* ret = fgets(buffer, sizeof(buffer) - 1, file);
            if (ret == NULL) {
                perror("Could not read stat file");
                fclose(file);
                return times;
            } else if (strncmp(buffer, "cpu", 3)) {
                break; // we got all of the cpu lines
            }

            sscanf(buffer,
                   "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestnice);

            // sum everything up (except guest and guestnice since they are already included
            // in user and nice, see http://unix.stackexchange.com/q/178045/20626)

            cpuStruct cpu = {idle + iowait, user + nice + system + irq + softirq + steal};
            times.push_back(cpu);
        }

        fclose(file);
        return times;
    }

    std::vector<double> calculateCpuPercentages(std::vector<cpuStruct> now, std::vector<cpuStruct> prev)
    {
        std::vector<double> times;
        if (now.size() != prev.size()) {
            return times;
        }

        for(unsigned int i=0; i<now.size(); i++) {
            cpuStruct n, p;
            n = now.at(i);
            p = prev.at(i);

            long long unsigned totald = ((n.idle + n.nonIdle) - (p.idle + p.nonIdle));
            times.push_back((totald - (n.idle - p.idle)) / (double)totald * 100.0);
        }

        return times;

        /*
          from : http://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
        PrevIdle = previdle + previowait
        Idle = idle + iowait

        PrevNonIdle = prevuser + prevnice + prevsystem + previrq + prevsoftirq + prevsteal
        NonIdle = user + nice + system + irq + softirq + steal

        PrevTotal = PrevIdle + PrevNonIdle
        Total = Idle + NonIdle

        # differentiate: actual value minus the previous one
        totald = Total - PrevTotal
        idled = Idle - PrevIdle

        CPU_Percentage = (totald - idled)/totald*/
    }

}
