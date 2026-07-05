import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0a0a0c"

    property string iconFont: "Material Symbols Rounded"

    ColumnLayout {
        anchors.fill: parent
        spacing: 20
        Layout.margins: 40

        Item { Layout.fillHeight: true }

        // Logo
        Text {
            text: "album"
            font.family: root.iconFont
            font.pixelSize: 80
            color: "#8b5cf6" // Accent color
            Layout.alignment: Qt.AlignHCenter
        }

        // Title
        Text {
            text: "Doremi"
            color: "white"
            font.pixelSize: 48
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Tu música, a tu manera"
            color: "#a0a0a0"
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
        }
        
        Item { Layout.preferredHeight: 40 }

        // Card Panel
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 380
            Layout.preferredHeight: 300
            color: "#18181a"
            radius: 16
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 32
                spacing: 16

                Text {
                    text: "Bienvenido a Doremi"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Inicia sesión con YouTube Music para\nsincronizar tu biblioteca y favoritos."
                    color: "#a0a0a0"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { Layout.fillHeight: true }

                Button {
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 50
                    Layout.alignment: Qt.AlignHCenter
                    enabled: !WelcomeCtrl.isLoggingIn
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#a78bfa" : "#8b5cf6") : "#2a2a2d"
                        radius: 25
                    }
                    
                    contentItem: Text {
                        text: WelcomeCtrl.isLoggingIn ? "Iniciando sesión..." : "Iniciar Sesión"
                        color: parent.enabled ? "black" : "#a0a0a0"
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: WelcomeCtrl.requestLogin()
                }

                Text {
                    text: WelcomeCtrl.statusText
                    color: WelcomeCtrl.isSuccess ? "#8b5cf6" : "#a0a0a0"
                    font.pixelSize: 14
                    visible: WelcomeCtrl.statusText !== ""
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
