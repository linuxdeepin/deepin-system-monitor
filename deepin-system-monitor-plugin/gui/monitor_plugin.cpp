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
#include "monitor_plugin.h"

// Qt
#include <QDBusConnectionInterface>
#include <DGuiApplicationHelper>
#include <DApplication>
#include <DApplicationHelper>
#include <DDBusSender>
#include <QIcon>
#include <DApplication>
#include <QGSettings>

namespace constantVal {
    const QString PLUGIN_STATE_KEY = "enable";
}

DWIDGET_USE_NAMESPACE

MonitorPlugin::MonitorPlugin(QObject *parent)
    : QObject (parent)
    , m_pluginLoaded(false)
    , m_dataTipsLabel(nullptr)
    , m_refershTimer(new QTimer(this))
{
    if (QGSettings::isSchemaInstalled("com.deepin.system.monitor.plugin")) {
        m_settings = new QGSettings("com.deepin.system.monitor.plugin", "/com/deepin/system/monitor/plugin/", this);
    }

    connect(m_refershTimer, &QTimer::timeout, this, &MonitorPlugin::udpateTipsInfo);
}

const QString MonitorPlugin::pluginName() const
{
    // 不要修改这个名字
    return QString("system-monitor");
}

const QString MonitorPlugin::pluginDisplayName() const
{
    return DApplication::translate("Plugin.DisplayName", "System Monitor");
}

void MonitorPlugin::init(PluginProxyInterface *proxyInter)
{
    // 第一次是为了正确加载翻译，第二次是为了避免影响dock的accessible
    QString applicationName = qApp->applicationName();
    qApp->setApplicationName("deepin-system-monitor-plugin");
    qApp->loadTranslator();
    qApp->setApplicationName(applicationName);

    m_proxyInter = proxyInter;

    if (!pluginIsDisable()) {
        loadPlugin();
    }

    calcCpuRate(m_totalCPU, m_availableCPU);
    calcNetRate(m_down, m_upload);
}

QWidget *MonitorPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return m_itemWidget;
}

void MonitorPlugin::pluginStateSwitched()
{
    bool pluginState = !m_proxyInter->getValue(this, constantVal::PLUGIN_STATE_KEY, true).toBool();
    m_proxyInter->saveValue(this, constantVal::PLUGIN_STATE_KEY, pluginState);

    refreshPluginItemsVisible();
}


bool MonitorPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, constantVal::PLUGIN_STATE_KEY, true).toBool();
}

QWidget *MonitorPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    m_dataTipsLabel->setSystemMonitorTipsText(QStringList() << "0.0" << "0.0" << "0KB/s" << "0KB/s");
    return m_dataTipsLabel.data();
}

const QString MonitorPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    DBusInterface::getInstance()->showOrHideDeepinSystemMonitorPluginPopupWidget();
    return "";

//    return "dbus-send --print-reply --dest=com.deepin.SystemMonitorPluginPopup /com/deepin/SystemMonitorPluginPopup com.deepin.SystemMonitorPluginPopup.showOrHideDeepinSystemMonitorPluginPopupWidget";
}

void MonitorPlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode);

    if (!pluginIsDisable()) {
        m_itemWidget->update();
    }
}

int MonitorPlugin::itemSortKey(const QString &itemKey)
{
    Dock::DisplayMode mode = displayMode();
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(mode);
    return m_proxyInter->getValue(this, key, DOCK_DEFAULT_POS).toInt();
}

void MonitorPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(displayMode());
    m_proxyInter->saveValue(this, key, order);
}

const QString MonitorPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QList<QVariant> items;
    items.reserve(1);
    //Add open System monitor mode button
    QMap<QString,QVariant> openSMO;
    openSMO["itemId"] = "openSystemMointor";
    openSMO["itemText"] = DApplication::translate("Plugin.OpenSystemMontitor", "Open");
    openSMO["isActive"] =true;
    items.push_back(openSMO);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    return QJsonDocument::fromVariant(menu).toJson();
}

void MonitorPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(checked);
    Q_UNUSED(itemKey);
    if (menuId == "openSystemMointor") {
        QProcess::startDetached("/usr/bin/deepin-system-monitor");
    }
}

void MonitorPlugin::udpateTipsInfo()
{
    // memory
    qlonglong memory = 0;
    qlonglong memoryAll = 0;
    calcMemRate(memory, memoryAll);
    QString memStr = QString("%1").arg(memory * 100.0 / memoryAll, 1, 'f', 1, QLatin1Char(' '));

    // CPU
    qlonglong totalCPU = 0;
    qlonglong availableCPU = 0;
    calcCpuRate(totalCPU, availableCPU);
    QString cpuStr = QString("%1").arg((((totalCPU - m_totalCPU) - (availableCPU - m_availableCPU)) * 100.0 / (totalCPU - m_totalCPU)), 1, 'f', 1, QLatin1Char(' '));
    m_totalCPU = totalCPU;
    m_availableCPU = availableCPU;

    // net
    qlonglong netUpload = 0;
    qlonglong netDownload = 0;
    double downRate = 0.0;
    double upRate = 0.0;

    RateUnit unit = RateByte;
    calcNetRate(netDownload, netUpload);
    downRate = autoRateUnits((netDownload - m_down) / (m_refershTimer->interval() / 1000), unit);
    QString downUnit = setRateUnitSensitive(unit);
    unit = RateByte;
    upRate = autoRateUnits((netUpload - m_upload) / (m_refershTimer->interval() /1000), unit);
    QString uploadUnit = setRateUnitSensitive(unit);
    QString downloadStr = QString("%1").arg(downRate, 1, 'f', 1, QLatin1Char(' ')) + downUnit;
    QString uploadStr = QString("%1").arg(upRate, 1, 'f', 1, QLatin1Char(' ')) + uploadUnit;

    m_down = netDownload;
    m_upload = netUpload;

    m_dataTipsLabel->setSystemMonitorTipsText(QStringList() << cpuStr << memStr << downloadStr << uploadStr);
}

void MonitorPlugin::loadPlugin()
{
    if (m_pluginLoaded)
        return;

    initPluginState();

    m_pluginLoaded = true;

    m_dataTipsLabel.reset(new SystemMonitorTipsWidget);
    m_dataTipsLabel->setObjectName("systemmonitorpluginlabel");

    m_refershTimer->setInterval(2000);
    m_refershTimer->start();

    m_itemWidget = new MonitorPluginButtonWidget;

    if (!m_isFirstInstall) {
        // 非初始状态
        if (m_proxyInter->getValue(this, constantVal::PLUGIN_STATE_KEY, true).toBool()) {
            m_proxyInter->itemAdded(this, pluginName());
        } else {
            m_proxyInter->saveValue(this, constantVal::PLUGIN_STATE_KEY, false);
            m_proxyInter->itemRemoved(this, pluginName());
        }
    } else {
        m_proxyInter->saveValue(this, constantVal::PLUGIN_STATE_KEY, false);
        m_proxyInter->itemRemoved(this, pluginName());
    }

    displayModeChanged(displayMode());
}

void MonitorPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        m_proxyInter->itemAdded(this, pluginName());
    }
}

void MonitorPlugin::initPluginState()
{
    if (m_settings == nullptr)
        return;
    if (m_settings->get("isfirstinstall").toBool()) {
        m_isFirstInstall = true;
        m_settings->set("isfirstinstall", false);
    } else {
        m_isFirstInstall = false;
    }
}

void MonitorPlugin::calcCpuRate(qlonglong &totalCPU, qlonglong &availableCPU)
{
    totalCPU = availableCPU = 0;
    bool ok = false;
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    QString line = stream.readLine();
    if (!line.isNull()) {
        QStringList list = line.split(QRegExp("\\s{1,}"));
        for (auto v = list.begin()+1; v != list.end(); ++v)
            totalCPU += (*v).toLongLong(&ok);
        if (list.size() > 4)
            availableCPU = list.at(4).toLongLong(&ok);
    }

    file.close();
}

void MonitorPlugin::calcMemRate(qlonglong &memory, qlonglong &memoryAll)
{
    memory = memoryAll = 0;
    bool ok = false;
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    qlonglong buff[16] = {0};
    for (int i = 0; i <= 15; ++i) {
        QString line = stream.readLine();
        QStringList list = line.split(QRegExp("\\s{1,}"));
        if (list.size() >= 2) {
            buff[i] = list.at(1).toLongLong(&ok);
        }
    }

    memoryAll = buff[0];
    memory = buff[0] - buff[2];

    file.close();
}

void MonitorPlugin::calcNetRate(qlonglong &netDown, qlonglong &netUpload)
{
    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    qlonglong down = 0;
    qlonglong upload = 0;
    QTextStream stream(&file);
    QString line = stream.readLine();
    line = stream.readLine();
    line = stream.readLine();
    while (!line.isNull()) {
        line = line.trimmed();
        QStringList list = line.split(QRegExp("\\s{1,}")); // match number >= 1 space character

        if (!list.isEmpty()) {
            down = list.at(1).toLongLong();
            upload = list.at(9).toLongLong();
        }

        netDown += down;
        netUpload += upload;
        line = stream.readLine();
    }

    file.close();
}

QString MonitorPlugin::setRateUnitSensitive(MonitorPlugin::RateUnit unit)
{
    switch (unit) {
        case RateUnit::RateBit:
            return QString("BIT/s");
        case RateUnit::RateByte:
            return QString("B/s");
        case RateUnit::RateKb:
            return QString("KB/s");
        case RateUnit::RateMb:
            return QString("MB/s");
        case RateUnit::RateGb:
            return QString("GB/s");
        case RateUnit::RateTb:
            return QString("TB/s");
        default:
            return QString("");
    }
}

double MonitorPlugin::autoRateUnits(qlonglong speed, RateUnit &unit)
{
    /* 自动判断合适的速率单位,默认传进来的是 Byte
     * bit    0 ~ 7 位 (不到 1 字节)
     * Byte   1    ~ 2^10  Byte
     * KB     2^10 ~ 2^20  Byte
     * MB     2^20 ~ 2^30  Byte
     * GB     2^30 ~ 2^40  Byte
     * TB     2^40 ~ 2^50  Byte
     */

    if (unit != RateByte) {
        return -1;
    }

    double sp = 0;
    if (0 <= speed && speed < qPow(2, 10)) {
        unit = RateByte;
        sp = speed;
    } else if (qPow(2, 10) <= speed && speed < qPow(2, 20)) {
        unit = RateKb;
        sp = static_cast<double>(speed / qPow(2, 10) * 1.0);
    } else if (qPow(2, 20) <= speed && speed < qPow(2, 30)) {
        unit = RateMb;
        sp = static_cast<double>(speed / qPow(2, 20) * 1.0);
    } else if (qPow(2, 30) <= speed && speed < qPow(2, 40)) {
        unit = RateGb;
        sp = static_cast<double>(speed / qPow(2, 30) * 1.0);
    } else if (qPow(2, 40) <= speed && speed < qPow(2, 50)) {
        unit = RateTb;
        sp = static_cast<double>(speed / qPow(2, 40) * 1.0);
    } else {
        unit = RateUnknow;
        qDebug()<<"本设备网络速率单位传输超过 TB, 或者低于 0 Byte.";
        sp = -1;
    }

    return sp;
}
