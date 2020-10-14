/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "process_entry.h"

#include "utils.h"

#include <QIcon>

using namespace Utils;

/**
 * @brief Process entry data
 */
class ProcessEntryData : public QSharedData
{
public:
    // constructor
    ProcessEntryData() {}
    // copy constructor
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
    // copy assignment
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
    // destructor
    ~ProcessEntryData() {}

    friend class ProcessEntry;

private:
    // pid
    pid_t m_pid {0};
    // user id
    uid_t m_uid {0};
    // group id
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
    // user name
    QString m_userName {};
    // memory
    qulonglong m_memory {0};

    // disk read speed
    qreal m_readBps {0};
    // disk write speed
    qreal m_writeBps {0};

    // transferred data
    qulonglong m_sentBytes {0};
    // received data
    qulonglong m_recvBytes {0};
    // outbound transfer speed
    qreal m_sentBps {0};
    // inbound transfer speed
    qreal m_recvBps {0};
};

// constructor
ProcessEntry::ProcessEntry()
    : data(new ProcessEntryData)
{
}

// copy constructor
ProcessEntry::ProcessEntry(const ProcessEntry &rhs)
    : data(rhs.data)
{
}

// copy assignment
ProcessEntry &ProcessEntry::operator=(const ProcessEntry &rhs)
{
    if (this != &rhs)
        data.operator = (rhs.data);
    return *this;
}

// destructor
ProcessEntry::~ProcessEntry() {}

// Get process Id
pid_t ProcessEntry::getPID() const
{
    return data->m_pid;
}

// Set process Id
void ProcessEntry::setPID(pid_t pid)
{
    data->m_pid = pid;
}

// Get user Id
uid_t ProcessEntry::getUID() const
{
    return data->m_uid;
}

// Set user id
void ProcessEntry::setUID(uid_t uid)
{
    data->m_uid = uid;
}

// Get group Id
gid_t ProcessEntry::getGID() const
{
    return data->m_gid;
}

// Set group Id
void ProcessEntry::setGID(gid_t gid)
{
    data->m_gid = gid;
}

// Get priority of the process
int ProcessEntry::getPriority() const
{
    return data->m_priority;
}

// Set process priority
void ProcessEntry::setPriority(int priority)
{
    data->m_priority = priority;
}

// Get state of the process
char ProcessEntry::getState() const
{
    return data->m_state;
}

// Set state of the process
void ProcessEntry::setState(char state)
{
    data->m_state = state;
}

// Get cpu usage of the process
qreal ProcessEntry::getCPU() const
{
    return data->m_cpu;
}

// Set cpu usage of the process
void ProcessEntry::setCPU(qreal cpu)
{
    data->m_cpu = cpu;
}

// Get icon of the process
QIcon ProcessEntry::getIcon() const
{
    return data->m_icon;
}

// Set icon for the process
void ProcessEntry::setIcon(const QIcon &icon)
{
    data->m_icon = icon;
}

// Get process name
QString ProcessEntry::getName() const
{
    return data->m_name;
}

// Set process name
void ProcessEntry::setName(const QString &name)
{
    data->m_name = name;
}

// Get process display name
QString ProcessEntry::getDisplayName() const
{
    return data->m_displayName;
}

// Set display name for the process
void ProcessEntry::setDisplayName(const QString &displayName)
{
    data->m_displayName = displayName;
}

// Get cmdline of the process
QString ProcessEntry::getCmdline() const
{
    return data->m_cmdline;
}

// Set cmdline for the process
void ProcessEntry::setCmdline(const QString &cmdline)
{
    data->m_cmdline = cmdline;
}

// Get start time of the process
time_t ProcessEntry::getStartTime() const
{
    return data->m_startTime;
}

// Set start time for the process
void ProcessEntry::setStartTime(time_t startTime)
{
    data->m_startTime = startTime;
}

// Get user name of the process
QString ProcessEntry::getUserName() const
{
    return data->m_userName;
}

// Set user name of the process
void ProcessEntry::setUserName(const QString &userName)
{
    data->m_userName = userName;
}

// Get memory of the process
qulonglong ProcessEntry::getMemory() const
{
    return data->m_memory;
}

// Set memory for the process
void ProcessEntry::setMemory(qulonglong memory)
{
    data->m_memory = memory;
}

// Get disk read speed of the process
qreal ProcessEntry::getReadBps() const
{
    return data->m_readBps;
}

// Set disk read speed for the process
void ProcessEntry::setReadBps(qreal readBps)
{
    data->m_readBps = readBps;
}

// Get disk write speed of the process
qreal ProcessEntry::getWriteBps() const
{
    return data->m_writeBps;
}

// Set disk write speed for the process
void ProcessEntry::setWriteBps(qreal writeBps)
{
    data->m_writeBps = writeBps;
}

// Get total number of bytes of outbound data transferred by the process
qulonglong ProcessEntry::getSentBytes() const
{
    return data->m_sentBytes;
}

// Set total number of bytes of outbound data transferred by the process
void ProcessEntry::setSentBytes(qulonglong sb)
{
    data->m_sentBytes = sb;
}

// Get total number of bytes of inbound data received by the process
qulonglong ProcessEntry::getRecvBytes() const
{
    return data->m_recvBytes;
}

// Set total number of bytes of inbound data received by the process
void ProcessEntry::setRecvBytes(qulonglong rb)
{
    data->m_recvBytes = rb;
}

// Get outbound transfer speed of the process
qreal ProcessEntry::getSentBps() const
{
    return data->m_sentBps;
}

// Set outbound transfer speed of the process
void ProcessEntry::setSentBps(qreal sentBps)
{
    data->m_sentBps = sentBps;
}

// Get inbound transfer speed of the process
qreal ProcessEntry::getRecvBps() const
{
    return data->m_recvBps;
}

// Set inbound transfer speed of the process
void ProcessEntry::setRecvBps(qreal recvBps)
{
    data->m_recvBps = recvBps;
}
