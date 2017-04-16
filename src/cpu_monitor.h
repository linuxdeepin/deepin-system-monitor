#ifndef CPUMONITOR_H
#define CPUMONITOR_H

#include <QThread>
#include <QTimer>

const int NUM_CPU_STATES = 10;

enum CPUStates
{
    S_USER = 0,
    S_NICE,
    S_SYSTEM,
    S_IDLE,
    S_IOWAIT,
    S_IRQ,
    S_SOFTIRQ,
    S_STEAL,
    S_GUEST,
    S_GUEST_NICE
};

typedef struct CPUData
{
    std::string cpu;
    size_t times[NUM_CPU_STATES];
} CPUData;

class CpuMonitor : public QThread
{
    Q_OBJECT

public:
    CpuMonitor(QObject *parent = 0);

    void readStatsCPU(std::vector<CPUData> & entries);

    size_t getIdleTime(const CPUData & e);
    size_t getActiveTime(const CPUData & e);

    void printStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2);
    
public slots:
    void test();

protected:
    void run();
    
private:
    QTimer *timer;
};

#endif
