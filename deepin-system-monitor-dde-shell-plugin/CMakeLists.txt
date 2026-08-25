# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(deepin-system-monitor-dde-shell-plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find dde-dock / dde-tray-loader plugin interface headers
find_package(PkgConfig REQUIRED)
pkg_check_modules(DdeDockInterface REQUIRED dde-dock)

# DTK for theme detection (DGuiApplicationHelper) and DTK QML module
find_package(Dtk${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)

# Qt components
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Qml Quick DBus Network Widgets LinguistTools)

# ── Translation files ──────────────────────────────────────────────────
file(GLOB APP_TS_FILES LIST_DIRECTORIES false translations/*.ts)
set_source_files_properties(${APP_TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/translations")
if(QT_VERSION_MAJOR LESS 6)
    qt5_add_translation(APP_QM_FILES ${APP_TS_FILES})
else()
    qt6_add_translation(APP_QM_FILES ${APP_TS_FILES})
endif()
add_custom_target(dde_shell_plugin_qm ALL DEPENDS ${APP_QM_FILES})

# ── Plugin shared library ─────────────────────────────────────────────
set(PLUGIN_NAME "system-monitor")

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")

add_library(${PLUGIN_NAME} SHARED
    systemmonitorplugin.h
    systemmonitorplugin.cpp
    systemmonitorapplet.h
    systemmonitorapplet.cpp
    systemmonitorprober.h
    systemmonitorprober.cpp
    resources/systemmonitor.qrc
)

target_include_directories(${PLUGIN_NAME} PUBLIC
    ${DdeDockInterface_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${QT_VERSION_MAJOR}::Core
    Dtk${QT_VERSION_MAJOR}::Gui
)

# ── Install ────────────────────────────────────────────────────────────
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib/dde-dock/plugins
)

install(FILES ${APP_QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-system-monitor-dde-shell-plugin/translations
)


