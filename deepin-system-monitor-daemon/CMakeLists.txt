# 等保需求, 1050集成放开
cmake_minimum_required(VERSION 3.0.0)

set(PROJECT_NAME deepin-system-monitor)
set(DAEMON_NAME ${PROJECT_NAME}-daemon)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(RESOURCES_FILE assets/${DAEMON_NAME}.qrc)

find_package(Qt5 COMPONENTS LinguistTools REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SYSTEMMONITORDATA}
    ${DtkCore_INCLUDE_DIRS}
    )

find_package(Qt5 COMPONENTS Core DBus REQUIRED)
find_package(DtkCore CONFIG REQUIRED)
set(HEADER
    systemmonitorservice.h
    settinghandler.h
    cpuprofile.h
    memoryprofile.h
    )
set(SRC
    systemmonitorservice.cpp
    settinghandler.cpp
    cpuprofile.cpp
    memoryprofile.cpp
    main.cpp
    )

QT5_ADD_DBUS_ADAPTOR(SRC
    misc/com.deepin.SystemMonitor.Daemon.xml
    systemmonitorservice.h
    SystemMonitorService
    )

#生成qm
file(GLOB APP_TS_FILES LIST_DIRECTORIES false translations/*.ts)
qt5_add_translation(APP_QM_FILES ${APP_TS_FILES})
add_custom_target(DAEMON_QMFILES ALL DEPENDS ${APP_QM_FILES})

add_executable(${DAEMON_NAME}
    ${HEADER}
    ${SRC}
    ${RESOURCES_FILE}
    )

target_link_libraries(
    ${DAEMON_NAME}
    Qt5::Core
    Qt5::DBus
    ${DtkCore_LIBRARIES}
    )

#安全测试加固编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -z relro -z now -z noexecstack -pie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -z relro -z now -z noexecstack -pie")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wl,-Map=deepin-system-monitor-daemon.map")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wl,-Map=deepin-system-monitor-daemon.map")

install(TARGETS ${DAEMON_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES deepin-system-monitor-daemon.desktop DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)
install(FILES com.deepin.SystemMonitor.Daemon.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
#安装翻译文件
install(FILES ${APP_QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-system-monitor-daemon/translations)
