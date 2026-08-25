// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEMMONITORPLUGIN_H
#define SYSTEMMONITORPLUGIN_H

#include "pluginsiteminterface_v3.h"
#include "systemmonitorapplet.h"

#include <QPointer>
#include <QScopedPointer>

class QQuickView;
class QLabel;

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

    QScopedPointer<SystemMonitorApplet> m_applet;
    mutable QPointer<QQuickView> m_cardView;

    class QuickPanelWidget;
    QScopedPointer<QuickPanelWidget> m_quickPanelWidget;
};

#endif // SYSTEMMONITORPLUGIN_H
