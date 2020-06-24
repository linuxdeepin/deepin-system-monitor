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
#include <QMutexLocker>
#include <QDebug>

#include "netif_packet_parser.h"

class NetifMonitorJob;

struct sock_io_stat_t {
    ino_t      ino;        // socket inode
    qulonglong rx_bytes;
    qulonglong rx_packets;
    qulonglong tx_bytes;
    qulonglong tx_packets;
};

using SockIOStat = QSharedPointer<struct sock_io_stat_t>;

class NetifMonitor : public QThread
{
    Q_OBJECT

public:
    explicit NetifMonitor(QObject *parent = nullptr);
    ~NetifMonitor() override;

    inline void requestQuit()
    {
        m_quitRequested.store(true);
        m_pktqWatcher.wakeAll();
    }

    inline bool getSockIOStatByInode(ino_t ino, SockIOStat &stat)
    {
        bool ok = false;

        m_sockIOStatMapLock.lock();

        if (m_sockIOStatMap.contains(ino)) {
            stat = m_sockIOStatMap[ino];
            m_sockIOStatMap.remove(ino);
            ok = true;
        }

        m_sockIOStatMapLock.unlock();

        return ok;
    }

    void run() override;

public Q_SLOTS:

private:
    PacketPayloadQueue  m_pendingPackets        {};
    QMutex              m_pktqLock              {};
    QWaitCondition      m_pktqWatcher           {};
    PacketPayloadQueue  m_localPendingPackets   {}; // local cache

    QMap<ino_t, SockIOStat> m_sockIOStatMap     {};
    QMutex                  m_sockIOStatMapLock {};

    QThread             m_packetMonitorThread;
    NetifMonitorJob    *m_packetMonitorJob     {};

    std::atomic_bool m_quitRequested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
    friend class NetifMonitorJob;
};

#endif // NETIF_MONITOR_H
