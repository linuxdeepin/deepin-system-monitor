/*
* Copyright (C) 2011 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "systemprotectionsetting.h"

#include <DSettingsWidgetFactory>
#include <DCommandLinkButton>
#include <DSwitchButton>
#include <DLineEdit>

#include <QStandardPaths>
#include <QApplication>
#include <DSettingsOption>
#include <QDebug>
#include <QDBusInterface>
#include <QProcess>
#include <QTimer>
#include <QDBusConnection>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QIntValidator>

#ifndef AlarmStatusOptionName
    #define AlarmStatusOptionName   "setting.systemprotection.alarm_switch"
    #define AlarmCpuUsageOptionName "setting.systemprotection.alarm_cpu_usage"
    #define AlarmMemUsageOptionName "setting.systemprotection.alarm_memory_usage"
    #define AlarmIntervalOptionName "setting.systemprotection.alarm_interval"
#endif

Q_GLOBAL_STATIC(SystemProtectionSetting, theInstance)

SystemProtectionSetting::SystemProtectionSetting(QObject *parent)
    : QObject(parent)
    , mBackend(nullptr)
    , mDsettings(nullptr)
    , mDaemonInterface("com.deepin.SystemMonitor.Daemon",
                       "/com/deepin/SystemMonitor",
                       "com.deepin.SystemMonitor.Daemon")
{
    mDaemonInterface.setParent(this);
    // json文件加载setting基本结构
    mDsettings = DSettings::fromJsonFile(":/resources/settings.json");
    // 更新数据后台并同步数据
    onUpdateNewBackend();
    // 连接设置数据更改信号
    connect(mDsettings, &DSettings::valueChanged, this, &SystemProtectionSetting::onSettingItemChanged);
    // 跳转通知按钮单独处理
    connect(mDsettings->option("setting.messgae.message_setting"), &Dtk::Core::DSettingsOption::valueChanged,
            this, &SystemProtectionSetting::onMessgaeSetting);
}

SystemProtectionSetting::~SystemProtectionSetting()
{
    if(mDsettings != nullptr) {
        mDsettings->deleteLater();
    }

    if(mBackend != nullptr) {
        mBackend->deleteLater();
    }
}

// 返回全局单例
SystemProtectionSetting* SystemProtectionSetting::instance()
{
    return theInstance;
}

QPair<QWidget*, QWidget*> SystemProtectionSetting::createSettingLinkButtonHandle(QObject *obj)
{
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    // 构建自定义Item
    QWidget* widget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addStretch();
    DCommandLinkButton* button = new DCommandLinkButton(tr("设置"), widget);
    layout->addWidget(button);
    widget->setLayout(layout);

    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, widget);
    // 获取初始值
    option->setValue(option->defaultValue());

    option->connect(button, &DCommandLinkButton::clicked, option, [=](){
        qDebug() << __FUNCTION__ << __LINE__ << "，will jump to dde setting center notification page!";
        SystemProtectionSetting::instance()->onMessgaeSetting("");
    });

    return optionWidget;
}

QPair<QWidget*, QWidget*> SystemProtectionSetting::createProtectionSwitchHandle(QObject *obj)
{
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    // 构建自定义Item
    QWidget* widget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(widget);
    QLabel *label = new QLabel(widget);
    label->setText(tr("(阈值可修改)"));
    layout->addWidget(label);
    layout->addStretch();

    DSwitchButton *button = new DSwitchButton(widget);
    layout->addWidget(button);
    widget->setLayout(layout);

    // 获取初始值
    button->setChecked(option->value().toBool());

    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, widget);

    // Item操作修改Setting值
    option->connect(button, &DSwitchButton::clicked, option, [=](bool checked){
        qDebug() << __FUNCTION__ << __LINE__ << ", new protection swich :" << checked;
        if(checked != option->value().toBool()) {
            option->setValue(checked);
        }
    });

    // 用于恢复默认时，Item的数据更新
    button->connect(option, &DSettingsOption::valueChanged, button, [=]() {
        if(option->value().toBool() != button->isChecked()) {
            button->setChecked(option->value().toBool());
        }
    });

    return optionWidget;
}

QPair<QWidget*, QWidget*> SystemProtectionSetting::createAlarmUsgaeSettingHandle(QObject *obj)
{
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    // 构建自定义Item
    const int lineEditWidth = 60;
    const int lineEditHeight = 36;
    const int labelWidth = 300;
    const int labelHeight = 17;
    const int widgetSpace = 15;

    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    // 构建编辑框
    DLineEdit *edit = new DLineEdit(widget);
    edit->setFixedSize(lineEditWidth, lineEditHeight);
    edit->setClearButtonEnabled(false);
    // 为编辑框设置数字输入限制
    QIntValidator *validator = new QIntValidator(edit);
    validator->setRange(1,100);
    edit->lineEdit()->setValidator(validator);

    // 构建提示语
    QLabel *label = new QLabel(widget);
    label->setText(QString(tr("数值范围: %1-%2")).arg(30).arg(100));
    label->setFixedSize(labelWidth, labelHeight);

    // 构建布局
    layout->addWidget(edit);
    layout->addSpacing(widgetSpace);
    layout->addWidget(label);
    layout->addStretch();
    widget->setLayout(layout);

    // 获取初始值
    edit->setText(option->value().toString());

    // 修改Item同步修改Setting数据
    option->connect(edit, &DLineEdit::focusChanged, option, [=] (bool onFocus) {
        if(onFocus == false) {
            if(edit->text().isEmpty() || edit->text().toInt() < 30 || edit->text().toInt() > 100) {
                 edit->setText(option->defaultValue().toString());
            }

            int newValue = edit->text().toInt();
            option->setValue(newValue);
        }
    } );

    // 用于恢复默认时，Item的数据更新
    edit->connect(option, &DSettingsOption::valueChanged, edit, [=]() {
        if(option->value().toInt() != edit->text().toInt()) {
            edit->setText(option->value().toString());
        }
    });

    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, widget);

    return optionWidget;
}

QPair<QWidget*, QWidget*> SystemProtectionSetting::createAlarmIntervalSettingHandle(QObject *obj)
{
    auto option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(obj);

    // 构建自定义Item
    const int lineEditWidth = 60;
    const int lineEditHeight = 36;
    const int labelWidth = 300;
    const int labelHeight = 17;
    const int widgetSpace = 15;

    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    // 构建编辑框
    DLineEdit *edit = new DLineEdit(widget);
    edit->setFixedSize(lineEditWidth, lineEditHeight);
    edit->setClearButtonEnabled(false);
    // 为编辑框设置数字输入限制
    QIntValidator *validator = new QIntValidator(edit);
    validator->setRange(1,60);
    edit->lineEdit()->setValidator(validator);

    // 构建提示语
    QLabel *label = new QLabel(widget);
    label->setText(QString(tr("数值范围: %1-%2")).arg(5).arg(60));
    label->setFixedSize(labelWidth, labelHeight);

    // 构建布局
    layout->addWidget(edit);
    layout->addSpacing(widgetSpace);
    layout->addWidget(label);
    layout->addStretch();
    widget->setLayout(layout);

    // 设定Item初始值
    edit->setText(option->value().toString());

    // 修改Item同步修改Setting数据
    option->connect(edit, &DLineEdit::focusChanged, option, [=] (bool onFocus) {
        if(onFocus == false) {
            if(edit->text().isEmpty() || edit->text().toInt() < 5 || edit->text().toInt() > 60) {
                 edit->setText(option->defaultValue().toString());
            }

            int newValue = edit->text().toInt();
            option->setValue(newValue);
        }
    } );

    // 用于恢复默认时，Item的数据更新
    edit->connect(option, &DSettingsOption::valueChanged, edit, [=]() {
        if(option->value().toInt() != edit->text().toInt()) {
            edit->setText(option->value().toString());
        }
    });

    QPair<QWidget *, QWidget *> optionWidget = DSettingsWidgetFactory::createStandardItem(QByteArray(), option, widget);

    return optionWidget;
}

void SystemProtectionSetting::onMessgaeSetting(QVariant value)
{
    Q_UNUSED(value);
    QString genericName;
    QFile desktopFile("/usr/share/applications/deepin-system-monitor.desktop");

    // 解析desktop文件，获取当前 GenericName
    if(desktopFile.exists() && desktopFile.open(QFile::ReadOnly)) {
        QLocale locale;
        QString searchString;
        searchString = QString("GenericName[%1]").arg(locale.name());

        while(desktopFile.atEnd() == false) {
            QString line = desktopFile.readLine();
            if(line.left(searchString.size()) == searchString) {
                genericName = line.mid(searchString.size() + 1);
                genericName = genericName.trimmed();
                break;
            }
        }
        desktopFile.close();
    }

    if(genericName.isEmpty() == false) {
        // 跳转到设置页并指定Item
        QDBusMessage showDDEControlCenterPage = QDBusMessage::createMethodCall("com.deepin.dde.ControlCenter",
                                                        "/com/deepin/dde/ControlCenter",
                                                        "com.deepin.dde.ControlCenter",
                                                        "ShowPage");
        QList<QVariant> args;
        args.append("notification");
        args.append(genericName);
        showDDEControlCenterPage.setArguments(args);

        QDBusMessage replyMsg = QDBusConnection::sessionBus().call(showDDEControlCenterPage);

        if(replyMsg.type() == QDBusMessage::ErrorMessage) {
            qWarning() << __FUNCTION__ << __LINE__ << ", dde control center dbus method call fail , error name :"
                       << replyMsg.errorName() << " , error msg :" << replyMsg.errorMessage();
        }

    } else {
        // 跳转到设置页
        // qdbus com.deepin.dde.ControlCenter /com/deepin/dde/ControlCenter com.deepin.dde.ControlCenter.ShowModule notification
        QDBusMessage showDDEControlCenter = QDBusMessage::createMethodCall("com.deepin.dde.ControlCenter",
                                                        "/com/deepin/dde/ControlCenter",
                                                        "com.deepin.dde.ControlCenter",
                                                        "ShowModule");
        QList<QVariant> args;
        args << "notification";
        showDDEControlCenter.setArguments(args);

        QDBusMessage replyMsg = QDBusConnection::sessionBus().call(showDDEControlCenter);

        if(replyMsg.type() == QDBusMessage::ErrorMessage) {
            qWarning() << __FUNCTION__ << __LINE__ << ", dde control center dbus method call fail , error name :"
                       << replyMsg.errorName() << " , error msg :" << replyMsg.errorMessage();
        }
    }
}

void SystemProtectionSetting::onSettingItemChanged(const QString &key, const QVariant &value)
{
    qDebug() << __FUNCTION__ << __LINE__ << ", key:" << key << ", value:" << value;
    // 使用QProcess执行dbus操作， 此操作是为了通知daemon进程，监测功能设置数据更改
    // 在执行 DSettingsDialog "恢复默认"按钮操作时，QDBusInterFace QDBusConnection 等操作会发生调用错误
    // 错误提示 org.freedesktop.DBus.Error.UnknownMethod
    // 采用QProcess执行dbus操作，不会发生此项报错，dbus调用正常

    QString cmd("qdbus com.deepin.SystemMonitor.Daemon /com/deepin/SystemMonitor com.deepin.SystemMonitor.Daemon.");
    bool needCall = false;

    // 拼接dbus调用命令字串
    if(key == AlarmStatusOptionName) {
        cmd.append("setSystemProtectionStatus "); // Method Name
        cmd.append(value.toString());             // value
        needCall = true;
    } else if (key == AlarmCpuUsageOptionName) {
        cmd.append("setAlarmUsageOfCpu "); // Method Name
        cmd.append(value.toString());      // value
        needCall = true;
    } else if (key == AlarmMemUsageOptionName) {
        cmd.append("setAlarmUsageOfMemory "); // Method Name
        cmd.append(value.toString());         // value
        needCall = true;
    } else if (key == AlarmIntervalOptionName) {
        cmd.append("setAlarmMsgInterval "); // Method Name
        cmd.append(value.toString());       // value
        needCall = true;
    }

    if(needCall)  {
        qDebug() << __FUNCTION__ << __LINE__ << "，dbus cmd：" << cmd;
        QProcess::startDetached(cmd);
    }
}

DSettings *SystemProtectionSetting::getDSettingPointor()
{
    return mDsettings;
}

void SystemProtectionSetting::onUpdateNewBackend()
{
    // 创建新的数据后端，应对可能的设置数据变化
    QString strConfigPath = QString("%1/%2/%3/protection.conf")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
                            .arg(qApp->organizationName())
                            .arg(qApp->applicationName());

    Dtk::Core::QSettingBackend *newBackend = new QSettingBackend(strConfigPath);
    // 为DSetting更新数据后端
    mDsettings->setBackend(newBackend);
    mDsettings->sync();

    // 销毁原来的数据后端
    if(mBackend != nullptr) {
        mBackend->deleteLater();
    }

    // 记录新的数据后端指针
    mBackend = newBackend;
}
