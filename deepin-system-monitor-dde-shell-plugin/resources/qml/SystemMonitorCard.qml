// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Effects
import org.deepin.dtk 1.0 as D

Item {
    id: root

    // dock 卡片窗口由框架给定尺寸（时尚模式卡片区约 150+dockSize/4 宽），
    // 根元素随窗口拉伸，内容按实际宽高自适应
    implicitWidth: compact || narrow ? 150 : 208
    implicitHeight: 40

    // ── 控制器数据收敛入口（空值保护：controller 缺失时降级为占位展示，
    // 不产生绑定异常，保证任务栏与其它卡片正常可用）──
    readonly property bool dataValid: systemMonitorController ? systemMonitorController.dataValid : false
    readonly property int cpuUsage: systemMonitorController ? systemMonitorController.cpuUsage : 0
    readonly property int memoryUsage: systemMonitorController ? systemMonitorController.memoryUsage : 0
    readonly property string uploadSpeedText: systemMonitorController ? systemMonitorController.uploadSpeedText : ""
    readonly property string downloadSpeedText: systemMonitorController ? systemMonitorController.downloadSpeedText : ""
    readonly property bool networkConnected: systemMonitorController ? systemMonitorController.networkConnected : false

    // PRD: 任务栏可用高度 < 28 时切换为单行紧凑态
    readonly property bool compact: height > 0 && height < 28
    // dock 时尚模式卡片区宽度有限（约 146px），不足以容纳双列布局时
    // 切换为合并双行：每行 = 负载 + 对应网速
    readonly property bool narrow: !compact && width > 0 && width < 180
    // 宽度不足时整体压缩：网速数值降为次级字号，保证不裁剪
    readonly property bool squeezed: width > 0 && width < 180
    readonly property bool dark: D.DTK.themeType === D.ApplicationHelper.DarkType
    // 图标尺寸跟随任务栏调节范围 24-32px，与邮箱卡片实现保持一致
    readonly property int iconSize: Math.max(24, Math.min(32, Math.round(height * 0.8)))
    readonly property int valueSize: compact ? (squeezed ? 10 : Math.max(11, Math.min(14, Math.round(height * 0.5))))
                                             : narrow ? Math.max(10, Math.min(11, Math.round(height * 0.26)))
                                             : Math.max(11, Math.min(15, Math.round(height * 0.3)))
    readonly property int labelSize: Math.max(9, valueSize - 3)

    // ── 三色负载状态：0 正常 / 1 预警 / 2 高危 ──
    // CPU ≥ 70% 或内存 ≥ 75% 对应项预警变橙；CPU ≥ 90% 或内存 ≥ 90% 对应项高危变红
    readonly property int cpuState: !dataValid ? 0
                                   : cpuUsage >= 90 ? 2
                                   : cpuUsage >= 70 ? 1 : 0
    readonly property int memState: !dataValid ? 0
                                   : memoryUsage >= 90 ? 2
                                   : memoryUsage >= 75 ? 1 : 0
    readonly property int overallState: Math.max(cpuState, memState)
    // 紧凑态：内存 ≥ 85% 且高于 CPU 时，左侧自动优先高亮切换展示内存；
    // 抢显后带滞回：内存回落至 80% 以下或 CPU 反超内存 +5% 才交还，避免临界值来回抖动
    property bool compactPreferMemory: false
    onMemoryUsageChanged: refreshCompactPreferMemory()
    onCpuUsageChanged: refreshCompactPreferMemory()
    onDataValidChanged: refreshCompactPreferMemory()
    function refreshCompactPreferMemory() {
        if (!dataValid)
            compactPreferMemory = false
        else if (compactPreferMemory)
            compactPreferMemory = memoryUsage >= 80 && cpuUsage < memoryUsage + 5
        else
            compactPreferMemory = memoryUsage >= 85 && memoryUsage > cpuUsage
    }

    // ── 主题色板（完整适配浅色/深色主题）──
    readonly property color colorText: dark ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(0, 0, 0, 0.92)
    readonly property color colorSecondary: dark ? Qt.rgba(1, 1, 1, 0.68) : Qt.rgba(0, 0, 0, 0.58)
    readonly property color colorDisabled: dark ? Qt.rgba(1, 1, 1, 0.4) : Qt.rgba(0, 0, 0, 0.35)
    readonly property color colorWarning: dark ? Qt.rgba(1, 0.66, 0.2, 1) : Qt.rgba(0.95, 0.55, 0, 1)
    readonly property color colorCritical: dark ? Qt.rgba(1, 0.38, 0.38, 1) : Qt.rgba(0.9, 0.2, 0.2, 1)

    function stateColor(state) {
        return state === 2 ? colorCritical : (state === 1 ? colorWarning : colorText)
    }

    // 网速文本拆分为数值与单位（"12 KB/s" → ["12", "KB/s"]）
    function splitSpeed(text) {
        const idx = text.lastIndexOf(" ")
        return idx > 0 ? [text.substring(0, idx), text.substring(idx + 1)] : [text, ""]
    }
    readonly property var upSpeed: splitSpeed(uploadSpeedText)
    readonly property var downSpeed: splitSpeed(downloadSpeedText)

    readonly property bool connected: dataValid && networkConnected

    // ── 按需可见采样：卡片不可见/任务栏隐藏时挂起轮询，恢复可见时立即刷新首帧 ──
    readonly property bool exposed: visible && Window.window !== null && Window.window.visible
    onExposedChanged: {
        if (systemMonitorController)
            systemMonitorController.setSamplingPaused(!exposed)
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.right: parent.right
        // surface 右缘落在卡片边饰带上，多留 8px 避让防 /s 尾巴被遮
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        // 紧凑态间距收紧到 3，保证大数值网速不裁剪
        spacing: root.compact ? 3 : (root.narrow ? 4 : 8)

        // ── 系统监视器图标：跟随系统深浅色主题，并按最高负载级别同步变色 ──
        // 图标槽位固定预留不参与压缩，图标加载失败也不影响文本区
        Item {
            Layout.preferredWidth: root.iconSize
            Layout.preferredHeight: root.iconSize
            Layout.minimumWidth: root.iconSize
            Layout.alignment: Qt.AlignVCenter

            D.DciIcon {
                id: iconItem
                anchors.fill: parent
                name: "deepin-system-monitor"
                sourceSize: Qt.size(root.iconSize, root.iconSize)
                palette: D.DTK.makeIconPalette(root.palette)
                theme: root.dark ? D.ApplicationHelper.DarkType : D.ApplicationHelper.LightType
            }

            // 预警/高危时对图标整体着色，正常负载保持主题原色
            MultiEffect {
                anchors.fill: parent
                source: iconItem
                visible: root.overallState > 0
                autoPaddingEnabled: false
                colorization: 1.0
                colorizationColor: root.stateColor(root.overallState)
            }
        }

        // ── 紧凑态：单行并排展示核心负载与实时下行网速 ──
        // fillWidth：撑满图标右侧剩余宽度，弹性间隔才能把网速段推贴右缘
        RowLayout {
            visible: root.compact
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 3

            Text {
                text: root.compactPreferMemory ? qsTr("MEM") : qsTr("CPU")
                color: root.compactPreferMemory ? root.stateColor(root.memState) : root.colorSecondary
                font.pixelSize: root.labelSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                readonly property int usage: root.compactPreferMemory ? root.memoryUsage
                                                                      : root.cpuUsage
                text: qsTr("%1%").arg(root.dataValid ? String(usage) : "--")
                color: !root.dataValid ? root.colorDisabled
                        : root.stateColor(root.compactPreferMemory ? root.memState : root.cpuState)
                font.pixelSize: root.valueSize
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            // 弹性间隔：网速段贴齐右缘
            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                visible: root.connected
                text: "↓"
                color: root.colorSecondary
                font.pixelSize: root.labelSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                visible: root.connected
                text: root.downSpeed[0]
                color: root.colorText
                font.pixelSize: root.valueSize
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                visible: root.connected
                text: root.downSpeed[1]
                color: root.colorSecondary
                font.pixelSize: root.labelSize
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            // 断网时右侧显示"未连接"
            Text {
                visible: root.dataValid && !root.networkConnected
                text: qsTr("Disconnected")
                color: root.colorDisabled
                font.pixelSize: root.valueSize
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
            // 采集服务异常或超时：降级展示占位符 --
            Text {
                visible: !root.dataValid
                text: "--"
                color: root.colorDisabled
                font.pixelSize: root.valueSize
                font.weight: Font.Medium
                renderType: Text.NativeRendering
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // ── 窄卡合并双行态：每行 = 负载 + 对应网速（适配时尚模式卡片区宽度）──
        ColumnLayout {
            visible: root.narrow
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            spacing: 2

            // 第一行：CPU + 上行网速
            RowLayout {
                spacing: 2
                Text {
                    text: qsTr("CPU")
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    text: qsTr("%1%").arg(root.dataValid ? String(root.cpuUsage) : "--")
                    color: root.dataValid ? root.stateColor(root.cpuState) : root.colorDisabled
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                // 弹性间隔：网速段贴齐右缘
                Item {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: "↑"
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: root.upSpeed[0]
                    color: root.colorText
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: root.upSpeed[1]
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
            }

            // 第二行：内存 + 下行网速（或断网提示）
            RowLayout {
                spacing: 2
                Text {
                    text: qsTr("MEM")
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    text: qsTr("%1%").arg(root.dataValid ? String(root.memoryUsage) : "--")
                    color: root.dataValid ? root.stateColor(root.memState) : root.colorDisabled
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Item {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: "↓"
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: root.downSpeed[0]
                    color: root.colorText
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                Text {
                    visible: root.connected
                    text: root.downSpeed[1]
                    color: root.colorSecondary
                    font.pixelSize: root.labelSize
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
                // 无活跃网卡连接：置灰提示未连接，CPU 与内存仍正常刷新
                Text {
                    visible: root.dataValid && !root.networkConnected
                    text: qsTr("Disconnected")
                    color: root.colorDisabled
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    Layout.alignment: Qt.AlignBottom
                }
            }
        }

        // ── 展开态：左列 CPU/内存 双行 + 右列 ↑/↓ 网速双行（宽度充足时）──
        RowLayout {
            visible: !root.compact && !root.narrow
            Layout.alignment: Qt.AlignVCenter
            spacing: 14

            // 左列：CPU / 内存
            ColumnLayout {
                spacing: 2

                RowLayout {
                    spacing: 3
                    Text {
                        text: qsTr("CPU")
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: qsTr("%1%").arg(root.dataValid ? String(root.cpuUsage) : "--")
                        color: root.dataValid ? root.stateColor(root.cpuState) : root.colorDisabled
                        font.pixelSize: root.valueSize
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                }

                RowLayout {
                    spacing: 3
                    Text {
                        text: qsTr("MEM")
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: qsTr("%1%").arg(root.dataValid ? String(root.memoryUsage) : "--")
                        color: root.dataValid ? root.stateColor(root.memState) : root.colorDisabled
                        font.pixelSize: root.valueSize
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                }
            }

            // 右列：↑ 上行 / ↓ 下行（按 B/s、K/s、M/s、G/s 自动换算）
            ColumnLayout {
                spacing: 2

                RowLayout {
                    visible: root.connected
                    spacing: 2
                    Text {
                        text: "↑"
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: root.upSpeed[0]
                        color: root.colorText
                        font.pixelSize: root.valueSize
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: root.upSpeed[1]
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                }

                RowLayout {
                    visible: root.connected
                    spacing: 2
                    Text {
                        text: "↓"
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: root.downSpeed[0]
                        color: root.colorText
                        font.pixelSize: root.valueSize
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                    Text {
                        text: root.downSpeed[1]
                        color: root.colorSecondary
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignBottom
                    }
                }

                // 无活跃网卡连接：右列置灰提示，左列 CPU 与内存仍正常刷新
                RowLayout {
                    visible: root.dataValid && !root.networkConnected
                    Layout.fillHeight: true
                    spacing: 3
                    Text {
                        text: qsTr("Network")
                        color: root.colorDisabled
                        font.pixelSize: root.labelSize
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Text {
                        text: qsTr("Disconnected")
                        color: root.colorDisabled
                        font.pixelSize: root.valueSize
                        font.weight: Font.Medium
                        renderType: Text.NativeRendering
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                // 采集服务异常或超时：降级展示占位符 --
                Text {
                    visible: !root.dataValid
                    text: "--"
                    color: root.colorDisabled
                    font.pixelSize: root.valueSize
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillHeight: true
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (systemMonitorController)
                systemMonitorController.openSystemMonitor()
        }
    }
}
