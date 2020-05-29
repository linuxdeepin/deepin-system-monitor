#include <QDebug>
#include <QDateTime>

#include "netif_monitor_job.h"
#include "netif_monitor.h"

#define PACKET_BATCH_DISPATCH_COUNT     8
#define PACKET_DISPATCH_IDLE_TIME       100

NetifMonitorJob::NetifMonitorJob(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &NetifMonitorJob::dispatchPackets);
}

void NetifMonitorJob::startMonitorJob()
{
    char errbuf[PCAP_ERRBUF_SIZE] {};
    int rc = 0;

    m_handle = pcap_create("any", errbuf);
    if (!m_handle) {
        qDebug() << "pcap_create failed: " << errbuf;
        return;
    }
    rc = pcap_setnonblock(m_handle, 1, errbuf);
    if (rc == -1) {
        qDebug() << "pcap_setnonblock failed: " << errbuf;
        pcap_close(m_handle);
        return;
    }

    rc = pcap_activate(m_handle);
    if (rc > 0) {
        qDebug() << "pcap_activate warning: " << pcap_statustostr(rc);
    } else if (rc < 0) {
        qDebug() << "pcap_setnonblock failed: " << pcap_statustostr(rc);
        pcap_close(m_handle);
        return;
    }

    m_timer->start();
}

void pcap_callback(u_char *context, const struct pcap_pkthdr *hdr, const u_char *packet)
{
    // packet payload calc
    qDebug() << "====================" << QDateTime::fromMSecsSinceEpoch(hdr->ts.tv_sec * 1000 + hdr->ts.tv_usec) << hdr->caplen;
    // try lock queue, if cant acquire, add to local queue
}

void NetifMonitorJob::dispatchPackets()
{
    int nr = 0;

    do {
        auto b = m_requested.load();
        if (b) {
            break;
        }
        nr = pcap_dispatch(m_handle, PACKET_BATCH_DISPATCH_COUNT, pcap_callback, (u_char *)this);
        if (nr == 0) {
            // if no packets are available at this moment,
            m_timer->start(PACKET_DISPATCH_IDLE_TIME);
            return;
        } else if (nr == -1) {
            qDebug() << "pcap_dispatch failed: " << pcap_geterr(m_handle);
            break;
        } else if (nr == -2) { // breakloop request
            break;
        }
    } while (true);

    pcap_close(m_handle);
}
