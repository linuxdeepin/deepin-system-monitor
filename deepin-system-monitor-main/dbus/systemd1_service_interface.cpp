// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemd1_service_interface.h"

#include "environment_file.h"
#include "ddlog.h"

#include <QVariant>
#include <QDBusInterface>

using namespace DDLog;

/*
 * Implementation of interface class Systemd1ServiceInterface
 */

// Constructor
Systemd1ServiceInterface::Systemd1ServiceInterface(const QString &service,
                                                   const QString &path,
                                                   const QDBusConnection &connection,
                                                   QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
    , m_DBusPropIface(new DBusPropertiesInterface(service, path, connection))
{
}

Systemd1ServiceInterface::~Systemd1ServiceInterface()
{
    m_DBusPropIface->deleteLater();
}

// Get main pid of service's process
QPair<ErrorContext, quint32> Systemd1ServiceInterface::getMainPID() const
{
    // error context
    ErrorContext ec;
    // pid
    quint32 pid {};

    // dbus interface method call: MainPID
    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "MainPID");
    ec = reply.first;
    if (ec)
        return {ec, pid};
    QDBusMessage msg = reply.second;

    // check reply
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(app) << "DBus error getting main PID:" << msg.errorMessage();
        return {ec, pid};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        // check return type
        if (v.variant().type() == QVariant::UInt) {
            pid = qvariant_cast<quint32>(v.variant());
        } else {
            qCWarning(app) << "Invalid PID type received:" << v.variant().typeName();
        }
    }

    return {ec, pid};
}

// Get memory usage of service's process
QPair<ErrorContext, quint64> Systemd1ServiceInterface::getMemoryCurrent() const
{
    qCDebug(app) << "Getting current memory usage for service";
    // error context
    ErrorContext ec;
    // memory
    quint64 mem {};

    // dbus interface method call: MemoryCurrent
    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "MemoryCurrent");
    ec = reply.first;
    if (ec)
        return {ec, mem};
    QDBusMessage msg = reply.second;

    // check reply
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(app) << "DBus error getting memory usage:" << msg.errorMessage();
        return {ec, mem};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        // check return type
        if (v.variant().type() == QVariant::ULongLong) {
            mem = qvariant_cast<quint64>(v.variant());
            qCDebug(app) << "Retrieved memory usage:" << mem << "bytes";
        } else {
            qCWarning(app) << "Invalid memory usage type received:" << v.variant().typeName();
        }
    }

    return {ec, mem};
}

// Get control group of service's process
QPair<ErrorContext, QString> Systemd1ServiceInterface::getControlGroup() const
{
    qCDebug(app) << "Getting control group for service";
    // error context
    ErrorContext ec;
    // control group string
    QString cg {};

    // dbus interface method call: ControlGroup
    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "ControlGroup");
    ec = reply.first;
    if (ec)
        return {ec, cg};
    QDBusMessage msg = reply.second;

    // check reply
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(app) << "DBus error getting control group:" << msg.errorMessage();
        return {ec, cg};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        // check return type
        if (v.variant().type() == QVariant::String) {
            cg = qvariant_cast<QString>(v.variant());
            qCDebug(app) << "Retrieved control group:" << cg;
        } else {
            qCWarning(app) << "Invalid control group type received:" << v.variant().typeName();
        }
    }

    return {ec, cg};
}

// Get environment file list of service
QPair<ErrorContext, EnvironmentFileList> Systemd1ServiceInterface::getEnvironmentFiles() const
{
    qCDebug(app) << "Getting environment files for service";
    // error context
    ErrorContext ec;
    // environment file list
    EnvironmentFileList list;

    // dbus interface method call: EnvironmentFiles
    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "EnvironmentFiles");
    ec = reply.first;
    if (ec)
        return {ec, list};
    QDBusMessage msg = reply.second;

    // check reply
    if (msg.type() == QDBusMessage::ErrorMessage) {
        qCWarning(app) << "DBus error getting environment files:" << msg.errorMessage();
        return {ec, list};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        QDBusArgument args = qvariant_cast<QDBusArgument>(v.variant());
        // check signature
        if (args.currentSignature() == "a(sb)") {
            args >> list;
            qCDebug(app) << "Retrieved" << list.size() << "environment files";
        } else {
            qCWarning(app) << "Invalid environment files signature received:" << args.currentSignature();
        }
    }
    return {ec, list};
}
