#include <DApplication>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>

#include "system_service_table_model.h"
#include "service/service_manager.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE
using namespace Utils;

SystemServiceTableModel::SystemServiceTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    auto *mgr = ServiceManager::instance();
    Q_ASSERT(mgr);
    connect(mgr, &ServiceManager::serviceStartupModeChanged, this, &SystemServiceTableModel::updateServiceState);
}

QVariant SystemServiceTableModel::data(const QModelIndex &index, int role) const
{
    QString buf;

    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_ServiceEntryList.size())
        return {};

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case kSystemServiceNameColumn:
            return m_ServiceEntryList.at(index.row()).getId();
        case kSystemServiceLoadStateColumn: {
            buf = m_ServiceEntryList.at(index.row()).getLoadState();
            return DApplication::translate("DBus.Unit.Load.State", buf.toUtf8());
        }
        case kSystemServiceActiveStateColumn: {
            buf = m_ServiceEntryList.at(index.row()).getActiveState();
            return DApplication::translate("DBus.Unit.Active.State", buf.toUtf8());
        }
        case kSystemServiceSubStateColumn:
            return m_ServiceEntryList.at(index.row()).getSubState();
        case kSystemServiceStateColumn: {
            buf = m_ServiceEntryList.at(index.row()).getState();
            return DApplication::translate("DBus.Unit.State", buf.toUtf8());
        }
        case kSystemServiceStartupModeColumn: {
            auto entry = m_ServiceEntryList.at(index.row());
            return DApplication::translate("DBus.Unit.Startup.Mode", entry.getStartupType().toUtf8());
        }
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
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return {};
}

int SystemServiceTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_ServiceEntryList.size();
}
int SystemServiceTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return kSystemServiceTableColumnCount;
}
QVariant SystemServiceTableModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case kSystemServiceNameColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceName);
        case kSystemServiceLoadStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceLoadState);
        case kSystemServiceActiveStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceActiveState);
        case kSystemServiceSubStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceSubState);
        case kSystemServiceStateColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceState);
        case kSystemServiceDescriptionColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceDescription);
        case kSystemServicePIDColumn:
            return DApplication::translate("Service.Table.Header", kSystemServicePID);
        case kSystemServiceStartupModeColumn:
            return DApplication::translate("Service.Table.Header", kSystemServiceStartupMode);
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::InitialSortOrderRole) {
        return QVariant::fromValue(Qt::DescendingOrder);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags SystemServiceTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SystemServiceTableModel::updateServiceState(const QString &sname, const QString &state)
{
    for (int i = 0; i < m_ServiceEntryList.size(); i++) {
        auto &entry = m_ServiceEntryList[i];
        if (entry.getId() == sname) {
            entry.setState(state);
            entry.setStartupType(ServiceManager::getServiceStartupType(entry.getId(), state));
            Q_EMIT dataChanged(index(i, kSystemServiceStateColumn), index(i, kSystemServiceStateColumn));
            Q_EMIT dataChanged(index(i, kSystemServiceStartupModeColumn), index(i, kSystemServiceStartupModeColumn));
            break;
        }
    }
}
