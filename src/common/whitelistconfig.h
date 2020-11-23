/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     tenghuan <tenghuan@uniontech.com>
*
* Maintainer: tenghuan <tenghuan@uniontech.com>
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
#ifndef WHITELISTCONFIG_H
#define WHITELISTCONFIG_H

#define JSON_CONFIG_FILE_PATH "/usr/share/deepin-system-monitor/translations/whitelist.json"

#include <QObject>

class WhiteListConfig : public QObject
{
    Q_OBJECT
public:
    explicit WhiteListConfig(QObject *parent = nullptr);
    static bool noShowProcess(const QString& strAppName);


private:
    static QString getFileContent();
    static void initWhiteList();
private:
    static QList<QString> m_listAppName;

};

#endif // WHITELISTCONFIG_H
