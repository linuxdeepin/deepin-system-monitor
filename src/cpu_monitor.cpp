#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "cpu_monitor.h"

CpuMonitor::CpuMonitor(QObject *parent) : QThread(parent)
{
    // timer = new QTimer();
    // connect(timer, SIGNAL(timeout()), this, SLOT(test()));
    // timer->start(1000);
}

void CpuMonitor::run()
{
    
}

void CpuMonitor::test()
{
    std::vector<CPUData> entries1;
    std::vector<CPUData> entries2;

    // snapshot 1
    readStatsCPU(entries1);

    // 100ms pause
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // snapshot 2
    readStatsCPU(entries2);

    // print output
    // printStats(entries1, entries2);
}

void CpuMonitor::readStatsCPU(std::vector<CPUData> & entries)
{
    std::ifstream fileStat("/proc/stat");

    std::string line;

    const std::string STR_CPU("cpu");
    const std::size_t LEN_STR_CPU = STR_CPU.size();
    const std::string STR_TOT("total");

    while(std::getline(fileStat, line)) {
        // CPU stats line found.
        if(!line.compare(0, LEN_STR_CPU, STR_CPU)) {
            std::istringstream ss(line);

            // Store entry.
            entries.emplace_back(CPUData());
            CPUData & entry = entries.back();

            // Read cpu label.
            ss >> entry.cpu;

            // Remove "cpu" from the label when it's a processor number.
            if(entry.cpu.size() > LEN_STR_CPU) {
                entry.cpu.erase(0, LEN_STR_CPU);
            } else {            // replace "cpu" with "tot" when it's total values
                entry.cpu = STR_TOT;
            }

            // Read times.
            for(int i = 0; i < NUM_CPU_STATES; ++i) {
                ss >> entry.times[i];
            }
        }
    }
}

size_t CpuMonitor::getIdleTime(const CPUData & e)
{
    return e.times[S_IDLE] + e.times[S_IOWAIT];
}

size_t CpuMonitor::getActiveTime(const CPUData & e)
{
    return e.times[S_USER] + e.times[S_NICE] + e.times[S_SYSTEM] + e.times[S_IRQ] + e.times[S_SOFTIRQ] + e.times[S_STEAL] + e.times[S_GUEST] + e.times[S_GUEST_NICE];
}

void CpuMonitor::printStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{
    const size_t NUM_ENTRIES = entries1.size();

    for(size_t i = 0; i < NUM_ENTRIES; ++i)
    {
        const CPUData & e1 = entries1[i];
        const CPUData & e2 = entries2[i];

        std::cout.width(6);
        std::cout << e1.cpu;

        const float ACTIVE_TIME = static_cast<float>(getActiveTime(e2) - getActiveTime(e1));
        const float IDLE_TIME   = static_cast<float>(getIdleTime(e2) - getIdleTime(e1));
        const float TOTAL_TIME  = ACTIVE_TIME + IDLE_TIME;

        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.width(6);
        std::cout.precision(2);
        std::cout << (100.f * ACTIVE_TIME / TOTAL_TIME) << "%" << std::endl;
    }
}
