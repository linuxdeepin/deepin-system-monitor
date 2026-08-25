// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0 as D

Item {
    id: root

    implicitWidth: 180
    implicitHeight: 40

    readonly property bool dark: D.DTK.themeType === D.ApplicationHelper.DarkType
    readonly property bool expanded: height > 42
    readonly property int valueSize: Math.max(11, Math.min(16, Math.round(Math.max(24, height - 12) * 0.4)))
    readonly property int labelSize: Math.max(9, Math.round(Math.max(24, height - 12) * 0.24))

    GridLayout {
        anchors.centerIn: parent
        columns: 2
        columnSpacing: 16
        rowSpacing: 2

        RowLayout {
            Layout.alignment: Qt.AlignBottom
            spacing: 3
            Text {
                text: qsTr("%1%").arg(systemMonitorApplet.cpuUsage)
                color: root.dark ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(0, 0, 0, 0.92)
                font.pixelSize: root.valueSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignBottom
            }
            Text {
                text: qsTr("CPU")
                color: root.dark ? Qt.rgba(1, 1, 1, 0.68) : Qt.rgba(0, 0, 0, 0.58)
                font.pixelSize: root.labelSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignBottom
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignBottom
            spacing: 3
            Text {
                text: qsTr("%1%").arg(systemMonitorApplet.memoryUsage)
                color: root.dark ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(0, 0, 0, 0.92)
                font.pixelSize: root.valueSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignBottom
            }
            Text {
                text: qsTr("MEM")
                color: root.dark ? Qt.rgba(1, 1, 1, 0.68) : Qt.rgba(0, 0, 0, 0.58)
                font.pixelSize: root.labelSize
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignBottom
            }
        }

        Text {
            visible: root.expanded
            text: qsTr("%1 ↓").arg(systemMonitorApplet.downloadSpeedText)
            color: root.dark ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(0, 0, 0, 0.48)
            font.pixelSize: root.labelSize
            renderType: Text.NativeRendering
        }

        Text {
            visible: root.expanded
            text: qsTr("%1 ↑").arg(systemMonitorApplet.uploadSpeedText)
            color: root.dark ? Qt.rgba(1, 1, 1, 0.55) : Qt.rgba(0, 0, 0, 0.48)
            font.pixelSize: root.labelSize
            renderType: Text.NativeRendering
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: systemMonitorApplet.openSystemMonitor()
    }
}
