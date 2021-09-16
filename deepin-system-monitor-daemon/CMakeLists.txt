cmake_minimum_required(VERSION 3.0.0)

set(DAEMON_NAME "deepin-system-monitor-daemon")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(RESOURCES_FILE assets/${DAEMON_NAME}.qrc)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SYSTEMMONITORDATA}
    )

find_package(Qt5 COMPONENTS Core DBus REQUIRED)
find_package(DtkCore CONFIG REQUIRED)

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

add_executable(${DAEMON_NAME}
    ${SRC}
    ${RESOURCES_FILE}
    )

target_link_libraries(
    ${DAEMON_NAME}
    Qt5::Core
    Qt5::DBus
    ${DtkCore_LIBRARIES}
    )

install(TARGETS ${DAEMON_NAME} DESTINATION bin)
install(FILES deepin-system-monitor-daemon.desktop DESTINATION /etc/xdg/autostart)
install(FILES com.deepin.SystemMonitor.Daemon.service DESTINATION /usr/share/dbus-1/services)
