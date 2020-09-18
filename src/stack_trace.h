/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include <memory>
#include <vector>
#include <fstream>
#include <locale>
#include <iomanip>

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

namespace util {

#define MAX_BACKTRACE_FRAMES 128
#define BUF_LEN 256

struct Free {
    void operator()(char **p)
    {
        free(p);
    }
};
using BacktraceSymbols = std::unique_ptr<char *, Free>;

static inline void printBacktrace(int signum)
{
    char ts[BUF_LEN] {};
    std::time_t tm = std::time(nullptr);
    auto tslen = std::strftime(ts, BUF_LEN, "%FT%T", std::localtime(&tm));
    ts[tslen] = 0;
    char logstr[BUF_LEN] {};
    auto len = snprintf(logstr, BUF_LEN, "deepin-system-monitor.%s.crash", ts);
    logstr[len] = 0;

    std::string logN {"/tmp/"};
    logN.append(logstr);
    std::ofstream log(logN, std::ios::out);

    std::vector<void *> stack(MAX_BACKTRACE_FRAMES);
    auto size = backtrace(stack.data(), int(stack.size()));
    BacktraceSymbols symbols(backtrace_symbols(stack.data(), size));

    if (symbols && size > 0) {
        log << "Stack trace:" << std::endl;

        for (int i = 0; i < size; i++) {
            log << '\t' << "#" << std::left << std::setw(4) << i << symbols.get()[i] << std::endl;
        }
    }

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;
    sigaction(signum, &act, nullptr);
    raise(signum);
}

void installCrashHandler()
{
    struct sigaction act {
    };
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = printBacktrace;

    sigaction(SIGSEGV, &act, nullptr);
    sigaction(SIGFPE, &act, nullptr);
    sigaction(SIGILL, &act, nullptr);
    sigaction(SIGABRT, &act, nullptr);
}

} // namespace util

#endif // STACK_TRACE_H
