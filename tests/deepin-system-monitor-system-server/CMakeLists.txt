# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# qt-autotest-generator (V-6444): deepin-system-monitor-system-server 单元测试模块
#
# 说明：system-server 模块与 deepin-system-monitor-server 各自包含同名类 MLogger
# （均无命名空间），不能并入同一个测试二进制（ODR/重复符号冲突），且与主测试
# 目标 deepin-system-monitor-test 隔离，故按模块独立建测试目标；
# 由 tests/CMakeLists.txt 末尾追加的 add_subdirectory 挂接（DOTEST=ON 时生效）。

cmake_minimum_required(VERSION 3.10)

set(SYS_SERVER_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-system-server)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB SYS_SERVER_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

add_executable(test_deepin-system-monitor-system-server
    ${SYS_SERVER_TEST_SOURCES}
    main.cpp
    3rdparty/stub/stub-shadow.cpp
    # ---- 被测 system-server 源码（与生产目标源码清单一致，不含 main.cpp）----
    ${SYS_SERVER_DIR}/src/systemdbusserver.cpp
    ${SYS_SERVER_DIR}/src/dkapture_manager.cpp
    ${SYS_SERVER_DIR}/src/logger.cpp
)

target_include_directories(test_deepin-system-monitor-system-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${CMAKE_SOURCE_DIR}
    ${SYS_SERVER_DIR}/src
)

target_compile_definitions(test_deepin-system-monitor-system-server PRIVATE
    ENABLE_DKAPTURE
)

# PolkitQt6/5-1 的 IMPORTED 目标是目录作用域的：生产模块在其自身目录内 find_package，
# 对本目录不可见，故此处需再次 find_package（幂等，仅在本目录作用域注册目标）
if(QT_VERSION_MAJOR LESS 6)
    find_package(PolkitQt5-1 REQUIRED)
else()
    find_package(PolkitQt6-1 REQUIRED)
endif()

# 系统 DBus 类需要 QDBusContext/QDBusConnection，logger 使用 DConfig
# 与生产目标一致：Qt5→PolkitQt5-1::Agent，Qt6→PolkitQt6-1::Agent；DTK 用根目录解析的 ${DTK_NS}
target_link_libraries(test_deepin-system-monitor-system-server PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::DBus
    $<$<VERSION_LESS:${QT_VERSION_MAJOR},6>:PolkitQt5-1::Agent>
    $<$<VERSION_GREATER_EQUAL:${QT_VERSION_MAJOR},6>:PolkitQt6-1::Agent>
    ${DTK_NS}::Core
    Threads::Threads
    GTest::GTest
    GTest::Main
    gcov
)

message(STATUS "UT: test_deepin-system-monitor-system-server configured with ${SYS_SERVER_TEST_SOURCES}")
