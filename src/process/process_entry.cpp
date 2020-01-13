#include <QIcon>

#include "process_entry.h"
#include "utils.h"

using namespace Utils;

class ProcessEntryData : public QSharedData
{
public:
    ProcessEntryData() {}
    ProcessEntryData(const ProcessEntryData &other)
        : QSharedData(other)
        , m_pid {other.m_pid}
        , m_priority {other.m_priority}
        , m_state {other.m_state}
        , m_cpu {other.m_cpu}
        , m_icon {other.m_icon}
        , m_name {other.m_name}
        , m_displayName {other.m_displayName}
        , m_userName {other.m_userName}
        , m_memory {other.m_memory}
        , m_readKbs {other.m_readKbs}
        , m_writeKbs {other.m_writeKbs}
        , m_sentBytes {other.m_sentBytes}
        , m_recvBytes {other.m_recvBytes}
        , m_sentKbs {other.m_sentKbs}
        , m_recvKbs {other.m_recvKbs}
    {
    }
    ProcessEntryData &operator=(const ProcessEntryData &other)
    {
        Q_UNUSED(padding);
        if (this != &other) {
            m_pid = other.m_pid;
            m_priority = other.m_priority;
            m_state = other.m_state;
            m_cpu = other.m_cpu;
            m_icon = other.m_icon;
            m_name = other.m_name;
            m_displayName = other.m_displayName;
            m_userName = other.m_userName;
            m_memory = other.m_memory;
            m_readKbs = other.m_readKbs;
            m_writeKbs = other.m_writeKbs;
            m_sentBytes = other.m_sentBytes;
            m_recvBytes = other.m_recvBytes;
            m_sentKbs = other.m_sentKbs;
            m_recvKbs = other.m_recvKbs;
        }
        return *this;
    }
    ~ProcessEntryData() {}

    friend class ProcessEntry;

private:
    // pid
    pid_t m_pid {0};
    // pirority
    int m_priority {0};
    // process status
    char m_state {};
    char padding[3];
    // cpu -> default(descending)
    qreal m_cpu {.0};
    // icon
    QIcon m_icon {};
    // process name
    QString m_name {};
    // displayName
    QString m_displayName {};
    // uid
    QString m_userName {};
    // mem
    qulonglong m_memory {0};
    // disk read/write stats
    qreal m_readKbs {0};
    qreal m_writeKbs {0};

    // network up/down stats
    qulonglong m_sentBytes {0};
    qulonglong m_recvBytes {0};
    qreal m_sentKbs {0};
    qreal m_recvKbs {0};
};

ProcessEntry::ProcessEntry()
    : data(new ProcessEntryData)
{
}

ProcessEntry::ProcessEntry(const ProcessEntry &rhs)
    : data(rhs.data)
{
}

ProcessEntry &ProcessEntry::operator=(const ProcessEntry &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

ProcessEntry::~ProcessEntry() {}

pid_t ProcessEntry::getPID() const
{
    return data->m_pid;
}

void ProcessEntry::setPID(pid_t pid)
{
    data->m_pid = pid;
}

int ProcessEntry::getPriority() const
{
    return data->m_priority;
}

void ProcessEntry::setPriority(int priority)
{
    data->m_priority = priority;
}

char ProcessEntry::getState() const
{
    return data->m_state;
}

void ProcessEntry::setState(char state)
{
    data->m_state = state;
}

qreal ProcessEntry::getCPU() const
{
    return data->m_cpu;
}

void ProcessEntry::setCPU(qreal cpu)
{
    data->m_cpu = cpu;
}

QIcon ProcessEntry::getIcon() const
{
    return data->m_icon;
}

void ProcessEntry::setIcon(const QIcon &icon)
{
    data->m_icon = icon;
}

QString ProcessEntry::getName() const
{
    return data->m_name;
}

void ProcessEntry::setName(const QString &name)
{
    data->m_name = name;
}

QString ProcessEntry::getDisplayName() const
{
    return data->m_displayName;
}

void ProcessEntry::setDisplayName(const QString &displayName)
{
    data->m_displayName = displayName;
}

QString ProcessEntry::getUserName() const
{
    return data->m_userName;
}

void ProcessEntry::setUserName(const QString &userName)
{
    data->m_userName = userName;
}

qulonglong ProcessEntry::getMemory() const
{
    return data->m_memory;
}

void ProcessEntry::setMemory(qulonglong memory)
{
    data->m_memory = memory;
}

qreal ProcessEntry::getReadKbs() const
{
    return data->m_readKbs;
}

void ProcessEntry::setReadKbs(qreal rkbs)
{
    data->m_readKbs = rkbs;
}

qreal ProcessEntry::getWriteKbs() const
{
    return data->m_writeKbs;
}

void ProcessEntry::setWriteKbs(qreal wkbs)
{
    data->m_writeKbs = wkbs;
}

qulonglong ProcessEntry::getSentBytes() const
{
    return data->m_sentBytes;
}

void ProcessEntry::setSentBytes(qulonglong sb)
{
    data->m_sentBytes = sb;
}

qulonglong ProcessEntry::getRecvBytes() const
{
    return data->m_recvBytes;
}

void ProcessEntry::setRecvBytes(qulonglong rb)
{
    data->m_recvBytes = rb;
}

qreal ProcessEntry::getSentKbs() const
{
    return data->m_sentKbs;
}

void ProcessEntry::setSentKbs(qreal skbs)
{
    data->m_sentKbs = skbs;
}

qreal ProcessEntry::getRecvKbs() const
{
    return data->m_recvKbs;
}

void ProcessEntry::setRecvKbs(qreal rkbs)
{
    data->m_recvKbs = rkbs;
}

bool ProcessEntry::operator<(const ProcessEntry &other) const
{
    return data < other.data;
}
