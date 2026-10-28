# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# popup 插件 GUI 单测独立可执行目标
# (popup 与主程序存在同名类 Application/MainWindow 等,不能混链,避免 ODR 冲突)

set(POPUP_UT_TARGET dsm-popup-ut)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(POPUP_DIR ${CMAKE_HOME_DIRECTORY}/deepin-system-monitor-plugin-popup)
set(MAIN_APP_DIR ${CMAKE_HOME_DIRECTORY}/deepin-system-monitor-main)

# config.h 供 hash.cpp / datadealsingleton.cpp include
configure_file(${CMAKE_HOME_DIRECTORY}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# POPUP_DIR 必须先于父目录链中的 MAIN_APP_DIR(popup 与主程序存在同名头文件
# application.h/logger.h 等),用 BEFORE 插入到父 include_directories 之前
include_directories(
    BEFORE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}
    ${POPUP_DIR}
    ${POPUP_DIR}/gui
    ${POPUP_DIR}/common
    ${POPUP_DIR}/dbus
    ${POPUP_DIR}/model
    ${POPUP_DIR}/process
    ${POPUP_DIR}/system
)

# 主程序 3rdparty 与 dde-dock 常量头(mainwindow.h 需要 dde-dock/constants.h)
include_directories(
    ${MAIN_APP_DIR}/3rdparty
    ${MAIN_APP_DIR}/3rdparty/include
    ${MAIN_APP_DIR}/3rdparty/libsmartcols/src
    /usr/include/dde-dock
)

# ---------- 源码集合:与 popup 插件目标保持一致(仅替换 main.cpp) ----------

set(HPP_MODEL
    ${MAIN_APP_DIR}/model/cpu_info_model.h
    ${MAIN_APP_DIR}/model/cpu_stat_model.h
    ${MAIN_APP_DIR}/model/cpu_list_model.h
    ${POPUP_DIR}/model/process_sort_filter_proxy_model.h
    ${POPUP_DIR}/model/process_table_model.h
)
set(CPP_MODEL
    ${MAIN_APP_DIR}/model/cpu_info_model.cpp
    ${MAIN_APP_DIR}/model/cpu_stat_model.cpp
    ${MAIN_APP_DIR}/model/cpu_list_model.cpp
    ${POPUP_DIR}/model/process_sort_filter_proxy_model.cpp
    ${POPUP_DIR}/model/process_table_model.cpp
)

set(CPP_GLOBAL
    ${POPUP_DIR}/common/utils.cpp
    ${MAIN_APP_DIR}/common/hash.cpp
    ${MAIN_APP_DIR}/common/common.cpp
    ${MAIN_APP_DIR}/common/thread_manager.cpp
    ${MAIN_APP_DIR}/common/time_period.cpp
    ${MAIN_APP_DIR}/common/error_context.cpp
    ${MAIN_APP_DIR}/common/han_latin.cpp
    ${MAIN_APP_DIR}/settings.cpp
    ${MAIN_APP_DIR}/common/perf.cpp
)

set(CPP_SYSTEM
    ${MAIN_APP_DIR}/system/diskio_info.cpp
    ${POPUP_DIR}/system/cpu_set.cpp
    ${MAIN_APP_DIR}/system/cpu.cpp
    ${POPUP_DIR}/system/device_db.cpp
    ${MAIN_APP_DIR}/system/mem.cpp
    ${MAIN_APP_DIR}/system/net_info.cpp
    ${MAIN_APP_DIR}/system/sys_info.cpp
    ${MAIN_APP_DIR}/system/system_monitor_thread.cpp
    ${MAIN_APP_DIR}/system/system_monitor.cpp
    ${MAIN_APP_DIR}/system/block_device_info_db.cpp
    ${MAIN_APP_DIR}/system/block_device.cpp
)

set(CPP_GUI
    ${POPUP_DIR}/gui/base/base_table_view.cpp
    ${MAIN_APP_DIR}/gui/base/base_header_view.cpp
    ${POPUP_DIR}/gui/base/base_item_delegate.cpp
    ${POPUP_DIR}/gui/mainwindow.cpp
    ${POPUP_DIR}/gui/datadealsingleton.cpp
    ${POPUP_DIR}/gui/process_table_view.cpp
    ${MAIN_APP_DIR}/gui/ui_common.cpp
    ${POPUP_DIR}/gui/cpu_widget.cpp
    ${POPUP_DIR}/gui/disk_widget.cpp
    ${POPUP_DIR}/gui/memory_widget.cpp
    ${POPUP_DIR}/gui/net_widget.cpp
    ${POPUP_DIR}/gui/process_widget.cpp
)

set(CPP_WM
    ${MAIN_APP_DIR}/wm/wm_window_list.cpp
    ${MAIN_APP_DIR}/wm/wm_atom.cpp
    ${MAIN_APP_DIR}/wm/wm_connection.cpp
    ${MAIN_APP_DIR}/wm/wm_info.cpp
    ${MAIN_APP_DIR}/wm/wm_window_tree.cpp
)

set(CPP_DBUS
    ${POPUP_DIR}/dbus/systemmonitordbusadaptor.cpp
    ${POPUP_DIR}/dbus/dbusdockinterface.cpp
    ${POPUP_DIR}/dbus/dbuscallmaininterface.cpp
    ${POPUP_DIR}/dbus/dbusayatanainterface.cpp
)

set(CPP_PROCESS
    ${MAIN_APP_DIR}/process/process_icon_cache.cpp
    ${MAIN_APP_DIR}/process/process_set.cpp
    ${POPUP_DIR}/process/process.cpp
    ${POPUP_DIR}/process/process_db.cpp
    ${MAIN_APP_DIR}/process/process_icon.cpp
    ${MAIN_APP_DIR}/process/desktop_entry_cache.cpp
    ${MAIN_APP_DIR}/process/desktop_entry_cache_updater.cpp
    ${MAIN_APP_DIR}/process/process_name.cpp
    ${MAIN_APP_DIR}/process/process_name_cache.cpp
    ${MAIN_APP_DIR}/process/process_controller.cpp
    ${MAIN_APP_DIR}/process/system_service_client.cpp
    ${MAIN_APP_DIR}/process/am_icon_manager.cpp
)

set(POPUP_UT_CPP
    main-ut.cpp
    ut_popup_stub.cpp
    ut_main_window.cpp
    ut_net_widget.cpp
    ut_disk_widget.cpp
)

add_executable(${POPUP_UT_TARGET}
    ${POPUP_UT_CPP}
    ${MAIN_APP_DIR}/common/base_thread.h
    ${POPUP_DIR}/application.cpp
    ${POPUP_DIR}/logger.cpp
    ${CPP_MODEL}
    ${CPP_GLOBAL}
    ${CPP_SYSTEM}
    ${CPP_GUI}
    ${CPP_WM}
    ${CPP_DBUS}
    ${CPP_PROCESS}
)

set_target_properties(${POPUP_UT_TARGET}
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# ---------- Mock 服务进程:提供私有总线上 Display1/Dock1 的真实 DBus 应答 ----------
# 与测试进程分离:PrimaryRect(4×qint16)等类型仅在本进程注册元类型,
# 消费侧(测试进程)未注册时 Qt 解组回 read 模式 QDBusArgument,与源码读取方式匹配
set(POPUP_MOCK_TARGET dsm-popup-mock-service)
add_executable(${POPUP_MOCK_TARGET}
    mock-display-service.cpp
)
set_target_properties(${POPUP_MOCK_TARGET}
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
target_include_directories(${POPUP_MOCK_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${POPUP_MOCK_TARGET}
    ${QT_NS}::Core
    ${QT_NS}::DBus
)

target_link_libraries(${POPUP_UT_TARGET}
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
    ${LIB_XCB}
    ${LIB_XEXT}
    ${LIB_ICCCM}
    ${LIB_NL3_LIBRARIES}
    ${LIB_NL3_ROUTE_LIBRARIES}
    ${LIB_UDEV_LIBRARIES}
    ${LIB_PCAP}
    Threads::Threads
    Qt${QT_VERSION_MAJOR}::Test
    GTest::GTest
    GTest::Main
    gmock
    gcov
)
