/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DBUS_PROPERTIES_INTERFACE_H
#define DBUS_PROPERTIES_INTERFACE_H

#include "common/error_context.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtDBus>

/**
 * @brief Proxy class for interface org.freedesktop.DBus.Properties
 */
class DBusPropertiesInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    /**
     * @brief staticInterfaceName Get interface name
     * @return Interface name
     */
    static inline const char *staticInterfaceName() { return "org.freedesktop.DBus.Properties"; }

public:
    /**
     * @brief DBusPropertiesInterface Overloaded constructor
     * @param service Service name
     * @param path Object path
     * @param connection DBus connection
     * @param parent Parent object
     */
    DBusPropertiesInterface(const QString &service, const QString &path,
                            const QDBusConnection &connection, QObject *parent = nullptr);
    ~DBusPropertiesInterface();

public Q_SLOTS:  // METHODS
    /**
     * @brief Get Get service's property
     * @param in0 Service name
     * @param in1 Property name
     * @return Error context and DBus Message resutl pair
     */
    inline QPair<ErrorContext, QDBusMessage> Get(const QString &in0, const QString &in1)
    {
        QList<QVariant> args;
        ErrorContext ec;

        args << QVariant::fromValue(in0) << QVariant::fromValue(in1);
        // make dbus call
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
    /**
     * @brief PropertiesChanged Service properties changed signal
     * @param ifname Interface name
     * @param changedProperties Changed properties map
     * @param invalidatedProperties Invalidated properties list
     */
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
