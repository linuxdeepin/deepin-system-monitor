// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator: /proc 沙箱夹具（test_process.cpp / test_process_db.cpp 共用）
//
// 原理：Process/ProcessDB 的读取链路全部走 "/proc/<pid>/<file>" 路径的 POSIX
// open/fopen/stat/opendir。本沙箱把伪造 PID（utFakePid）的前缀重定向到
// QTemporaryDir 内的真实临时文件，读到的内容由用例完全控制（等价类/边界值注入），
// 其余路径一律用 raw syscall 直穿（不经过 inline-hook，天然无递归、无副作用）。
//
// - open    → 匹配前缀: syscall(SYS_openat, 临时文件)；否则 raw 直穿
// - fopen   → 匹配前缀: fdopen(syscall(SYS_openat))；否则 raw 直穿
// - stat    → 匹配前缀: syscall(SYS_newfstatat, 临时路径)；否则 raw 直穿
// - opendir → 仅在 armOpendir() 后生效：返回预先真实打开的临时 fd 目录 DIR*，
//             或注入失败（nullptr）模拟 /proc/<pid>/fd 不存在
// - read/fgets/fread/close/fclose/readdir/closedir 一律真实调用，作用于沙箱
//   返回的真实 fd/FILE*/DIR*，无需打桩

#pragma once

#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>
#include <QDir>
#include <QString>

#include <stubext.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

// 伪造 PID：固定常量，避免与真实进程冲突（沙箱只拦截该前缀）
inline pid_t utFakePid()
{
    return 424242;
}

class ProcFsSandbox
{
public:
    explicit ProcFsSandbox(stub_ext::StubExt &stub)
        : m_stub(stub)
    {
        QDir(m_dir.path()).mkpath(QStringLiteral("fd"));
        // ::open 是变参函数，stub-ext 的 set_lamda 无法为变参生成 shadow；
        // 改用基类 Stub::set（无类型地址补丁）+ 静态变参转发函数
        s_active = this;
        m_stub.set(::open, openHook);
        m_stub.set_lamda(::fopen, [this](const char *path, const char *mode) -> FILE * {
            QString rel;
            int flags = O_RDONLY;
            int mode0 = 0;
            if (mode && strchr(mode, 'w')) {
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                mode0 = 0666;
            } else if (mode && strchr(mode, 'a')) {
                flags = O_WRONLY | O_CREAT | O_APPEND;
                mode0 = 0666;
            }
            std::string redirected;
            const char *realPath = path;
            if (match(path, rel)) {
                redirected = m_dir.filePath(rel).toStdString();
                realPath = redirected.c_str();
            }
            int fd = rawOpen(realPath, flags, mode0);
            if (fd < 0)
                return nullptr;
            return ::fdopen(fd, mode ? mode : "r");
        });
        m_stub.set_lamda(::stat, [this](const char *path, struct stat *buf) -> int {
            std::string redirected;
            const char *realPath = path;
            QString rel;
            if (match(path, rel)) {
                redirected = m_dir.filePath(rel).toStdString();
                realPath = redirected.c_str();
            }
            // stat 的内核原语，绕过本桩自身，无递归
            return int(syscall(SYS_newfstatat, AT_FDCWD, realPath, buf, 0));
        });
    }

    ~ProcFsSandbox()
    {
        disarmOpendir();
        m_stub.reset(::stat);
        m_stub.reset(::fopen);
        m_stub.reset(::open);
        s_active = nullptr;
    }

    ProcFsSandbox(const ProcFsSandbox &) = delete;
    ProcFsSandbox &operator=(const ProcFsSandbox &) = delete;

    // 写入一个伪 /proc 文件的内容；name ∈ {stat,status,statm,cmdline,environ,io,schedstat}
    void setFile(const QString &name, const QByteArray &content)
    {
        QFile f(m_dir.filePath(name));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(content);
            f.close();
        }
    }

    // 删除伪文件 → 后续 open 得到 ENOENT（open 失败分支）
    void removeFile(const QString &name)
    {
        QFile::remove(m_dir.filePath(name));
    }

    // 在伪 fd 目录里创建真实 socket 文件（stat 命中 S_ISSOCK + 唯一 inode）
    bool addSocket(const QString &name)
    {
        const std::string path = m_dir.filePath(QStringLiteral("fd/") + name).toStdString();
        int s = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (s < 0)
            return false;
        struct sockaddr_un addr {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        bool ok = ::bind(s, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0;
        ::close(s);
        return ok;
    }

    // 在伪 fd 目录里创建符号链接（target 可不存在 → stat 失败分支）
    bool addSymlink(const QString &name, const QString &target)
    {
        const std::string link = m_dir.filePath(QStringLiteral("fd/") + name).toStdString();
        const std::string dst = target.toStdString();
        return ::symlink(dst.c_str(), link.c_str()) == 0;
    }

    // 伪 fd 目录内文件的真实沙箱路径（供用例构造符号链接目标）
    QString sockPath(const QString &name) const
    {
        return m_dir.filePath(QStringLiteral("fd/") + name);
    }

    // 预备伪 fd 目录的 DIR*，并挂上 opendir 拦截；fail=true 时 opendir 返回
    // nullptr（模拟 /proc/<pid>/fd 打开失败分支）
    bool armOpendir(bool fail = false)
    {
        if (!fail) {
            const std::string fdpath = m_dir.filePath(QStringLiteral("fd")).toStdString();
            m_fdDir = ::opendir(fdpath.c_str());
            if (!m_fdDir)
                return false;
        }
        m_opendirFail = fail;
        m_stub.set_lamda(::opendir, [this](const char *path) -> DIR * {
            QString rel;
            if (match(path, rel) && rel == QStringLiteral("fd")) {
                if (m_opendirFail)
                    return nullptr;
                // 所有权移交：被测方法持有 uDir 并 closedir，沙箱不再重复释放
                DIR *d = m_fdDir;
                m_fdDir = nullptr;
                return d;   // 预先真实打开的临时目录
            }
            return nullptr;   // 沙箱窗口内不应有其他 opendir 调用
        });
        m_opendirArmed = true;
        return true;
    }

    void disarmOpendir()
    {
        if (m_opendirArmed) {
            m_stub.reset(::opendir);
            m_opendirArmed = false;
        }
        if (m_fdDir) {
            ::closedir(m_fdDir);
            m_fdDir = nullptr;
        }
    }

private:
    static bool match(const char *path, QString &out)
    {
        if (!path)
            return false;
        const QString p = QString::fromLocal8Bit(path);
        const QString prefix = QStringLiteral("/proc/%1/").arg(utFakePid());
        if (!p.startsWith(prefix))
            return false;
        out = p.mid(prefix.size());
        // 防目录穿越；仅允许单段文件名或 "fd/<name>"
        if (out.isEmpty() || out.contains(QStringLiteral("..")))
            return false;
        return true;
    }

    static int rawOpen(const char *path, int flags, int mode)
    {
        return int(syscall(SYS_openat, AT_FDCWD, path, flags, mode));
    }

    // ::open 的替换实现（变参）：前缀匹配重定向到临时文件，否则 raw 直穿
    static int openHook(const char *path, int flags, ...)
    {
        int mode = 0;
        if (flags & (O_CREAT | O_TMPFILE)) {
            va_list ap;
            va_start(ap, flags);
            mode = va_arg(ap, int);
            va_end(ap);
        }
        QString rel;
        if (s_active && match(path, rel))
            return rawOpen(s_active->m_dir.filePath(rel).toStdString().c_str(),
                           flags, mode);
        return rawOpen(path, flags, mode);
    }

private:
    inline static ProcFsSandbox *s_active = nullptr;
    stub_ext::StubExt &m_stub;
    QTemporaryDir m_dir;
    DIR *m_fdDir = nullptr;
    bool m_opendirArmed = false;
    bool m_opendirFail = false;
};
