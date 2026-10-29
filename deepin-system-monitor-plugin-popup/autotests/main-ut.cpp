// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// dsm-popup-ut 测试入口:popup 插件 GUI 单测(独立于主程序测试可执行)

#include <gtest/gtest.h>
#include <QApplication>
#include <unistd.h>

#include "application.h"
#include "ut_popup_stub.h"

// 部分 gcc 版本的 gcov.h 未包 extern "C",直接声明避免 C++ 名字修饰
extern "C" void __gcov_dump(void);

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    fprintf(stderr, "[ut] starting private session bus...\n");
    ut_popup_stub::startPrivateSessionBus();
    fprintf(stderr, "[ut] starting mock display service...\n");
    ut_popup_stub::startMockService();
    fprintf(stderr, "[ut] creating Application...\n");
    Application app(argc, argv);

    // 外部依赖统一打桩(DBus/合成器/键盘区域注册/detached 进程/监控线程启动事件)
    fprintf(stderr, "[ut] installing stubs...\n");
    ut_popup_stub::installAll();

    fprintf(stderr, "[ut] init gtest...\n");
    ::testing::InitGoogleTest(&argc, argv);

    fprintf(stderr, "[ut] running tests...\n");
    auto c = RUN_ALL_TESTS();

    // 结束 Mock 服务进程与私有总线
    ut_popup_stub::stopMockService();

    // 退出析构竞态规避(与主程序测试一致):显式导出覆盖数据后直接终止进程,
    // 跳过 Qt/静态析构链,避免 gcda 丢失
    __gcov_dump();
    _exit(c);
}
