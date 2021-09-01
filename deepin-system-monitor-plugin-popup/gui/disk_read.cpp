#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include <QTextStream>

#include "disk_read.h"


ReadDiskThread::ReadDiskThread()
{

}

ReadDiskThread::~ReadDiskThread()
{
//    wait();
}

void ReadDiskThread::run()
{
    while(true) {

        //读Disk数据
        qreal recvBps;
        qreal sentBps;
        unsigned long long totalRecvBytes;
        unsigned long long totalSentBytes;

        bool retDiskPer = readDiskData(recvBps, sentBps, totalRecvBytes, totalSentBytes);
        if(retDiskPer == true)
            emit signal_DiskData(recvBps, sentBps, totalRecvBytes, totalSentBytes);

        sleep(2);
    }
}

int recvw = 1;
bool ReadDiskThread::readDiskData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes)
{
    recvw++;
    recvBps = recvw;
    recvw++;
    sentBps = recvw;

    totalRecvBytes = 100;
    totalSentBytes = 200;
    return true;
}
