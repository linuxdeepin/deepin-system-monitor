/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
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

#include "settings.h"

#include <QStandardPaths>
#include <QApplication>
#include <QSettings>

Q_GLOBAL_STATIC(Settings, theInstance)
Settings::Settings()
{
    const QString &filepath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + qApp->organizationName() + "/" + qApp->applicationName() + "/" + "config.conf";
    m_settings = new QSettings(filepath, QSettings::IniFormat);
}

Settings::~Settings()
{
    m_settings->deleteLater();
}

Settings *Settings::instance()
{
    return theInstance();
}

void Settings::flush()
{
    m_settings->sync();
}

QVariant Settings::getOption(const QString &key, const QVariant &defaultValue)
{
    m_settings->beginGroup("settings");
    const QVariant &result = m_settings->value(key, defaultValue);
    m_settings->endGroup();
    return result;
}

void Settings::setOption(const QString &key, const QVariant &value)
{
    m_settings->beginGroup("settings");
    m_settings->setValue(key, value);
    m_settings->endGroup();
}
