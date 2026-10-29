// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// deepin-system-monitor-server 单元测试入口（qt-autotest-generator 生成）
// 非GUI服务进程：仅需 QCoreApplication；屏蔽会话总线真实注册，保证 DBusServer
// 构造/通知路径在无 DBus 环境下确定可复现（QProcess 外部进程一律打桩）。

#include <QCoreApplication>
#include <gtest/gtest.h>

#include "stubext.h"

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    return c;
}
