// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// deepin-system-monitor-plugin 单元测试入口（qt-autotest-generator 生成）
// 使用 DApplication + offscreen 平台，与插件运行环境（dock 内 QApplication）一致，
// 避免 GUI 环境依赖；插件源码中的 DGuiApplicationHelper/DConfig/DDBusSender 均依赖 DApplication。

#include <DApplication>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    DApplication app(argc, argv);
    app.setApplicationName("deepin-system-monitor-plugin-test");
    app.setApplicationVersion("1.0");

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    return c;
}
