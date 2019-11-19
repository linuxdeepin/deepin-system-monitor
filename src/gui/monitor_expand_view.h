#ifndef MONITOR_EXPAND_VIEW_H
#define MONITOR_EXPAND_VIEW_H

#include <DFrame>

DWIDGET_USE_NAMESPACE

class CpuMonitor;
class MemoryMonitor;
class NetworkMonitor;

class MonitorExpandView : public DFrame
{
    Q_OBJECT

public:
    explicit MonitorExpandView(QWidget *parent = nullptr);
    ~MonitorExpandView() = default;

private:
    CpuMonitor *m_cpuMonitor;
    MemoryMonitor *m_memoryMonitor;
    NetworkMonitor *m_networkMonitor;
};

#endif  // MONITOR_EXPAND_VIEW_H
