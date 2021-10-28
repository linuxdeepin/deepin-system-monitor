/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
// project self
#include "datadealsingleton.h"
#include "system/diskio_info.h"
#include "system/mem.h"
#include "system/net_info.h"
#include "system/system_monitor.h"
#include "system/device_db.h"
#include "model/cpu_info_model.h"
#include "system/block_device_info_db.h"
#include "common/datacommon.h"
#include "dbus/dbuscallmaininterface.h"
#include "config.h"

//Qt
#include <QProcess>

#ifdef IS_LOONGARCH_TYPE
#define POPUP_WAITING_TIME 1000
#else
#define POPUP_WAITING_TIME 500
#endif


QMutex DataDealSingleton::mutex;
QAtomicPointer<DataDealSingleton> DataDealSingleton::instance;

DataDealSingleton &DataDealSingleton::getInstance()
{
    if (instance.testAndSetOrdered(nullptr, nullptr))
    {
        QMutexLocker locker(&mutex);

        instance.testAndSetOrdered(nullptr, new DataDealSingleton);
    }
    return *instance;
}

bool DataDealSingleton::readCpuPer(qreal &cpuPer)
{
    internalMutex.lockForRead();
    cpuPer = CPUInfoModel::instance()->cpuAllPercent();
    internalMutex.unlock();
    return true;
}

bool DataDealSingleton::readMemInfo(QString &memUsage, QString &memTotal, QString &memPercent, QString &swapUsage, QString &swapTotal, QString &swapPercent)
{
    internalMutex.lockForRead();
    MemInfo *curMemInfo = core::system::DeviceDB::instance()->memInfo();
    memUsage = formatUnit((curMemInfo->memTotal() - curMemInfo->memAvailable()) << 10, B, 1);
    memTotal = formatUnit(curMemInfo->memTotal() << 10, B, 1);
    memPercent = QString::number((curMemInfo->memTotal() - curMemInfo->memAvailable()) * 1. / curMemInfo->memTotal() * 100, 'f', 1);

    swapUsage = formatUnit((curMemInfo->swapTotal() - curMemInfo->swapFree()) << 10, B, 1);
    swapTotal = formatUnit(curMemInfo->swapTotal() << 10, B, 1);
    swapPercent = QString::number((curMemInfo->swapTotal() - curMemInfo->swapFree()) * 1. / curMemInfo->swapTotal() * 100, 'f', 1);
    internalMutex.unlock();

    return true;
}

bool DataDealSingleton::readNetInfo(QString &netReceive, QString &netTotalReceive, QString &netSend, QString &totalSend)
{
    internalMutex.lockForRead();
    NetInfo *netInfo = core::system::DeviceDB::instance()->netInfo();
    netReceive = formatUnit(netInfo->recvBps(), B, 1, true);
    netSend = formatUnit(netInfo->sentBps(), B, 1, true);
    netTotalReceive = formatUnit(netInfo->totalRecvBytes(), B, 1);
    totalSend = formatUnit(netInfo->totalSentBytes(), B, 1);
    internalMutex.unlock();

    return true;
}

bool DataDealSingleton::readDiskInfo(QString &diskRead, QString &diskTotalSize, QString &diskWrite, QString &diskAvailable)
{
    internalMutex.lockForRead();
    core::system::DiskIOInfo *diskIOInfo = core::system::DeviceDB::instance()->diskIoInfo();

    diskRead = formatUnit(diskIOInfo->diskIoReadBps(), B, 1, true);
    diskWrite = formatUnit(diskIOInfo->diskIoWriteBps(), B, 1, true);

    const QList<BlockDevice> &infoDB = DeviceDB::instance()->blockDeviceInfoDB()->deviceList();
    QMap<QString, BlockDevice> mapInfo;
    qulonglong totalDiskAva = 0;
    for (int i = 0; i < infoDB.size(); ++i) {
        mapInfo.insert(infoDB[i].deviceName(), infoDB[i]);

        totalDiskAva += infoDB[i].capacity();
    }

    diskTotalSize = formatUnit(totalDiskAva, B, 1);
    internalMutex.unlock();

    return true;
}

bool DataDealSingleton::sendJumpWidgetMessage(const QString &dbusMessage)
{
    //1000ms内重复点击,不响应
    if (m_popupTrickTimer->isActive()) {
        return false;
        }
    m_popupTrickTimer->start();
    //1.先唤醒主进程
    bool rt = QProcess::startDetached(Globals::DEEPIN_SYSTEM_MONITOR_PATH);
    if (true == rt) {
        //2.跳转DBUS
        QTimer::singleShot(POPUP_WAITING_TIME, this, [=]() {
            DbusCallMainInterface::getInstance()->jumpWidget(dbusMessage);
        });
    }
    return rt;
}

DataDealSingleton::DataDealSingleton(QObject *parent)
    :QObject (parent)
    , m_popupTrickTimer(new QTimer(this))
    {
    m_popupTrickTimer->setInterval(1000);
    m_popupTrickTimer->setSingleShot(true);

    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &DataDealSingleton::sigDataUpdate);
}

DataDealSingleton::~DataDealSingleton()
{

}
