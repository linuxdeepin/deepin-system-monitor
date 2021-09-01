#ifndef READ_CPU_H
#define READ_CPU_H

#include <QDebug>
#include <QProcess>
#include <QThread>


class ReadCpuThread : public QThread
{
    Q_OBJECT

public:
    ReadCpuThread();
    ~ReadCpuThread();

protected:
    void run() override;
private:
    qreal m_cpuPer = 0;
    bool readCpuPer(qreal &cpuPer);


signals:
    void signal_cpuPer(qreal cpuper);
};

#endif // WLAN_MANAGEER_H
