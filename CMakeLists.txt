cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(deepin-system-monitor)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS X11Extras REQUIRED)
find_package(Qt5 COMPONENTS DBus REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)
find_package(Qt5 COMPONENTS LinguistTools REQUIRED)

set(APP_RES_ROOT assets)
set(APP_RESOURCES ${APP_RES_ROOT}/${CMAKE_PROJECT_NAME}.qrc)
set(DESKTOP_FILE ${CMAKE_PROJECT_NAME}.desktop)
set(POLICY_FILE com.deepin.pkexec.${CMAKE_PROJECT_NAME}.policy)

add_subdirectory(assets)
add_subdirectory(src)
add_subdirectory(translations)

# stub
add_subdirectory(tools)
# stub
add_subdirectory(docs)
# stub
add_subdirectory(tests)
