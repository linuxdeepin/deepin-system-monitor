/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "systemd1_unit_interface.h"

#include "dbus_common.h"

using namespace DBus::Common;

/*
 * Implementation of interface class Systemd1UnitInterface
 */

Systemd1UnitInterface::Systemd1UnitInterface(const QString &service, const QString &path,
                                             const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
    , m_DBusPropIface(new DBusPropertiesInterface(service, path, connection))
{
}

Systemd1UnitInterface::~Systemd1UnitInterface()
{
    m_DBusPropIface->deleteLater();
}

QPair<ErrorContext, QString> Systemd1UnitInterface::getId() const
{
    ErrorContext ec;
    QString id {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "Id");
    ec = reply.first;
    if (ec)
        return {ec, id};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, id};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            id = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, id};
}
QPair<ErrorContext, QString> Systemd1UnitInterface::getLoadState() const
{
    ErrorContext ec;
    QString state {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "LoadState");
    ec = reply.first;
    if (ec)
        return {ec, state};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, state};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            state = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, state};
}
QPair<ErrorContext, QString> Systemd1UnitInterface::getActiveState() const
{
    ErrorContext ec;
    QString state {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "ActiveState");
    ec = reply.first;
    if (ec)
        return {ec, state};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, state};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            state = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, state};
}
QPair<ErrorContext, QString> Systemd1UnitInterface::getSubState() const
{
    ErrorContext ec;
    QString sub {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "SubState");
    ec = reply.first;
    if (ec)
        return {ec, sub};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, sub};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            sub = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, sub};
}
QPair<ErrorContext, QString> Systemd1UnitInterface::getDescription() const
{
    ErrorContext ec;
    QString desc {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "Description");
    ec = reply.first;
    if (ec)
        return {ec, desc};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, desc};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            desc = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, desc};
}
QPair<ErrorContext, bool> Systemd1UnitInterface::canReload() const
{
    ErrorContext ec;
    bool re {false};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "CanReload");
    ec = reply.first;
    if (ec)
        return {ec, re};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, re};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::Bool) {
            re = qvariant_cast<bool>(v.variant());
        }
    }

    return {ec, re};
}
QPair<ErrorContext, bool> Systemd1UnitInterface::canStart() const
{
    ErrorContext ec;
    bool re {false};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "CanStart");
    ec = reply.first;
    if (ec)
        return {ec, re};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, re};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::Bool) {
            re = qvariant_cast<bool>(v.variant());
        }
    }

    return {ec, re};
}
QPair<ErrorContext, bool> Systemd1UnitInterface::canStop() const
{
    ErrorContext ec;
    bool re {false};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "CanStop");
    ec = reply.first;
    if (ec)
        return {ec, re};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, re};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::Bool) {
            re = qvariant_cast<bool>(v.variant());
        }
    }

    return {ec, re};
}
