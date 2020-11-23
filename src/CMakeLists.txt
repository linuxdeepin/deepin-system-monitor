set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(APP_NAME ${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")
set(CMAKE_CXX_FLAGS "-Wl,--as-need -fPIE")
set(CMAKE_EXE_LINKER_FLAGS "-pie")
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

find_package(DtkCore REQUIRED)
find_package(DtkGui REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(DtkWm REQUIRED)
find_library(LIB_PCAP NAMES pcap REQUIRED)
find_library(LIB_PROCPS NAMES procps REQUIRED)
find_library(LIB_XCB NAMES xcb REQUIRED)
find_library(LIB_XEXT NAMES Xext REQUIRED)
find_library(LIB_ICCCM NAMES xcb-icccm REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)

set(APP_HEADERS
    config.h
    environments.h
    accessible.h
    accessibledefine.h
    utils.h
    application.h
    stack_trace.h
    common/error_context.h
    common/hash.h
    common/han_latin.h
    common/collator.h
    common/perf.h
    common/whitelistconfig.h
    cpu_monitor.h
    memory_monitor.h
    network_monitor.h
    disk_monitor.h
    compact_cpu_monitor.h
    compact_memory_monitor.h
    compact_network_monitor.h
    compact_disk_monitor.h
    find_window_title.h
    smooth_curve_generator.h
    constant.h
    settings.h
    model/process_table_model.h
    model/process_sort_filter_proxy_model.h
    model/system_service_table_model.h
    model/system_service_sort_filter_proxy_model.h
    model/cpu_stat_model.h
    model/mem_stat_model.h
    model/netif_stat_model.h
    model/block_dev_stat_model.h
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
    gui/xwin_kill_preview_widget.h
    gui/xwin_kill_preview_background_widget.h
    gui/cpu_detail_view_widget.h
    gui/cpu_summary_view_widget.h
    gui/mem_detail_view_widget.h
    gui/mem_summary_view_widget.h
    gui/block_dev_detail_view_widget.h
    gui/block_dev_summary_view_widget.h
    gui/netif_detail_view_widget.h
    gui/netif_summary_view_widget.h
    gui/detail_view_stacked_widget.h
    gui/view/graphics_stat_item.h
    gui/view/graphics_stat_grid_item.h
    gui/view/graphics_stat_grid_layout.h
    gui/view/graphics_info_grid_item.h
    gui/view/graphics_info_grid_layout.h
    gui/view/graphics_cpu_stat_item.h
    gui/view/graphics_cpu_stat_detail_view.h
    gui/view/graphics_cpu_stat_summary_view.h
    gui/view/graphics_cpu_stat_grid_layout.h
    gui/view/graphics_cpu_info_grid_item.h
    gui/view/graphics_cpu_info_grid_layout.h
    gui/view/graphics_mem_stat_item.h
    gui/view/graphics_mem_stat_detail_view.h
    gui/view/graphics_mem_stat_summary_view.h
    gui/view/graphics_mem_stat_grid_layout.h
    gui/view/graphics_mem_info_grid_item.h
    gui/view/graphics_mem_info_grid_layout.h
    gui/view/graphics_block_dev_stat_item.h
    gui/view/graphics_block_dev_stat_detail_view.h
    gui/view/graphics_block_dev_stat_summary_view.h
    gui/view/graphics_block_dev_stat_grid_layout.h
    gui/view/graphics_block_dev_info_grid_item.h
    gui/view/graphics_block_dev_info_grid_layout.h
    gui/view/graphics_netif_stat_item.h
    gui/view/graphics_netif_stat_detail_view.h
    gui/view/graphics_netif_stat_summary_view.h
    gui/view/graphics_netif_stat_grid_layout.h
    gui/view/graphics_netif_info_grid_item.h
    gui/view/graphics_netif_info_grid_layout.h
    gui/view/graphics_stat_summary_view_layout.h
    gui/view/graphics_stat_summary_view.h
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
    process/system_stat.h
    process/stats_collector.h
    process/desktop_entry_stat.h
    process/netif_monitor.h
    process/netif_monitor_job.h
    process/netif_packet_parser.h
    service/service_manager_worker.h
    service/service_manager.h
    service/system_service_entry_data.h
    service/system_service_entry.h
    wm/wm_info.h
    system/cpu_info_manager.h
    system/mem_info_manager.h
    system/block_dev_info_manager.h
    system/netif_info_manager.h
    system/stat_manager.h
)
    
set(APP_SOURCES
    main.cpp
    application.cpp
    utils.cpp
    common/error_context.cpp
    common/hash.cpp
    common/collator.cpp
    common/han_latin.cpp
    common/perf.cpp
    common/whitelistconfig.cpp
    cpu_monitor.cpp
    memory_monitor.cpp
    network_monitor.cpp
    disk_monitor.cpp
    compact_cpu_monitor.cpp
    compact_memory_monitor.cpp
    compact_network_monitor.cpp
    compact_disk_monitor.cpp
    find_window_title.cpp
    smooth_curve_generator.cpp
    settings.cpp
    model/system_service_table_model.cpp
    model/system_service_sort_filter_proxy_model.cpp
    model/process_table_model.cpp
    model/process_sort_filter_proxy_model.cpp
    model/cpu_stat_model.cpp
    model/mem_stat_model.cpp
    model/netif_stat_model.cpp
    model/block_dev_stat_model.cpp
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
    gui/xwin_kill_preview_widget.cpp
    gui/xwin_kill_preview_background_widget.cpp
    gui/cpu_detail_view_widget.cpp
    gui/cpu_summary_view_widget.cpp
    gui/mem_detail_view_widget.cpp
    gui/mem_summary_view_widget.cpp
    gui/block_dev_detail_view_widget.cpp
    gui/block_dev_summary_view_widget.cpp
    gui/netif_detail_view_widget.cpp
    gui/netif_summary_view_widget.cpp
    gui/detail_view_stacked_widget.cpp
    gui/view/graphics_stat_item.cpp
    gui/view/graphics_stat_grid_item.cpp
    gui/view/graphics_stat_grid_layout.cpp
    gui/view/graphics_info_grid_item.cpp
    gui/view/graphics_info_grid_layout.cpp
    gui/view/graphics_cpu_stat_item.cpp
    gui/view/graphics_cpu_stat_detail_view.cpp
    gui/view/graphics_cpu_stat_summary_view.cpp
    gui/view/graphics_cpu_stat_grid_layout.cpp
    gui/view/graphics_cpu_info_grid_item.cpp
    gui/view/graphics_cpu_info_grid_layout.cpp
    gui/view/graphics_mem_stat_item.cpp
    gui/view/graphics_mem_stat_detail_view.cpp
    gui/view/graphics_mem_stat_summary_view.cpp
    gui/view/graphics_mem_stat_grid_layout.cpp
    gui/view/graphics_mem_info_grid_item.cpp
    gui/view/graphics_mem_info_grid_layout.cpp
    gui/view/graphics_block_dev_stat_item.cpp
    gui/view/graphics_block_dev_stat_detail_view.cpp
    gui/view/graphics_block_dev_stat_summary_view.cpp
    gui/view/graphics_block_dev_stat_grid_layout.cpp
    gui/view/graphics_block_dev_info_grid_item.cpp
    gui/view/graphics_block_dev_info_grid_layout.cpp
    gui/view/graphics_netif_stat_item.cpp
    gui/view/graphics_netif_stat_detail_view.cpp
    gui/view/graphics_netif_stat_summary_view.cpp
    gui/view/graphics_netif_stat_grid_layout.cpp
    gui/view/graphics_netif_info_grid_item.cpp
    gui/view/graphics_netif_info_grid_layout.cpp
    gui/view/graphics_stat_summary_view_layout.cpp
    gui/view/graphics_stat_summary_view.cpp
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
    process/netif_monitor.cpp
    process/netif_monitor_job.cpp
    process/netif_packet_parser.cpp
    process/process_controller.cpp
    process/process_stat.cpp
    process/system_stat.cpp
    process/stats_collector.cpp
    process/desktop_entry_stat.cpp
    service/service_manager_worker.cpp
    service/service_manager.cpp
    service/system_service_entry_data.cpp
    service/system_service_entry.cpp
    wm/wm_info.cpp
    system/cpu_info_manager.cpp
    system/mem_info_manager.cpp
    system/block_dev_info_manager.cpp
    system/netif_info_manager.cpp
    system/stat_manager.cpp
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
    dtkwm
    ${LIB_PCAP}
    ${LIB_PROCPS}
    ICU::i18n
    ICU::uc
    ${LIB_XCB}
    ${LIB_XEXT}
    ${LIB_ICCCM}
)

install(TARGETS ${APP_NAME} DESTINATION bin)
