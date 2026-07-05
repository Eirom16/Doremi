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
                text: HomeCtrl.welcomeMessage
                color: "white"
                font.bold: true
                font.pixelSize: 32
                Layout.topMargin: 16
                Layout.bottomMargin: 16
            }
            
            // Loading State (Skeletons)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 32
                visible: HomeCtrl.viewState === "loading"
                
                Repeater {
                    model: 3
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 16
                        
                        // Skeleton Section Title
                        Rectangle {
                            width: 200
                            height: 24
                            color: "#333333"
                            radius: 4
                        }
                        
                        // Skeleton Cards
                        RowLayout {
                            spacing: 16
                            Repeater {
                                model: 5
                                Rectangle {
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
                        }
                    }
                }
            }
            
            // Error State
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: HomeCtrl.viewState === "error"
                spacing: 16
                
                Text {
                    text: "Error al cargar la página de inicio."
                    color: "#a0a0a0"
                    font.pixelSize: 18
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Button {
                    text: "Reintentar"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: HomeCtrl.requestRetry()
                }
            }
            
            // Content State (Sections)
            Repeater {
                model: HomeCtrl.viewState === "content" ? HomeCtrl.sections : 0
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    
                    Text {
                        text: modelData.title
                        color: "white"
                        font.bold: true
                        font.pixelSize: 24
                    }
                    
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 260
                        orientation: ListView.Horizontal
                        spacing: 16
                        clip: true
                        model: modelData.items
                        
                        delegate: HomeCard {
                            itemId: modelData.id
                            title: modelData.title
                            subtitle: modelData.subtitle
                            thumbnail: modelData.thumbnail
                            itemType: modelData.itemType
                            
                            onCardClicked: {
                                if (itemType === "song") {
                                    HomeCtrl.requestPlay(itemId, itemType, title, subtitle, thumbnail);
                                } else {
                                    HomeCtrl.requestNavigate(itemId, itemType, title, subtitle, thumbnail);
                                }
                            }
                            
                            onPlayClicked: {
                                HomeCtrl.requestPlay(itemId, itemType, title, subtitle, thumbnail);
                            }
                        }
                    }
                }
            }
            
            Item { Layout.preferredHeight: 120 } // Bottom padding for PlayerBar
        }
    }
}
