cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if (NOT DEFINED VERSION)
    set(VERSION 5.9.1000)
endif()

project(deepin_system_monitor_proj)

option(DOTEST "option for test" OFF)

# 是否开启单元测试编译
#set(DOTEST ON)

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

#判断龙芯架构
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips64")
    SET(IS_LOONGARCH_TYPE 1)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "__longarch64")
    SET(NOT_LOONGARCH_TYPE )
else()
    SET(IS_LOONGARCH_TYPE 0)
endif()

#系统监视器主应用
ADD_SUBDIRECTORY(deepin-system-monitor-main)
#系统监视器插件
ADD_SUBDIRECTORY(deepin-system-monitor-plugin)
#系统监视器插件弹出应用
ADD_SUBDIRECTORY(deepin-system-monitor-plugin-popup)
#系统监视器安全等级保护守护应用
ADD_SUBDIRECTORY(deepin-system-monitor-daemon)
#单元测试
if(DOTEST)
    ADD_SUBDIRECTORY(tests)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
