#ifndef PROCESS_TABLE_MODEL_H
#define PROCESS_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QMap>

#include "process/process_entry.h"
#include "process/system_monitor.h"

constexpr const char *kProcessName = QT_TRANSLATE_NOOP("Process.Table.Header", "Name");
constexpr const char *kProcessCPU = QT_TRANSLATE_NOOP("Process.Table.Header", "CPU");
constexpr const char *kProcessUser = QT_TRANSLATE_NOOP("Process.Table.Header", "User");
constexpr const char *kProcessMemory = QT_TRANSLATE_NOOP("Process.Table.Header", "Memory");
constexpr const char *kProcessUpload = QT_TRANSLATE_NOOP("Process.Table.Header", "Upload");
constexpr const char *kProcessDownload = QT_TRANSLATE_NOOP("Process.Table.Header", "Download");
constexpr const char *kProcessDiskRead = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk read");
constexpr const char *kProcessDiskWrite = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk write");
constexpr const char *kProcessPID = QT_TRANSLATE_NOOP("Process.Table.Header", "PID");
constexpr const char *kProcessNice = QT_TRANSLATE_NOOP("Process.Table.Header", "Nice");
constexpr const char *kProcessPriority = QT_TRANSLATE_NOOP("Process.Table.Header", "Priority");

class ProcessTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessTableModel)

public:
    enum Column {
        kProcessNameColumn = 0,
        kProcessCPUColumn,
        kProcessUserColumn,
        kProcessMemoryColumn,
        kProcessUploadColumn,
        kProcessDownloadColumn,
        kProcessDiskReadColumn,
        kProcessDiskWriteColumn,
        kProcessPIDColumn,
        kProcessNiceColumn,
        kProcessPriorityColumn,

        kProcessColumnCount
    };

    explicit ProcessTableModel(QObject *parent = nullptr);

    void updateProcessList(const QList<ProcessEntry> list);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    char getProcessState(pid_t pid) const;
    SystemMonitor::ProcessPriority getProcessPriorityStub(pid_t pid) const;
    inline int getProcessPriority(pid_t pid) const
    {
        int row = m_processMap.value(pid);
        int prio = m_processList.at(row).getPriority();
        return prio;
    }

Q_SIGNALS:
    void modelUpdated();

private Q_SLOTS:
    void removeProcessEntry(pid_t pid);
    void updateProcessState(pid_t pid, char state);
    void updateProcessPriority(pid_t pid, int priority);

private:
    QList<ProcessEntry> m_processList;
    QMap<pid_t, int> m_processMap;  // <pid, index>
};

#endif  // PROCESS_TABLE_MODEL_H
