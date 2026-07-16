# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Qml Quick DBus Network)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

qt_add_executable(deepin-system-monitor-dde-shell-plugin
    main.cpp
    systemmonitorapplet.h
    systemmonitorapplet.cpp
    systemmonitorprober.h
    systemmonitorprober.cpp
)

qt_add_qml_module(deepin-system-monitor-dde-shell-plugin
    URI org.deepin.systemmonitor
    VERSION 1.0
    QML_FILES
        package/main.qml
)

target_link_libraries(deepin-system-monitor-dde-shell-plugin PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Network
)

install(TARGETS deepin-system-monitor-dde-shell-plugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
