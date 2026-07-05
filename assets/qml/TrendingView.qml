import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0a0a0c"
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 24
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        
        ColumnLayout {
            width: root.width - 48
            spacing: 32
            
            Text {
                text: "Tendencias Globales"
                color: "white"
                font.bold: true
                font.pixelSize: 32
                Layout.topMargin: 16
                Layout.bottomMargin: 16
            }
            
            // Loading State (Skeletons)
            GridView {
                Layout.fillWidth: true
                Layout.preferredHeight: 600
                cellWidth: 196
                cellHeight: 266
                visible: TrendingCtrl.viewState === "loading"
                model: 15
                
                delegate: Rectangle {
                    width: 180
                    height: 250
                    color: "#18181a"
                    radius: 12
                    
                    Rectangle {
                        y: 16
                        width: 148
                        height: 148
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: "#2a2a2d"
                        radius: 8
                    }
                    
                    Rectangle {
                        y: 180
                        width: 120
                        height: 16
                        x: 16
                        color: "#2a2a2d"
                        radius: 4
                    }
                    
                    Rectangle {
                        y: 204
                        width: 80
                        height: 12
                        x: 16
                        color: "#2a2a2d"
                        radius: 4
                    }
                }
            }
            
            // Error State
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: TrendingCtrl.viewState === "error"
                spacing: 16
                
                Text {
                    text: "Error al cargar las tendencias."
                    color: "#a0a0a0"
                    font.pixelSize: 18
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Button {
                    text: "Reintentar"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: TrendingCtrl.requestRetry()
                }
            }
            
            // Content State (Grid)
            GridView {
                id: grid
                Layout.fillWidth: true
                Layout.preferredHeight: Math.ceil(TrendingCtrl.items.length / Math.floor((root.width - 48) / 196)) * 266
                cellWidth: 196
                cellHeight: 266
                visible: TrendingCtrl.viewState === "content"
                model: TrendingCtrl.items
                clip: true
                
                delegate: HomeCard {
                    itemId: modelData.id
                    title: modelData.title
                    subtitle: modelData.subtitle
                    thumbnail: modelData.thumbnail
                    itemType: modelData.itemType
                    
                    onCardClicked: {
                        if (itemType === "song") {
                            TrendingCtrl.requestPlay(itemId, itemType, title, subtitle, thumbnail);
                        } else {
                            TrendingCtrl.requestNavigate(itemId, itemType);
                        }
                    }
                    
                    onPlayClicked: {
                        TrendingCtrl.requestPlay(itemId, itemType, title, subtitle, thumbnail);
                    }
                }
            }
            
            Item { Layout.preferredHeight: 120 } // Bottom padding for PlayerBar
        }
    }
}
