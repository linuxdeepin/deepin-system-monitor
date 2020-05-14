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
        , m_uid {other.m_uid}
        , m_gid {other.m_gid}
        , m_priority {other.m_priority}
        , m_state {other.m_state}
        , m_cpu {other.m_cpu}
        , m_icon {other.m_icon}
        , m_name {other.m_name}
        , m_displayName {other.m_displayName}
        , m_cmdline {other.m_cmdline}
        , m_startTime {other.m_startTime}
        , m_userName {other.m_userName}
        , m_memory {other.m_memory}
        , m_readBps {other.m_readBps}
        , m_writeBps {other.m_writeBps}
        , m_sentBytes {other.m_sentBytes}
        , m_recvBytes {other.m_recvBytes}
        , m_sentBps {other.m_sentBps}
        , m_recvBps {other.m_recvBps}
    {
    }
    ProcessEntryData &operator=(const ProcessEntryData &other)
    {
        Q_UNUSED(padding);
        if (this != &other) {
            m_pid = other.m_pid;
            m_uid = other.m_uid;
            m_gid = other.m_gid;
            m_priority = other.m_priority;
            m_state = other.m_state;
            m_cpu = other.m_cpu;
            m_icon = other.m_icon;
            m_name = other.m_name;
            m_displayName = other.m_displayName;
            m_cmdline = other.m_cmdline;
            m_startTime = other.m_startTime;
            m_userName = other.m_userName;
            m_memory = other.m_memory;
            m_readBps = other.m_readBps;
            m_writeBps = other.m_writeBps;
            m_sentBytes = other.m_sentBytes;
            m_recvBytes = other.m_recvBytes;
            m_sentBps = other.m_sentBps;
            m_recvBps = other.m_recvBps;
        }
        return *this;
    }
    ~ProcessEntryData() {}

    friend class ProcessEntry;

private:
    // pid
    pid_t m_pid {0};
    uid_t m_uid {0};
    gid_t m_gid {0};    // primary group
    // pirority
    int m_priority {0};
    // process status
    char m_state {};
    char padding[3];
    // cpu -> default(descending)
    qreal m_cpu {0.};
    // icon
    QIcon m_icon {};
    // process name
    QString m_name {};
    // displayName
    QString m_displayName {};
    // cmdline
    QString m_cmdline {};
    // process start time in seconds since epoch
    time_t m_startTime {};
    // uid
    QString m_userName {};
    // mem
    qulonglong m_memory {0};

    // disk read/write stats
    qreal m_readBps {0};
    qreal m_writeBps {0};

    // network up/down stats
    qulonglong m_sentBytes {0};
    qulonglong m_recvBytes {0};
    qreal m_sentBps {0};
    qreal m_recvBps {0};
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
        data.operator = (rhs.data);
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

uid_t ProcessEntry::getUID() const
{
    return data->m_uid;
}

void ProcessEntry::setUID(uid_t uid)
{
    data->m_uid = uid;
}

gid_t ProcessEntry::getGID() const
{
    return data->m_gid;
}

void ProcessEntry::setGID(gid_t gid)
{
    data->m_gid = gid;
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

QString ProcessEntry::getCmdline() const
{
    return data->m_cmdline;
}

void ProcessEntry::setCmdline(const QString &cmdline)
{
    data->m_cmdline = cmdline;
}

time_t ProcessEntry::getStartTime() const
{
    return data->m_startTime;
}

void ProcessEntry::setStartTime(time_t startTime)
{
    data->m_startTime = startTime;
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

qreal ProcessEntry::getReadBps() const
{
    return data->m_readBps;
}

void ProcessEntry::setReadBps(qreal readBps)
{
    data->m_readBps = readBps;
}

qreal ProcessEntry::getWriteBps() const
{
    return data->m_writeBps;
}

void ProcessEntry::setWriteBps(qreal writeBps)
{
    data->m_writeBps = writeBps;
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

qreal ProcessEntry::getSentBps() const
{
    return data->m_sentBps;
}

void ProcessEntry::setSentBps(qreal sentBps)
{
    data->m_sentBps = sentBps;
}

qreal ProcessEntry::getRecvBps() const
{
    return data->m_recvBps;
}

void ProcessEntry::setRecvBps(qreal recvBps)
{
    data->m_recvBps = recvBps;
}

bool ProcessEntry::operator<(const ProcessEntry &other) const
{
    return data < other.data;
}
