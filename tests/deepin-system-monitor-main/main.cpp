// Copyright (C) 2011 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// test_main.cpp 测试入口

#include <QApplication>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <application.h>
#include <main_window.h>

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

#include <QTest>
#include <unistd.h>

// 部分 gcc 版本的 gcov.h 未包 extern "C"，直接声明避免 C++ 名字修饰
extern "C" void __gcov_dump(void);

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    Application app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    // 退出析构竞态规避：多夹具（MainWindow/泄漏的页面部件/DTK 单例）共存时，
    // ~QApplication 的 post-routine 与静态对象析构链会触碰悬空连接导致段错误，
    // gcov 退出钩子不执行、全部 gcda 丢失。这里显式导出覆盖数据后直接终止进程，
    // 跳过 Qt/静态析构（测试断言已全部完成，行为等价于正常退出）。
    __gcov_dump();
    _exit(c);
}
