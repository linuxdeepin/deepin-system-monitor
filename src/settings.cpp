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

#include "gui/ui_common.h"
#include "constant.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

std::atomic<Settings *> Settings::m_instance;
std::mutex Settings::m_mutex;

Settings::Settings(QObject *parent)
    : QObject(parent)
{
    m_settings = new QSettings(QDir(configPath()).filePath("config.conf"), QSettings::IniFormat);
    m_groupName = "settings";
}

Settings::~Settings()
{
    m_settings->sync();
    m_settings->deleteLater();
}

QString Settings::configPath()
{
    return QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first())
                .filePath(qApp->organizationName()))
           .filePath(qApp->applicationName());
}

QVariant Settings::getOption(const QString &key)
{
    m_settings->beginGroup(m_groupName);
    QVariant result;
    if (m_settings->contains(key)) {
        result = m_settings->value(key);
    } else {
        result = QVariant();
    }
    m_settings->endGroup();

    return result;
}

void Settings::init()
{
    if (getOption("process_tab_index").isNull()) {
        setOption("process_tab_index", 0);
    }

    if (getOption("display_mode").isNull()) {
        setOption("display_mode", kDisplayModeCompact);
    }

    if (getOption("window_width").isNull()) {
        setOption("window_width", Constant::WINDOW_MIN_WIDTH);
    }

    if (getOption("window_height").isNull()) {
        setOption("window_height", Constant::WINDOW_MIN_HEIGHT);
    }
}

void Settings::setOption(const QString &key, const QVariant &value)
{
    m_settings->beginGroup(m_groupName);
    m_settings->setValue(key, value);
    m_settings->endGroup();
}
