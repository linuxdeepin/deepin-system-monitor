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

#ifndef PROCESS_TABLE_MODEL_H
#define PROCESS_TABLE_MODEL_H

#include "process/process_entry.h"
#include "process/system_monitor.h"

#include <QAbstractTableModel>
#include <QList>
#include <QMap>

// name column display
constexpr const char *kProcessName = QT_TRANSLATE_NOOP("Process.Table.Header", "Name");
// cpu column display
constexpr const char *kProcessCPU = QT_TRANSLATE_NOOP("Process.Table.Header", "CPU");
// user column display
constexpr const char *kProcessUser = QT_TRANSLATE_NOOP("Process.Table.Header", "User");
// memory column display
constexpr const char *kProcessMemory = QT_TRANSLATE_NOOP("Process.Table.Header", "Memory");
// upload column display
constexpr const char *kProcessUpload = QT_TRANSLATE_NOOP("Process.Table.Header", "Upload");
// download column display
constexpr const char *kProcessDownload = QT_TRANSLATE_NOOP("Process.Table.Header", "Download");
// disk read column display
constexpr const char *kProcessDiskRead = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk read");
// disk write column display
constexpr const char *kProcessDiskWrite = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk write");
// pid column display
constexpr const char *kProcessPID = QT_TRANSLATE_NOOP("Process.Table.Header", "PID");
// nice column display
constexpr const char *kProcessNice = QT_TRANSLATE_NOOP("Process.Table.Header", "Nice");
// priority column display
constexpr const char *kProcessPriority = QT_TRANSLATE_NOOP("Process.Table.Header", "Priority");

/**
 * @brief Process table model class
 */
class ProcessTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessTableModel)

public:
    /**
     * @brief Process table column index
     */
    enum Column {
        kProcessNameColumn = 0, // name column index
        kProcessCPUColumn, // cpu column index
        kProcessUserColumn, // user column index
        kProcessMemoryColumn, // memory column index
        kProcessUploadColumn, // upload column index
        kProcessDownloadColumn, // download column index
        kProcessDiskReadColumn, // disk read column index
        kProcessDiskWriteColumn, // disk write column index
        kProcessPIDColumn, // pid column index
        kProcessNiceColumn, // nice column index
        kProcessPriorityColumn, // priority column index

        kProcessColumnCount // total number of columns
    };

    /**
     * @brief Model constructor
     * @param parent Parent object
     */
    explicit ProcessTableModel(QObject *parent = nullptr);

    /**
     * @brief Update process model with the data provided by list
     * @param list Process entry list
     */
    void updateProcessList(const QList<ProcessEntry> &list);

    /**
     * @brief Returns the number of rows under the given parent
     * @param parent Parent index
     * @return Row count
     */
    int rowCount(const QModelIndex &parent = {}) const override;
    /**
     * @brief Returns the number of columns for the children of the given parent
     * @param parent Parent index
     * @return Column count
     */
    int columnCount(const QModelIndex &parent = {}) const override;
    /**
     * @brief Returns the data stored under the given role for the item referred to by the index
     * @param index Index of the data
     * @param role Role of the data
     * @return Data of specified role referred with index within model
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /**
     * @brief Returns the data for the given role and section in the header with the specified orientation
     * @param section Section of the header
     * @param orientation Orientation of the header
     * @param role Data role
     * @return Data of the give role & section with specified orientation within data source
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    /**
     * @brief Returns the item flags for the given index
     * @param index Model index to get flags for
     * @return Combination of item flags
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /**
     * @brief Fetches any available data for the items with the parent specified by the parent index
     * @param parent Parent index
     */
    void fetchMore(const QModelIndex &parent) override;
    /**
     * @brief Check if more data can be fetched for parent index
     * @param parent Parent index
     * @return Returns true if there is more data available for parent; otherwise returns false
     */
    bool canFetchMore(const QModelIndex &parent) const override;

    /**
     * @brief Get process state with specified pid
     * @param pid Id of the process to get state for
     * @return Process state string
     */
    char getProcessState(pid_t pid) const;
    /**
     * @brief Get process priority enum type
     * @param pid Id of the process to get priority stub for
     * @return Process priority enum type
     */
    SystemMonitor::ProcessPriority getProcessPriorityStub(pid_t pid) const;
    /**
     * @brief Get process priority with specified pid
     * @param pid Id of the process to get priority for
     * @return Process priority
     */
    inline int getProcessPriority(pid_t pid) const
    {
        int row = m_processMap.value(pid);
        int prio = m_processList.at(row).getPriority();
        return prio;
    }
    /**
     * @brief Get process entry from list with specified pid
     * @param pid Process id
     * @return Process entry item
     */
    inline ProcessEntry getProcessEntry(pid_t pid) const
    {
        if (m_processMap.contains(pid)) {
            auto idx = m_processMap[pid];
            if (idx >= 0 && idx < m_processList.size()) {
                return m_processList[idx];
            }
        }
        return {};
    }

Q_SIGNALS:
    /**
     * @brief Model updated signal
     */
    void modelUpdated();

private Q_SLOTS:
    /**
     * @brief Remove process entry from model with specified pid
     * @param pid Process id
     */
    void removeProcessEntry(pid_t pid);
    /**
     * @brief Update the state of the process entry with specified pid
     * @param pid Process id
     * @param state Process state
     */
    void updateProcessState(pid_t pid, char state);
    /**
     * @brief Update priority of the process entry with specified pid
     * @param pid Process id
     * @param priority Process priority
     */
    void updateProcessPriority(pid_t pid, int priority);

private:
    // Process entry list cache
    QList<ProcessEntry> m_processList;
    // Process pid to <index> mapping
    QMap<pid_t, int> m_processMap;  // <pid, index>
    // current loaded items (into the model)
    int m_nr {};
};

#endif  // PROCESS_TABLE_MODEL_H
