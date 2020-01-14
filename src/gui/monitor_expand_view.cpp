#include <DApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <QVBoxLayout>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "monitor_expand_view.h"
#include "network_monitor.h"

DWIDGET_USE_NAMESPACE

MonitorExpandView::MonitorExpandView(QWidget *parent)
    : DFrame(parent)
{
    auto margin = DStyle::pixelMetric(style(), DStyle::PM_ContentsMargins);

    setAutoFillBackground(false);
    setBackgroundRole(DPalette::Window);
    setFrameStyle(DFrame::NoFrame);

    m_cpuMonitor = new CpuMonitor(this);
    m_memoryMonitor = new MemoryMonitor(this);
    m_networkMonitor = new NetworkMonitor(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
