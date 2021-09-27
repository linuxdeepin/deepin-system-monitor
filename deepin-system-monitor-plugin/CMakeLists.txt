cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(deepin-system-monitor-plugin LANGUAGES CXX)

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

set(APP_RES_ROOT asserts)
set(APP_RESOURCES ${APP_RES_ROOT}/${PROJECT_NAME}.qrc)

# Sources files
file(GLOB SRCS "*.h" "*.cpp" "gui/*.h" "gui/*.cpp" "dbus/*.h" "dbus/*.cpp")

find_package(PkgConfig REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(DtkWidget REQUIRED)

pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)
pkg_check_modules(QGSettings REQUIRED gsettings-qt)

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PROJECT_NAME} SHARED ${SRCS} ${APP_RESOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)
target_include_directories(${PROJECT_NAME} PUBLIC
  ${DtkWidget_INCLUDE_DIRS}
  ${DFrameworkDBus_INCLUDE_DIRS}
  ${QGSettings_INCLUDE_DIRS}
  ${DdeDockInterface_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${Qt5DBus_LIBRARIES}
  ${DtkWidget_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Svg_LIBRARIES}
  ${DFrameworkDBus_LIBRARIES}
  ${QGSettings_LIBRARIES}
)

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

    # generate qm
    message(${CMAKE_CURRENT_SOURCE_DIR}/translations)
    execute_process(COMMAND bash "translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/translations)
endif ()

INSTALL(FILES com.deepin.dde.dock.module.system-monitor.gschema.xml DESTINATION /usr/share/glib-2.0/schemas)
INSTALL(FILES com.deepin.system.monitor.plugin.gschema.xml DESTINATION /usr/share/glib-2.0/schemas)
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION /usr/lib/dde-dock/plugins)
#国际化翻译文件路径
file(GLOB QM_FILES "translations/*.qm")
INSTALL(FILES ${QM_FILES}
        DESTINATION "/usr/share/deepin-system-monitor-plugin/translations")
