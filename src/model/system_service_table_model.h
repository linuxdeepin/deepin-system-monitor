#ifndef SYSTEM_SERVICE_TABLE_MODEL_H
#define SYSTEM_SERVICE_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>

#include "service/system_service_entry.h"

constexpr const char* kSystemServiceName = QT_TRANSLATE_NOOP("SystemServiceTableModel", "Name");
constexpr const char* kSystemServiceLoadState =
    QT_TRANSLATE_NOOP("SystemServiceTableModel", "Load");
constexpr const char* kSystemServiceActiveState =
    QT_TRANSLATE_NOOP("SystemServiceTableModel", "Active");
//: sub state (running status)
constexpr const char* kSystemServiceSubState = QT_TRANSLATE_NOOP("SystemServiceTableModel", "Sub");
//: state
constexpr const char* kSystemServiceState = QT_TRANSLATE_NOOP("SystemServiceTableModel", "State");
constexpr const char* kSystemServiceDescription =
    QT_TRANSLATE_NOOP("SystemServiceTableModel", "Description");
constexpr const char* kSystemServicePID = QT_TRANSLATE_NOOP("SystemServiceTableModel", "PID");

class SystemServiceTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemServiceTableModel)

public:
    enum SystemServiceTableColumn {
        kSystemServiceNameColumn = 0,
        kSystemServiceLoadStateColumn,
        kSystemServiceActiveStateColumn,
        kSystemServiceSubStateColumn,
        kSystemServiceStateColumn,
        kSystemServiceDescriptionColumn,
        kSystemServicePIDColumn,

        kSystemServiceTableColumnCount
    };

    SystemServiceTableModel(QObject* parent = nullptr);

    inline void setServiceEntryList(const QList<SystemServiceEntry>& list)
    {
        beginResetModel();
        m_ServiceEntryList = list;
        endResetModel();
    }

    inline SystemServiceEntry getSystemServiceEntry(const QModelIndex& index) const
    {
        if (index.isValid() && index.row() >= 0 && index.row() < m_ServiceEntryList.size())
            return m_ServiceEntryList.at(index.row());
        else {
            return {};
        }
    }

    inline void updateServiceEntry(int row)
    {
        if (row >= 0)
            dataChanged(index(row, 0), index(row, columnCount() - 1));
    }

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    QList<SystemServiceEntry> m_ServiceEntryList;
};

#endif  // SYSTEM_SERVICE_TABLE_MODEL_H
