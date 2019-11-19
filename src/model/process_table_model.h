#ifndef PROCESS_TABLE_MODEL_H
#define PROCESS_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QMap>

#include "process/process_entry.h"

constexpr const char *kProcessName = QT_TRANSLATE_NOOP("Process.Table.Header", "Name");
constexpr const char *kProcessCPU = QT_TRANSLATE_NOOP("Process.Table.Header", "CPU");
constexpr const char *kProcessUser = QT_TRANSLATE_NOOP("Process.Table.Header", "User");
constexpr const char *kProcessMemory = QT_TRANSLATE_NOOP("Process.Table.Header", "Memory");
constexpr const char *kProcessUpload = QT_TRANSLATE_NOOP("Process.Table.Header", "Upload");
constexpr const char *kProcessDownload = QT_TRANSLATE_NOOP("Process.Table.Header", "Download");
constexpr const char *kProcessDiskRead = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk Read");
constexpr const char *kProcessDiskWrite = QT_TRANSLATE_NOOP("Process.Table.Header", "Disk Write");
constexpr const char *kProcessPID = QT_TRANSLATE_NOOP("Process.Table.Header", "PID");

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

        kProcessColumnCount
    };

    explicit ProcessTableModel(QObject *parent = nullptr);

    void updateProcessList(const QList<ProcessEntry> &list);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void modelUpdated();

private Q_SLOTS:
    void removeProcessEntry(pid_t pid);
    void updateProcessState(pid_t pid, char state);

private:
    QList<ProcessEntry> m_processList;
    QMap<pid_t, int> m_processMap;  // <pid, index>
};

#endif  // PROCESS_TABLE_MODEL_H
