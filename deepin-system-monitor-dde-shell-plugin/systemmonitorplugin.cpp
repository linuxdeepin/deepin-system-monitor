// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorplugin.h"

#include "systemmonitorapplet.h"

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <DGuiApplicationHelper>
#include <DStandardPaths>

DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

#define SYSTEM_MONITOR_KEY "system-monitor"
#define STATE_KEY "enable"

// ── SystemMonitorPlugin ────────────────────────────────────────────────

SystemMonitorPlugin::SystemMonitorPlugin(QObject *parent)
    : QObject(parent)
{
}

SystemMonitorPlugin::~SystemMonitorPlugin()
{
    if (m_cardView) {
        delete m_cardView;
        m_cardView = nullptr;
    }
}

const QString SystemMonitorPlugin::pluginName() const
{
    return QStringLiteral("system-monitor");
}

const QString SystemMonitorPlugin::pluginDisplayName() const
{
    return QCoreApplication::translate("Plugin.DisplayName", "System Monitor");
}

void SystemMonitorPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    // Load translations via DTK helper (searches standard data dirs)
    const QString trName = QStringLiteral("deepin-system-monitor-dde-shell-plugin");
    QStringList trDirs;
    const auto dataDirs = DStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const auto &dir : dataDirs) {
        trDirs << QDir(dir).filePath(QStringLiteral("deepin-system-monitor-dde-shell-plugin/translations"));
    }
    DGuiApplicationHelper::loadTranslator(trName, trDirs, { QLocale::system() });

    // Create applet (backend data model)
    m_applet.reset(new SystemMonitorApplet(this));

    refreshPluginItemsVisible();
}

QWidget *SystemMonitorPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return nullptr;
}

Dock::PluginFlags SystemMonitorPlugin::flags() const
{
    return Dock::Attribute_HasCard;
}

bool SystemMonitorPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, STATE_KEY, true).toBool();
}

void SystemMonitorPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());
    refreshPluginItemsVisible();
}

int SystemMonitorPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, -1).toInt();
}

void SystemMonitorPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

const QString SystemMonitorPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QJsonObject menuObj;
    QJsonArray items;

    QJsonObject openItem;
    openItem["itemId"] = "open";
    openItem["itemText"] = QCoreApplication::translate("Plugin.OpenSystemMonitor", "Open");
    openItem["isActive"] = true;
    items.append(openItem);

    menuObj["items"] = items;
    return QJsonDocument(menuObj).toJson();
}

void SystemMonitorPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    if (menuId == QLatin1String("open")) {
        m_applet->openSystemMonitor();
    }
}

void SystemMonitorPlugin::refreshIcon(const QString &itemKey)
{
    Q_UNUSED(itemKey)
}

QString SystemMonitorPlugin::cardItemKey() const
{
    return QStringLiteral("system-monitor-card");
}

QWindow *SystemMonitorPlugin::cardWindow() const
{
    if (m_cardView) {
        return m_cardView;
    }

    auto view = new QQuickView;
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->rootContext()->setContextProperty(QStringLiteral("systemMonitorApplet"), m_applet.data());
    view->setSource(QUrl(QStringLiteral("qrc:/systemmonitor/SystemMonitorCard.qml")));
    if (view->status() == QQuickView::Error) {
        delete view;
        return nullptr;
    }

    // Parent to this plugin so the window is destroyed before unload
    QQmlEngine::setObjectOwnership(view, QQmlEngine::CppOwnership);
    connect(this, &QObject::destroyed, view, [view](QObject*){ delete view; });

    m_cardView = view;
    return m_cardView;
}

void SystemMonitorPlugin::refreshPluginItemsVisible()
{
    if (!m_proxyInter) {
        return;
    }

    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, SYSTEM_MONITOR_KEY);
        return;
    }

    m_proxyInter->itemAdded(this, SYSTEM_MONITOR_KEY);
}

