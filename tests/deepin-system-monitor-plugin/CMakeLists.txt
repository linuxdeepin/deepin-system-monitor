# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# qt-autotest-generator: deepin-system-monitor-plugin 单元测试模块
#
# 说明：插件模块的 ddlog.h 以不同的 Q_LOGGING_CATEGORY 字符串（org.deepin.system-monitor.plugin）
# 与主程序（org.deepin.system-monitor）内联定义同名分类，且插件源码不在主测试目标
# deepin-system-monitor-test 的源码清单中，无法并入同一二进制，故按模块独立建测试目标；
# 由 tests/CMakeLists.txt 末尾追加的 add_subdirectory 挂接（DOTEST=ON 时生效）。
# API 宏说明：dde-dock 6.x（DOCK_API_VERSION >= 2.0.0）下编译进目标的是
# USE_API_QUICKPANEL20 分支（PluginsItemInterfaceV2），DDE_DOCK_NEW_VERSION 未定义，
# 与插件真实构建（pkg-config dde-dock 版本判据）保持一致。

cmake_minimum_required(VERSION 3.10)

set(PLUGIN_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-plugin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DdeDock REQUIRED dde-dock)

# 上层 tests/CMakeLists.txt 未导入 Svg / Test 组件，本模块自导入
# （monitorpluginbuttonwidget.cpp 用 QSvgRenderer，各测试用 QSignalSpy）
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)

file(GLOB PLUGIN_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(test_deepin-system-monitor-plugin
    ${PLUGIN_TEST_SOURCES}
    main.cpp
    3rdparty/stub/stub-shadow.cpp
    # ---- 被测插件源码（7 个目标文件） ----
    ${PLUGIN_DIR}/gui/monitor_plugin.cpp
    ${PLUGIN_DIR}/gui/monitorpluginbuttonwidget.cpp
    ${PLUGIN_DIR}/gui/systemmonitortipswidget.cpp
    ${PLUGIN_DIR}/gui/commoniconbutton.cpp
    ${PLUGIN_DIR}/gui/quickpanelwidget.cpp
    ${PLUGIN_DIR}/logger.cpp
    ${PLUGIN_DIR}/dbus/dbusinterface.cpp
    # ---- 图标回退资源（paintEvent 内 :/icons/deepin/builtin/... 路径） ----
    ${PLUGIN_DIR}/asserts/deepin-system-monitor-plugin.qrc
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
)

target_include_directories(test_deepin-system-monitor-plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${CMAKE_SOURCE_DIR}
    ${PLUGIN_DIR}
    ${PLUGIN_DIR}/gui
    ${PLUGIN_DIR}/dbus
    ${DdeDock_INCLUDE_DIRS}
)

target_link_libraries(test_deepin-system-monitor-plugin PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::Widgets
    ${QT_NS}::Gui
    ${QT_NS}::DBus
    ${QT_NS}::Svg
    ${QT_NS}::Test
    ${DTK_NS}::Core
    ${DTK_NS}::Gui
    ${DTK_NS}::Widget
    Threads::Threads
    GTest::GTest
    GTest::Main
    gmock
    gmock_main
    gcov
)

message(STATUS "UT: test_deepin-system-monitor-plugin configured with ${PLUGIN_TEST_SOURCES}")
