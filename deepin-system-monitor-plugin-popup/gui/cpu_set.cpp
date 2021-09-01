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
#include "cpu_set.h"
#include "common/common.h"
#include "common/sample.h"

#include <QMap>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <QProcess>

#include <ctype.h>
#include <errno.h>
#include <sched.h>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

#define PROC_PATH_STAT "/proc/stat"
#define PROC_PATH_CPUINFO "/proc/cpuinfo"


void print_errno(decltype(errno) e, const QString msg)
{
    qWarning() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
}

CPUSet::CPUSet()
{
}

CPUSet::~CPUSet()
{
}

struct uFileClose {
    void operator()(FILE *fp)
    {
        fclose(fp);
    }
};


using uFile = std::unique_ptr<FILE, uFileClose>;
void CPUSet::read_stats()
{
    FILE *fp;
    uFile fPtr;
    QByteArray line(BUFSIZ, '\0');
    int ncpu = 0;
    int nr;

    if (!(fp = fopen(PROC_PATH_STAT, "r"))) {
        print_errno(errno, QString("open %1 failed").arg(PROC_PATH_STAT));
        return;
    } // ::if(fopen)
    fPtr.reset(fp);

    while (fgets(line.data(), BUFSIZ, fp)) {
        if (!strncmp(line.data(), "cpu ", 4)) {
            if (!m_stat) {
                m_stat = std::make_shared<struct cpu_stat_t>();
                *m_stat = {};
            }

            if (m_stat) {
                // all cpu stat in jiffies
                nr = sscanf(line.data() + 5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                            &m_stat->user,
                            &m_stat->nice,
                            &m_stat->sys,
                            &m_stat->idle,
                            &m_stat->iowait,
                            &m_stat->hardirq,
                            &m_stat->softirq,
                            &m_stat->steal,
                            &m_stat->guest,
                            &m_stat->guest_nice);

                if (nr == 10) {
                    // usage calc
                    QByteArray cpu {"cpu"};
                    m_stat->cpu = cpu;

                    if (!m_usage) {
                        m_usage = std::make_shared<struct cpu_usage_t>();
                        *m_usage = {};
                    }

                    if (m_usage) {
                        m_usage->cpu = cpu;
                        m_usage->total = m_stat->user + m_stat->nice + m_stat->sys + m_stat->idle + m_stat->iowait + m_stat->hardirq + m_stat->softirq + m_stat->steal;
                        m_usage->idle = m_stat->idle + m_stat->iowait;
                    } // ::if(usage)
                } else
                    print_errno(errno, QString("read %1 failed, cpu").arg(PROC_PATH_STAT));
            }
        }  else if (!strncmp(line.data(), "btime", 5)) {
            // read boot time in seconds since epoch
            struct timeval btime {
            };
            long nsec {};
            int nm = sscanf(line.data() + 5, "%ld", &nsec);
            if (nm == 1) {
                btime.tv_sec = nsec;
                btime.tv_usec = 0;

                // set sysinfo btime
//                auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
//                monitor->sysInfo()->set_btime(btime);
            } // ::if(nm == 1)
        } // ::if(btime)
    } // ::while(fgets)

    if (ferror(fp))
        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_STAT));
}


//qreal cpupc(const SampleFrame<cpu_usage_t> *lhs, const SampleFrame<cpu_usage_t> *rhs)
//   {
//       if (!lhs)
//           return 0;

//       if (!rhs)
//           return qreal(lhs->stat->total - lhs->stat->idle) * 1. / lhs->stat->total * 100;

//       auto totald = (rhs->stat->total > lhs->stat->total) ? (rhs->stat->total - lhs->stat->total) : 0;
//       auto idled = (rhs->stat->idle > lhs->stat->idle) ? (rhs->stat->idle - lhs->stat->idle) : 0;

//       return qreal(totald - idled) * 1. / totald * 100;
//   }

//QList<qreal> cpuPercentList() const
//{
//    QList<qreal> percentList;
//    const auto &usageSampleList = m_singleUsageSample.values();
//    for (const auto &sample : usageSampleList) {
//        const auto &pair = sample->recentSamplePair();
//        percentList << CPUUsageSampleFrame::cpupc(pair.first, pair.second);
//    }
//    return percentList;
//}

//qreal cpuAllPercent() const
//{
//    auto pair = m_overallUsageSample->recentSamplePair();
//    return CPUUsageSampleFrame::cpupc(pair.first, pair.second);
//}
