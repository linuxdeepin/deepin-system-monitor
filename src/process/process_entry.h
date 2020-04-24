#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H

#include <QSharedDataPointer>

#include "utils.h"

using namespace Utils;

class ProcessEntryData;
class QIcon;

class ProcessEntry
{
public:
    ProcessEntry();
    ProcessEntry(const ProcessEntry &);
    ProcessEntry &operator=(const ProcessEntry &);
    ~ProcessEntry();

    pid_t getPID() const;
    void setPID(pid_t pid);

    int getPriority() const;
    void setPriority(int priority);

    char getState() const;
    void setState(char state);

    qreal getCPU() const;
    void setCPU(qreal cpu);

    QIcon getIcon() const;
    void setIcon(const QIcon &icon);

    QString getName() const;
    void setName(const QString &name);

    QString getDisplayName() const;
    void setDisplayName(const QString &displayName);

    QString getUserName() const;
    void setUserName(const QString &userName);

    qulonglong getMemory() const;
    void setMemory(qulonglong shm);

    qulonglong getSharedMemory() const;
    void setSharedMemory(qulonglong shm);

    qreal getDiskRead() const;
    void setDiskRead(qreal diskRead);

    qreal getDiskWrite() const;
    void setDiskWrite(qreal diskWrite);

    void setDiskStats(const DiskStatus &stats);

    qulonglong getSentBytes() const;
    void setSentBytes(qulonglong sentBytes);

    qulonglong getRecvBytes() const;
    void setRecvBytes(qulonglong recvBytes);

    qreal getSentKbs() const;
    void setSentKbs(qreal sentKbs);

    qreal getRecvKbs() const;
    void setRecvKbs(qreal recvKbs);

    void setNetworkStats(const NetworkStatus stats);

    bool operator<(const ProcessEntry &other) const;

private:
    QSharedDataPointer<ProcessEntryData> data;
};

#endif  // PROCESS_ENTRY_H
