// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// deepin-system-monitor-dde-shell-plugin 单元测试入口
// 使用 QApplication + offscreen 平台，与插件运行环境（dde-shell/dock 内
// QApplication）一致，避免 GUI 环境依赖；卡片 QML（cardWindow）依赖
// QtQuick/Qml 与 DTK QML 模块，offscreen 下可正常加载。

#include <QApplication>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setApplicationName("deepin-system-monitor-dde-shell-plugin-test");
    app.setApplicationVersion("1.0");

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    return c;
}
