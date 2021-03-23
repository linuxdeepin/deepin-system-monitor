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
#include "whitelistconfig.h"
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

QList<QString> WhiteListConfig::m_listAppName = {};

WhiteListConfig::WhiteListConfig(QObject *parent) : QObject(parent)
{

}

// 解析JSON字符串 将解析结果存储起来
void WhiteListConfig::initWhiteList()
{
    m_listAppName.clear();
    QString strContent = getFileContent(); //  获取配置文件中的内容
    QJsonParseError json_error;
    QJsonDocument parse_doucment = QJsonDocument::fromJson(strContent.toLocal8Bit(), &json_error); // 将字符串转成JSonDocument
    if (json_error.error == QJsonParseError::NoError) { // 判断是否是合法JSON格式
        if (parse_doucment.isObject()) { //  判断是否是个object
            QJsonObject obj = parse_doucment.object(); //  转成JsonObject
            QJsonValue data = obj.take("APP_NAME");  //  获取APP_NAME的值
            if (data.isArray()) { // 是否是数组
                QJsonArray array = data.toArray();
                for (int i = 0; i < array.size(); ++i) {
                    m_listAppName.append(array[i].toString()); // 取出配置内容并存储
                }

            }
        }
    }
}

bool WhiteListConfig::noShowProcess(const QString &strAppName)
{
    initWhiteList(); // 初始化白名单信息

    return m_listAppName.contains(strAppName);

}

bool WhiteListConfig::isHpidPrcoress(pid_t pid, const QMultiMap<pid_t, pid_t> &ctopMap)
{
    QFile file(HPID_CONFIG_FILE_PATH);

    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly)) { //  判断文件打开是否成功
        qInfo() << "Open File Failed ::" << HPID_CONFIG_FILE_PATH << file.error();
        return false;
    }

    QString pidlstStr = file.readAll().trimmed();
    file.close();

    if (pidlstStr.isEmpty()) {
        qInfo() << "WhiteListConfig::isHpidPrcoress pidlstStr is Empty";
        return false;
    }

    const QStringList &pidList = pidlstStr.split(",");
    if (pidList.isEmpty()) {
        qInfo() << "WhiteListConfig::isHpidPrcoress pidList is Empty " << pidlstStr;
        return false;
    }

    std::function<bool(pid_t hpid, pid_t cpid)> anyRootIsMatchPPid;
    // find if any ancestor processes is gui application
    anyRootIsMatchPPid = [&](pid_t hpid, pid_t cpid) -> bool {
        bool b = false;
        b = (hpid == cpid);
        if (!b && ctopMap.contains(cpid))
        {
            b = anyRootIsMatchPPid(hpid, ctopMap.value(cpid));
        }
        return b;
    };

    for (const QString &hpidstr : pidList) {
        if (anyRootIsMatchPPid(hpidstr.toInt(), pid)) {
            return true;
        }
    }
    return false;
}

//  获取文件内容
QString WhiteListConfig::getFileContent()
{
    QFile file(JSON_CONFIG_FILE_PATH);

    if (!file.exists())
        return "";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { //  判断文件打开是否成功
        qInfo() << "Open File Failed ::" << JSON_CONFIG_FILE_PATH;
        return "";
    }
    QTextStream txtInput(&file);
    QString lineStr;
    while (!txtInput.atEnd()) { // 循环读到文件末尾
        lineStr = txtInput.readAll(); //  保存读取到的文件
    }
    file.close(); // 关闭文件
    return lineStr;
}
