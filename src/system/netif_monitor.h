#ifndef NETIF_MONITOR_H
#define NETIF_MONITOR_H

#include "netif_packet_capture.h"

class NetifMonitor : public QObject
{
    Q_OBJECT
public:
    explicit NetifMonitor(QObject *parent = nullptr);
    ~NetifMonitor() override;

public:
    inline bool getSockIOStatByInode(ino_t ino, SockIOStat &stat)
    {
        return m_netifPacketCapture->getSockIOStatByInode(ino, stat);
    }

private:
    NetifPacketCapture *m_netifPacketCapture;
};

#endif // NETIF_MONITOR_H
