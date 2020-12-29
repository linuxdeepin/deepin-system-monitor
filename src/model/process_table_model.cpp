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

#include "process_table_model.h"

#include "common/common.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>

#include <QDebug>
#include <QIcon>
#include <QStyleOption>

DWIDGET_USE_NAMESPACE
using namespace common;
using namespace common::format;

// model constructor
ProcessTableModel::ProcessTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    // update model's process list cache on process list updated signal
    //    connect(smo->jobInstance(), &StatsCollector::processListUpdated,
    //            this, &ProcessTableModel::updateProcessList);
    // remove process entry from model's cache on process ended signal
    //    connect(smo, &SystemMonitor::processEnded, this, &ProcessTableModel::removeProcessEntry);
    // update process's state in model's cache on process paused signal
    //    connect(smo, &SystemMonitor::processPaused, this,
    //            &ProcessTableModel::updateProcessState);
    // update process's state in model's cache on process resumed signal
    //    connect(smo, &SystemMonitor::processResumed, this,
    //            &ProcessTableModel::updateProcessState);
    // remove process entry from model's cache on process killed signal
    //    connect(smo, &SystemMonitor::processKilled, this,
    //            &ProcessTableModel::removeProcessEntry);
    // update process's priority in model's cache on process priority changed signal
    //    connect(smo, &SystemMonitor::processPriorityChanged, this,
    //            &ProcessTableModel::updateProcessPriority);
}

// update process model with the data provided by list
void ProcessTableModel::updateProcessList(const ProcessSet &set)
{
    //    QHash<pid_t, ProcessEntry> hash;
    //    int row;

    //    for (const auto &entry : list) {
    //        if (m_processMap.contains(entry.getPID())) {
    //            // update the entry if already exists
    //            row = m_processMap.value(entry.getPID());
    //            m_processList.replace(row, entry);
    //            Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
    //        } else {
    //            // add the entry to cache otherwise
    //            row = m_processList.size();
    //            beginInsertRows({}, row, row);
    //            m_processList << entry;
    //            m_processMap[entry.getPID()] = row;
    //            ++m_nr;
    //            endInsertRows();
    //        }
    //        hash[entry.getPID()] = entry;
    //    }
    //    // remove the entry from cache if entry with same pid does not exist in the updated list
    //    QMutableListIterator<ProcessEntry> it(m_processList);
    //    while (it.hasNext()) {
    //        it.next();
    //        if (hash.contains(it.value().getPID())) {
    //            continue;
    //        } else {
    //            pid_t pid = it.value().getPID();
    //            row = m_processMap.value(pid);
    //            beginRemoveRows({}, row, row);
    //            it.remove();
    //            m_processMap.remove(pid);
    //            for (int &value : m_processMap) {
    //                if (value > row)
    //                    --value;
    //            }
    //            if (m_nr)
    //                m_nr--;
    //            endRemoveRows();
    //        }
    //    }
    m_procList = set.getPIDList();
    m_procSet = set;

    Q_EMIT modelUpdated();
}

// returns the number of rows under the given parent
int ProcessTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_nr;
}

// returns the number of columns for the children of the given parent
int ProcessTableModel::columnCount(const QModelIndex &) const
{
    return kProcessColumnCount;
}

// returns the data for the given role and section in the header with the specified orientation
QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (section) {
        case kProcessNameColumn: {
            // name column display text
            return DApplication::translate("Process.Table.Header", kProcessName);
        }
        case kProcessCPUColumn: {
            // cpu column display text
            return DApplication::translate("Process.Table.Header", kProcessCPU);
        }
        case kProcessUserColumn:
            // user column display text
            return DApplication::translate("Process.Table.Header", kProcessUser);
        case kProcessMemoryColumn:
            // memory column display text
            return DApplication::translate("Process.Table.Header", kProcessMemory);
        case kProcessUploadColumn:
            // upload column display text
            return DApplication::translate("Process.Table.Header", kProcessUpload);
        case kProcessDownloadColumn:
            // download column display text
            return DApplication::translate("Process.Table.Header", kProcessDownload);
        case kProcessDiskReadColumn:
            // disk read column display text
            return DApplication::translate("Process.Table.Header", kProcessDiskRead);
        case kProcessDiskWriteColumn:
            // disk write column display text
            return DApplication::translate("Process.Table.Header", kProcessDiskWrite);
        case kProcessPIDColumn:
            // pid column display text
            return DApplication::translate("Process.Table.Header", kProcessPID);
        case kProcessNiceColumn:
            // nice column display text
            return DApplication::translate("Process.Table.Header", kProcessNice);
        case kProcessPriorityColumn:
            // priority column display text
            return DApplication::translate("Process.Table.Header", kProcessPriority);
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        // default header section alignment
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        // sort section descending by default
        return QVariant::fromValue(Qt::DescendingOrder);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

// returns the data stored under the given role for the item referred to by the index
QVariant ProcessTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    // validate index
    if (index.row() < 0 || index.row() >= m_procList.size())
        return {};

    int row = index.row();
    auto proc = m_procSet.getProcessById(m_procList[row]);
    if (!proc.isValid())
        return {};

    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        QString name;
        switch (index.column()) {
        case kProcessNameColumn: {
            // prepended tag based on process state
            name = proc.displayName();
            switch (proc.state()) {
            case 'Z':
                name = QString("(%1) %2")
                       .arg(DApplication::translate("Process.Table", "No response"))
                       .arg(name);
                break;
            case 'T':
                name = QString("(%1) %2")
                       .arg(DApplication::translate("Process.Table", "Suspend"))
                       .arg(name);
                break;
            }
            return name;
        }
        case kProcessCPUColumn:
            // formated cpu percent utilization
            return QString("%1%").arg(proc.cpu(), 0, 'f', 1);
        case kProcessUserColumn:
            // process's user name
            return proc.userName();
        case kProcessMemoryColumn:
            // formatted memory usage
            return formatUnit(proc.memory(), KB);
        case kProcessUploadColumn:
            // formatted upload speed text
            return formatUnit(proc.sentBps(), B, 1, true);
        case kProcessDownloadColumn:
            // formated download speed text
            return formatUnit(proc.recvBps(), B, 1, true);
        case kProcessDiskReadColumn:
            // formatted disk read speed text
            return formatUnit(proc.readBps(), B, 1, true);
        case kProcessDiskWriteColumn:
            // formatted disk write speed text
            return formatUnit(proc.writeBps(), B, 1, true);
        case kProcessPIDColumn: {
            // process pid text
            return QString("%1").arg(proc.pid());
        }
        case kProcessNiceColumn: {
            // process priority text
            return QString("%1").arg(proc.priority());
        }
        case kProcessPriorityColumn: {
            // process priority enum text representation
            return getPriorityName(proc.priority());
        }
        default:
            break;
        }
    } else if (role == Qt::DecorationRole) {
        switch (index.column()) {
        case kProcessNameColumn:
            // process icon
            return proc.icon();
        default:
            return {};
        }
    } else if (role == Qt::UserRole) {
        // get process's raw data
        switch (index.column()) {
        case kProcessNameColumn:
            return proc.name();
        case kProcessMemoryColumn:
            return proc.memory();
        case kProcessCPUColumn:
            return proc.cpu();
        case kProcessUploadColumn:
            return proc.sentBps();
        case kProcessDownloadColumn:
            return proc.recvBps();
        case kProcessPIDColumn:
            return proc.pid();
        case kProcessDiskReadColumn:
            return proc.readBps();
        case kProcessDiskWriteColumn:
            return proc.writeBps();
        case kProcessNiceColumn:
            return proc.priority();
        default:
            return {};
        }
    } else if (role == (Qt::UserRole + 1)) {
        // get process's extra data
        switch (index.column()) {
        case kProcessUploadColumn:
            return proc.sentBytes();
        case kProcessDownloadColumn:
            return proc.recvBytes();
        default:
            return {};
        }
    } else if (role == Qt::TextAlignmentRole) {
        // default data alignment
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::UserRole + 2) {
        // text color role based on process's state
        if (index.column() == kProcessNameColumn) {
            char state = proc.state();
            if (state == 'Z' || state == 'T') {
                return QVariant(int(DPalette::TextWarning));
            }
        }
        return {};
    }
    return {};
}

// returns the item flags for the given index
Qt::ItemFlags ProcessTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ProcessTableModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;

    // fetch at most 100 items at a time
    int left = m_procList.size() - m_nr;
    int more = qMin(100, left);

    if (more <= 0)
        return;

    beginInsertRows(QModelIndex(), m_nr, m_nr + more - 1);
    m_nr += more;
    endInsertRows();
}

bool ProcessTableModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    return (m_nr < m_procList.size());
}

// get process priority enum type
ProcessPriority ProcessTableModel::getProcessPriority(pid_t pid) const
{
    if (m_procList.contains(pid)) {
        int prio = m_procSet.getProcessById(pid).priority();
        return getProcessPriorityStub(prio);
    }

    return kInvalidPriority;
}

// remove process entry from model with specified pid
void ProcessTableModel::removeProcess(pid_t pid)
{
    if (m_procList.contains(pid)) {
        int row = m_procList.indexOf(pid);
        beginRemoveRows({}, row, row);
        m_procList.removeAt(row);
        m_procSet.removeProcess(pid);
        if (m_nr)
            m_nr--;
        endRemoveRows();
    }
}

// update the state of the process entry with specified pid
void ProcessTableModel::updateProcessState(pid_t pid, char state)
{
    if (m_procList.contains(pid)) {
        int row = m_procList.indexOf(pid);
        m_procSet.updateProcessState(pid, state);
        Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}

// update priority of the process entry with specified pid
void ProcessTableModel::updateProcessPriority(pid_t pid, int priority)
{
    if (m_procList.contains(pid)) {
        int row = m_procList.indexOf(pid);
        m_procSet.updateProcessPriority(pid, priority);
        Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}
