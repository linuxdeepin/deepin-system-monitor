/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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
#ifndef MONITORPLUGIN_H
#define MONITORPLUGIN_H

//  smo-plugin
#include <dde-dock/pluginsiteminterface.h>
#include "monitorpluginbuttonwidget.h"

#include "dbus/dbusinterface.h"

// Qt
#include <QDBusInterface>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>
#include <DGuiApplicationHelper>

#define DOCK_DEFAULT_POS   0

class QGSettings;
//!
//! \brief The MonitorPlugin class
//! 实现插件接口对象的类
//!
class MonitorPlugin : public QObject, PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "system-monitor.json")
public:
    //!
    //! \brief MonitorPlugin 构造函数
    //! \param parent 传递的父对象指针
    //!
    explicit MonitorPlugin(QObject *parent = Q_NULLPTR);

    // 以下是必须要实现的PluginsItemInterface接口
    //!
    //! \brief pluginName 返回插件名称，用于在dde-dock内部管理插件时使用
    //! \return 插件名称
    //!
    const QString pluginName() const Q_DECL_OVERRIDE;
    //!
    //! \brief pluginDisplayName 返回插件名称，用于在界面上显示
    //! \return 返回插件名称
    //!
    const QString pluginDisplayName() const Q_DECL_OVERRIDE;
    //!
    //! \brief init 插件初始化入口函数
    //! \param proxyInter 主程序的进程
    //!
    void init(PluginProxyInterface *proxyInter) Q_DECL_OVERRIDE;
    //!
    //! \brief itemWidget 返回插件主控件，用于显示在dde-dock面板上
    //! \param itemKey 返回插件主控件
    //! \return
    //!
    QWidget *itemWidget(const QString &itemKey) Q_DECL_OVERRIDE;
    // 以上是必须要实现的PluginsItemInterface接口

    //!
    //! \brief pluginStateSwitched 当插件的禁用状态被用户改变时此接口被调用
    //!
    void pluginStateSwitched() Q_DECL_OVERRIDE;

    //!
    //! \brief pluginIsAllowDisable 返回插件是否允许被禁用（默认不允许被禁用)
    //! \return
    //!
    bool pluginIsAllowDisable() Q_DECL_OVERRIDE { return true; }

    //!
    //! \brief pluginIsDisable 返回插件当前是否处于被禁用的状态
    //! \return
    //!
    bool pluginIsDisable() Q_DECL_OVERRIDE;

    //!
    //! \brief itemTipsWidget 返回鼠标悬浮在插件主控件上时显示的提示框控件
    //! \param itemKey
    //! \return 悬浮窗控件
    //!
    QWidget *itemTipsWidget(const QString &itemKey) Q_DECL_OVERRIDE;

    //!
    //! \brief itemCommand 返回鼠标左键点击插件主控件后要执行的命令数据
    //! \param itemKey
    //! \return
    //!
    const QString itemCommand(const QString &itemKey) Q_DECL_OVERRIDE;

    //!
    //! \brief displayModeChanged dde-dock显示模式发生改变时此接口被调用
    //! \param displayMode
    //!
    void displayModeChanged(const Dock::DisplayMode displayMode) Q_DECL_OVERRIDE;

    //!
    //! \brief itemSortKey 返回插件主控件的排序位置
    //! \param itemKey
    //! \return
    //!
    int itemSortKey(const QString &itemKey) Q_DECL_OVERRIDE;

    //!
    //! \brief setSortKey 重新设置主控件新的排序位置（用户拖动了插件控件后)
    //! \param itemKey
    //! \param order
    //!
    void setSortKey(const QString &itemKey, const int order) Q_DECL_OVERRIDE;

    //!
    //! \brief itemContextMenu 返回鼠标右键点击插件主控件后要显示的菜单数据
    //! \param itemKey
    //! \return
    //!
    const QString itemContextMenu(const QString &itemKey) Q_DECL_OVERRIDE;

    //!
    //! \brief invokedMenuItem 菜单项被点击后的回调函数
    //! \param itemKey
    //! \param menuId
    //! \param checked
    //!
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) Q_DECL_OVERRIDE;

private slots:
    //!
    //! \brief changeTheme 切换系统主题时，更新插件界面
    //!
    void changeTheme(Dtk::Gui::DGuiApplicationHelper::ColorType themeType);

    //!
    //! \brief udpateTipsInfo 更新CPU MEM NET信息
    //!
    void udpateTipsInfo();

private:
    //!
    //! \brief loadPlugin 加载插件
    //!
    void loadPlugin();

    //!
    //! \brief refreshPluginItemsVisible 刷新插件项显示隐藏
    //!
    void refreshPluginItemsVisible();


    QString KB(long k);
    QString BS(long b);
    QString NB(long b);
private:
    bool m_pluginLoaded;

    MonitorPluginButtonWidget *m_itemWidget = nullptr;

    QLabel *m_tipsLabel;
    QGSettings *m_settings;

    long int i, db, ub, dbt, ubt, dbt1, ubt1, dbt0, ubt0, tt0, idle0;
    QTimer *m_refershTimer;

    QString startup;

};

#endif // MONITORPLUGIN_H
