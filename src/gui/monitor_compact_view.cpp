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
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_diskMonitor, 0, Qt::AlignHCenter);

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    //    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
