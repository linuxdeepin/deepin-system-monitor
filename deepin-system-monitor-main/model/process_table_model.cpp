// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ddlog.h"
#include "process_table_model.h"
#include "process/process_db.h"
#include "common/common.h"

#include <QDebug>
#include <QTimer>
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPlatformTheme>
#include <QPointer>
using namespace common;
using namespace common::format;
using namespace DDLog;
DGUI_USE_NAMESPACE   // using namespace Dtk::Gui;

// model constructor
ProcessTableModel::ProcessTableModel(QObject *parent, const QString &username)
    : QAbstractTableModel(parent)
{
    setUserModeName(username);
    qCInfo(app) << "Initializing ProcessTableModel for user:" << username;
    
    //update model's process list cache on process list updated signal
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    connect(monitor, &SystemMonitor::statInfoUpdated, this, &ProcessTableModel::updateProcessList);

    //remove process entry from model's cache on process ended signal
    connect(ProcessDB::instance(), &ProcessDB::processEnded, this, &ProcessTableModel::removeProcess);
    //update process's state in model's cache on process paused signal
    connect(ProcessDB::instance(), &ProcessDB::processPaused, this,
            &ProcessTableModel::updateProcessState);
    //update process's state in model's cache on process resumed signal
    connect(ProcessDB::instance(), &ProcessDB::processResumed, this,
            &ProcessTableModel::updateProcessState);
    //remove process entry from model's cache on process killed signal
    connect(ProcessDB::instance(), &ProcessDB::processKilled, this,
            &ProcessTableModel::removeProcess);
    //update process's priority in model's cache on process priority changed signal
    connect(ProcessDB::instance(), &ProcessDB::processPriorityChanged, this,
            &ProcessTableModel::updateProcessPriority);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=]() {
        qCDebug(app) << "theme changed, updating process list";
        updateProcessList();
    });
}

char ProcessTableModel::getProcessState(pid_t pid) const
{
    qCDebug(app) << "Getting process state for PID:" << pid;
    if (m_procIdList.contains(pid)) {
        return ProcessDB::instance()->processSet()->getProcessById(pid).state();
    }

    qCDebug(app) << "Process with PID" << pid << "not in the list";
    return 0;
}

Process ProcessTableModel::getProcess(pid_t pid) const
{
    qCDebug(app) << "Getting process for PID:" << pid;
    if (m_procIdList.contains(pid)) {
        return ProcessDB::instance()->processSet()->getProcessById(pid);
    }

    qCDebug(app) << "Process with PID" << pid << "not in the list";
    return Process();
}

// update process model with the data provided by list
void ProcessTableModel::updateProcessList()
{
    qCDebug(app) << "Updating process list";
    if (m_userModeName.isNull()) {
        qCDebug(app) << "User mode name is null, delaying update";
        QTimer::singleShot(0, this, SLOT(updateProcessListDelay()));
    } else {
        qCDebug(app) << "User mode name is set, updating for user:" << m_userModeName;
        QTimer::singleShot(0, this, SLOT(updateProcessListWithUserSpecified()));
    }
}

void ProcessTableModel::updateProcessListWithUserSpecified()
{
    qCDebug(app) << "Updating process list for specified user:" << m_userModeName;
    ProcessSet *processSet = ProcessDB::instance()->processSet();
    const QList<pid_t> &newpidlst = processSet->getPIDList();
    beginRemoveRows({}, 0, m_procIdList.size());
    endRemoveRows();
    m_procIdList.clear();
    m_processList.clear();
    int raw;
    for (const auto &pid : newpidlst) {
        Process changedProc = processSet->getProcessById(pid);
        // 确保进程有效且用户名匹配
        if (changedProc.isValid() && changedProc.userName() == m_userModeName) {
            // qCDebug(app) << "Adding process with PID:" << pid << "for user" << m_userModeName;
            raw = m_procIdList.size();
            beginInsertRows({}, raw, raw);
            m_procIdList << pid;
            m_processList << changedProc;
            endInsertRows();
        }
    }

    qCDebug(app) << "Process list updated for user" << m_userModeName;
    Q_EMIT modelUpdated();
}

void ProcessTableModel::updateProcessListDelay()
{
    qCDebug(app) << "Updating process list with delay";
    ProcessSet *processSet = ProcessDB::instance()->processSet();
    const QList<pid_t> &newpidlst = processSet->getPIDList();
    QList<pid_t> oldpidlst = m_procIdList;

    for (const auto &pid : newpidlst) {
        Process proc = processSet->getProcessById(pid);
        // 只处理有效进程
        if (!proc.isValid()) {
            qCDebug(app) << "Skipping invalid process with PID:" << pid;
            continue;
        }
        
        int row = m_procIdList.indexOf(pid);
        if (row >= 0) {
            // qCDebug(app) << "Updating process at row:" << row;
            // update
            m_processList[row] = proc;
            Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
        } else {
            // insert
            // qCDebug(app) << "Inserting new process with PID:" << pid;
            row = m_procIdList.size();
            beginInsertRows({}, row, row);
            m_procIdList << pid;
            m_processList << proc;
            endInsertRows();
        }
    }

    // remove
    for (const auto &pid : oldpidlst) {
        if (!newpidlst.contains(pid)) {
            // qCDebug(app) << "Removing process with PID:" << pid;
            int row = m_procIdList.indexOf(pid);
            beginRemoveRows({}, row, row);
            m_procIdList.removeAt(row);
            m_processList.removeAt(row);
            endRemoveRows();
        }
    }

    qCDebug(app) << "Delayed process list update finished";
    Q_EMIT modelUpdated();
}

// returns the number of rows under the given parent
int ProcessTableModel::rowCount(const QModelIndex &) const
{
    // qCDebug(app) << "Getting row count:" << m_procIdList.size();
    return m_procIdList.size();
}

// returns the number of columns for the children of the given parent
int ProcessTableModel::columnCount(const QModelIndex &) const
{
    // qCDebug(app) << "Getting column count:" << kProcessColumnCount;
    return kProcessColumnCount;
}

// returns the data for the given role and section in the header with the specified orientation
QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    qCDebug(app) << "Getting header data for section:" << section << "role:" << role;
    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        switch (section) {
        case kProcessNameColumn: {
            // name column display text
            return QApplication::translate("Process.Table.Header", kProcessName);
        }
        case kProcessCPUColumn: {
            // cpu column display text
            return QApplication::translate("Process.Table.Header", kProcessCPU);
        }
        case kProcessUserColumn:
            // user column display text
            return QApplication::translate("Process.Table.Header", kProcessUser);
        case kProcessMemoryColumn:
            // memory column display text
            return QApplication::translate("Process.Table.Header", kProcessMemory);
        case kProcessShareMemoryColumn:
            // memory column display text
            return QApplication::translate("Process.Table.Header", kProcessShareMemory);
        case kProcessVTRMemoryColumn:
            // memory column display text
            return QApplication::translate("Process.Table.Header", kProcessVtrMemory);
        case kProcessUploadColumn:
            // upload column display text
            return QApplication::translate("Process.Table.Header", kProcessUpload);
        case kProcessDownloadColumn:
            // download column display text
            return QApplication::translate("Process.Table.Header", kProcessDownload);
        case kProcessDiskReadColumn:
            // disk read column display text
            return QApplication::translate("Process.Table.Header", kProcessDiskRead);
        case kProcessDiskWriteColumn:
            // disk write column display text
            return QApplication::translate("Process.Table.Header", kProcessDiskWrite);
        case kProcessPIDColumn:
            // pid column display text
            return QApplication::translate("Process.Table.Header", kProcessPID);
        case kProcessNiceColumn:
            // nice column display text
            return QApplication::translate("Process.Table.Header", kProcessNice);
        case kProcessPriorityColumn:
            // priority column display text
            return QApplication::translate("Process.Table.Header", kProcessPriority);
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        qCDebug(app) << "Returning text alignment for header";
        // default header section alignment
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        qCDebug(app) << "Returning initial sort order for header";
        // sort section descending by default
        return QVariant::fromValue(Qt::DescendingOrder);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

// returns the data stored under the given role for the item referred to by the index
QVariant ProcessTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qCDebug(app) << "Invalid index, returning empty QVariant";
        return {};
    }

    // validate index
    if (index.row() < 0 || index.row() >= m_processList.size()) {
        qCDebug(app) << "Index out of bounds, returning empty QVariant";
        return {};
    }

    int row = index.row();
    const Process &proc = m_processList[row];
    if (!proc.isValid()) {
        qCDebug(app) << "Process at row" << row << "is invalid";
        return {};
    }

    // qCDebug(app) << "Getting data for row:" << row << "column:" << index.column() << "role:" << role;
    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole) {
        QString name;
        switch (index.column()) {
        case kProcessNameColumn: {
            // prepended tag based on process state
            name = proc.displayName();
            switch (proc.state()) {
            case 'Z':
                qCDebug(app) << "Process state is Zombie";
                name = QString("(%1) %2")
                               .arg(QApplication::translate("Process.Table", "No response"))
                               .arg(name);
                break;
            case 'T':
                qCDebug(app) << "Process state is Suspended";
                name = QString("(%1) %2")
                               .arg(QApplication::translate("Process.Table", "Suspend"))
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
            return formatUnit_memory_disk(proc.memory(), KB);
        case kProcessShareMemoryColumn:
            // formatted memory usage
            return formatUnit_memory_disk(proc.sharememory(), KB);
        case kProcessVTRMemoryColumn:
            // formatted memory usage
            return formatUnit_memory_disk(proc.vtrmemory(), KB);
        case kProcessUploadColumn:
            // formatted upload speed text
            return formatUnit_net(8 * proc.sentBps(), B, 1, true);
        case kProcessDownloadColumn:
            // formated download speed text
            return formatUnit_net(8 * proc.recvBps(), B, 1, true);
        case kProcessDiskReadColumn:
            // formatted disk read speed text
            return formatUnit_memory_disk(proc.readBps(), B, 1, true);
        case kProcessDiskWriteColumn:
            // formatted disk write speed text
            return formatUnit_memory_disk(proc.writeBps(), B, 1, true);
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
            qCDebug(app) << "Returning decoration role for process name";
            // process icon
            return proc.icon();
        default:
            return {};
        }
    } else if (role == Qt::UserRole) {
        qCDebug(app) << "Returning user role data";
        // get process's raw data
        switch (index.column()) {
        case kProcessNameColumn:
            return proc.name();
        case kProcessMemoryColumn:
            return proc.memory();
        case kProcessShareMemoryColumn:
            return proc.sharememory();
        case kProcessVTRMemoryColumn:
            return proc.vtrmemory();
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
        qCDebug(app) << "Returning user role + 1 data";
        // get process's extra data
        switch (index.column()) {
        case kProcessUploadColumn:
            return proc.sentBps();
        case kProcessDownloadColumn:
            return proc.recvBps();
        default:
            return {};
        }
    } else if (role == Qt::TextAlignmentRole) {
        qCDebug(app) << "Returning text alignment role";
        // default data alignment
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::UserRole + 2) {
        qCDebug(app) << "Returning user role + 2 data";
        // text color role based on process's state
        if (index.column() == kProcessNameColumn) {
            char state = proc.state();
            if (state == 'Z' || state == 'T') {
                qCDebug(app) << "Returning warning color for process state:" << state;
                return QVariant(int(Dtk::Gui::DPalette::TextWarning));
            }
        }
        return {};
    } else if (role == Qt::UserRole + 3) {
        qCDebug(app) << "Returning app type";
        return proc.appType();
    } else if (role == Qt::UserRole + 4) {
        qCDebug(app) << "Returning cmdline string";
        QString cmdlineStr = proc.cmdlineString();
        if (!cmdlineStr.isEmpty())
            return cmdlineStr;
        else
            return QString("%1").arg(proc.name());
    }
    return {};
}

// returns the item flags for the given index
Qt::ItemFlags ProcessTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        qCDebug(app) << "Invalid index, returning NoItemFlags";
        return Qt::NoItemFlags;
    }

    qCDebug(app) << "Returning item flags for index";
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

// get process priority enum type
ProcessPriority ProcessTableModel::getProcessPriority(pid_t pid) const
{
    qCDebug(app) << "Getting process priority for PID:" << pid;
    int row = m_procIdList.indexOf(pid);
    if (row >= 0) {
        int prio = ProcessDB::instance()->processSet()->getProcessById(pid).priority();
        qCDebug(app) << "Process found, priority value:" << prio;
        return getProcessPriorityStub(prio);
    }

    qCDebug(app) << "Process with PID" << pid << "not found, returning invalid priority";
    return kInvalidPriority;
}

int ProcessTableModel::getProcessPriorityValue(pid_t pid) const
{
    qCDebug(app) << "Getting process priority value for PID:" << pid;
    int row = m_procIdList.indexOf(pid);
    int priority = row >= 0 ? ProcessDB::instance()->processSet()->getProcessById(pid).priority() : kNormalPriority;
    qCDebug(app) << "Priority value for PID" << pid << "is" << priority;
    return priority;
}

// remove process entry from model with specified pid
void ProcessTableModel::removeProcess(pid_t pid)
{
    qCInfo(app) << "Removing process with PID:" << pid;
    int row = m_procIdList.indexOf(pid);
    if (row >= 0) {
        qCDebug(app) << "Process with PID" << pid << "found at row" << row << ", removing";
        beginRemoveRows(QModelIndex(), row, row);
        m_procIdList.removeAt(row);
        m_processList.removeAt(row);
        endRemoveRows();
        qCInfo(app) << "Process removed successfully";
    } else {
        qCWarning(app) << "Failed to remove process: PID" << pid << "not found";
    }
}

// update the state of the process entry with specified pid
void ProcessTableModel::updateProcessState(pid_t pid, char state)
{
    qCInfo(app) << "Updating process state. PID:" << pid << "New state:" << state;
    int row = m_procIdList.indexOf(pid);
    if (row >= 0) {
        qCDebug(app) << "Process with PID" << pid << "found at row" << row << ", updating state";
        m_processList[row].setState(state);
        Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
        qCInfo(app) << "Process state updated successfully";
    } else {
        qCWarning(app) << "Failed to update process state: PID" << pid << "not found";
    }
}

// update priority of the process entry with specified pid
void ProcessTableModel::updateProcessPriority(pid_t pid, int priority)
{
    qCInfo(app) << "Updating process priority. PID:" << pid << "New priority:" << priority;
    int row = m_procIdList.indexOf(pid);
    if (row >= 0) {
        qCDebug(app) << "Process with PID" << pid << "found at row" << row << ", updating priority";
        m_processList[row].setPriority(priority);
        Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
        qCInfo(app) << "Process priority updated successfully";
    } else {
        qCWarning(app) << "Failed to update process priority: PID" << pid << "not found";
    }
}

void ProcessTableModel::setUserModeName(const QString &userName)
{
    qCDebug(app) << "Setting user mode name to:" << userName;
    if (userName != m_userModeName) {
        qCInfo(app) << "Changing user mode from" << m_userModeName << "to" << userName;
        m_userModeName = userName;
        updateProcessListWithUserSpecified();
    }
}

qreal ProcessTableModel::getTotalCPUUsage()
{
    qCDebug(app) << "Calculating total CPU usage";
    qreal cpuUsage = 0;
    for (const auto &proc : m_processList) {
        cpuUsage += proc.cpu();
    }
    return cpuUsage;
}
qreal ProcessTableModel::getTotalMemoryUsage()
{
    qCDebug(app) << "Calculating total memory usage";
    qreal memUsage = 0;
    for (const auto &proc : m_processList) {
        memUsage += proc.memory();
    }
    return memUsage;
}
qreal ProcessTableModel::getTotalDownload()
{
    qCDebug(app) << "Calculating total download";
    qreal download = 0;
    for (const auto &proc : m_processList) {
        download += proc.recvBps();
    }
    return download;
}
qreal ProcessTableModel::getTotalUpload()
{
    qCDebug(app) << "Calculating total upload";
    qlonglong upload = 0;
    for (const auto &proc : m_processList) {
        upload += proc.sentBps();
    }
    return upload;
}

qreal ProcessTableModel::getTotalVirtualMemoryUsage()
{
    qCDebug(app) << "Calculating total virtual memory usage";
    qlonglong vtmem = 0;
    for (const auto &proc : m_processList) {
        vtmem += proc.vtrmemory();
    }
    return vtmem;
}
qreal ProcessTableModel::getTotalSharedMemoryUsage()
{
    qCDebug(app) << "Calculating total shared memory usage";
    qlonglong smem = 0;
    for (const auto &proc : m_processList) {
        smem += proc.sharememory();
    }
    return smem;
}
qreal ProcessTableModel::getTotalDiskRead()
{
    qCDebug(app) << "Calculating total disk read";
    qlonglong diskread = 0;
    for (const auto &proc : m_processList) {
        diskread += proc.readBps();
    }
    return diskread;
}
qreal ProcessTableModel::getTotalDiskWrite()
{
    qCDebug(app) << "Calculating total disk write";
    qlonglong diskwrite = 0;
    for (const auto &proc : m_processList) {
        diskwrite += proc.writeBps();
    }
    return diskwrite;
}
