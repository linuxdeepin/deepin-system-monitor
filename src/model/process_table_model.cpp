#include <DApplication>
#include <QDebug>
#include <QIcon>

#include "process/system_monitor.h"
#include "process_table_model.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE

ProcessTableModel::ProcessTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::processListUpdated, this,
                &ProcessTableModel::updateProcessList);

        connect(sysmon, &SystemMonitor::processEnded, this, &ProcessTableModel::removeProcessEntry);
        connect(sysmon, &SystemMonitor::processPaused, this,
                &ProcessTableModel::updateProcessState);
        connect(sysmon, &SystemMonitor::processResumed, this,
                &ProcessTableModel::updateProcessState);
        connect(sysmon, &SystemMonitor::processKilled, this,
                &ProcessTableModel::removeProcessEntry);

        sysmon->updateStatus();
    }
}

void ProcessTableModel::updateProcessList(const QList<ProcessEntry> &list)
{
    QHash<pid_t, ProcessEntry> hash;
    int row;
    for (const auto &entry : list) {
        if (m_processMap.contains(entry.getPID())) {
            // update
            row = m_processMap.value(entry.getPID());
            m_processList.replace(row, entry);
            Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
        } else {
            // insert
            row = m_processList.size();
            beginInsertRows({}, row, row);
            m_processList << entry;
            m_processMap[entry.getPID()] = row;
            endInsertRows();
        }
        hash[entry.getPID()] = entry;
    }
    // remove
    QMutableListIterator<ProcessEntry> it(m_processList);
    while (it.hasNext()) {
        it.next();
        if (hash.contains(it.value().getPID())) {
            continue;
        } else {
            pid_t pid = it.value().getPID();
            row = m_processMap.value(pid);
            beginRemoveRows({}, row, row);
            it.remove();
            m_processMap.remove(pid);
            for (int &value : m_processMap) {
                if (value > row)
                    --value;
            }
            endRemoveRows();
        }
    }

    Q_EMIT modelUpdated();
}

int ProcessTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_processList.size();
}

int ProcessTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return kProcessColumnCount;
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
                case kProcessNameColumn: {
                    return DApplication::translate("Process.Table.Header", kProcessName);
                }
                case kProcessCPUColumn: {
                    return DApplication::translate("Process.Table.Header", kProcessCPU);
                }
                case kProcessUserColumn:
                    return DApplication::translate("Process.Table.Header", kProcessUser);
                case kProcessMemoryColumn:
                    return DApplication::translate("Process.Table.Header", kProcessMemory);
                case kProcessUploadColumn:
                    return DApplication::translate("Process.Table.Header", kProcessUpload);
                case kProcessDownloadColumn:
                    return DApplication::translate("Process.Table.Header", kProcessDownload);
                case kProcessDiskReadColumn:
                    return DApplication::translate("Process.Table.Header", kProcessDiskRead);
                case kProcessDiskWriteColumn:
                    return DApplication::translate("Process.Table.Header", kProcessDiskWrite);
                case kProcessPIDColumn:
                    //                    return DApplication::translate("Service.Table.Header",
                    //                    kProcessPID);
                    return QString(kProcessPID);
                default:
                    break;
            }
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        return QVariant::fromValue(Qt::DescendingOrder);
    } else if (role == Qt::SizeHintRole) {
        QModelIndex idx;
        int maxWidth = 0;
        for (int row = 0; row < rowCount(); row++) {
            QVariant v = {};

            idx = index(row, section);
            if (idx.isValid())
                v = idx.data(Qt::SizeHintRole);

            if (v.isValid()) {
                if (maxWidth < v.toSize().width())
                    maxWidth = v.toSize().width();
            }
        }
        // TODO: fix size hint
        return {QSize(maxWidth + 10 * 3 + 8, 36 + 1)};
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant ProcessTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_processList.size())
        return {};

    if (role == Qt::DisplayRole) {
        QString name;
        switch (index.column()) {
            case kProcessNameColumn: {
                name = m_processList.at(index.row()).getDisplayName();
                switch (m_processList.at(index.row()).getState()) {
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
                return QString("%1%").arg(m_processList.at(index.row()).getCPU(), 0, 'f', 1);
            case kProcessUserColumn:
                return m_processList.at(index.row()).getUserName();
            case kProcessMemoryColumn:
                return formatByteCount(m_processList.at(index.row()).getMemory());
            case kProcessUploadColumn:
                return formatBandwidth(qulonglong(m_processList.at(index.row()).getSentKbs()));
            case kProcessDownloadColumn:
                return formatBandwidth(qulonglong(m_processList.at(index.row()).getRecvKbs()));
            case kProcessDiskReadColumn:
                return QString("%1/s").arg(
                    formatByteCount(qulonglong(m_processList.at(index.row()).getDiskRead())));
            case kProcessDiskWriteColumn:
                return QString("%1/s").arg(
                    formatByteCount(qulonglong(m_processList.at(index.row()).getDiskWrite())));
            case kProcessPIDColumn: {
                return QString("%1").arg(m_processList.at(index.row()).getPID());
            }
            default:
                break;
        }
    } else if (role == Qt::DecorationRole) {
        switch (index.column()) {
            case kProcessNameColumn:
                return QVariant(m_processList.at(index.row()).getIcon());
            default:
                return {};
        }
    } else if (role == Qt::UserRole) {
        switch (index.column()) {
            case kProcessNameColumn:
                return QVariant(m_processList.at(index.row()).getName());
            case kProcessMemoryColumn:
                return QVariant(m_processList.at(index.row()).getMemory());
            case kProcessCPUColumn:
                return QVariant(m_processList.at(index.row()).getCPU());
            case kProcessUploadColumn:
                return QVariant(m_processList.at(index.row()).getSentKbs());
            case kProcessDownloadColumn:
                return QVariant(m_processList.at(index.row()).getRecvKbs());
            case kProcessPIDColumn:
                return QVariant(m_processList.at(index.row()).getPID());
            case kProcessDiskReadColumn:
                return QVariant(m_processList.at(index.row()).getDiskRead());
            case kProcessDiskWriteColumn:
                return QVariant(m_processList.at(index.row()).getDiskWrite());
            default:
                return {};
        }
    } else if (role == (Qt::UserRole + 1)) {
        switch (index.column()) {
            case kProcessUploadColumn:
                return QVariant(m_processList.at(index.row()).getSentBytes());
            case kProcessDownloadColumn:
                return QVariant(m_processList.at(index.row()).getRecvBytes());
            default:
                return {};
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::SizeHintRole) {
        QFont font = DApplication::font();
        QFontMetrics fm(font);
        return QSize(fm.boundingRect(data(index).toString()).width(), 32);
    }
    return {};
}

Qt::ItemFlags ProcessTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

char ProcessTableModel::getProcessState(pid_t pid)
{
    int row = m_processMap.value(pid);
    return m_processList.at(row).getState();
}

void ProcessTableModel::removeProcessEntry(pid_t pid)
{
    if (m_processMap.contains(pid)) {
        int row = m_processMap.value(pid);
        beginRemoveRows({}, row, row);
        m_processList.removeAt(row);
        m_processMap.remove(pid);
        for (int &value : m_processMap) {
            if (value > row)
                --value;
        }
        endRemoveRows();
    }
}

void ProcessTableModel::updateProcessState(pid_t pid, char state)
{
    if (m_processMap.contains(pid)) {
        int row = m_processMap.value(pid);
        ProcessEntry e = m_processList.at(row);
        e.setState(state);
        m_processList[row] = e;
        Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}
