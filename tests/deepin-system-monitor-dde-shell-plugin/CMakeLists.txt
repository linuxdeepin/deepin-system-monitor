# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# qt-autotest-generator (V-7756): deepin-system-monitor-dde-shell-plugin 单元测试模块
#
# 说明：dde-shell 卡片插件（SystemMonitorPlugin/SystemMonitorController/
# SystemMonitorProber）与主程序、dock 插件均存在无命名空间类且依赖 dde-dock
# V3 插件接口（pluginsiteminterface_v3.h），无法并入其它测试目标，按模块独立
# 建测试二进制；由 tests/CMakeLists.txt 末尾追加的 add_subdirectory 挂接
# （DOTEST=ON 时生效）。
# 打桩策略：/proc 读取经 POSIX ::open 前缀重定向沙箱（procsandbox.h）注入内容，
# 不依赖真实 /proc 数值；DBus 激活与 QProcess::startDetached 均以 stub-ext 打桩。

cmake_minimum_required(VERSION 3.10)

set(DDE_SHELL_PLUGIN_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-dde-shell-plugin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DdeDock REQUIRED dde-dock)

# 上层 tests/CMakeLists.txt 未导入 Qml/Quick/Test 组件，本模块自导入
# （卡片 QML 经 QQuickView 加载，QSignalSpy/QTest 需要 Test 组件）
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED)

file(GLOB DDE_SHELL_PLUGIN_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(test_deepin-system-monitor-dde-shell-plugin
    ${DDE_SHELL_PLUGIN_TEST_SOURCES}
    main.cpp
    3rdparty/stub/stub-shadow.cpp
    # ---- 被测插件源码 ----
    ${DDE_SHELL_PLUGIN_DIR}/systemmonitorplugin.cpp
    ${DDE_SHELL_PLUGIN_DIR}/systemmonitorcontroller.cpp
    ${DDE_SHELL_PLUGIN_DIR}/systemmonitorprober.cpp
    # ---- 卡片 QML 资源（cardWindow 的 qrc:/systemmonitor/SystemMonitorCard.qml） ----
    ${DDE_SHELL_PLUGIN_DIR}/resources/systemmonitor.qrc
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
)

target_include_directories(test_deepin-system-monitor-dde-shell-plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${CMAKE_SOURCE_DIR}
    ${DDE_SHELL_PLUGIN_DIR}
    ${DdeDock_INCLUDE_DIRS}
)

target_link_libraries(test_deepin-system-monitor-dde-shell-plugin PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::Widgets
    ${QT_NS}::Gui
    ${QT_NS}::Qml
    ${QT_NS}::Quick
    ${QT_NS}::DBus
    ${QT_NS}::Network
    ${QT_NS}::Test
    ${DTK_NS}::Core
    ${DTK_NS}::Gui
    Threads::Threads
    GTest::GTest
    GTest::Main
    gmock
    gmock_main
    gcov
)

# qt-autotest-generator: 私有方法（preferredNetworkInterfaces/formatTransferRate/
# refreshSystemStats 等）无公开入口，仅能用 -fno-access-control 直调
# （deepin-mail 等项目同款方案；仅影响编译期访问检查，不改变代码生成）
target_compile_options(test_deepin-system-monitor-dde-shell-plugin PRIVATE -fno-access-control)

message(STATUS "UT: test_deepin-system-monitor-dde-shell-plugin configured with ${DDE_SHELL_PLUGIN_TEST_SOURCES}")
