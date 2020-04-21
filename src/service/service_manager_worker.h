#ifndef SERVICE_MANAGER_WORKER_H
#define SERVICE_MANAGER_WORKER_H

#include <QList>

#include "service/system_service_entry.h"
#include "dbus/unit_info.h"
#include "dbus/unit_file_info.h"

class ServiceManagerWorker : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManagerWorker(QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(const QList<SystemServiceEntry> list);

public Q_SLOTS:
    void startJob();

private:
    inline static QString readUnitDescriptionFromUnitFile(const QString &path);
};

#endif // SERVICE_MANAGER_WORKER_H
