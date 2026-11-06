// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <QStandardPaths>

#include "helper.hpp"

namespace util {

#define MAX_BACKTRACE_FRAMES 128
#define BUF_LEN 256

// Precomputed values for async-signal-safe access in signal handler
static char g_cachePath[BUF_LEN] = {0};
static bool g_isOldVersion = false;

/**
 * @brief Print stack trace when specified signal caught
 * @param signum Signal that caused crash
 */
static inline void printStacktrace(int signum)
{
    // Build timestamp using async-signal-safe functions only
    char ts[BUF_LEN] = {};
    time_t tm = time(nullptr);
    struct tm tm_result;
    gmtime_r(&tm, &tm_result);
    snprintf(ts, BUF_LEN, "%04d-%02d-%02dT%02d:%02d:%02d",
             tm_result.tm_year + 1900, tm_result.tm_mon + 1, tm_result.tm_mday,
             tm_result.tm_hour, tm_result.tm_min, tm_result.tm_sec);

    // Build log file path from precomputed cache path
    char logPath[BUF_LEN * 2] = {};
    snprintf(logPath, sizeof(logPath), "%sdeepin-system-monitor.%s.crash",
             g_cachePath, ts);

    // Open log file using async-signal-safe open()
    int fd = open(logPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // Get backtrace and write directly to fd (async-signal-safe, no malloc)
    void *stack[MAX_BACKTRACE_FRAMES];
    int size = backtrace(stack, MAX_BACKTRACE_FRAMES);

    if (size > 0) {
        if (fd >= 0) {
            // best-effort write; in signal handler context we cannot safely retry on partial write
            const char header[] = "Stack trace:\n";
            ssize_t wr = write(fd, header, sizeof(header) - 1);
            (void)wr;
            backtrace_symbols_fd(stack, size, fd);
        }
        // Also write to stderr for console visibility
        backtrace_symbols_fd(stack, size, STDERR_FILENO);
    }

    if (fd >= 0) {
        close(fd);
    }

    // reset signal handler to default
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;
    sigaction(signum, &act, nullptr);
    pid_t pid = fork();
    if (pid == 0) {
        // 先睡眠 1 秒，等待旧应用退出。注意睡眠时间不足则会启动失败
        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        nanosleep(&ts, nullptr);

        // 通过命令行调用 D-Bus 重新启动系统监视器
        // 使用 execlp 而非 Qt D-Bus API，避免 fork 后复用父进程总线连接导致死锁
        if (g_isOldVersion) {
            // 旧系统：请求 com.deepin.SessionManager 启动应用
            execlp("/usr/bin/dbus-send", "dbus-send",
                   "--session",
                   "--print-reply",
                   "--dest=com.deepin.SessionManager",
                   "/com/deepin/StartManager",
                   "com.deepin.StartManager.Launch",
                   "string:/usr/share/applications/deepin-system-monitor.desktop",
                   (char *)nullptr);
        } else {
            // 新系统：请求 org.desktopspec.ApplicationManager1 启动应用
            execlp("/usr/bin/gdbus", "gdbus",
                   "call",
                   "--session",
                   "--dest", "org.desktopspec.ApplicationManager1",
                   "--object-path", "/org/desktopspec/ApplicationManager1/deepin_2dsystem_2dmonitor",
                   "--method", "org.desktopspec.ApplicationManager1.Application.Launch",
                   "",
                   "@as []",
                   "@a{sv} {}",
                   (char *)nullptr);
        }
        _exit(1);
    }

    // raise origin signal after fork to ensure restart code is reachable
    raise(signum);
}

/**
 * @brief Install crash handler for application ifself
 */
void installCrashHandler()
{
    // Precompute cache path for async-signal-safe access in signal handler
    auto cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cachePath.endsWith("/")) cachePath.append("/");
    snprintf(g_cachePath, BUF_LEN, "%s", cachePath.toStdString().c_str());

    // Precompute version info for async-signal-safe access in signal handler
    g_isOldVersion = common::systemInfo().isOldVersion();

    // action to take when signal get caught
    struct sigaction act {
    };
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = printStacktrace;

    // Invalid memory reference
    sigaction(SIGSEGV, &act, nullptr);
    // Floating-point exception
    sigaction(SIGFPE, &act, nullptr);
    // Illegal Instruction
    sigaction(SIGILL, &act, nullptr);
    // Abort signal from abort call
    sigaction(SIGABRT, &act, nullptr);
}

}   // namespace util

#endif   // STACK_TRACE_H
