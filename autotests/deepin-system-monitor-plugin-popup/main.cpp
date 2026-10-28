// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// popup 插件单元测试入口（qt-autotest-generator 生成）
// 使用 popup 的 Application（其构造会 attach SystemMonitorThread/NetifMonitorThread，
// 与真实插件进程启动路径一致）+ offscreen 平台，避免 GUI 环境依赖

#include <DApplication>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    Application app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    return c;
}
