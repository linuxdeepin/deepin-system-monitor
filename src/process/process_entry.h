#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

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

    uid_t getUID() const;
    void setUID(uid_t uid);

    gid_t getGID() const;
    void setGID(gid_t gid);

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

    QString getCmdline() const;
    void setCmdline(const QString &cmdline);

    time_t getStartTime() const;
    void setStartTime(time_t startTime);

    QString getUserName() const;
    void setUserName(const QString &userName);

    qulonglong getMemory() const;
    void setMemory(qulonglong memory);

    qreal getReadBps() const;
    void setReadBps(qreal readBps);

    qreal getWriteBps() const;
    void setWriteBps(qreal writeBps);

    qulonglong getSentBytes() const;
    void setSentBytes(qulonglong sb);

    qulonglong getRecvBytes() const;
    void setRecvBytes(qulonglong rb);

    qreal getSentBps() const;
    void setSentBps(qreal sentBps);

    qreal getRecvBps() const;
    void setRecvBps(qreal recvBps);

    bool operator<(const ProcessEntry &other) const;

private:
    QExplicitlySharedDataPointer<ProcessEntryData> data;
};

Q_DECLARE_METATYPE(ProcessEntry)

#endif  // PROCESS_ENTRY_H
