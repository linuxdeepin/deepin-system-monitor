# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# daemon 模块单元测试独立可执行目标 (qt-autotest-generator 生成)
# 与 popup 插件单测同一模式：独立目标避免并入 tests/ 主测试二进制的
# GLOB_RECURSE 收集（daemon 源码不在主测试目标的 APP_CPP 集合内）
# daemon 为非 GUI 模块：QCoreApplication 入口即可

set(DSM_DAEMON_UT_TARGET test_deepin-system-monitor-daemon)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(DAEMON_DIR ${CMAKE_HOME_DIRECTORY}/deepin-system-monitor-daemon)
set(STUB_EXT_DIR ${CMAKE_HOME_DIRECTORY}/tests/3rdparty/stub-ext)

include_directories(
    ${DAEMON_DIR}/src
    ${STUB_EXT_DIR}
)

set(DSM_DAEMON_UT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cpuprofile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_memoryprofile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_settinghandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_systemmonitorservice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_free_functions.cpp
)

# daemon 全部源码（与 daemon/CMakeLists.txt 的 GLOB_RECURSE src/*.cpp 等价）
# + stub-shadow（stub_ext 链接期支撑）+ daemon 资源 qrc（:/resources/settings.json）
add_executable(${DSM_DAEMON_UT_TARGET}
    ${DSM_DAEMON_UT_SOURCES}
    ${DAEMON_DIR}/src/cpuprofile.cpp
    ${DAEMON_DIR}/src/memoryprofile.cpp
    ${DAEMON_DIR}/src/logger.cpp
    ${DAEMON_DIR}/src/settinghandler.cpp
    ${DAEMON_DIR}/src/systemmonitorservice.cpp
    ${DAEMON_DIR}/src/plugin.cpp
    ${STUB_EXT_DIR}/stub-shadow.cpp
    ${DAEMON_DIR}/assets/deepin-system-monitor-daemon.qrc
)

set_target_properties(${DSM_DAEMON_UT_TARGET} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(${DSM_DAEMON_UT_TARGET} PRIVATE
    ${DAEMON_DIR}/src
    ${STUB_EXT_DIR}
)

target_link_libraries(${DSM_DAEMON_UT_TARGET} PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::DBus
    ${DTK_NS}::Core
    Qt${QT_VERSION_MAJOR}::Test
    GTest::GTest
    gmock
    Threads::Threads
    gcov
)
