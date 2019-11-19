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
    setAutoFillBackground(false);
    setBackgroundRole(DPalette::Window);
    setFrameStyle(DFrame::NoFrame);

    m_cpuMonitor = new CpuMonitor(this);
    m_memoryMonitor = new MemoryMonitor(this);
    m_networkMonitor = new NetworkMonitor(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_memoryMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(m_networkMonitor, 0, Qt::AlignHCenter);

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    layout->setContentsMargins(margin, margin, margin, margin);
    setLayout(layout);
}
