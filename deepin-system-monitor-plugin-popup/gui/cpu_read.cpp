#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include <QTextStream>

#include "cpu_read.h"


ReadCpuThread::ReadCpuThread()
{

}

ReadCpuThread::~ReadCpuThread()
{
//    wait();
}

void ReadCpuThread::run()
{
    while(true) {

        //读cpu百分比netWorkItem item;
        bool retCpuPer = readCpuPer(m_cpuPer);
        if(retCpuPer == true)
            emit signal_cpuPer(m_cpuPer);

//        QList<QString> netWorksList;
//        bool retList = readCpuList(netWorksList);
//        if(retList == true)
//            emit signal_cpuList(netWorksList);

//        netWorksList.clear();

        sleep(2);
    }
}

int dfd = 1;
bool ReadCpuThread::readCpuPer(qreal &cpuPer)
{
    dfd++;
    cpuPer = dfd;
//    qInfo()<<"ReadCpuThread::readCpuPer: "<<cpuPer;
    return true;
}
