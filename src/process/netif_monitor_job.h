#ifndef NETIF_MONITOR_JOB_H
#define NETIF_MONITOR_JOB_H

#include <pcap.h>

#include <atomic>
#include <QThread>
#include <QTimer>

#include "netif_packet_parser.h"
#include "system_stat.h"

class NetifMonitor;

void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

class NetifMonitorJob : public QObject
{
    Q_OBJECT

public:
    explicit NetifMonitorJob(NetifMonitor *, QObject *parent = nullptr);

    inline void requestQuit()
    {
        m_quitRequested.store(true);
    }

public Q_SLOTS:
    void startMonitorJob();

private:
    void dispatchPackets();
    void refreshIfAddrsHashCache();

private:
    SockStatMap     m_sockStats {};
    QMap<uint64_t, int> m_ifaddrsHashCache; // key: hash code generated from ifaddr; value: not used

    QTimer *m_timer {};

    NetifMonitor       *m_netifMonitor         {};
    pcap_t             *m_handle               {};
    PacketPayloadQueue  m_localPendingPackets  {};

    std::atomic_bool m_quitRequested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
};

#endif // NETIF_MONITOR_JOB_H
