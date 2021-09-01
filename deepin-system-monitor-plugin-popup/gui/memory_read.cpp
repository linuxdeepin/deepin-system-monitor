#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include <QTextStream>

#include "memory_read.h"


ReadMemThread::ReadMemThread()
{

}

ReadMemThread::~ReadMemThread()
{
//    wait();
}

void ReadMemThread::run()
{
    while(true) {

        //读Mem数据
        qreal recvBps;
        qreal sentBps;
        unsigned long long totalRecvBytes;
        unsigned long long totalSentBytes;

        bool retMemPer = readMemData(recvBps, sentBps, totalRecvBytes, totalSentBytes);
        if(retMemPer == true)
            emit signal_MemData(recvBps, sentBps, totalRecvBytes, totalSentBytes);

        sleep(2);
    }
}


bool ReadMemThread::readMemData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes)
{
//    recvw++;
//    recvBps = recvw;
//    recvw++;
//    sentBps = recvw;

//    totalRecvBytes = 100;
//    totalSentBytes = 200;
    return true;
}
