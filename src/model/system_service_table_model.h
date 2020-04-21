#ifndef SYSTEM_SERVICE_TABLE_MODEL_H
#define SYSTEM_SERVICE_TABLE_MODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>
#include <QHash>

#include "service/system_service_entry.h"
#include "dbus/dbus_common.h"
#include "service/service_manager.h"

using namespace DBus::Common;

constexpr const char *kSystemServiceName = QT_TRANSLATE_NOOP("Service.Table.Header", "Name");
constexpr const char *kSystemServiceLoadState = QT_TRANSLATE_NOOP("Service.Table.Header", "Load");
constexpr const char *kSystemServiceActiveState =
    QT_TRANSLATE_NOOP("Service.Table.Header", "Active");
//: sub state (running status)
constexpr const char *kSystemServiceSubState = QT_TRANSLATE_NOOP("Service.Table.Header", "Sub");
//: state
constexpr const char *kSystemServiceState = QT_TRANSLATE_NOOP("Service.Table.Header", "State");
//: service startup mode
constexpr const char *kSystemServiceStartupMode = QT_TRANSLATE_NOOP("Service.Table.Header", "Startup Type");
constexpr const char *kSystemServiceDescription =
    QT_TRANSLATE_NOOP("Service.Table.Header", "Description");
constexpr const char *kSystemServicePID = QT_TRANSLATE_NOOP("Service.Table.Header", "PID");

class SystemServiceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum SystemServiceTableColumn {
        kSystemServiceNameColumn = 0,
        kSystemServiceLoadStateColumn,
        kSystemServiceActiveStateColumn,
        kSystemServiceSubStateColumn,
        kSystemServiceStateColumn,
        kSystemServiceDescriptionColumn,
        kSystemServicePIDColumn,
        kSystemServiceStartupModeColumn,

        kSystemServiceTableColumnCount
    };

    explicit SystemServiceTableModel(QObject *parent = nullptr);

    inline void reset()
    {
        beginRemoveRows({}, 0, m_svcList.size() - 1);
        m_svcList.clear();
        m_svcMap.clear();
        endRemoveRows();
    }

    void updateServiceEntry(const SystemServiceEntry &entry);

    inline QString getUnitFileState(const QModelIndex &index)
    {
        if (!index.isValid())
            return {};

        return m_svcMap[m_svcList[index.row()]].getState();
    }

    inline QString getUnitFileName(const QModelIndex &index)
    {
        if (!index.isValid())
            return {};

        return m_svcMap[m_svcList[index.row()]].getId();
    }

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;

Q_SIGNALS:
    void currentRowChanged(int row);

private Q_SLOTS:
    void updateServiceList(const QList<SystemServiceEntry> &list);

private:
    // service name
    QList<QString>                      m_svcList   {};
    // name - entry
    QHash<QString, SystemServiceEntry>  m_svcMap    {};
    // current loaded items (into the model)
    int m_nr {};
};

#endif  // SYSTEM_SERVICE_TABLE_MODEL_H
