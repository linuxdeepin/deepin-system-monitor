#include <QDebug>

#include "netif_monitor.h"
#include "netif_monitor_job.h"

NetifMonitor::NetifMonitor(QObject *parent) : QThread(parent)
{
    // packet monitor job
    m_packetMonitorJob = new NetifMonitorJob(this);
    m_packetMonitorJob->moveToThread(&m_packetMonitorThread);
    connect(&m_packetMonitorThread, &QThread::finished, m_packetMonitorJob, &QObject::deleteLater);
    connect(&m_packetMonitorThread, &QThread::started, m_packetMonitorJob, &NetifMonitorJob::startMonitorJob);
    m_packetMonitorThread.start();
}

NetifMonitor::~NetifMonitor()
{
    m_packetMonitorJob->requestQuit();
    m_packetMonitorThread.quit();
    m_packetMonitorThread.wait();
}

void NetifMonitor::run()
{
    while (!m_quitRequested.load()) {
        m_pktqLock.lock();      // +++m_pktqLock+++
        m_pktqWatcher.wait(&m_pktqLock);

        if (m_quitRequested.load())
            break;

        m_localPendingPackets.append(m_pendingPackets);
        m_pendingPackets.clear();
        m_pktqLock.unlock();    // ---m_pktqLock---

        // process payload queue
        while (!m_localPendingPackets.isEmpty()) {
            auto payload = m_localPendingPackets.dequeue();

            // lock sockiostatmap
            m_sockIOStatMapLock.lock();     // +++m_sockIOStatMapLock+++

            // sum up sock io stat by inode
            auto ino = payload->ino;
            if (m_sockIOStatMap.contains(ino)) {
                auto &hist = m_sockIOStatMap[ino];
                if (payload->direction == kInboundPacket) {
                    hist->rx_bytes += payload->payload;
                    hist->rx_packets++;
                } else if (payload->direction == kOutboundPacket) {
                    hist->tx_bytes += payload->payload;
                    hist->tx_packets++;
                }
            } else {
                auto stat = QSharedPointer<struct sock_io_stat_t>::create();
                stat->ino = payload->ino;
                if (payload->direction == kInboundPacket) {
                    stat->rx_bytes = payload->payload;
                    stat->rx_packets++;
                } else if (payload->direction == kOutboundPacket) {
                    stat->tx_bytes = payload->payload;
                    stat->tx_packets++;
                }
                m_sockIOStatMap[stat->ino] = stat;
            }

            // unlock sockiostatmap
            m_sockIOStatMapLock.unlock();   // ---m_sockIOStatMapLock---
        }
    }
}
