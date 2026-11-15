// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include <fcntl.h>
#include <string.h>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "helper.hpp"

namespace util {

#define MAX_BACKTRACE_FRAMES 128
#define BUF_LEN 256

// Pre-computed at installCrashHandler() time; safe to read inside the
// signal handler because it is a plain bool set before any signal
// can arrive via the installed handler.
static bool g_isOldVersion = false;

/**
 * @brief Print stack trace when specified signal caught
 * @param signum Signal that caused crash
 *
 * This function runs in a signal-handler context and must only call
 * async-signal-safe functions.  In particular it must NOT use any
 * function that internally calls malloc() (including Qt string APIs,
 * std::ofstream, backtrace_symbols, etc.), because the signal may
 * have been delivered while the malloc arena lock was held, causing
 * a deadlock.
 */
static inline void printStacktrace(int signum)
{
    // timestamp buffer
    char ts[BUF_LEN] {};
    // current timestamp
    time_t tm = time(nullptr);
    // format current local time as standard iso format string
    auto tslen = strftime(ts, BUF_LEN, "%FT%T", localtime(&tm));
    ts[tslen] = 0;
    // log filename buffer
    char logstr[BUF_LEN] {};
    // format log filename
    auto len = snprintf(logstr, BUF_LEN, "deepin-system-monitor.%s.crash", ts);
    logstr[len] = 0;

    // Build the cache directory path using only async-signal-safe
    // calls (getenv).  QStandardPaths::writableLocation is NOT
    // async-signal-safe (it allocates QStrings internally).
    const char *cacheDir = getenv("XDG_CACHE_HOME");
    char dirBuf[BUF_LEN * 2] {};
    if (cacheDir && cacheDir[0]) {
        snprintf(dirBuf, sizeof(dirBuf), "%s/deepin", cacheDir);
    } else {
        const char *home = getenv("HOME");
        if (home && home[0]) {
            snprintf(dirBuf, sizeof(dirBuf), "%s/.cache/deepin", home);
        } else {
            // fallback: current directory
            snprintf(dirBuf, sizeof(dirBuf), ".");
        }
    }

    // Full path to the crash log file
    char logPath[BUF_LEN * 3] {};
    snprintf(logPath, sizeof(logPath), "%s/%s", dirBuf, logstr);

    // Open the log file using open() (async-signal-safe)
    int fd = open(logPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // Write a header
    if (fd >= 0) {
        const char header[] = "Stack trace:\n";
        write(fd, header, strlen(header));
    }

    // Capture backtrace and write symbols directly to the fd using
    // backtrace_symbols_fd() — this avoids the malloc() that
    // backtrace_symbols() performs internally.
    void *stack[MAX_BACKTRACE_FRAMES];
    int size = backtrace(stack, MAX_BACKTRACE_FRAMES);
    if (size > 0 && fd >= 0) {
        backtrace_symbols_fd(stack, size, fd);
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
    // raise origin signal
    raise(signum);

    pid_t pid = fork();
    if (pid == 0) {
        // 先睡眠 1 秒，等待旧应用退出。注意睡眠时间不足则会启动失败
        struct timespec time;
        time.tv_sec = 1;
        time.tv_nsec = 0;
        nanosleep(&time, nullptr);

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
}

/**
 * @brief Install crash handler for application ifself
 */
void installCrashHandler()
{
    // Pre-compute version info before installing the signal handler.
    // common::systemInfo() calls DSysInfo::majorVersion() which is NOT
    // async-signal-safe, so it must not be called from printStacktrace().
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
