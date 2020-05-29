#ifndef NETIF_MONITOR_JOB_H
#define NETIF_MONITOR_JOB_H

#include <pcap.h>

#include <atomic>
#include <QThread>
#include <QTimer>

void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);

class NetifMonitorJob : public QObject
{
    Q_OBJECT

public:
    explicit NetifMonitorJob(QObject *parent = nullptr);

    inline void request()
    {
        m_requested.store(true);
    }

public Q_SLOTS:
    void startMonitorJob();

Q_SIGNALS:

private Q_SLOTS:
    void dispatchPackets();

private:
    QTimer *m_timer {};

    pcap_t *m_handle {};

    std::atomic_bool m_requested {false};

    friend void pcap_callback(u_char *, const struct pcap_pkthdr *, const u_char *);
};

#endif // NETIF_MONITOR_JOB_H
