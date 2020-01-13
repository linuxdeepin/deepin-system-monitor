#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H

#include <QExplicitlySharedDataPointer>

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
    void setMemory(qulonglong memory);

    qreal getReadKbs() const;
    void setReadKbs(qreal rkbs);

    qreal getWriteKbs() const;
    void setWriteKbs(qreal wkbs);

    qulonglong getSentBytes() const;
    void setSentBytes(qulonglong sb);

    qulonglong getRecvBytes() const;
    void setRecvBytes(qulonglong rb);

    qreal getSentKbs() const;
    void setSentKbs(qreal skbs);

    qreal getRecvKbs() const;
    void setRecvKbs(qreal rkbs);

    bool operator<(const ProcessEntry &other) const;

private:
    QExplicitlySharedDataPointer<ProcessEntryData> data;
};

#endif  // PROCESS_ENTRY_H
