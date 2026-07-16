// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQuick.Window 2.11
import org.deepin.systemmonitor 1.0

Window {
    id: root
    width: 280
    height: 120
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: "#2d2d2d"
    visible: true
    title: qsTr("System Monitor")

    Rectangle {
        id: card
        anchors.fill: parent
        color: "#2d2d2d"
        radius: 8

        ColumnLayout {
            id: mainColumn
            anchors.centerIn: parent
            spacing: 6

            // Row 1: CPU + Memory
            RowLayout {
                spacing: 16

                // CPU usage
                RowLayout {
                    spacing: 3

                    Text {
                        text: qsTr("%1%").arg(SystemMonitorApplet.cpuUsage)
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        renderType: Text.NativeRendering
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        text: qsTr("CPU")
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                    }
                }

                // Memory usage
                RowLayout {
                    spacing: 3

                    Text {
                        text: qsTr("%1%").arg(SystemMonitorApplet.memoryUsage)
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        renderType: Text.NativeRendering
                        horizontalAlignment: Text.AlignRight
                    }

                    Text {
                        text: qsTr("内存")
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                    }
                }
            }

            // Row 2: Network speed
            RowLayout {
                spacing: 16

                Text {
                    text: qsTr("%1 ↓").arg(SystemMonitorApplet.downloadSpeedText)
                    color: "#aaaaaa"
                    font.pixelSize: 11
                    renderType: Text.NativeRendering
                }

                Text {
                    text: qsTr("%1 ↑").arg(SystemMonitorApplet.uploadSpeedText)
                    color: "#aaaaaa"
                    font.pixelSize: 11
                    renderType: Text.NativeRendering
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: SystemMonitorApplet.openSystemMonitor()
        }
    }
}
