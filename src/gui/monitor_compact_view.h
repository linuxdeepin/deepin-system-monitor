#ifndef MONITOR_COMPACT_VIEW_H
#define MONITOR_COMPACT_VIEW_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class CompactCpuMonitor;
class CompactMemoryMonitor;
class CompactNetworkMonitor;
class CompactDiskMonitor;

class MonitorCompactView : public DFrame
{
    Q_OBJECT

public:
    explicit MonitorCompactView(QWidget *parent = nullptr);
    ~MonitorCompactView() = default;

private:
    CompactCpuMonitor *m_cpuMonitor;
    CompactMemoryMonitor *m_memoryMonitor;
    CompactNetworkMonitor *m_networkMonitor;
    CompactDiskMonitor *m_diskMonitor;
};

#endif  // MONITOR_COMPACT_VIEW_H
