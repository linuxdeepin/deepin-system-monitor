# SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

file(GLOB APP_PLUGIN_TS_FILES LIST_DIRECTORIES false *.ts)
qt5_add_translation(APP_PLUGIN_QM_FILES ${APP_PLUGIN_TS_FILES})
add_custom_target(PLUGINQMFILES ALL DEPENDS ${APP_PLUGIN_QM_FILES})
install(FILES ${APP_PLUGIN_QM_FILES} DESTINATION share/${CMAKE_PROJECT_NAME}/translations)
