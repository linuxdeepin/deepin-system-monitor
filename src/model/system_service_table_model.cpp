#include <QColor>
#include <QDebug>

#include "system_service_table_model.h"
#include "utils.h"

using namespace Utils;

SystemServiceTableModel::SystemServiceTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

QVariant SystemServiceTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_ServiceEntryList.size())
        return {};

    if (role == Qt::DisplayRole)
        switch (index.column()) {
            case kSystemServiceNameColumn:
                return m_ServiceEntryList.at(index.row()).getId();
            case kSystemServiceLoadStateColumn:
                return m_ServiceEntryList.at(index.row()).getLoadState();
            case kSystemServiceActiveStateColumn:
                return m_ServiceEntryList.at(index.row()).getActiveState();
            case kSystemServiceSubStateColumn:
                return m_ServiceEntryList.at(index.row()).getSubState();
            case kSystemServiceStateColumn:
                return m_ServiceEntryList.at(index.row()).getState();
            case kSystemServiceDescriptionColumn:
                return m_ServiceEntryList.at(index.row()).getDescription();
            case kSystemServicePIDColumn: {
                quint32 pid = m_ServiceEntryList.at(index.row()).getMainPID();

                QVariant v = (pid == 0) ? QVariant() : QVariant(pid);
                return v;
            }
            default:
                break;
        }
    else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return {};
}

int SystemServiceTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return m_ServiceEntryList.size();
}
int SystemServiceTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return kSystemServiceTableColumnCount;
}
QVariant SystemServiceTableModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
                case kSystemServiceNameColumn:
                    return tr(kSystemServiceName);
                case kSystemServiceLoadStateColumn:
                    return tr(kSystemServiceLoadState);
                case kSystemServiceActiveStateColumn:
                    return tr(kSystemServiceActiveState);
                case kSystemServiceSubStateColumn:
                    return tr(kSystemServiceSubState);
                case kSystemServiceStateColumn:
                    return tr(kSystemServiceState);
                case kSystemServicePIDColumn:
                    return tr(kSystemServicePID);
                case kSystemServiceDescriptionColumn:
                    return tr(kSystemServiceDescription);
                default:
                    break;
            }
        }  // TODO: else if (orientation == Qt::Vertical)??
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        return QVariant::fromValue(Qt::DescendingOrder);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags SystemServiceTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
