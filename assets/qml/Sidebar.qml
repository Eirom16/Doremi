import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: SidebarCtrl.isCompact ? 72 : 210
    height: parent ? parent.height : 800
    color: "#0a0a0c"

    property string iconFont: "Material Symbols Rounded"

    // Line separator on the right
    Rectangle {
        width: 1
        height: parent.height
        anchors.right: parent.right
        color: "#1a1a1a"
    }

    // List Model for Nav Items
    ListModel {
        id: navModel
        ListElement { routeId: "home"; iconName: "home"; label: "Inicio" }
        ListElement { routeId: "trending"; iconName: "trending_up"; label: "Explorar" }
        ListElement { routeId: "library"; iconName: "library_music"; label: "Biblioteca" }
        ListElement { routeId: "history"; iconName: "history"; label: "Historial" }
        ListElement { routeId: "downloads"; iconName: "download"; label: "Descargas" }
        ListElement { routeId: "stats"; iconName: "bar_chart"; label: "Estadísticas" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 24
        anchors.bottomMargin: 16
        spacing: 4

        Repeater {
            model: navModel
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.margins: SidebarCtrl.isCompact ? 4 : 8
                radius: 8
                
                property bool isActive: SidebarCtrl.activeRoute === model.routeId
                
                color: navMouse.containsMouse ? (isActive ? "#331db954" : "#1a1a1a") : (isActive ? "#1a1db954" : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: SidebarCtrl.isCompact ? 0 : 16
                    spacing: 16
                    
                    Item { Layout.fillWidth: true; visible: SidebarCtrl.isCompact } // Center icon when compact
                    
                    Text {
                        text: model.iconName
                        font.family: root.iconFont
                        font.pixelSize: 24
                        color: parent.parent.isActive ? "#8b5cf6" : (navMouse.containsMouse ? "white" : "#b3b3b3")
                    }
                    
                    Text {
                        text: model.label
                        color: parent.parent.isActive ? "white" : (navMouse.containsMouse ? "white" : "#b3b3b3")
                        font.pixelSize: 14
                        font.bold: parent.parent.isActive
                        visible: !SidebarCtrl.isCompact
                        Layout.fillWidth: true
                    }
                    
                    Item { Layout.fillWidth: true; visible: SidebarCtrl.isCompact }
                }

                MouseArea {
                    id: navMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: SidebarCtrl.navigate(model.routeId)
                }
            }
        }

        Item { Layout.fillHeight: true } // Spacer
    }
}
