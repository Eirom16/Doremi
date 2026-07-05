import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0a0a0c"

    property string iconFont: "Material Symbols Rounded"

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        
        ColumnLayout {
            width: root.width
            spacing: 0

            // Hero Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 340
                color: "transparent"

                // Dynamic background gradient based on show cover
                Rectangle {
                    anchors.fill: parent
                    opacity: 0.3
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: ShowCtrl.dominantColor !== "" ? ShowCtrl.dominantColor : "#1a1a1a" }
                        GradientStop { position: 1.0; color: "#0a0a0c" }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 40
                    spacing: 32

                    // Show Cover
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 240
                        Layout.alignment: Qt.AlignBottom
                        color: "#18181a"
                        radius: 12
                        
                        visible: ShowCtrl.viewState !== "loading"
                        
                        Image {
                            id: showImage
                            anchors.fill: parent
                            source: ShowCtrl.showCover !== "" ? ShowCtrl.showCover : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    width: 240
                                    height: 240
                                    radius: 12
                                }
                            }
                        }
                    }
                    
                    // Cover Skeleton
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 240
                        Layout.alignment: Qt.AlignBottom
                        color: "#18181a"
                        radius: 12
                        visible: ShowCtrl.viewState === "loading"
                    }

                    // Metadata
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        spacing: 8

                        Text {
                            text: "Podcast"
                            color: "white"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        Text {
                            text: ShowCtrl.viewState === "loading" ? "Cargando..." : ShowCtrl.showTitle
                            color: "white"
                            font.pixelSize: 48
                            font.bold: true
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        
                        Text {
                            text: ShowCtrl.showAuthor
                            color: "white"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }
                    }
                }
            }
            
            // Actions Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: "transparent"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    spacing: 24
                    
                    // Follow Button
                    Button {
                        Layout.preferredHeight: 36
                        background: Rectangle {
                            color: "transparent"
                            border.color: parent.hovered ? "white" : "#a0a0a0"
                            border.width: 1
                            radius: 18
                        }
                        contentItem: Text {
                            text: "SEGUIR"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: parent.hovered ? "white" : "#a0a0a0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        leftPadding: 16
                        rightPadding: 16
                    }
                    
                    Item { Layout.fillWidth: true } // Spacer
                }
            }
            
            // Description
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 40
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                visible: ShowCtrl.viewState !== "loading"
                
                Text {
                    text: "Acerca de"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                }
                
                Text {
                    text: ShowCtrl.showDescription
                    color: "#a0a0a0"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.maximumWidth: 800
                }
            }

            // Episodes List
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 40
                Layout.topMargin: 0
                spacing: 16
                
                Text {
                    text: "Episodios"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                // Actual Episodes
                Repeater {
                    model: ShowCtrl.episodes
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        color: hoverHandler.hovered ? "#1a1a1a" : "transparent"
                        radius: 8
                        
                        HoverHandler { id: hoverHandler }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: ShowCtrl.requestPlayEpisode(index)
                        }
                        
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 8
                            
                            Text {
                                text: modelData.title
                                color: "white"
                                font.pixelSize: 16
                                font.weight: Font.Bold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            
                            Text {
                                text: modelData.description
                                color: "#a0a0a0"
                                font.pixelSize: 14
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            
                            RowLayout {
                                spacing: 16
                                
                                // Play icon
                                Rectangle {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    radius: 16
                                    color: "white"
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "play_arrow"
                                        font.family: root.iconFont
                                        font.pixelSize: 20
                                        color: "black"
                                    }
                                }
                                
                                Text {
                                    text: modelData.publishedAt
                                    color: "white"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                }
                                
                                Text {
                                    text: modelData.duration
                                    color: "#a0a0a0"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 120 } // Bottom padding
        }
    }
}
