#include <QVariant>

#include <QDBusInterface>
#include "environment_file.h"
#include "systemd1_service_interface.h"

/*
 * Implementation of interface class Systemd1ServiceInterface
 */

Systemd1ServiceInterface::Systemd1ServiceInterface(const QString &service, const QString &path,
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

QPair<ErrorContext, quint32> Systemd1ServiceInterface::getMainPID() const
{
    ErrorContext ec;
    quint32 pid {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "MainPID");
    ec = reply.first;
    if (ec)
        return {ec, pid};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, pid};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::UInt) {
            pid = qvariant_cast<quint32>(v.variant());
        }
    }

    return {ec, pid};
}

QPair<ErrorContext, quint64> Systemd1ServiceInterface::getMemoryCurrent() const
{
    ErrorContext ec;
    quint64 mem {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "MemoryCurrent");
    ec = reply.first;
    if (ec)
        return {ec, mem};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, mem};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::ULongLong) {
            mem = qvariant_cast<quint64>(v.variant());
        }
    }

    return {ec, mem};
}

QPair<ErrorContext, QString> Systemd1ServiceInterface::getControlGroup() const
{
    ErrorContext ec;
    QString cg {};

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "ControlGroup");
    ec = reply.first;
    if (ec)
        return {ec, cg};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, cg};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        if (v.variant().type() == QVariant::String) {
            cg = qvariant_cast<QString>(v.variant());
        }
    }

    return {ec, cg};
}

QPair<ErrorContext, EnvironmentFileList> Systemd1ServiceInterface::getEnvironmentFiles() const
{
    ErrorContext ec;
    EnvironmentFileList list;

    auto reply = m_DBusPropIface->Get(staticInterfaceName(), "EnvironmentFiles");
    ec = reply.first;
    if (ec)
        return {ec, list};
    QDBusMessage msg = reply.second;

    if (msg.type() == QDBusMessage::ErrorMessage) {
        return {ec, list};
    } else {
        Q_ASSERT(msg.type() == QDBusMessage::ReplyMessage);
        QDBusVariant v = qvariant_cast<QDBusVariant>(msg.arguments()[0]);
        QDBusArgument args = qvariant_cast<QDBusArgument>(v.variant());
        if (args.currentSignature() == "a(sb)") {
            args >> list;
        }
    }
    return {ec, list};
}
