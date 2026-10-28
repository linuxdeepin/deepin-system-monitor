// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_POPUP_STUB_H
#define UT_POPUP_STUB_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QRect>
#include <QSet>
#include <QString>
#include <QVariant>

class QDBusObjectPath;
class QObject;

/*!
 * \brief popup 插件单测共享打桩设施
 *
 * 外部依赖分两层:
 *  - 进程内桩(打桩库):DBus 接口有效性 / QObject::property 登记表 /
 *    合成器状态 / 键盘区域注册 / detached 进程 / DataDealSingleton 数据源 /
 *    监控线程启动。
 *  - 私有总线 Mock 服务(独立进程 dsm-popup-mock-service):提供
 *    Display1(Monitors/PrimaryRect)与 Dock1 的真实 Properties.Get 应答,
 *    使源码中的 QDBusArgument 解组路径得到真实编组数据。
 * 各用例仅通过 knob(全局开关/计数器/接口属性表)控制桩行为,TearDown 复位。
 */
namespace ut_popup_stub {

// ---- 计数器与观测值 ----
extern int g_startDetachedCount;        // QProcess::startDetached 静态调用次数
extern int g_threadStartCount;          // 线程 start() 拦截次数
extern QString g_lastDetachedProgram;   // 最近一次 detached 启动的程序名
extern int g_registerRegionCount;       // DRegionMonitor::registerRegion 调用次数
extern int g_jumpWidgetCount;           // DataDealSingleton::sendJumpWidgetMessage 调用次数
extern QString g_lastJumpMessage;       // 最近一次跳转消息码

// ---- DataDealSingleton 数据源(读网/读盘/跳转) knob ----
extern bool g_readNetInfoResult;         // readNetInfo 返回值
extern QString g_netReceive;            // 网络下行速率("12.5 KB/s")
extern QString g_netTotalReceive;       // 累计接收("1.2 MB")
extern QString g_netSend;               // 网络上行速率
extern QString g_totalSend;             // 累计发送
extern bool g_readDiskInfoResult;       // readDiskInfo 返回值
extern QString g_diskRead;              // 磁盘读速率
extern QString g_diskTotalSize;         // 磁盘总容量
extern QString g_diskWrite;             // 磁盘写速率
extern QString g_diskAvailable;         // 磁盘可用容量
extern bool g_jumpMessageResult;        // sendJumpWidgetMessage 返回值

// ---- 行为开关 ----
extern bool g_hasComposite;             // DWindowManagerHelper::hasComposite 返回值
extern bool g_displayInterValid;        // Display 接口(m_displayInter 等) isValid 兜底
extern bool g_monitorLoopValid;         // getDisplayScreen 内显示器监视接口 isValid 兜底

// ---- QDBusInterface 控制 ----
// 按指针登记"连接有效"的接口(对应 m_displayInter / m_daemonDockInter 等)
void setInterfaceValid(const QObject *iface, bool valid);
// 按指针登记接口属性表,property(name) 直接返回登记值
void setInterfaceProperty(const QObject *iface, const QString &name, const QVariant &value);
// Mock Display1 的 PrimaryRect 属性值(Properties.Get 应答)
void setPrimaryRectReply(bool enabled, const QRect &rect);
// Mock Display1 的 Monitors 属性值(Properties.Get 应答)
void setMonitorPathsReply(bool enabled, const QList<QDBusObjectPath> &paths);

// ---- 安装与复位 ----
void installAll();                      // 进程级安装全部桩(幂等,main 中调用一次)
bool startPrivateSessionBus();          // 启动私有 dbus-daemon 并设置总线地址(须在 Application 构造前调用)
bool startMockService();                // 拉起 Mock 服务进程并等待服务注册完成
void stopMockService();                 // 结束 Mock 服务进程与私有总线(退出前调用)
void resetKnobs();                      // 复位全部 knob/计数器(各用例 SetUp/TearDown 调用)

}  // namespace ut_popup_stub

#endif  // UT_POPUP_STUB_H
