import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Rectangle {
    id: root
    width: 480
    height: 320
    color: "transparent"
    
    property string errorText: ""
    property bool isValidating: false
    
    Connections {
        target: SudoCtrl
        function onErrorOccurred(msg) {
            root.errorText = msg
            passwordInput.forceActiveFocus()
        }
        function onValidatingChanged(validating) {
            root.isValidating = validating
        }
    }

    Rectangle {
        id: dialogBg
        anchors.fill: parent
        anchors.margins: 16
        color: "#18181a" // Surface color
        radius: 12
        border.color: "#2a2a2a" // Border color
        border.width: 1

        MultiEffect {
            source: dialogBg
            anchors.fill: dialogBg
            shadowEnabled: true
            shadowBlur: 32
            shadowColor: "#80000000"
            shadowVerticalOffset: 8
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                
                Text {
                    text: "security"
                    font.family: "Material Symbols Rounded"
                    font.pixelSize: 32
                    color: "#8b5cf6" // Accent color
                }
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    
                    Text {
                        text: "Se requiere autenticación"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    
                    Text {
                        text: "Por favor, introduce tu contraseña."
                        color: "#b3b3b3"
                        font.pixelSize: 12
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: closeMouse.containsMouse ? "#33ffffff" : "transparent"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "close"
                        font.family: "Material Symbols Rounded"
                        font.pixelSize: 20
                        color: "#b3b3b3"
                    }
                    
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SudoCtrl.requestCancel()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2a2a2a"
            }

            Text {
                text: "Doremi necesita privilegios administrativos para realizar esta acción. Ingresa la contraseña de sudo."
                color: "#b3b3b3"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#0a0a0c"
                    radius: 6
                    border.color: passwordInput.activeFocus ? "#8b5cf6" : "#2a2a2a"
                    
                    TextInput {
                        id: passwordInput
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: "white"
                        font.pixelSize: 14
                        echoMode: visibilityBtn.visibleMode ? TextInput.Normal : TextInput.Password
                        clip: true
                        enabled: !root.isValidating
                        
                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Contraseña de sudo"
                            color: "#5a5a5a"
                            font.pixelSize: 14
                            visible: parent.text === "" && !parent.activeFocus
                        }
                        
                        Keys.onReturnPressed: SudoCtrl.requestAccept(passwordInput.text)
                    }
                }
                
                Rectangle {
                    id: visibilityBtn
                    width: 40
                    height: 40
                    radius: 6
                    color: visibilityMouse.containsMouse ? "#33ffffff" : "transparent"
                    
                    property bool visibleMode: false
                    
                    Text {
                        anchors.centerIn: parent
                        text: visibilityBtn.visibleMode ? "visibility_off" : "visibility"
                        font.family: "Material Symbols Rounded"
                        font.pixelSize: 20
                        color: "#b3b3b3"
                    }
                    
                    MouseArea {
                        id: visibilityMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: visibilityBtn.visibleMode = !visibilityBtn.visibleMode
                    }
                }
            }

            Text {
                text: root.errorText
                color: "#ff5252" // Error color
                font.pixelSize: 12
                visible: root.errorText !== ""
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            
            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                
                Item { Layout.fillWidth: true } // Spacer
                
                // Cancel button
                Rectangle {
                    width: 100
                    height: 36
                    radius: 18
                    color: cancelMouse.containsMouse ? "#33ffffff" : "transparent"
                    border.color: "#b3b3b3"
                    border.width: 1
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Cancelar"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SudoCtrl.requestCancel()
                        enabled: !root.isValidating
                    }
                }
                
                // Confirm button
                Rectangle {
                    width: 120
                    height: 36
                    radius: 18
                    color: confirmMouse.containsMouse ? "#a78bfa" : "#8b5cf6" // Accent / Accent bright
                    opacity: root.isValidating ? 0.5 : 1.0
                    
                    Text {
                        anchors.centerIn: parent
                        text: root.isValidating ? "Verificando..." : "Confirmar"
                        color: "#000000" // Text on accent
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    MouseArea {
                        id: confirmMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: SudoCtrl.requestAccept(passwordInput.text)
                        enabled: !root.isValidating
                    }
                }
            }
        }
    }
    
    Component.onCompleted: {
        passwordInput.forceActiveFocus()
    }
}
