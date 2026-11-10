// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// deepin-system-monitor-system-server 单元测试入口（qt-autotest-generator 生成）
// 非GUI服务进程：仅需 QCoreApplication；全局屏蔽 libdkapture.so 真实加载，
// 保证 DKaptureManager 行为在无 eBPF 库环境下确定可复现。

#include <QCoreApplication>
#include <QLibrary>
#include <gtest/gtest.h>

#include "stubext.h"

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

// 全局测试环境：在整个测试进程生命周期内屏蔽 QLibrary::load，
// 使 DKaptureManager 构造函数走"库加载失败"分支（与 CI 无 dkapture 环境一致）
namespace {
stub_ext::StubExt g_globalStub;

class SystemServerTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        g_globalStub.set_lamda(ADDR(QLibrary, load), [](QLibrary *) -> bool {
            return false;
        });
    }
};
}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new SystemServerTestEnvironment);

    auto c = RUN_ALL_TESTS();
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
    return c;
}
