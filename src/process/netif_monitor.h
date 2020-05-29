#ifndef NETIF_MONITOR_H
#define NETIF_MONITOR_H

#include <mutex>
#include <thread>

#include <QThread>
#include <QSharedPointer>
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>

#include "netif_monitor_job.h"

enum packet_type {
    kTCPPacketType,
    kUDPPacketType,
    kSCTPPacketType
};

struct packet_payload_t {
    struct timeval ts;
    packet_type    type;
    ino_t          ino;
    qulonglong     rx_bytes;
    qulonglong     tx_bytes;
};

struct sock_io_stat_t {
    ino_t      ino;        // socket inode
    qulonglong rx_bytes;
    qulonglong rx_packets;
    qulonglong tx_bytes;
    qulonglong tx_packets;
};

using PacketPayload = QSharedPointer<struct packet_payload_t>;
using SockIOStat = QSharedPointer<struct sock_io_stat_t>;

class NetifMonitor : public QThread
{
    Q_OBJECT

public:
    explicit NetifMonitor(QObject *parent = nullptr);
    ~NetifMonitor() override;

    void run() override;



Q_SIGNALS:

public Q_SLOTS:

private:
    QQueue<PacketPayload> m_pendingPackets {};
    QMutex m_qlock {};
    QWaitCondition m_qwatcher {};

    QMap<ino_t, SockIOStat> m_sockIOStatMap {};
    QMutex m_mlock {};

    QThread m_jobThread;
    NetifMonitorJob *m_monitorJob {};

    static std::atomic<NetifMonitor *> m_instance;
    static std::mutex m_mutex;
};

#endif // NETIF_MONITOR_H
