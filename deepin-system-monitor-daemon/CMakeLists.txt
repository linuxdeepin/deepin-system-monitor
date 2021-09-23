# 等保需求, 1050集成放开
cmake_minimum_required(VERSION 3.0.0)

set(PROJECT_NAME deepin-system-monitor)
set(DAEMON_NAME ${PROJECT_NAME}-daemon)

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

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

    # generate qm
    message(${CMAKE_CURRENT_SOURCE_DIR}/translations)
    execute_process(COMMAND bash "translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/translations)
endif ()

file(GLOB DAEMON_QM_FILES "translations/*.qm")


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
install(FILES ${DAEMON_QM_FILES} DESTINATION share/${DAEMON_NAME}/translations)
