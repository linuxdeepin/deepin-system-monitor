// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 服务名校验与存在性匹配的纯函数集合。
// header-only：system-server 与 tests 单测共用同一份实现，避免逻辑重复与漂移。

#ifndef SERVICE_NAME_VALIDATOR_H
#define SERVICE_NAME_VALIDATOR_H

#include <QByteArray>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <climits>      // SHRT_MAX

/**
 * @brief 校验传入的 systemctl 服务名是否合法。
 *
 * 采用白名单正则锚定整串匹配，统一拦截空串、超长串、控制字符
 * （制表符/换行等）以及缺少 `.service` 后缀等非法输入，
 * 取代原先只拦截 ';' 与空格的黑名单 contains 判断。
 *
 * 字符集覆盖 systemd service 单元名合法字符（含模板单元 `getty@.service`
 * 及实例 `getty@tty1.service`），并强制 `.service` 后缀——与调用方
 * `ServiceManager` 传入的 systemd `Unit.Id`（完整单元名）语义一致。
 */
inline bool isValidServiceName(const QString &name)
{
    if (name.isEmpty() || name.size() > SHRT_MAX) {
        return false;
    }
    // ^/$ 锚定整串，避免退化为子串匹配；\.service 强制后缀
    static const QRegularExpression kServiceNameRe("^[A-Za-z0-9_.:@-]+\\.service$");
    return kServiceNameRe.match(name).hasMatch();
}

/**
 * @brief 在 `systemctl list-unit-files` 的输出中判断服务是否存在。
 *
 * 按行解析，逐行取第一列（完整服务名）与 @p name 做精确相等比较，
 * 取代原先对整段输出做字节级子串匹配的 contains 判断，避免把某真实
 * 服务名的子串（如传入 "dbus" 命中 "dbus.service"）误判为存在。
 *
 * 头行（"UNIT FILE …"）与尾行（"N unit files listed."）因第一列
 * 与目标名不等而自然被跳过。
 */
inline bool serviceExistsInList(const QString &name, const QByteArray &listOutput)
{
    const QByteArray target = name.toUtf8();
    const QList<QByteArray> lines = listOutput.split('\n');
    for (const QByteArray &rawLine : lines) {
        const QByteArray line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        // list-unit-files 输出以空白分隔；第一列为完整服务名。
        // 仅当该列以 .service 结尾才视为候选，使头行（"UNIT FILE …"）
        // 与尾行（"N unit files listed."）被天然跳过，函数可独立成立。
        const QByteArray first = line.split(' ').first();
        if (first.endsWith(".service") && first == target) {
            return true;
        }
    }
    return false;
}

#endif // SERVICE_NAME_VALIDATOR_H
