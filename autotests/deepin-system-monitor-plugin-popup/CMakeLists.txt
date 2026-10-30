# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# qt-autotest-generator: deepin-system-monitor-plugin-popup GUI 单元测试模块
#
# 说明：popup 模块与 deepin-system-monitor-main 存在同名类（ProcessTableView /
# BaseTableView / ProcessTableModel 等，均无命名空间），若并入主测试二进制
# deepin-system-monitor-test 会产生 ODR/重复符号冲突，故按模块独立建测试目标；
# 由 tests/CMakeLists.txt 末尾追加的 add_subdirectory 挂接（DOTEST=ON 时生效）。

cmake_minimum_required(VERSION 3.10)

set(POPUP_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-plugin-popup)
set(MAIN_APP_DIR ${CMAKE_SOURCE_DIR}/deepin-system-monitor-main)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB POPUP_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

# common/base_thread.h 为仅头文件 Q_OBJECT 类（无 .cpp），AUTOMOC 未能为其生成 moc，
# 显式 qt_generate_moc 补上，避免链接期 BaseThread::staticMetaObject/vtable 未定义
qt_generate_moc(${MAIN_APP_DIR}/common/base_thread.h
    ${CMAKE_CURRENT_BINARY_DIR}/moc_base_thread.cpp)

add_executable(test_deepin-system-monitor-plugin-popup
    ${POPUP_TEST_SOURCES}
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_base_thread.cpp
    ${POPUP_DIR}/asserts/deepin-system-monitor-plugin-popup.qrc
    3rdparty/stub/stub-shadow.cpp
    # ---- 被测 popup GUI 源码 ----
    ${POPUP_DIR}/gui/memory_widget.cpp
    ${POPUP_DIR}/gui/cpu_widget.cpp
    ${POPUP_DIR}/gui/process_widget.cpp
    ${POPUP_DIR}/gui/process_table_view.cpp
    ${POPUP_DIR}/gui/datadealsingleton.cpp
    ${POPUP_DIR}/gui/base/base_table_view.cpp
    ${POPUP_DIR}/gui/base/base_item_delegate.cpp
    # ---- popup model / dbus / system / process / common ----
    ${POPUP_DIR}/model/process_table_model.cpp
    ${POPUP_DIR}/model/process_sort_filter_proxy_model.cpp
    ${POPUP_DIR}/common/utils.cpp
    ${POPUP_DIR}/logger.cpp
    ${POPUP_DIR}/dbus/dbuscallmaininterface.cpp
    ${POPUP_DIR}/dbus/dbusdockinterface.cpp
    ${POPUP_DIR}/dbus/dbusayatanainterface.cpp
    ${POPUP_DIR}/dbus/systemmonitordbusadaptor.cpp
    ${POPUP_DIR}/system/cpu_set.cpp
    ${POPUP_DIR}/system/device_db.cpp
    ${POPUP_DIR}/process/process.cpp
    ${POPUP_DIR}/process/process_db.cpp
    # ---- main-app 依赖（与 popup 目标源码清单一致）----
    ${MAIN_APP_DIR}/model/cpu_info_model.cpp
    ${MAIN_APP_DIR}/model/cpu_stat_model.cpp
    ${MAIN_APP_DIR}/model/cpu_list_model.cpp
    ${MAIN_APP_DIR}/common/hash.cpp
    ${MAIN_APP_DIR}/common/common.cpp
    ${MAIN_APP_DIR}/common/thread_manager.cpp
    ${MAIN_APP_DIR}/common/time_period.cpp
    ${MAIN_APP_DIR}/common/error_context.cpp
    ${MAIN_APP_DIR}/common/han_latin.cpp
    ${MAIN_APP_DIR}/common/perf.cpp
    ${MAIN_APP_DIR}/settings.cpp
    ${MAIN_APP_DIR}/system/diskio_info.cpp
    ${MAIN_APP_DIR}/system/cpu.cpp
    ${MAIN_APP_DIR}/system/mem.cpp
    ${MAIN_APP_DIR}/system/net_info.cpp
    ${MAIN_APP_DIR}/system/sys_info.cpp
    ${MAIN_APP_DIR}/system/system_monitor_thread.cpp
    ${MAIN_APP_DIR}/system/system_monitor.cpp
    ${MAIN_APP_DIR}/system/block_device_info_db.cpp
    ${MAIN_APP_DIR}/system/block_device.cpp
    ${MAIN_APP_DIR}/gui/base/base_header_view.cpp
    ${MAIN_APP_DIR}/gui/ui_common.cpp
    ${MAIN_APP_DIR}/process/process_icon_cache.cpp
    ${MAIN_APP_DIR}/process/process_set.cpp
    ${MAIN_APP_DIR}/process/process_icon.cpp
    ${MAIN_APP_DIR}/process/desktop_entry_cache.cpp
    ${MAIN_APP_DIR}/process/desktop_entry_cache_updater.cpp
    ${MAIN_APP_DIR}/process/process_name.cpp
    ${MAIN_APP_DIR}/process/process_name_cache.cpp
    ${MAIN_APP_DIR}/process/process_controller.cpp
    ${MAIN_APP_DIR}/process/system_service_client.cpp
    ${MAIN_APP_DIR}/process/am_icon_manager.cpp
    ${POPUP_DIR}/application.cpp
    ${MAIN_APP_DIR}/wm/wm_window_list.cpp
    ${MAIN_APP_DIR}/wm/wm_atom.cpp
    ${MAIN_APP_DIR}/wm/wm_connection.cpp
    ${MAIN_APP_DIR}/wm/wm_info.cpp
    ${MAIN_APP_DIR}/wm/wm_window_tree.cpp
)

# 目录级 include（BEFORE 前置 popup 路径，确保 popup 自带头文件优先于
# tests/CMakeLists.txt 目录属性里继承的 deepin-system-monitor-main 路径，
# 避免同名头文件（如 model/process_table_model.h）解析到主程序版本）
include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${POPUP_DIR}
    ${POPUP_DIR}/gui
    ${POPUP_DIR}/model
    ${POPUP_DIR}/dbus
    ${POPUP_DIR}/process
    ${POPUP_DIR}/system
    ${POPUP_DIR}/common
)

target_include_directories(test_deepin-system-monitor-plugin-popup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stub
    ${CMAKE_SOURCE_DIR}
    ${POPUP_DIR}
    ${POPUP_DIR}/gui
    ${POPUP_DIR}/model
    ${POPUP_DIR}/dbus
    ${POPUP_DIR}/process
    ${POPUP_DIR}/system
    ${POPUP_DIR}/common
    ${MAIN_APP_DIR}
)

target_link_libraries(test_deepin-system-monitor-plugin-popup PRIVATE
    ${QT_NS}::Core
    ${QT_NS}::Widgets
    ${QT_NS}::Gui
    ${QT_NS}::DBus
    ${QT_NS}::Concurrent
    ${DTK_NS}::Core
    ${DTK_NS}::Gui
    ${DTK_NS}::Widget
    ICU::i18n
    ICU::uc
    ${LIB_PCAP}
    ${LIB_XCB}
    ${LIB_XEXT}
    ${LIB_ICCCM}
    ${LIB_NL3_LIBRARIES}
    ${LIB_NL3_ROUTE_LIBRARIES}
    ${LIB_UDEV_LIBRARIES}
    Threads::Threads
    GTest::GTest
    GTest::Main
    gmock
    gmock_main
    gcov
)

message(STATUS "UT: test_deepin-system-monitor-plugin-popup configured with ${POPUP_TEST_SOURCES}")

# qt-autotest-generator: Process/ProcessDB 的私有方法（read*/readSockInodes/
# sendSignalToProcess 等）无公开入口且无入站调用方，仅能用 -fno-access-control
# 直调（deepin-mail 等项目同款方案；仅影响编译期访问检查，不改变代码生成）
target_compile_options(test_deepin-system-monitor-plugin-popup PRIVATE -fno-access-control)

# qt-autotest-generator: QSignalSpy 需要 Qt::Test 模块（APPEND，不改已有行）
target_link_libraries(test_deepin-system-monitor-plugin-popup PRIVATE ${QT_NS}::Test)
