#ifndef DBUS_PROPERTIES_INTERFACE_H
#define DBUS_PROPERTIES_INTERFACE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtDBus>

#include "common/error_context.h"

/*
 * Proxy class for interface org.freedesktop.DBus.Properties
 */
class DBusPropertiesInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() { return "org.freedesktop.DBus.Properties"; }

public:
    DBusPropertiesInterface(const QString &service, const QString &path,
                            const QDBusConnection &connection, QObject *parent = nullptr);

    ~DBusPropertiesInterface();

public Q_SLOTS:  // METHODS
    // param: in0 service name
    // param: in1 property name
    inline QPair<ErrorContext, QDBusMessage> Get(const QString &in0, const QString &in1)
    {
        QList<QVariant> args;
        ErrorContext ec;

        args << QVariant::fromValue(in0) << QVariant::fromValue(in1);
        QDBusMessage msg = callWithArgumentList(QDBus::Block, QStringLiteral("Get"), args);
        if (lastError().isValid()) {
            ec.setCode(ErrorContext::kErrorTypeDBus);
            ec.setSubCode(lastError().type());
            ec.setErrorName(lastError().name());
            ec.setErrorMessage(lastError().message());
        }

        return {ec, msg};
    }

Q_SIGNALS:  // SIGNALS
    void PropertiesChanged(const QString &ifname, const QVariantMap &changedProperties,
                           const QStringList &invalidatedProperties);
};

namespace org {
namespace freedesktop {
namespace DBus {
typedef ::DBusPropertiesInterface Properties;
}
}  // namespace freedesktop
}  // namespace org
#endif
