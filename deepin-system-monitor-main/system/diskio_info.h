/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DISKIO_INFO_H
#define DISKIO_INFO_H

#include <QMap>
#include <memory>

namespace core {
namespace system {

struct disk_io_stat {
    unsigned long long total_ios;
    unsigned long long read_ios;            // # of reads completed
    unsigned long long read_sectors;        // # of sectors read
    unsigned long long write_ios;           // # of writes completed
    unsigned long long write_sectors;       // # of sectors written
    unsigned long long discard_ios;         // # of discards completed
    unsigned long long discard_sectors;     // # of sectors discarded
};

class DiskIOInfo
{
    enum StatIndex { kLastStat = 0, kCurrentStat = 1, kStatCount = kCurrentStat + 1 };

public:
    explicit DiskIOInfo();

    void update();
    qreal diskIoReadBps();
    qreal diskIoWriteBps();

private:
    void readDiskIOStats();
    void calDiskIoStates();

private:
    QMap<QString, std::shared_ptr<disk_io_stat>> m_diskIoStatMap[kStatCount];
    timeval timevalList[kStatCount] = {timeval{0, 0}, timeval{0, 0}};

    qreal m_readBps = 0;
    qreal m_writeBps = 0;
};

} // namespace system
} // namespace core

#endif // DISKIO_INFO_H
