// Copyright (C) 2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AM_ICON_MANAGER_H
#define AM_ICON_MANAGER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QMap>
#include <QString>
#include <QList>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <sys/types.h>

namespace core {
namespace process {

/**
 * @brief Process group information from AM Identify
 * 
 * Contains information about the application instance that a process belongs to.
 * Multiple processes from the same application instance (e.g., linglong apps)
 * will have the same instancePath.
 */
struct AMProcessGroupInfo
{
    QString appId;              // Application ID from AM (e.g., "deepin-editor")
    QString linglongAppId;     // Full linglong appId for ll-cli (e.g., "org.deepin.editor"), empty if not linglong
    QString instancePath;       // Instance object path from AM
    QString iconName;           // Icon name for this application
    QString displayName;        // Localized display name from AM (e.g., "QQ")
    bool isValid() const { return !appId.isEmpty() && !instancePath.isEmpty(); }
    bool isLinglong() const { return !linglongAppId.isEmpty(); }
};

/**
 * @brief Application Manager (AM) icon manager
 * 
 * This class provides icon retrieval through Application Manager DBus interface
 * on V25+ systems. It maintains a cache of pid to icon mappings
 * to avoid repeated DBus calls.
 * 
 * Also provides process grouping information for applications like linglong
 * that run multiple processes in the same cgroup/instance.
 */
class AMIconManager : public QObject
{
    Q_OBJECT

public:
    explicit AMIconManager(QObject *parent = nullptr);
    ~AMIconManager();

    /**
     * @brief Get singleton instance (thread-safe)
     * @return AMIconManager instance
     */
    static AMIconManager *instance();

    /**
     * @brief Get icon name by PID through AM
     * @param pid Process ID
     * @return Icon name (empty if not found or AM unavailable)
     */
    QString getIconByPid(pid_t pid);

    /**
     * @brief Get process group information by PID
     * @param pid Process ID
     * @return Process group info containing appId, instancePath, etc.
     */
    AMProcessGroupInfo getProcessGroupInfo(pid_t pid);

    /**
     * @brief Check if AM service is available
     * @return true if available (V25+ system)
     */
    bool isAMAvailable() const;

    /**
     * @brief Remove cached icon for a pid
     * @param pid Process ID
     */
    void removeCachedIcon(pid_t pid);

    /**
     * @brief Clear all cached icons
     */
    void clearCache();

private:
    /**
     * @brief Initialize AM DBus interface
     */
    void initializeAMInterface();

    /**
     * @brief Get icon for an appId
     * @param appId Application ID (from AM.Identify)
     * @return Icon name (empty if not found)
     */
    QString getIconByAppId(const QString &appId);

    /**
     * @brief Get localized display name for an appId
     * @param appId Application ID (from AM.Identify)
     * @return Display name (empty if not found)
     */
    QString getAppNameByAppId(const QString &appId);

    /**
     * @brief Identify application by pidfd and get full info
     * @param pidfd Process file descriptor
     * @return Process group info
     */
    AMProcessGroupInfo identifyProcess(int pidfd);

    QString resolveLinglongAppId(const QString &appId);

private:
    QDBusInterface *m_amInterface {nullptr};
    bool m_amAvailable {false};
    int m_systemMajorVersion {-1};

    // Primary cache: pid -> icon (fast lookup for known processes)
    QMap<pid_t, QString> m_pidToIconCache;

    // Secondary cache: appId -> icon (avoid repeated Properties.Get for same app)
    QMap<QString, QString> m_appIdToIconCache;
    QMap<QString, QString> m_appIdToNameCache;

    // PID to process group info cache
    QMap<pid_t, AMProcessGroupInfo> m_pidToGroupCache;

    // linglong appId cache: AM appId -> linglong reverse domain appId
    QHash<QString, QString> m_linglongAppIdCache;

    mutable std::mutex m_cacheMutex;
    
    // Thread-safe singleton
    static std::atomic<AMIconManager *> m_instance;
    static std::mutex m_mutex;
};

} // namespace process
} // namespace core

#endif // AM_ICON_MANAGER_H
