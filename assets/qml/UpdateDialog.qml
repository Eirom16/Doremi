import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: windowRoot
    anchors.fill: parent

    Rectangle {
        id: root
        anchors.fill: parent
        color: "#18181a" // Match dark theme popup
        radius: 12

        property string currentAccent: "#8b5cf6"
        property string iconFont: "Material Symbols Rounded"

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                width: root.width
                height: root.height
                radius: 12
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: Qt.rgba(139/255, 92/255, 246/255, 0.15) // dim purple

                    Text {
                        anchors.centerIn: parent
                        text: "new_releases"
                        font.family: root.iconFont
                        font.pixelSize: 28
                        color: root.currentAccent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: "Actualización Disponible"
                        color: "white"
                        font.pixelSize: 22
                        font.bold: true
                    }
                    Text {
                        text: "Versión " + UpdateCtrl.version
                        color: root.currentAccent
                        font.pixelSize: 14
                    }
                }

                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: closeHover.hovered ? "#33ffffff" : "transparent"
                    Layout.alignment: Qt.AlignTop

                    HoverHandler { id: closeHover }
                    TapHandler { onTapped: UpdateCtrl.requestClose() }

                    Text {
                        anchors.centerIn: parent
                        text: "close"
                        font.family: root.iconFont
                        font.pixelSize: 20
                        color: closeHover.hovered ? "white" : "#a0a0a0"
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2a2a2a"
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            // Release Notes
            Text {
                text: "Notas de la versión:"
                color: "#a0a0a0"
                font.pixelSize: 14
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1e1e20"
                radius: 8

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 12
                    clip: true

                    TextArea {
                        text: UpdateCtrl.notes
                        color: "white"
                        font.pixelSize: 14
                        readOnly: true
                        wrapMode: Text.WordWrap
                        background: null
                    }
                }
            }

            // Progress Area
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: UpdateCtrl.isDownloading || UpdateCtrl.isInstallSuccess || UpdateCtrl.isInstallFailed

                ProgressBar {
                    Layout.fillWidth: true
                    value: UpdateCtrl.downloadProgress
                    to: 100
                    background: Rectangle {
                        color: "#2a2a2a"
                        radius: 4
                        implicitHeight: 8
                    }
                    contentItem: Item {
                        implicitHeight: 8
                        Rectangle {
                            width: parent.parent.visualPosition * parent.width
                            height: parent.height
                            radius: 4
                            color: UpdateCtrl.isInstallFailed ? "#ef4444" : root.currentAccent
                        }
                    }
                }

                Text {
                    text: UpdateCtrl.statusMessage
                    color: UpdateCtrl.isInstallFailed ? "#ef4444" : "#a0a0a0"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                spacing: 12

                // Github Button
                Rectangle {
                    width: 44
                    height: 44
                    radius: 8
                    color: gitHover.hovered ? "#33ffffff" : "transparent"
                    border.color: "#33ffffff"
                    border.width: 1

                    HoverHandler { id: gitHover }
                    TapHandler { onTapped: UpdateCtrl.openGithub() }

                    Text {
                        anchors.centerIn: parent
                        text: "open_in_new"
                        font.family: root.iconFont
                        font.pixelSize: 20
                        color: "white"
                    }
                }

                Item { Layout.fillWidth: true } // spacer

                // Postpone Button
                Rectangle {
                    implicitWidth: 120
                    implicitHeight: 44
                    radius: 8
                    color: postponeHover.hovered ? "#33ffffff" : "transparent"
                    border.color: "#33ffffff"
                    border.width: 1
                    visible: !UpdateCtrl.isDownloading && !UpdateCtrl.isReadyToRestart

                    HoverHandler { id: postponeHover }
                    TapHandler { onTapped: UpdateCtrl.requestClose() }

                    Text {
                        anchors.centerIn: parent
                        text: "Posponer"
                        color: "white"
                        font.pixelSize: 15
                        font.bold: true
                    }
                }

                // Update / Restart Button
                Rectangle {
                    implicitWidth: 160
                    implicitHeight: 44
                    radius: 8
                    color: !enabled ? "#a0a0a0" : (updateHover.hovered ? "#a78bfa" : root.currentAccent)
                    enabled: !UpdateCtrl.isDownloading || UpdateCtrl.isReadyToRestart

                    HoverHandler { id: updateHover }
                    TapHandler {
                        onTapped: {
                            if (UpdateCtrl.isReadyToRestart) {
                                UpdateCtrl.requestRestart();
                            } else {
                                UpdateCtrl.requestDownload();
                            }
                        }
                    }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        
                        Text {
                            text: UpdateCtrl.isReadyToRestart ? "check_circle" : "download"
                            font.family: root.iconFont
                            font.pixelSize: 20
                            color: "white"
                        }
                        
                        Text {
                            text: UpdateCtrl.isReadyToRestart ? "Reiniciar Ahora" : "Actualizar"
                            color: "white"
                            font.pixelSize: 15
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
