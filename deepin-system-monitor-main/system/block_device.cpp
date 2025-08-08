// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "block_device.h"
#include "ddlog.h"
#include <QDateTime>
#include <QFile>
#include <QSharedData>
#include <QTextStream>
#include "system/sys_info.h"
#include "common/common.h"
namespace core {
namespace system {

BlockDevice::BlockDevice()
    : d(new BlockDevicePrivate())
{
    qCDebug(app) << "BlockDevice default constructor";
}
BlockDevice::BlockDevice(const BlockDevice &other)
    : d(other.d)
{
    qCDebug(app) << "BlockDevice copy constructor";
}
BlockDevice &BlockDevice::operator=(const BlockDevice &rhs)
{
    qCDebug(app) << "BlockDevice assignment operator";
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
BlockDevice::~BlockDevice()
{
    // qCDebug(app) << "BlockDevice destructor";
}
void BlockDevice::setDeviceName(const QByteArray &deviceName)
{
    qCDebug(app) << "Setting device name to" << deviceName;
    d->name = deviceName;
    readDeviceInfo();
}

void BlockDevice::readDeviceInfo()
{
    qCDebug(app) << "Reading device info for" << d->name;
    QFile file(PROC_PATH_DISK);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(app) << "Failed to open" << PROC_PATH_DISK;
        return;
    }
    QList<QStringList> strList;
    QTextStream in(&file);

    QString line;
    do{
        line = in.readLine();
        if (!line.contains("loop")){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            strList << line.split(" ", QString::SkipEmptyParts);
#else
            strList << line.split(" ", Qt::SkipEmptyParts);
#endif
        }
    }while (!line.isNull());
    file.close();
    qCDebug(app) << "Read" << strList.size() << "lines from" << PROC_PATH_DISK;

    for (int i = 0; i < strList.size(); ++i) {
        QStringList deviceInfo = strList[i];
        if (deviceInfo.size() > 16 && deviceInfo[2] == d->name) {
            qCDebug(app) << "Found device" << d->name << "in disk stats";
            m_time_sec = QDateTime::currentSecsSinceEpoch();
            timevalList[0] = timevalList[1];
            timevalList[1] = SysInfo::instance()->uptime();

            qint64 interval = m_time_sec - d->_time_Sec > 0 ? m_time_sec - d->_time_Sec : 1;
            calcDiskIoStates(deviceInfo);
            readDeviceModel();
            d->capacity = readDeviceSize(deviceInfo[2]);
            if (d->read_iss != 0)
                d->r_ps = (deviceInfo[3].toULongLong() - d->read_iss) / static_cast<quint64>(interval);
            if (d->blk_read != 0)
                d->rsec_ps = (deviceInfo[5].toULongLong() - d->blk_read) / static_cast<quint64>(interval);
            if (d->blk_wrtn != 0)
                d->wsec_ps = (deviceInfo[9].toULongLong() - d->blk_wrtn) / static_cast<quint64>(interval);
            if (d->read_merged != 0)
                d->rrqm_ps = (deviceInfo[4].toULongLong() - d->read_merged) / static_cast<quint64>(interval);
            if (d->write_com != 0)
                d->w_ps = (deviceInfo[7].toULongLong() - d->write_com) / static_cast<quint64>(interval);
            if (d->write_merged != 0)
                d->wrqm_ps = (deviceInfo[8].toULongLong() - d->write_merged) / static_cast<quint64>(interval);
            d->blk_read = deviceInfo[5].toULongLong();
            d->bytes_read = deviceInfo[5].toULongLong() * SECTOR_SIZE;
            if (deviceInfo[3].toULongLong() != 0)
                d->p_rrqm =  deviceInfo[4].toDouble() / deviceInfo[3].toDouble() * 100;
            d->tps = deviceInfo[3].toULongLong() + deviceInfo[7].toULongLong();
            d->blk_wrtn = deviceInfo[9].toULongLong();
            d->bytes_wrtn = deviceInfo[9].toULongLong() * SECTOR_SIZE;
            if (deviceInfo[7].toULongLong() != 0)
                d->p_wrqm = deviceInfo[8].toULongLong() / deviceInfo[7].toULongLong() * 100;
            d->read_iss = deviceInfo[3].toULongLong();
            d->write_com = deviceInfo[7].toULongLong();
            d->read_merged = deviceInfo[4].toULongLong();
            d->write_merged = deviceInfo[8].toULongLong();
            d->discard_sector = deviceInfo[16].toULongLong();
            d->_time_Sec = QDateTime::currentSecsSinceEpoch();
            qCDebug(app) << "Finished processing device info for" << d->name;
            break;
        }
    }

}

void BlockDevice::readDeviceModel()
{
    QString Path = QString(SYSFS_PATH_MODEL).arg(d->name.data());
    qCDebug(app) << "Reading device model from" << Path;
    QFile file(Path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(app) << "Failed to open" << Path;
        return;
    }
    QString model = file.readLine().replace("\n", "").trimmed();
    d->model = model;
    qCDebug(app) << "Device model for" << d->name << "is" << model;
    file.close();
}

quint64 BlockDevice::readDeviceSize(const QString &deviceName)
{
    QString path = QString(SYSFS_PATH_SIZE).arg(deviceName);
    qCDebug(app) << "Reading device size from" << path;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(app) << "Failed to open" << path;
        return 0;
    }
    quint64 size = file.readLine().replace("\n", "").trimmed().toULongLong() * SECTOR_SIZE;
    file.close();
    qCDebug(app) << "Device" << deviceName << "size is" << size;
    return size;
}

void BlockDevice::calcDiskIoStates(const QStringList &diskInfo)
{
    qCDebug(app) << "Calculating disk IO states for" << d->name;
    quint64 curr_read_sector =  diskInfo[5].toULongLong();
    quint64 curr_write_sector = diskInfo[9].toULongLong();
    quint64 curr_discard_sector = diskInfo[16].toULongLong();
    timeval cur_time = timevalList[1];
    timeval prev_time = timevalList[0];

    // read increment between interval
    auto rdiff = (curr_read_sector > d->blk_read) ? (curr_read_sector - d->blk_read) : 0;
    // write increment between interval
    auto wdiff = (curr_write_sector > d->blk_wrtn) ? (curr_write_sector - d->blk_wrtn) : 0;
    // discarded increment between interval
    auto ddiff = (curr_discard_sector > d->discard_sector) ? (curr_discard_sector - d->discard_sector) : 0;
    // calculate actual size
    auto rsize = rdiff * SECTOR_SIZE;
    auto wsize = (wdiff + ddiff) * SECTOR_SIZE;
    //  auto interval = (m_time_sec > d->_time_Sec) ? (m_time_sec - d->_time_Sec) : 1;
    auto ltime = prev_time.tv_sec + prev_time.tv_usec * 1. / 1000000;
    auto rtime = cur_time.tv_sec + cur_time.tv_usec * 1. / 1000000;
    auto interval = (rtime > ltime) ? (rtime - ltime) : 1;

    d->read_speed = rsize / static_cast<quint64>(interval);
    d->wirte_speed = wsize / static_cast<quint64>(interval);
    qCDebug(app) << d->name << "read speed:" << d->read_speed << "B/s, write speed:" << d->wirte_speed << "B/s";
}

} // namespace system
} // namespace core
