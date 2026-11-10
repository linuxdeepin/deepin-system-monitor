# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# qt-autotest-generator (V-6444): deepin-system-monitor-server 单元测试模块
#
# 说明：server 模块与 deepin-system-monitor-system-server 各自包含同名类 MLogger
# （均无命名空间），不能并入同一个测试二进制（ODR/重复符号冲突），且与主测试
# 目标 deepin-system-monitor-test 隔离，故按模块独立建测试目标；
# 由 tests/CMakeLists.txt 末尾追加的 add_subdirectory 挂接（DOTEST=ON 时生效）。

cmake_minimum_required(VERSION 3.10)

set(SERVER_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-server)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB SERVER_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(test_deepin-system-monitor-server
    ${SERVER_TEST_SOURCES}
    main.cpp
    3rdparty/stub/stub-shadow.cpp
    # ---- 被测 server 源码（与生产目标源码清单一致，不含 main.cpp）----
    ${SERVER_DIR}/src/dbusserver.cpp
    ${SERVER_DIR}/src/logger.cpp
)

target_include_directories(test_deepin-system-monitor-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${CMAKE_SOURCE_DIR}
    ${SERVER_DIR}/src
)

target_link_libraries(test_deepin-system-monitor-server PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::DBus
    ${DTK_NS}::Core
    Threads::Threads
    GTest::GTest
    GTest::Main
    gcov
)

message(STATUS "UT: test_deepin-system-monitor-server configured with ${SERVER_TEST_SOURCES}")
