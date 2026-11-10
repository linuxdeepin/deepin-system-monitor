// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator: /proc 沙箱夹具（test_systemmonitorprober.cpp /
// test_systemmonitorplugin.cpp 共用）
//
// 原理：SystemMonitorProber 的读取链路全部走 QFile（QFSFileEngine →
// qt_safe_open → POSIX ::open）。本沙箱把 /proc/stat、/proc/meminfo、
// /proc/net/dev 三个路径重定向到 QTemporaryDir 内的真实临时文件，读到的
// 内容由用例完全控制（等价类/边界值注入），其余路径一律 raw syscall 直穿
// （不经过 inline-hook，天然无递归、无副作用）。QFile 的 open/readLine 均
// 为真实调用，仅路径被重定向，保真度最高。
//
// - ::open → 匹配注入路径: syscall(SYS_openat, 临时文件)；否则 raw 直穿
// - 失败注入：把路径重定向到必然不存在的临时文件（ENOENT → QFile::open false）
// - read/fgets/readLine/close 一律真实调用，作用于沙箱返回的真实 fd，无需打桩

#pragma once

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QTemporaryDir>

#include <stubext.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

class ProcSandbox
{
public:
    explicit ProcSandbox(stub_ext::StubExt &stub)
    {
        s_active = this;
        // ::open 是变参函数，stub-ext 的 set_lamda 无法为变参生成 shadow；
        // 改用基类 Stub::set（无类型地址补丁）+ 静态变参转发函数
        stub.set(::open, openHook);
    }

    ~ProcSandbox() { s_active = nullptr; }

    ProcSandbox(const ProcSandbox &) = delete;
    ProcSandbox &operator=(const ProcSandbox &) = delete;

    // 把 procPath（如 "/proc/stat"）重定向到写入 content 的临时文件
    void inject(const QString &procPath, const QByteArray &content)
    {
        const QString target = m_dir.filePath(QStringLiteral("proc_%1").arg(m_map.size()));
        QFile file(target);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_map.insert(procPath, QString());
            return;
        }
        file.write(content);
        file.close();
        m_map.insert(procPath, target);
    }

    // 把 procPath 重定向到必然不存在的临时文件，模拟打开失败（ENOENT）
    void injectFailure(const QString &procPath)
    {
        m_map.insert(procPath, m_dir.filePath(QStringLiteral("missing_%1").arg(m_map.size())));
    }

private:
    static int rawOpen(const char *path, int flags, int mode)
    {
        return static_cast<int>(syscall(SYS_openat, AT_FDCWD, path, flags, mode));
    }

    static int openHook(const char *path, int flags, ...)
    {
        int mode0 = 0600;
        if (flags & (O_CREAT | O_TMPFILE)) {
            va_list ap;
            va_start(ap, flags);
            mode0 = va_arg(ap, int);
            va_end(ap);
        }
        const char *realPath = path;
        std::string redirected;
        if (s_active) {
            const QString target = s_active->m_map.value(QString::fromUtf8(path));
            if (!target.isEmpty()) {
                redirected = target.toStdString();
                realPath = redirected.c_str();
            }
        }
        return rawOpen(realPath, flags, mode0);
    }

    QTemporaryDir m_dir;
    QHash<QString, QString> m_map;   // /proc/stat → 临时文件路径

    inline static ProcSandbox *s_active = nullptr;
};
