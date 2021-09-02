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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


// dsmPluginPopup
#include "constants.h"
#include "dbus/systemmonitordbusadaptor.h"
#include "dbus/dbusdockinterface.h"
#include "cpu_widget.h"
#include "net_widget.h"
#include "memory_widget.h"
#include "disk_widget.h"
#include "process_widget.h"

// Qt
#include <QListView>
#include <DBlurEffectWidget>
#include <DWindowManagerHelper>
#include <DRegionMonitor>
#include <DWidget>

// Dtk
#include <com_deepin_daemon_display_monitor.h>
#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_daemon_display.h>


DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QPropertyAnimation)
QT_FORWARD_DECLARE_CLASS(QSequentialAnimationGroup)


using DBusDisplay = com::deepin::daemon::Display;
using DisplayMonitor = com::deepin::daemon::display::Monitor;
using DBusDaemonDock = com::deepin::dde::daemon::Dock;

/*!
 * \~chinese \class MainWindow
 * \~chinese \brief 主窗口类
 */
class MainWindow : public DBlurEffectWidget
{
    Q_OBJECT
    Q_PROPERTY(int width READ getWidth WRITE setFixedWidth)
    Q_PROPERTY(int x READ getX WRITE setX)
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public Q_SLOTS:
    /*!
     * \~chinese \name Toggle
     * \~chinese \brief 切换主窗口显示或隐藏的状态
     */
    void Toggle();
    /*!
     * \~chinese \name showAni
     * \~chinese \brief 主窗口显示时的动画
     */
    void showAni();
    /*!
     * \~chinese \name hideAni
     * \~chinese \brief 主窗口隐藏时的动画
     */
    void hideAni();

    void startLoader();

    void Show();
    void Hide();

private Q_SLOTS:
    /*!
     * \~chinese \name geometryChanged
     * \~chinese \brief 根据dock和屏幕的大小改变窗口的大小,根据dock的位置调整主窗口的位置
     */
    void geometryChanged();
    /*!
     * \~chinese \name setX
     * \~chinese \brief 设置主窗口X轴的起始坐标
     * \~chinese \param X轴坐标位置
     */
    void setX(int x);
    /*!
     * \~chinese \name CompositeChanged
     * \~chinese \brief 是否开启动画效果
     */
    void CompositeChanged();

    void registerMonitor();

private:
    /*!
     * \~chinese \name initUI
     * \~chinese \brief 初始化界面布局
     */
    void initUI();
    /*!
     * \~chinese \name initAni
     * \~chinese \brief 初始化动画效果
     */
    void initAni();
    /*!
     * \~chinese \name initConnect
     * \~chinese \brief 初始化信号与槽的连接
     */
    void initConnect();
    /*!
     * \~chinese \name adjustPosition
     * \~chinese \brief 调整主界面位置
     */
    void adjustPosition();
    /*!
     * \~chinese \name getDisplayScreen
     * \~chinese \brief 获取显示屏幕的坐标
     */
    QRect getDisplayScreen();


    int getWidth() const { return this->width(); }
    int getX() const { return this->pos().x(); }


    bool initDBus();
    void changeTheme(DApplicationHelper::ColorType themeType);

protected:
    /*!
     * \~chinese \name initConnect
     * \~chinese \brief 重写mouseMoveEvent事件禁止窗口被移动
     */
    virtual void mouseMoveEvent(QMouseEvent *event) override;
private slots:
    void slotShowOrHideSystemMonitorPluginPopupWidget();

private:
    DBusDisplay *m_displayInter;
    DBusDaemonDock *m_daemonDockInter;
    DBusDockInterface *m_dockInter;
    SystemMonitorDBusAdaptor *m_systemMonitorDbusAdaptor;
    DRegionMonitor *m_regionMonitor;

    DWidget *m_content;
//    IconButton *m_clearButton;
//    ListView *m_listview;
//    ClipboardModel *m_model;
//    ItemDelegate *m_itemDelegate;

    QPropertyAnimation *m_xAni;
    QPropertyAnimation *m_widthAni;
    QSequentialAnimationGroup *m_aniGroup;
    QRect m_rect;
    DWindowManagerHelper *m_wmHelper;
    bool m_hasComposite = false;


    CpuWidget *m_cpuMonitor {};
    NetWidget *m_netWidget {};
    DiskWidget *m_diskWidget {};
    MemoryWidget *m_memoryWidget {};
    ProcessWidget *m_processWidget{};
};

#endif // MAINWINDOW_H
