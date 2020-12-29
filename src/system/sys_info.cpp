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
#include "sys_info.h"

#include "common/time_period.h"
#include "common/sample.h"
#include "common/common.h"

#include <DSysInfo>

#include <QString>
#include <QtDBus>

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <errno.h>
#include <stdio.h>
#include <sys/sysinfo.h>

using namespace common::alloc;
using namespace common::core;
using namespace common::error;
DCORE_USE_NAMESPACE

#define PROC_PATH_FILE_NR "/proc/sys/fs/file-nr"
#define PROC_PATH_UPTIME "/proc/uptime"
#define PROC_PATH_STAT "/proc/stat"
#define PROC_PATH_LOADAVG "/proc/loadavg"

namespace core {
namespace system {

SysInfo::SysInfo()
    : d(new SysInfoPrivate())
{
    d->loadAvg = std::make_shared<struct load_avg_t>();
}
SysInfo::SysInfo(const SysInfo &other)
    : d(other.d)
{
}
SysInfo &SysInfo::operator=(const SysInfo &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
SysInfo::~SysInfo()
{
}

void SysInfo::readSysInfo()
{
    d->uid = getuid();
    d->euid = geteuid();
    d->gid = getgid();
    d->egid = getegid();

    d->user_name = userName(d->uid);
    d->effective_user_name = userName(d->euid);
    if (d->effective_user_name.isEmpty())
        d->effective_user_name = QByteArray(getenv("USER"));

    d->group_name = groupName(d->gid);
    d->effective_group_name = groupName(d->egid);
    if (d->effective_group_name.isEmpty())
        d->effective_group_name = QByteArray(getenv("GROUP"));

    d->nfds = read_file_nr();
    d->hostname = read_hostname();
    d->arch = read_arch();
    d->version = read_version();
    read_uptime(d->uptime);
    read_btime(d->btime);
    read_loadavg(d->loadAvg);
}

quint32 SysInfo::read_file_nr()
{
    FILE *fp;
    uFile fPtr;
    char buf[128] {};
    unsigned int file_nr = 0;

    errno = 0;
    if ((fp = fopen(PROC_PATH_UPTIME, "r"))) {
        fPtr.reset(fp);

        if (fgets(buf, sizeof(buf), fp)) {
            int nm = sscanf(buf, "%u", &file_nr);
            if (nm == 1)
                return file_nr;

            print_errno(errno, QString("read %1 failed").arg(PROC_PATH_FILE_NR));
            return 0;
        } // ::if(fgets)
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_FILE_NR));
    return 0;
}

QString SysInfo::read_hostname()
{
    QDBusInterface busIf("org.freedesktop.hostname1",
                         "/org/freedesktop/hostname1",
                         "org.freedesktop.hostname1",
                         QDBusConnection::systemBus());
    auto name = busIf.property("Hostname");
    return name.toString();
}

QString SysInfo::read_arch()
{
    QString buffer {};

    struct utsname os {
    };
    auto rc = uname(&os);
    if (!rc)
        buffer = os.machine;

    return buffer;
}

QString SysInfo::read_version()
{
    auto ver = QString("%1 %2")
                   .arg(DSysInfo::deepinVersion())
                   .arg(DSysInfo::deepinTypeDisplayName());
    return ver;
}

void SysInfo::read_uptime(struct timeval &uptime)
{
    FILE *fp;
    uFile fPtr;
    char buf[128] {};
    long up_sec, up_usec;

    errno = 0;
    if ((fp = fopen(PROC_PATH_UPTIME, "r"))) {
        fPtr.reset(fp);

        if (fgets(buf, sizeof(buf), fp)) {
            int nm = sscanf(buf, "%ld.%ld", &up_sec, &up_usec);
            if (nm == 2) {
                uptime.tv_sec = up_sec;
                uptime.tv_usec = up_usec * 10000;

                return;
            }
        } // ::if(gets)

        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_UPTIME));
        return;
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_UPTIME));
}

void SysInfo::read_btime(struct timeval &btime)
{
    FILE *fp;
    uFile fPtr;
    char line[4095] {};
    long nsec {0};

    errno = 0;
    if ((fp = fopen(PROC_PATH_STAT, "r"))) {
        fPtr.reset(fp);

        while (fgets(line, sizeof(line), fp)) {
            if (!strncmp(line, "btime", 5)) {
                int nm = sscanf(line + 5, "%ld", &nsec);
                if (nm == 1) {
                    btime.tv_sec = nsec;
                    btime.tv_usec = 0;

                    return;
                }
            } // ::if(cmp)
        } // ::while

        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_STAT));
        return;
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_STAT));
}

void SysInfo::read_loadavg(LoadAvg &loadAvg)
{
    struct sysinfo info {
    };
    int rc = sysinfo(&info);
    if (!rc) {
        loadAvg->lavg_1m = info.loads[0];
        loadAvg->lavg_5m = info.loads[1];
        loadAvg->lavg_15m = info.loads[2];

        return;
    }

    print_errno(errno, QString("call sysinfo failed"));
}

} // namespace system
} // namespace core
