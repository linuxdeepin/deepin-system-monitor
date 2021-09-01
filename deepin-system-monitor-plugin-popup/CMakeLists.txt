cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(deepin-system-monitor-plugin-popup LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(APP_RES_ROOT asserts)
set(APP_RESOURCES ${APP_RES_ROOT}/${PROJECT_NAME}.qrc)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")
set(CMAKE_CXX_FLAGS "-Wl,--as-need -fPIE")
set(CMAKE_EXE_LINKER_FLAGS "-pie")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions("-DQT_NO_DEBUG_OUTPUT")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/environments.h.in ${CMAKE_CURRENT_BINARY_DIR}/environments.h @ONLY)

include(CheckIncludeFiles)
check_include_files("sys/random.h" HAVE_SYS_RANDOM_H)
include(CheckSymbolExists)
check_symbol_exists(syscall "unistd.h" HAVE_SYS_CALL)
check_symbol_exists(SYS_getrandom "sys/syscall.h" HAVE_SYS_CALL)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include(FindPkgConfig)

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS X11Extras REQUIRED)
find_package(Qt5 COMPONENTS DBus REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)
find_package(Qt5 COMPONENTS LinguistTools REQUIRED)

find_package(DtkCore REQUIRED)
find_package(DtkGui REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(Qt5Svg REQUIRED)
find_library(LIB_PROPS NAMES procps REQUIRED)
find_library(LIB_XCB NAMES xcb REQUIRED)
find_library(LIB_XEXT NAMES Xext REQUIRED)
find_library(LIB_ICCCM NAMES xcb-icccm REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)
find_package(DFrameworkdbus REQUIRED)

#pkg_check_modules(QGSettings REQUIRED gsettings-qt)

pkg_check_modules(DdeDockInterface REQUIRED dde-dock)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")

pkg_search_module(LIB_NL3 REQUIRED libnl-3.0)
pkg_search_module(LIB_NL3_ROUTE REQUIRED libnl-route-3.0)
pkg_search_module(LIB_UDEV REQUIRED libudev)
include_directories(${LIB_NL3_INCLUDE_DIRS})
include_directories(${LIB_NL3_ROUTE_INCLUDE_DIRS})
include_directories(${LIB_UDEV_INCLUDE_DIRS})

#include_directories(${QGSettings_INCLUDE_DIRS})

FILE(GLOB_RECURSE SRCS "*.h" "*.cpp")

SET(HPP_MODEL
    model/cpu_info_model.h
    model/cpu_stat_model.h
    model/cpu_list_model.h
    model/process_sort_filter_proxy_model.h
    model/process_table_model.h
)

SET(CPP_MODEL
    model/cpu_info_model.cpp
    model/cpu_stat_model.cpp
    model/cpu_list_model.cpp
    model/process_sort_filter_proxy_model.cpp
    model/process_table_model.cpp
)

SET(HPP_GLOBAL
    common/datacommon.h
    common/common.h
    common/utils.h
    common/hash.h
    common/sample.h
    common/stack_trace.h
    common/thread_manager.h
    common/time_period.h
    common/base_thread.h
    common/error_context.h
    common/han_latin.h
    common/settings.h
    common/perf.h
)

SET(CPP_GLOBAL
    common/utils.cpp
    common/hash.cpp
    common/common.cpp
    common/thread_manager.cpp
    common/time_period.cpp
    common/error_context.cpp
    common/han_latin.cpp
    common/settings.cpp
    common/perf.cpp
)

SET(HPP_SYSTEM
    system/private/cpu_p.h
    system/private/cpu_set_p.h
    system/private/mem_p.h
    system/private/sys_info_p.h
    system/private/block_device_p.h
    system/diskio_info.h
    system/cpu_set.h
    system/cpu.h
    system/device_db.h
    system/mem.h
    system/net_info.h
    system/packet.h
    system/sys_info.h

    system/system_monitor_thread.h
    system/system_monitor.h
    system/block_device_info_db.h
    system/block_device.h
)

SET(CPP_SYSTEM
    system/diskio_info.cpp
    system/cpu_set.cpp
    system/cpu.cpp
    system/device_db.cpp
    system/mem.cpp
    system/net_info.cpp
    system/sys_info.cpp
    system/system_monitor_thread.cpp
    system/system_monitor.cpp
    system/block_device_info_db.cpp
    system/block_device.cpp
)

SET(HPP_GUI
    gui/base/base_table_view.h
    gui/base/base_header_view.h
    gui/base/base_item_delegate.h
    gui/mainwindow.h
    gui/datadealsingleton.h
    gui/process_table_view.h
    gui/ui_common.h
    gui/cpu_widget.h
    gui/disk_widget.h
    gui/memory_widget.h
    gui/net_widget.h
    gui/process_widget.h
)


SET(CPP_GUI
    gui/base/base_table_view.cpp
    gui/base/base_header_view.cpp
    gui/base/base_item_delegate.cpp
    gui/mainwindow.cpp
    gui/datadealsingleton.cpp
    gui/process_table_view.cpp
    gui/ui_common.cpp
    gui/cpu_widget.cpp
    gui/disk_widget.cpp
    gui/memory_widget.cpp
    gui/net_widget.cpp
    gui/process_widget.cpp
)


SET(HPP_WM
    wm/wm_window_list.h
    wm/wm_atom.h
    wm/wm_connection.h
    wm/wm_info.h
    wm/wm_window_tree.h
)

SET(CPP_WM
    wm/wm_window_list.cpp
    wm/wm_atom.cpp
    wm/wm_connection.cpp
    wm/wm_info.cpp
    wm/wm_window_tree.cpp
)

SET(HPP_DBUS
    dbus/systemmonitordbusadaptor.h
    dbus/dbusdockinterface.h
)

SET(CPP_DBUS
    dbus/systemmonitordbusadaptor.cpp
    dbus/dbusdockinterface.cpp
)

SET(HPP_PROCESS
    process/private/process_p.h
    process/process_icon_cache.h
    process/process_set.h
    process/process.h
    process/process_db.h
    process/process_icon.h
    process/desktop_entry_cache.h
    process/desktop_entry_cache_updater.h
    process/process_name.h
    process/process_name_cache.h
    process/process_controller.h
)

SET(CPP_PROCESS
    process/process_icon_cache.cpp
    process/process_set.cpp
    process/process.cpp
    process/process_db.cpp
    process/process_icon.cpp
    process/desktop_entry_cache.cpp
    process/desktop_entry_cache_updater.cpp
    process/process_name.cpp
    process/process_name_cache.cpp
    process/process_controller.cpp
)
set(APP_HPP
    application.h
    ${HPP_MODEL}
    ${HPP_SYSTEM}
    ${HPP_GLOBAL}
    ${HPP_GUI}
    ${HPP_DBUS}
    ${HPP_PROCESS}
    ${HPP_WM}
)

set(APP_CPP
    application.cpp
    main.cpp
    ${CPP_MODEL}
    ${CPP_SYSTEM}
    ${CPP_GLOBAL}
    ${CPP_GUI}
    ${CPP_DBUS}
    ${CPP_PROCESS}
    ${CPP_WM}
)

add_executable(${PROJECT_NAME}
    ${APP_HPP}
    ${APP_CPP}
    ${PROJECT_SOURCE_DIR}/${APP_RESOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
#    ${Qt5Widgets_INCLUDE_DIRS}
#    ${DtkWidget_INCLUDE_DIRS}
    ${DdeDockInterface_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::DBus
    Qt5::Concurrent
    Qt5::X11Extras
    Qt5::Svg
    dtkcore
    dtkwidget
    dtkgui
    ${LIB_PROCPS}
    ICU::i18n
    ICU::uc
    ${LIB_XCB}
    ${LIB_XEXT}
    ${LIB_ICCCM}
    ${LIB_NL3_LIBRARIES}
    ${LIB_NL3_ROUTE_LIBRAIES}
    ${LIB_UDEV_LIBRARIES}
    ${LIB_DDEDOCK}
    ${LIB_DFRAMEWORKDBUS}
#    gsettings-qt
    dframeworkdbus
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/com.deepin.SystemMonitorPluginPopup.service DESTINATION /usr/share/dbus-1/services)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/deepin-system-monitor-plugin-popup.desktop DESTINATION /etc/xdg/autostart)
message(${CMAKE_CURRENT_SOURCE_DIR} )
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/com.deepin.SystemMonitorPluginPopup.conf DESTINATION /etc/dbus-1/system.d)



