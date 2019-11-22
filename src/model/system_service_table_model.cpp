#include <DApplication>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>

#include "system_service_table_model.h"
#include "utils.h"

DWIDGET_USE_NAMESPACE
using namespace Utils;

SystemServiceTableModel::SystemServiceTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

QVariant SystemServiceTableModel::data(const QModelIndex& index, int role) const
{
    QString buf;

    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_ServiceEntryList.size())
        return {};

    if (role == Qt::DisplayRole)
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
    } else if (role == Qt::SizeHintRole) {
        QFont font = DApplication::font();
        QFontMetrics fm(font);
        switch (index.column()) {
            case kSystemServiceNameColumn:
                return QSize(fm.boundingRect(m_ServiceEntryList.at(index.row()).getId()).width(),
                             32);
            case kSystemServiceLoadStateColumn:
                return QSize(
                    fm.boundingRect(m_ServiceEntryList.at(index.row()).getLoadState()).width(), 32);
            case kSystemServiceActiveStateColumn:
                return QSize(
                    fm.boundingRect(m_ServiceEntryList.at(index.row()).getActiveState()).width(),
                    32);
            case kSystemServiceSubStateColumn:
                return QSize(
                    fm.boundingRect(m_ServiceEntryList.at(index.row()).getSubState()).width(), 32);
            case kSystemServiceStateColumn:
                return QSize(fm.boundingRect(m_ServiceEntryList.at(index.row()).getState()).width(),
                             32);
            case kSystemServiceDescriptionColumn:
                return QSize(
                    fm.boundingRect(m_ServiceEntryList.at(index.row()).getDescription()).width(),
                    32);
            case kSystemServicePIDColumn: {
                QString pidStr = QString("%1").arg(m_ServiceEntryList.at(index.row()).getMainPID());
                return QSize(fm.boundingRect(pidStr).width(), 32);
            }
            default:
                return {QSize(200, 32)};
        }
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
                    return DApplication::translate("Service.Table.Header", kSystemServiceName);
                case kSystemServiceLoadStateColumn:
                    return DApplication::translate("Service.Table.Header", kSystemServiceLoadState);
                case kSystemServiceActiveStateColumn:
                    return DApplication::translate("Service.Table.Header",
                                                   kSystemServiceActiveState);
                case kSystemServiceSubStateColumn:
                    return DApplication::translate("Service.Table.Header", kSystemServiceSubState);
                case kSystemServiceStateColumn:
                    return DApplication::translate("Service.Table.Header", kSystemServiceState);
                case kSystemServicePIDColumn:
                    //                    return DApplication::translate("Service.Table.Header",
                    //                    kSystemServicePID);
                    return QString(kSystemServicePID);
                case kSystemServiceDescriptionColumn:
                    return DApplication::translate("Service.Table.Header",
                                                   kSystemServiceDescription);
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
        return {QSize(maxWidth + 10 + 10, 36 + 1)};
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags SystemServiceTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
