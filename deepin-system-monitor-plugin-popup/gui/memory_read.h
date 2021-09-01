#ifndef READ_MEM_H
#define READ_MEM_H

#include <QDebug>
#include <QProcess>
#include <QThread>


class ReadMemThread : public QThread
{
    Q_OBJECT

public:
    ReadMemThread();
    ~ReadMemThread();

protected:
    void run() override;
private:
    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
    bool readMemData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes);

signals:
    void signal_MemData(qreal recvBps, qreal sentBps, unsigned long long totalRecvBytes, unsigned long long totalSentBytes);
};

#endif // WLAN_MANAGEER_H
