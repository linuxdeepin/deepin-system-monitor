// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// daemon 单元测试入口（qt-autotest-generator 生成）
// daemon 为非 GUI 模块（QObject/QTimer/QDBus），使用 QCoreApplication；
// 所有可写路径经 QStandardPaths::setTestModeEnabled 重定向到构建目录 .qttest，
// 测试夹具再用 stub 将 ConfigLocation 指向各自的 QTemporaryDir 实现用例间隔离。

#include <QCoreApplication>
#include <QStandardPaths>
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 环境隔离：可写位置重定向到 <binary_dir>/.qttest
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("deepin-system-monitor-ut");
    QCoreApplication::setApplicationName("deepin-system-monitor-daemon-ut");

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();

    // 冲刷 deferred delete（MLogger/DConfig、SettingHandler/DSettings 与
    // plugin.cpp 的 monitorService 等对象均通过 deleteLater 释放）
    for (int i = 0; i < 20; ++i) {
        app.processEvents();
    }
    return c;
}
