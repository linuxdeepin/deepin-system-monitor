// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "diskio_info.h"
#include "ddlog.h"
#include "common/common.h"
#include "system/sys_info.h"

#include <ctype.h>
#include <errno.h>
#include <sched.h>

#include <QScopedArrayPointer>

using namespace common::error;
using namespace common::alloc;
using namespace DDLog;

namespace core {
namespace system {

#define MAX_NAME_LEN 128
#define PROC_PATH_DISK      "/proc/diskstats"
#define SYSFS_PATH_BLOCK    "/sys/block"

DiskIOInfo::DiskIOInfo()
{
    qCDebug(app) << "DiskIOInfo constructor";
}

DiskIOInfo::~DiskIOInfo()
{
    // qCDebug(app) << "DiskIOInfo destructor";
}

qreal DiskIOInfo::diskIoReadBps()
{
    return m_readBps;
}

qreal DiskIOInfo::diskIoWriteBps()
{
    return m_writeBps;
}

void DiskIOInfo::readDiskIOStats()
{
    qCDebug(app) << "Reading disk IO stats from" << PROC_PATH_DISK;
    bool b = false;
    FILE *fp;
    const size_t bsiz = 2048;
    QScopedArrayPointer<char> line(new char[bsiz] {});
    unsigned int major, minor;
    char dev_name[MAX_NAME_LEN + 1];

    // ignore any partition stats here, ref: sysstat#common.c#is_device
    auto is_block_dev = [ = ](char *dev_name) -> bool {
        char syspath[PATH_MAX];
        char *slash;

        // replace any '/' characters in device names if any
        while ((slash = strchr(dev_name, '/')))
        {
            *slash = '!';
        }

        snprintf(syspath, sizeof(syspath), "%s/%s", SYSFS_PATH_BLOCK, dev_name);
        return (!access(syspath, F_OK));
    };

    if ((fp = fopen(PROC_PATH_DISK, "r")) == nullptr) {
        qCWarning(app) << "Failed to open" << PROC_PATH_DISK << ":" << strerror(errno);
        return;
    }

    timevalList[kLastStat] = timevalList[kCurrentStat];
    m_diskIoStatMap[kLastStat].clear();
    m_diskIoStatMap[kLastStat] = m_diskIoStatMap[kCurrentStat];
    m_diskIoStatMap[kCurrentStat].clear();

    while (fgets(line.data(), bsiz, fp)) {
        int rc;
        auto stat = std::make_shared<disk_io_stat>();
        //************************1**2**3****4********5********6*******7********************8********9**
        rc = sscanf(line.data(), "%u %u %128s %llu %*u %llu %*u %llu %*u %llu %*u %*u %*u %*u %llu %*u %llu",
                    &major,
                    &minor,
                    dev_name,
                    &stat->read_ios,
                    &stat->read_sectors,
                    &stat->write_ios,
                    &stat->write_sectors,
                    &stat->discard_ios,
                    &stat->discard_sectors);
        // discard io stats might not be available
        if (rc >= 7) {
            if (is_block_dev(dev_name)) {
                // per block dev stats
                qCDebug(app) << "Parsed disk stats for device:" << dev_name;
                timevalList[kCurrentStat] = SysInfo::instance()->uptime();
                m_diskIoStatMap[kCurrentStat][dev_name] = stat;
                b = true;
            }
        }
    }
    b = !ferror(fp) && b;
    fclose(fp);
    if (!b) {
        qCWarning(app) << "Failed to read disk I/O statistics from" << PROC_PATH_DISK << ":" << strerror(errno);
    }
    qCDebug(app) << "Finished reading disk IO stats. Found" << m_diskIoStatMap[kCurrentStat].size() << "block devices.";
}

void DiskIOInfo::calDiskIoStates()
{
    qCDebug(app) << "Calculating disk IO states...";
    qulonglong cur_read_sectors = 0;
    qulonglong cur_write_sectors = 0;
    qulonglong cur_discard_sectors = 0;
    timeval cur_time = timevalList[kCurrentStat];;

    qulonglong prev_read_sectors = 0;
    qulonglong prev_write_sectors = 0;
    qulonglong prev_discard_sectors = 0;
    timeval prev_time = timevalList[kLastStat];

    for (auto iter = m_diskIoStatMap[kCurrentStat].begin(); iter != m_diskIoStatMap[kCurrentStat].end(); iter++) {
        auto disk_io_iter = iter.value();
        if (disk_io_iter) {
            cur_read_sectors += disk_io_iter->read_sectors;
            cur_write_sectors += disk_io_iter->write_sectors;
            cur_discard_sectors += disk_io_iter->discard_sectors;
        }
    }

    for (auto iter = m_diskIoStatMap[kLastStat].begin(); iter != m_diskIoStatMap[kLastStat].end(); iter++) {
        auto disk_io_iter = iter.value();
        if (disk_io_iter) {
            prev_read_sectors += disk_io_iter->read_sectors;
            prev_write_sectors += disk_io_iter->write_sectors;
            prev_discard_sectors += disk_io_iter->discard_sectors;
        }
    }

    qCDebug(app) << "Current sectors: read=" << cur_read_sectors << "write=" << cur_write_sectors;
    qCDebug(app) << "Previous sectors: read=" << prev_read_sectors << "write=" << prev_write_sectors;

    // read increment between interval
    auto rdiff = (cur_read_sectors > prev_read_sectors) ? (cur_read_sectors - prev_read_sectors) : 0;
    // write increment between interval
    auto wdiff = (cur_write_sectors > prev_write_sectors) ? (cur_write_sectors - prev_write_sectors) : 0;
    // discarded increment between interval
    auto ddiff = (cur_discard_sectors > prev_discard_sectors) ? (cur_discard_sectors - prev_discard_sectors) : 0;

    // calculate actual size
    auto rsize = rdiff * SECTOR_SIZE;
    auto wsize = (wdiff + ddiff) * SECTOR_SIZE;

    auto ltime = prev_time.tv_sec + prev_time.tv_usec * 1. / 1000000;
    auto rtime = cur_time.tv_sec + cur_time.tv_usec * 1. / 1000000;
    auto interval = (rtime > ltime) ? (rtime - ltime) : 1;

    m_readBps = rsize / interval;
    m_writeBps = wsize / interval;
    qCDebug(app) << "Calculated disk IO: Read=" << m_readBps << "B/s, Write=" << m_writeBps << "B/s";
}

void DiskIOInfo::update()
{
    qCDebug(app) << "Updating DiskIOInfo...";
    readDiskIOStats();

    calDiskIoStates();
    qCDebug(app) << "DiskIOInfo update finished.";
}

} // namespace system
} // namespace core
