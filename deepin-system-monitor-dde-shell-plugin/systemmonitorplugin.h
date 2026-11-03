// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEMMONITORPLUGIN_H
#define SYSTEMMONITORPLUGIN_H

#include "pluginsiteminterface_v3.h"
#include "systemmonitorcontroller.h"

#include <QPointer>
#include <QScopedPointer>

class QLabel;
class QQuickView;

/**
 * @brief The SystemMonitorPlugin class
 * 系统监视器 dock 插件（V3 卡片插件）：托盘隐藏项作为 fashion 消息载体，
 * 时尚模式下在卡片区域展示 SystemMonitorCard.qml。
 */
class SystemMonitorPlugin : public QObject, public PluginsItemInterfaceV3
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV3)
    Q_PLUGIN_METADATA(IID ModuleInterface_iid_V3 FILE "system-monitor.json")

public:
    explicit SystemMonitorPlugin(QObject *parent = nullptr);
    ~SystemMonitorPlugin() override;

    // V1 required interfaces
    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;

    // V2 interfaces
    Dock::PluginFlags flags() const override;
    bool pluginIsDisable() override;
    void pluginStateSwitched() override;
    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
    void refreshIcon(const QString &itemKey) override;

    // V3 card interfaces
    QString cardItemKey() const override;
    QWindow *cardWindow() const override;

private:
    void refreshPluginItemsVisible();
    QWidget *trayCarrierWidget();

    QScopedPointer<SystemMonitorController> m_controller;
    mutable QPointer<QQuickView> m_cardView;
    QScopedPointer<QLabel> m_trayCarrier;
};

#endif // SYSTEMMONITORPLUGIN_H
