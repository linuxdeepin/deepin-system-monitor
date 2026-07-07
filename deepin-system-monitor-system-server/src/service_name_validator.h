// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 服务名校验与存在性匹配的纯函数集合。
// header-only：system-server 与 tests 单测共用同一份实现，避免逻辑重复与漂移。

#ifndef SERVICE_NAME_VALIDATOR_H
#define SERVICE_NAME_VALIDATOR_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <climits>      // SHRT_MAX

namespace ServiceNameValidatorPrivate {

inline bool isHexDigit(ushort ch)
{
    return (ch >= '0' && ch <= '9')
            || (ch >= 'a' && ch <= 'f')
            || (ch >= 'A' && ch <= 'F');
}

inline ushort hexValue(ushort ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch - 'A' + 10;
}

inline bool isAllowedUnitNameLiteral(ushort ch)
{
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return true;
    }
    if (ch >= 'a' && ch <= 'z') {
        return true;
    }

    switch (ch) {
    case '_':
    case ':':
    case '.':
    case '-':
    case '@':
        return true;
    default:
        return false;
    }
}

inline bool isFieldSeparator(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\v' || ch == '\f';
}

} // namespace ServiceNameValidatorPrivate

/**
 * @brief 校验传入的 systemctl 服务名是否合法。
 *
 * 该接口只接收 systemd service unit 的完整名称。这里按 unit name 的
 * 参数语义做显式检查：拒绝空串、过长参数、路径分隔符、空白/控制字符
 * 和未转义的特殊字符；仅允许 systemd 的 C 风格字节转义（如 \x20）。
 */
inline bool isValidServiceName(const QString &name)
{
    if (name.isEmpty() || name.size() > SHRT_MAX) {
        return false;
    }

    static const QString kServiceSuffix(".service");
    if (!name.endsWith(kServiceSuffix)) {
        return false;
    }

    const QString prefix = name.left(name.size() - kServiceSuffix.size());
    if (prefix.isEmpty() || prefix.startsWith('.')) {
        return false;
    }

    for (int i = 0; i < name.size(); ++i) {
        const ushort ch = name.at(i).unicode();
        if (ch == '/') {
            return false;
        }

        if (ch == '\\') {
            if (i + 3 >= name.size()
                    || name.at(i + 1).unicode() != 'x'
                    || !ServiceNameValidatorPrivate::isHexDigit(name.at(i + 2).unicode())
                    || !ServiceNameValidatorPrivate::isHexDigit(name.at(i + 3).unicode())) {
                return false;
            }
            const ushort escaped = (ServiceNameValidatorPrivate::hexValue(name.at(i + 2).unicode()) << 4)
                    | ServiceNameValidatorPrivate::hexValue(name.at(i + 3).unicode());
            if (escaped == 0 || escaped == '/') {
                return false;
            }
            i += 3;
            continue;
        }

        if (!ServiceNameValidatorPrivate::isAllowedUnitNameLiteral(ch)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 在 `systemctl list-unit-files` 的输出中判断服务是否存在。
 *
 * 按行读取 `list-unit-files` 输出，用空白分隔第一列后做精确比较。
 * 这样既不受普通空格、tab 或多空格排版影响，也不会把子串误判为存在。
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
        int fieldEnd = 0;
        while (fieldEnd < line.size() && !ServiceNameValidatorPrivate::isFieldSeparator(line.at(fieldEnd))) {
            ++fieldEnd;
        }

        const QByteArray first = line.left(fieldEnd);
        if (first.endsWith(".service") && first == target) {
            return true;
        }
    }
    return false;
}

#endif // SERVICE_NAME_VALIDATOR_H
