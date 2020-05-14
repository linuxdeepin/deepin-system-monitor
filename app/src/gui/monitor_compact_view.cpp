#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <QVBoxLayout>

#include "compact_cpu_monitor.h"
#include "compact_disk_monitor.h"
#include "compact_memory_monitor.h"
#include "compact_network_monitor.h"
#include "monitor_compact_view.h"
#include "process/system_monitor.h"

DWIDGET_USE_NAMESPACE

MonitorCompactView::MonitorCompactView(QWidget *parent)
    : DFrame(parent)
{
    setAutoFillBackground(false);
    setBackgroundRole(DPalette::Window);
    setFrameStyle(DFrame::NoFrame);

    m_cpuMonitor = new CompactCpuMonitor(this);
    m_memoryMonitor = new CompactMemoryMonitor(this);
    m_networkMonitor = new CompactNetworkMonitor(this);
    m_diskMonitor = new CompactDiskMonitor(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_diskMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    setLayout(layout);
}
