set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(APP_NAME ${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_definitions("-DQT_NO_DEBUG_OUTPUT")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/environments.h.in ${CMAKE_CURRENT_BINARY_DIR}/environments.h @ONLY)

include(CheckIncludeFiles)
check_include_files ("sys/random.h" HAVE_SYS_RANDOM_H)
include(CheckSymbolExists)
check_symbol_exists(syscall "unistd.h" HAVE_SYS_CALL)
check_symbol_exists(SYS_getrandom "sys/syscall.h" HAVE_SYS_GETRANDOM)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include(FindPkgConfig)

find_package(DtkCore REQUIRED)
find_package(DtkGui REQUIRED)
find_package(DtkWidget REQUIRED)
find_library(LIB_PCAP NAMES pcap REQUIRED)
find_library(LIB_PROCPS NAMES procps REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)

pkg_search_module(LIB_NL3 REQUIRED libnl-3.0)
pkg_search_module(LIB_NL3_ROUTE REQUIRED libnl-route-3.0)
include_directories(${LIB_NL3_INCLUDE_DIRS})
include_directories(${LIB_NL3_ROUTE_INCLUDE_DIRS})


set(APP_HEADERS
    config.h
    environments.h
    utils.h
    stack_trace.h
    common/error_context.h
    common/hash.h
    common/han_latin.h
    common/collator.h
    cpu_monitor.h
    memory_monitor.h
    network_monitor.h
    disk_monitor.h
    compact_cpu_monitor.h
    compact_memory_monitor.h
    compact_network_monitor.h
    compact_disk_monitor.h
    smooth_curve_generator.h
    constant.h
    settings.h
    model/process_table_model.h
    model/process_sort_filter_proxy_model.h
    model/system_service_table_model.h
    model/system_service_sort_filter_proxy_model.h
    gui/ui_common.h
    gui/toolbar.h
    gui/main_window.h
    gui/base_table_view.h
    gui/base_item_delegate.h
    gui/base_header_view.h
    gui/process_table_view.h
    gui/process_page_widget.h
    gui/service_name_sub_input_dialog.h
    gui/system_service_table_view.h
    gui/system_service_page_widget.h
    gui/monitor_expand_view.h
    gui/monitor_compact_view.h
    gui/priority_slider.h
    gui/kill_process_confirm_dialog.h
    gui/process_attribute_dialog.h
    gui/dialog/error_dialog.h
    dbus/systemd1_manager_interface.h
    dbus/systemd1_service_interface.h
    dbus/systemd1_unit_interface.h
    dbus/dbus_properties_interface.h
    dbus/dbus_common.h
    dbus/environment_file.h
    dbus/unit_file_info.h
    dbus/unit_info.h
    process/process_entry.h
    process/system_monitor.h
    process/priority_controller.h
    process/process_controller.h
    process/process_stat.h
    process/stats_collector.h
    process/desktop_entry_cache.h
    service/service_manager_worker.h
    service/service_manager.h
    service/system_service_entry_data.h
    service/system_service_entry.h
    system/device_id_cache.h
    system/netif.h
    system/packet.h
    system/netif_monitor.h
    system/netif_info_cache.h
    system/netif_event_filter.h
    system/netif_packet_capture.h
    system/netif_packet_parser.h
    system/system_stat.h
)
    
set(APP_SOURCES
    main.cpp
    utils.cpp
    common/error_context.cpp
    common/hash.cpp
    common/collator.cpp
    common/han_latin.cpp
    cpu_monitor.cpp
    memory_monitor.cpp
    network_monitor.cpp
    disk_monitor.cpp
    compact_cpu_monitor.cpp
    compact_memory_monitor.cpp
    compact_network_monitor.cpp
    compact_disk_monitor.cpp
    smooth_curve_generator.cpp
    settings.cpp
    model/system_service_table_model.cpp
    model/system_service_sort_filter_proxy_model.cpp
    model/process_table_model.cpp
    model/process_sort_filter_proxy_model.cpp
    gui/toolbar.cpp
    gui/system_service_table_view.cpp
    gui/main_window.cpp
    gui/system_service_page_widget.cpp
    gui/process_page_widget.cpp
    gui/service_name_sub_input_dialog.cpp
    gui/process_table_view.cpp
    gui/base_table_view.cpp
    gui/base_item_delegate.cpp
    gui/base_header_view.cpp
    gui/dialog/error_dialog.cpp
    gui/monitor_expand_view.cpp
    gui/monitor_compact_view.cpp
    gui/kill_process_confirm_dialog.cpp
    gui/process_attribute_dialog.cpp
    gui/priority_slider.cpp
    gui/ui_common.cpp
    dbus/environment_file.cpp
    dbus/unit_file_info.cpp
    dbus/unit_info.cpp
    dbus/systemd1_manager_interface.cpp
    dbus/systemd1_service_interface.cpp
    dbus/systemd1_unit_interface.cpp
    dbus/dbus_properties_interface.cpp
    process/process_entry.cpp
    process/system_monitor.cpp
    process/priority_controller.cpp
    process/process_controller.cpp
    process/process_stat.cpp
    process/stats_collector.cpp
    process/desktop_entry_cache.cpp
    service/service_manager_worker.cpp
    service/service_manager.cpp
    service/system_service_entry_data.cpp
    service/system_service_entry.cpp
    system/device_id_cache.cpp
    system/netif_monitor.cpp
    system/netif_info_cache.cpp
    system/netif_event_filter.cpp
    system/netif_packet_capture.cpp
    system/netif_packet_parser.cpp
    system/system_stat.cpp
)

add_executable(${APP_NAME}
    ${APP_HEADERS}
    ${APP_SOURCES}
    ${CMAKE_SOURCE_DIR}/${APP_RESOURCES}
)

target_link_libraries(${APP_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::DBus
    Qt5::Concurrent
    Qt5::X11Extras
    dtkcore
    dtkwidget
    dtkgui
    ${LIB_PCAP}
    ${LIB_PROCPS}
    ICU::i18n
    ICU::uc
    ${LIB_NL3_LIBRARIES}
    ${LIB_NL3_ROUTE_LIBRARIES}
)

install(TARGETS ${APP_NAME} DESTINATION bin)
