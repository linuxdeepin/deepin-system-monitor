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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
