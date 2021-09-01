#ifndef READ_DISK_H
#define READ_DISK_H

#include <QDebug>
#include <QProcess>
#include <QThread>


class ReadDiskThread : public QThread
{
    Q_OBJECT

public:
    ReadDiskThread();
    ~ReadDiskThread();

protected:
    void run() override;
private:
    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
    bool readDiskData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes);

signals:
    void signal_DiskData(qreal recvBps, qreal sentBps, unsigned long long totalRecvBytes, unsigned long long totalSentBytes);
};

#endif // WLAN_MANAGEER_H
