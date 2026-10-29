// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "am_icon_manager.h"
#include "ddlog.h"

#include <DSysInfo>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDebug>
#include <QDBusUnixFileDescriptor>
#include <DDesktopEntry>
#include <cerrno>
#include <cstring>
#include <signal.h>

#include "3rdparty/include/pidfd-utils.h"

using namespace DDLog;

namespace core {
namespace process {

static const QString kDesktopEntryIconKey = "Desktop Entry";
static const QString kDefaultKey = "default";

std::atomic<AMIconManager *> AMIconManager::m_instance {nullptr};
std::mutex AMIconManager::m_mutex {};

AMIconManager::AMIconManager(QObject *parent)
    : QObject(parent)
{
    qCInfo(app) << "AMIconManager created";

    m_systemMajorVersion = DSysInfo::majorVersion().toInt();
    qCDebug(app) << "AMIconManager created, system version:" << m_systemMajorVersion;

    if (m_systemMajorVersion >= 25) {
        QDBusConnectionInterface *sessionBus = QDBusConnection::sessionBus().interface();
        m_amAvailable = sessionBus->isServiceRegistered(
            "org.desktopspec.ApplicationManager1"
        );

        if (m_amAvailable) {
            qCInfo(app) << "AM service available on V25+ system";
            initializeAMInterface();
        } else {
            qCDebug(app) << "V25+ system but AM service not registered";
        }
    } else {
        qCInfo(app) << "Pre-V25 system";
        m_amAvailable = false;
    }
}

AMIconManager::~AMIconManager()
{
    qCDebug(app) << "AMIconManager destroyed";
    if (m_amInterface) {
        delete m_amInterface;
        m_amInterface = nullptr;
    }
}

AMIconManager *AMIconManager::instance()
{
    AMIconManager *sin = m_instance.load();
    if (!sin) {
        std::lock_guard<std::mutex> lock(m_mutex);
        sin = m_instance.load();
        if (!sin) {
            sin = new AMIconManager();
            m_instance.store(sin);
        }
    }
    return sin;
}

void AMIconManager::initializeAMInterface()
{
    if (!m_amAvailable) {
        return;
    }

    if (!m_amInterface) {
        m_amInterface = new QDBusInterface(
            "org.desktopspec.ApplicationManager1",
            "/org/desktopspec/ApplicationManager1",
            "org.desktopspec.ApplicationManager1",
            QDBusConnection::sessionBus()
        );

        if (!m_amInterface->isValid()) {
            qCWarning(app) << "Failed to create AM DBus interface";
            m_amAvailable = false;
        } else {
            qCDebug(app) << "AM DBus interface initialized successfully";
        }
    }
}

QString AMIconManager::getIconByAppId(const QString &appId)
{
    if (appId.isEmpty()) {
        return QString();
    }

    // Check appId cache first (avoid repeated Properties.Get calls)
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_appIdToIconCache.contains(appId)) {
            return m_appIdToIconCache[appId];
        }
    }

    // Build Application object path with underscore escaping
    // D-Bus object path escape: _ → _5F, - → _2d
    QString escapedAppId = appId;
    escapedAppId.replace("_", "_5F");
    escapedAppId.replace("-", "_2d");
    QString appPath = QString("/org/desktopspec/ApplicationManager1/%1").arg(escapedAppId);

    // Use org.freedesktop.DBus.Properties.Get to read Icons property
    QDBusInterface propsInterface(
        "org.desktopspec.ApplicationManager1",
        appPath,
        "org.freedesktop.DBus.Properties",
        QDBusConnection::sessionBus()
    );

    if (!propsInterface.isValid()) {
        qCWarning(app) << "Failed to create Properties interface for" << appPath;
        return QString();
    }

    QDBusReply<QDBusVariant> iconsReply = propsInterface.call(
        "Get",
        "org.desktopspec.ApplicationManager1.Application",
        "Icons"
    );

    if (!iconsReply.isValid()) {
        qCWarning(app) << "Failed to get Icons property:" << iconsReply.error().message();
        return QString();
    }

    QDBusVariant dbusVariant = iconsReply.value();
    QVariant iconsVariant = dbusVariant.variant();

    // Icons property is a{ss} (QStringMap), comes wrapped in QDBusArgument
    QMap<QString, QString> icons;
    QDBusArgument dbusArg = iconsVariant.value<QDBusArgument>();
    dbusArg >> icons;

    QString iconName = icons.value(kDesktopEntryIconKey);
    if (iconName.isEmpty()) {
        iconName = icons.value(kDefaultKey);
    }

    // Cache the result
    if (!iconName.isEmpty()) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_appIdToIconCache[appId] = iconName;
    }

    return iconName;
}

QString AMIconManager::getAppNameByAppId(const QString &appId)
{
    if (appId.isEmpty()) {
        return QString();
    }

    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_appIdToNameCache.contains(appId)) {
            return m_appIdToNameCache[appId];
        }
    }

    // Build Application object path with underscore escaping
    QString escapedAppId = appId;
    escapedAppId.replace("_", "_5F");
    escapedAppId.replace("-", "_2d");
    QString appPath = QString("/org/desktopspec/ApplicationManager1/%1").arg(escapedAppId);

    // Get Name property from AM
    QDBusInterface propsInterface(
        "org.desktopspec.ApplicationManager1",
        appPath,
        "org.freedesktop.DBus.Properties",
        QDBusConnection::sessionBus()
    );

    if (!propsInterface.isValid()) {
        return QString();
    }

    QDBusReply<QDBusVariant> nameReply = propsInterface.call(
        "Get",
        "org.desktopspec.ApplicationManager1.Application",
        "Name"
    );

    if (!nameReply.isValid()) {
        return QString();
    }

    // Name is a{ss} (localized name map), get "default" key
    QVariant nameVar = nameReply.value().variant();
    QMap<QString, QString> names;
    QDBusArgument dbusArg = nameVar.value<QDBusArgument>();
    dbusArg >> names;

    // Try locale-specific names first, fall back to "default"
    QString displayName;
    QLocale locale;
    QString localeKey = locale.name();
    if (names.contains(localeKey)) {
        displayName = names[localeKey];
    } else if (names.contains(kDefaultKey)) {
        displayName = names[kDefaultKey];
    } else if (!names.isEmpty()) {
        displayName = names.first();
    }

    // Cache the result
    if (!displayName.isEmpty()) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_appIdToNameCache[appId] = displayName;
    }

    return displayName;
}

AMProcessGroupInfo AMIconManager::identifyProcess(int pidfd)
{
    AMProcessGroupInfo result;

    if (!m_amInterface || !m_amInterface->isValid()) {
        qCWarning(app) << "AM interface not available or invalid";
        return result;
    }

    QDBusMessage reply = m_amInterface->call(
        "Identify",
        QVariant::fromValue(QDBusUnixFileDescriptor{static_cast<qint32>(pidfd)})
    );

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(app) << "AM Identify call failed:" << reply.errorMessage();
        return result;
    }

    QList<QVariant> args = reply.arguments();

    if (args.size() < 3) {
        qCWarning(app) << "AM returned unexpected number of arguments:" << args.size() << "expected 3";
        return result;
    }

    // Parse the three return values: appId, instancePath, instanceInfo
    result.appId = args.at(0).toString();

    // args[1] is QDBusObjectPath for instance
    if (args.at(1).canConvert<QDBusObjectPath>()) {
        QDBusObjectPath instancePath = args.at(1).value<QDBusObjectPath>();
        result.instancePath = instancePath.path();
    } else if (args.at(1).canConvert<QString>()) {
        result.instancePath = args.at(1).toString();
    } else {
        qCWarning(app) << "Failed to convert instance path, typeName:" << args.at(1).typeName();
    }



    if (result.appId.isEmpty()) {
        qCWarning(app) << "AM returned empty appId for pidfd" << pidfd;
        return result;
    }

    // Get icon name
    result.iconName = getIconByAppId(result.appId);
    result.displayName = getAppNameByAppId(result.appId);

    result.linglongAppId = resolveLinglongAppId(result.appId);

    return result;
}

QString AMIconManager::resolveLinglongAppId(const QString &appId)
{
    if (m_linglongAppIdCache.contains(appId)) {
        return m_linglongAppIdCache[appId];
    }

    QString desktopPath = QString("/var/lib/linglong/entries/apps/share/applications/%1.desktop").arg(appId);
    if (!QFile::exists(desktopPath)) {
        m_linglongAppIdCache[appId] = QString();
        return QString();
    }
    DDesktopEntry entry(desktopPath);
    QString linglongId = entry.stringValue("X-linglong");
    m_linglongAppIdCache[appId] = linglongId;
    return linglongId;
}

QString AMIconManager::getIconByPid(pid_t pid)
{
    if (!m_amAvailable || !m_amInterface) {
        return QString();
    }

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_pidToIconCache.contains(pid)) {
            return m_pidToIconCache[pid];
        }
    }

    int pidfd = pidfd_open(pid, 0);
    if (pidfd < 0) {
        return QString();
    }

    AMProcessGroupInfo info = identifyProcess(pidfd);
    close(pidfd);

    if (!info.isValid()) {
        return QString();
    }

    // Cache both icon and group info
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_pidToIconCache[pid] = info.iconName;
        m_pidToGroupCache[pid] = info;
    }

    return info.iconName;
}

AMProcessGroupInfo AMIconManager::getProcessGroupInfo(pid_t pid)
{
    if (!m_amAvailable || !m_amInterface) {
        return AMProcessGroupInfo();
    }

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_pidToGroupCache.contains(pid)) {
            return m_pidToGroupCache[pid];
        }
    }

    int pidfd = pidfd_open(pid, 0);
    if (pidfd < 0) {
        qCWarning(app) << "pidfd_open failed for pid" << pid << "errno:" << errno;
        return AMProcessGroupInfo();
    }

    AMProcessGroupInfo info = identifyProcess(pidfd);
    close(pidfd);

    if (info.isValid()) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_pidToGroupCache[pid] = info;
    }

    return info;
}

bool AMIconManager::isAMAvailable() const
{
    return m_amAvailable;
}

void AMIconManager::removeCachedIcon(pid_t pid)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_pidToIconCache.remove(pid);
    m_pidToGroupCache.remove(pid);
}

void AMIconManager::clearCache()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_pidToIconCache.clear();
    m_appIdToIconCache.clear();
    m_pidToGroupCache.clear();
}

} // namespace process
} // namespace core
