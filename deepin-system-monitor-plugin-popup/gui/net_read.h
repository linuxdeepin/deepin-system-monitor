#ifndef READ_NET_H
#define READ_NET_H

#include <QDebug>
#include <QProcess>
#include <QThread>


class ReadNetThread : public QThread
{
    Q_OBJECT

public:
    ReadNetThread();
    ~ReadNetThread();

protected:
    void run() override;
private:
    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
    bool readNetData(qreal &recvBps, qreal &sentBps, unsigned long long &totalRecvBytes, unsigned long long &totalSentBytes);

signals:
    void signal_NetData(qreal recvBps, qreal sentBps, unsigned long long totalRecvBytes, unsigned long long totalSentBytes);
};

#endif // WLAN_MANAGEER_H
