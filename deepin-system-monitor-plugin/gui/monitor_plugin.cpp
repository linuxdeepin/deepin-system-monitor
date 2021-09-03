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
//#include <QGSettings>

namespace constantVal {
    const QString PLUGIN_STATE_KEY = "enable";
}

DWIDGET_USE_NAMESPACE

MonitorPlugin::MonitorPlugin(QObject *parent)
    : QObject (parent)
    , m_pluginLoaded(false)
    , m_tipsLabel(new QLabel)
    , m_refershTimer(new QTimer(this))
{
    i=db=ub=dbt=ubt=dbt1=ubt1=dbt0=ubt0=0;
    m_tipsLabel->setVisible(false);
    m_tipsLabel->setObjectName("system-monitor");
//    m_tipsLabel->setStyleSheet("color:white; padding:0px 3px;");

    m_refershTimer->setInterval(2000);
    m_refershTimer->start();

    changeTheme(DGuiApplicationHelper::instance()->themeType());

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MonitorPlugin::changeTheme);

    connect(m_refershTimer, &QTimer::timeout, this, &MonitorPlugin::udpateTipsInfo);
}

const QString MonitorPlugin::pluginName() const
{
    // 不要修改这个名字
    return QString("system-monitor");
}

const QString MonitorPlugin::pluginDisplayName() const
{
//    return DApplication::translate("Plugin.DisplayName", "System Monitor Plugin");
    return QString("System Monitor Plugin");
}

void MonitorPlugin::init(PluginProxyInterface *proxyInter)
{
    // 第一次是为了正确加载翻译，第二次是为了避免影响dock的accessible
//    QString applicationName = qApp->applicationName();
//    qApp->setApplicationName("deepin-system-monitor-plugin");
//    qApp->loadTranslator();
//    qApp->setApplicationName(applicationName);

    m_proxyInter = proxyInter;

    //初始化系统监视器的时候,默认不加载插件
    m_proxyInter->saveValue(this, constantVal::PLUGIN_STATE_KEY, false);

    refreshPluginItemsVisible();
    ///////////////
//    if (!pluginIsDisable()) {
//        loadPlugin();
//    }
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
    Dtk::Gui::DGuiApplicationHelper::ColorType colorType = DGuiApplicationHelper::instance()->themeType();
    QString txtColor;
    switch (colorType) {
    case Dtk::Gui::DGuiApplicationHelper::LightType:
        txtColor = "black";
        break;
    case Dtk::Gui::DGuiApplicationHelper::DarkType:
        txtColor = "white";
        break;
    default:
        txtColor = "white";
        break;
    }

    m_tipsLabel->setText(QString("<font style='color:%1;'>CPU:%2%&nbsp;&nbsp;&nbsp;</font>").arg(txtColor).arg(0)
                         + QString("<font style='color:red;'>↓</font>")
                         + QString("<font style='color:%1;'>%2kb/s&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br/></font>").arg(txtColor).arg(0)
                         + QString("<font style='color:%1;'>MEM: %2%&nbsp;&nbsp;&nbsp;</font>").arg(txtColor).arg(0)
                         + QString("<font style='color:blue;'>↑</font>")
                         + QString("<font style='color:%1;'>%2kb/s&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</font>").arg(txtColor).arg(0));

    return m_tipsLabel;
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
//    openSMO["itemText"] = DApplication::translate("Plugin.OpenSystemMontitor", "open system monitor");
    openSMO["itemText"] = "openSystemMointor";
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
//        DDBusSender()
//            .service("com.deepin.dde.ControlCenter")
//            .interface("com.deepin.dde.ControlCenter")
//            .path("/com/deepin/dde/ControlCenter")
//            .method("ShowPage")
//            .arg(QString("notification"))
//            .arg(QString("System Notification"))
//            .call();
    }
}

void MonitorPlugin::changeTheme(Dtk::Gui::DGuiApplicationHelper::ColorType themeType)
{
    QString txtColor;
    switch (themeType) {
    case Dtk::Gui::DGuiApplicationHelper::LightType:
        txtColor = "black";
        break;
    case Dtk::Gui::DGuiApplicationHelper::DarkType:
        txtColor = "white";
        break;
    default:
        txtColor = "white";
        break;
    }
    QPalette pa = m_tipsLabel->palette();
    pa.setBrush(QPalette::WindowText, pa.brightText());
    m_tipsLabel->setPalette(pa);
}

QString MonitorPlugin::KB(long k)
{
    QString s = "";
    if(k > 999999){
        s = QString::number(k/(1024*1024.0),'f',2) + "gb";
    }else{
        if(k > 999){
            s = QString::number(k/1024.0,'f',2) + "mb";
        }else{
            s = QString::number(k/1.0,'f',2) + "kb";
        }
    }
    return s;
}

QString MonitorPlugin::BS(long b)
{
    QString s = "";
    if(b > 999999999){
        //s = QString("%1").arg(b/(1024*1024*1024.0), 6, 'f', 2, QLatin1Char(' ')) + "GB";
        s = QString::number(b/(1024*1024*1024.0), 'f', 2) + "gb";
    }else{
        if(b > 999999){
            //s = QString("%1").arg(b/(1024*1024.0), 6, 'f', 2, QLatin1Char(' ')) + "MB";
            s = QString::number(b/(1024*1024.0), 'f', 2) + "mb";
        }else{
            if(b>999){
                //s = QString("%1").arg(b/1024.0, 6, 'f', 2, QLatin1Char(' ')) + "KB";
                s = QString::number(b/(1024.0), 'f',2) + "kb";
            }else{
                s = b + "B";
            }
        }
    }
    return s;
}

QString MonitorPlugin::NB(long b)
{
    QString s = "";
    if (b>999) {
        s = QString("%1").arg(b/1024, 5, 'f', 0, QLatin1Char(' ')) + "kb";
    } else { // <1K => 0
        s = QString("%1").arg(0, 5, 'f', 0, QLatin1Char(' ')) + "kb";
    }
    return s;
}

void MonitorPlugin::udpateTipsInfo()
{
    // uptime
    QFile file;
    QString l;

    // memory
    file.setFileName("/proc/meminfo");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    long mt = l.replace("MemTotal:","").replace("kB","").replace(" ","").toLong();
    l = file.readLine();
    l = file.readLine();
    long ma = l.replace("MemAvailable:","").replace("kB","").replace(" ","").toLong();
    l = file.readLine();
    l = file.readLine();
    file.close();
    long mu = mt - ma;
    qreal mp = mu*100.0/mt;
    QString mem = "MEM: " + QString("%1 / %2 = %3").arg(KB(mu)).arg(KB(mt)).arg(QString::number(mp) + "%");

    // CPU
    file.setFileName("/proc/stat");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    QByteArray ba;
    ba = l.toLatin1();
    const char *ch;
    ch = ba.constData();
    char cpu[5];
    long user,nice,sys,idle,iowait,irq,softirq,tt;
    sscanf(ch,"%s%ld%ld%ld%ld%ld%ld%ld",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);
    tt = user + nice + sys + idle + iowait + irq + softirq;
    file.close();
    QString cusage = "";
    qreal cp = ((tt-tt0)-(idle-idle0))*100.0/(tt-tt0);
    if(i>0) cusage = "CPU: " + QString::number(cp) + "%";
    idle0 = idle;
    tt0 = tt;

    // net
    file.setFileName("/proc/net/dev");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    l = file.readLine();
    dbt1 = ubt1 = 0;
    while(!file.atEnd()){
        l = file.readLine();
        QStringList list = l.split(QRegExp("\\s{1,}"));
        db = list.at(1).toLong();
        ub = list.at(9).toLong();
        dbt1 += db;
        ubt1 += ub;
    }
    file.close();
    QString dss = "";
    QString uss = "";
    if (i > 0) {
        long ds = dbt1 - dbt0;
        long us = ubt1 - ubt0;
        dss = NB(ds) + "/s";
        uss = NB(us) + "/s";
        dbt0 = dbt1;
        ubt0 = ubt1;
    }
    QString netspeed = "↑" + uss + "\n↓" + dss;
    QString net = "UPB: " + BS(ubt1) + "  " + uss + "\nDNB: " + BS(dbt1) + "  " + dss;

    i++;
    if (i>2) i = 2;

    // draw
    Dtk::Gui::DGuiApplicationHelper::ColorType colorType = DGuiApplicationHelper::instance()->themeType();
    QString txtColor;
    switch (colorType) {
    case Dtk::Gui::DGuiApplicationHelper::LightType:
        txtColor = "black";
        break;
    case Dtk::Gui::DGuiApplicationHelper::DarkType:
        txtColor = "white";
        break;
    default:
        txtColor = "white";
        break;
    }
    QString cpStr = QString::number(cp, 'f', 1);
    if (cpStr.length() == 3) {
        cpStr = QString("   ") + cpStr;
    }

    QString mpStr = QString::number(mp, 'f', 1);
    if (mpStr.length() == 3) {
        mpStr = QString("   ") + mpStr;
    }

    m_tipsLabel->setText(QString("<font style='color:%1;'>CPU:&nbsp;&nbsp;%2%</font>").arg(txtColor).arg(cpStr)
                         + QString("<font style='color:red;'>↓</font>")
                         + QString("<font style='color:%1;'>%2&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br/></font>").arg(txtColor).arg(dss)
                         + QString("<font style='color:%1;'>MEM:&nbsp;%2%&nbsp;&nbsp;&nbsp;&nbsp;</font>").arg(txtColor).arg(mpStr)
                         + QString("<font style='color:blue;'>↑</font>")
                         + QString("<font style='color:%1;'>%2&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</font>").arg(txtColor).arg(uss));

}

void MonitorPlugin::loadPlugin()
{
    if (m_pluginLoaded)
        return;

    m_pluginLoaded = true;

    m_itemWidget = new MonitorPluginButtonWidget;

    m_proxyInter->itemAdded(this, pluginName());

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
