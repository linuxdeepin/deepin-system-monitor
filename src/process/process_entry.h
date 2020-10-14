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

#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

class ProcessEntryData;
class QIcon;

/**
 * @brief Process info entry
 */
class ProcessEntry
{
public:
    /**
     * @brief Default constructor
     */
    ProcessEntry();
    /**
     * @brief Copy constructor
     */
    ProcessEntry(const ProcessEntry &);
    /**
     * @brief Copy assignment
     * @param entry Source entry
     * @return Copy assigned entry
     */
    ProcessEntry &operator=(const ProcessEntry &entry);
    /**
    * @brief Destroctor
    */
    ~ProcessEntry();

    /**
     * @brief Get process Id
     * @return Process Id
     */
    pid_t getPID() const;
    /**
     * @brief Set process Id
     * @param pid Process Id
     */
    void setPID(pid_t pid);

    /**
     * @brief Get user Id
     * @return User Id of the process
     */
    uid_t getUID() const;
    /**
     * @brief Set user id
     * @param uid User Id of the process
     */
    void setUID(uid_t uid);

    /**
     * @brief Get group Id
     * @return Group Id of the process
     */
    gid_t getGID() const;
    /**
     * @brief Set group Id
     * @param gid Group Id of the process
     */
    void setGID(gid_t gid);

    /**
     * @brief Get priority of the process
     * @return Process priority
     */
    int getPriority() const;
    /**
     * @brief Set process priority
     * @param priority Process priority
     */
    void setPriority(int priority);

    /**
     * @brief Get state of the process
     * @return State of the process
     */
    char getState() const;
    /**
     * @brief Set state of the process
     * @param state State of the process
     */
    void setState(char state);

    /**
     * @brief Get cpu usage of the process
     * @return Cpu usage
     */
    qreal getCPU() const;
    /**
     * @brief Set cpu usage of the process
     * @param cpu Cpu usage
     */
    void setCPU(qreal cpu);

    /**
     * @brief Get icon of the process
     * @return Icon for the process
     */
    QIcon getIcon() const;
    /**
     * @brief Set icon for the process
     * @param icon Icon for the process
     */
    void setIcon(const QIcon &icon);

    /**
     * @brief Get process name
     * @return Process name
     */
    QString getName() const;
    /**
     * @brief Set process name
     * @param name Process name
     */
    void setName(const QString &name);

    /**
     * @brief Get process display name
     * @return Display name of the process
     */
    QString getDisplayName() const;
    /**
     * @brief Set display name for the process
     * @param Display name for the process
     */
    void setDisplayName(const QString &displayName);

    /**
     * @brief Get cmdline of the process
     * @return Cmdline of the process
     */
    QString getCmdline() const;
    /**
     * @brief Set cmdline for the process
     * @param cmdline Cmdline for the process
     */
    void setCmdline(const QString &cmdline);

    /**
     * @brief Get start time of the process
     * @return Start time of the process
     */
    time_t getStartTime() const;
    /**
     * @brief Set start time for the process
     * @param startTime Start time of the process
     */
    void setStartTime(time_t startTime);

    /**
     * @brief Get user name of the process
     * @return User name of the process
     */
    QString getUserName() const;
    /**
     * @brief Set user name of the process
     * @param userName User name of the process
     */
    void setUserName(const QString &userName);

    /**
     * @brief Get memory of the process
     * @return Memory of the process
     */
    qulonglong getMemory() const;
    /**
     * @brief Set memory for the process
     * @param memory Process memory
     */
    void setMemory(qulonglong memory);

    /**
     * @brief Get disk read speed of the process
     * @return Disk read speed of the process (Bytes per second)
     */
    qreal getReadBps() const;
    /**
     * @brief Set disk read speed for the process
     * @param readBps Disk read speed of the process (Bytes per second)
     */
    void setReadBps(qreal readBps);

    /**
     * @brief Get disk write speed of the process
     * @return Disk write speed of the process (Bytes per second)
     */
    qreal getWriteBps() const;
    /**
     * @brief Set disk write speed for the process
     * @param writeBps Disk write speed of the process (Bytes per second)
     */
    void setWriteBps(qreal writeBps);

    /**
     * @brief Get total number of bytes of outbound data transferred by the process
     * @return Outbound data transferred in bytes
     */
    qulonglong getSentBytes() const;
    /**
     * @brief Set total number of bytes of outbound data transferred by the process
     * @param sb Outbound data transferred in bytes
     */
    void setSentBytes(qulonglong sb);

    /**
     * @brief Get total number of bytes of inbound data received by the process
     * @return Inbound data received in bytes
     */
    qulonglong getRecvBytes() const;
    /**
     * @brief Set total number of bytes of inbound data received by the process
     * @param rb Inbound data received in bytes
     */
    void setRecvBytes(qulonglong rb);

    /**
     * @brief Get outbound transfer speed of the process
     * @return Outbound transfer speed of the process (Bytes per second)
     */
    qreal getSentBps() const;
    /**
     * @brief Set outbound transfer speed of the process
     * @param sentBps Outbound transfer speed of the process (Bytes per second)
     */
    void setSentBps(qreal sentBps);

    /**
     * @brief Get inbound transfer speed of the process
     * @return Inbound transfer speed of the process (Bytes per second)
     */
    qreal getRecvBps() const;
    /**
     * @brief Set inbound transfer speed of the process
     * @param recvBps Inbound transfer speed of the process (Bytes per second)
     */
    void setRecvBps(qreal recvBps);

private:
    // Explicitly shared entry data
    QExplicitlySharedDataPointer<ProcessEntryData> data;
};

Q_DECLARE_METATYPE(ProcessEntry)

#endif  // PROCESS_ENTRY_H
