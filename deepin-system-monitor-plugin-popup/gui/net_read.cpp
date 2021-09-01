#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include <QTextStream>

#include "net_read.h"


ReadNetThread::ReadNetThread()
{

}

ReadNetThread::~ReadNetThread()
{
//    wait();
}

void ReadNetThread::run()
{
    while(true) {

        //读Net数据
        qreal recvBps;
        qreal sentBps;
        unsigned long long totalRecvBytes;
        unsigned long long totalSentBytes;

        bool retNetPer = readNetData(recvBps, sentBps, totalRecvBytes, totalSentBytes);
        if(retNetPer == true)
            emit signal_NetData(recvBps, sentBps, totalRecvBytes, totalSentBytes);

        sleep(2);
    }
}

int recvBpstest = 1;
bool ReadNetThread::readNetData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes)
{
    recvBpstest++;
    recvBps = recvBpstest;
    recvBpstest++;
    sentBps = recvBpstest;

    totalRecvBytes = 100;
    totalSentBytes = 200;
    return true;
}
