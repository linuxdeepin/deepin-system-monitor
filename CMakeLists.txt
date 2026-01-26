cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if (NOT DEFINED VERSION)
    set(VERSION 5.9.1000)
endif()

project(deepin_system_monitor_proj)

option(DOTEST "option for test" OFF)

# 是否开启单元测试编译
#set(DOTEST ON)

include(GNUInstallDirs)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   set(CMAKE_INSTALL_PREFIX /usr)
endif ()

#compile flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")
  # Enable Qt builtin debug mode
  add_definitions("-DQT_MESSAGELOGCONTEXT")
  add_definitions("-DQT_NO_WARNING_OUTPUT")
else()
  # -Wl, -O2 Enable linker optimizations
  # -Wl, --gc-sections Remove unused code resulting from -fdsta-sections and
  # -ffunction-sections
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2 -Wl,--gc-sections")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -Wl,--gc-sections")
endif()

#安全加固编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -pie -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -pie -fstack-protector-all")

#判断龙芯架构
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips64")
    SET(IS_LOONGARCH_TYPE 1)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "__longarch64")
    SET(NOT_LOONGARCH_TYPE )
else()
    SET(IS_LOONGARCH_TYPE 0)
endif()

# Auto-detect Qt version (tries Qt6 first, falls back to Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Found Qt version: ${QT_VERSION_MAJOR}")

# Map to DTK version (Qt6→DTK6, Qt5→DTK5)
if (QT_VERSION_MAJOR MATCHES 6)
    set(DTK_VERSION_MAJOR 6)
else()
    set(DTK_VERSION_MAJOR)
endif()
message(STATUS "Build with DTK: ${DTK_VERSION_MAJOR}")
set(QT_NS Qt${QT_VERSION_MAJOR})
set(DTK_NS Dtk${DTK_VERSION_MAJOR})

#系统监视器主应用
ADD_SUBDIRECTORY(deepin-system-monitor-main)
#系统监视器插件
ADD_SUBDIRECTORY(deepin-system-monitor-plugin)
#系统监视器插件弹出应用
ADD_SUBDIRECTORY(deepin-system-monitor-plugin-popup)
#系统监视器安全等级保护守护应用
ADD_SUBDIRECTORY(deepin-system-monitor-daemon)
#系统监视器提示
ADD_SUBDIRECTORY(deepin-system-monitor-server)
#系统监视器后端提权处理服务
ADD_SUBDIRECTORY(deepin-system-monitor-system-server)

#单元测试
if(DOTEST)
    ADD_SUBDIRECTORY(tests)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

#安装日志收集工具配置
set(journel_system_monitor_main assets/logconfig/org.deepin.system-monitor.json)
install(FILES ${journel_system_monitor_main} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-log-viewer/deepin-log.conf.d/)

#安装DConfig配置
set(APPID org.deepin.system-monitor)
set(system_monitor_main assets/configs/org.deepin.system-monitor.json)
set(system_monitor_daemon assets/configs/org.deepin.system-monitor.daemon.json)
set(system_monitor_plugin assets/configs/org.deepin.system-monitor.plugin.json)
set(system_monitor_plugin_popup assets/configs/org.deepin.system-monitor.plugin.popup.json)
set(system_monitor_plugin_server assets/configs/org.deepin.system-monitor.server.json)

# DTK6 提供 dtk_add_config_meta_files 命令，DTK5 需要手动安装
# 需要显式引入 DConfig 模块以确保 DSG_DATA_DIR 在当前作用域可用
if (DTK_VERSION_MAJOR EQUAL 6)
    find_package(Dtk${DTK_VERSION_MAJOR}DConfig QUIET)
endif()

if (COMMAND dtk_add_config_meta_files)
    dtk_add_config_meta_files(APPID ${APPID} FILES ${system_monitor_main})
    dtk_add_config_meta_files(APPID ${APPID} FILES ${system_monitor_daemon})
    dtk_add_config_meta_files(APPID ${APPID} FILES ${system_monitor_plugin})
    dtk_add_config_meta_files(APPID ${APPID} FILES ${system_monitor_plugin_popup})
    dtk_add_config_meta_files(APPID ${APPID} FILES ${system_monitor_plugin_server})
    message("-- DConfig is supported by DTK (using dtk_add_config_meta_files)")
else()
    install(FILES ${system_monitor_main} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${APPID}/)
    install(FILES ${system_monitor_daemon} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${APPID}/)
    install(FILES ${system_monitor_plugin} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${APPID}/)
    install(FILES ${system_monitor_plugin_popup} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${APPID}/)
    install(FILES ${system_monitor_plugin_server} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dsg/configs/${APPID}/)
    message("-- DConfig fallback: installing config files directly")
endif()
