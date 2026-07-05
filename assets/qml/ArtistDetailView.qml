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

                // Dynamic background gradient based on artist avatar
                Rectangle {
                    anchors.fill: parent
                    opacity: 0.3
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: ArtistCtrl.dominantColor !== "" ? ArtistCtrl.dominantColor : "#1a1a1a" }
                        GradientStop { position: 1.0; color: "#0a0a0c" }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 40
                    spacing: 32

                    // Artist Avatar
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 240
                        Layout.alignment: Qt.AlignBottom
                        color: "#18181a"
                        radius: 120 // Circular for artist
                        
                        visible: ArtistCtrl.viewState !== "loading"
                        
                        Image {
                            id: artistImage
                            anchors.fill: parent
                            source: ArtistCtrl.artistAvatar !== "" ? ArtistCtrl.artistAvatar : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    width: 240
                                    height: 240
                                    radius: 120
                                }
                            }
                        }
                    }
                    
                    // Avatar Skeleton
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 240
                        Layout.alignment: Qt.AlignBottom
                        color: "#18181a"
                        radius: 120
                        visible: ArtistCtrl.viewState === "loading"
                    }

                    // Metadata
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        spacing: 8

                        Text {
                            text: "Artista"
                            color: "white"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        Text {
                            text: ArtistCtrl.viewState === "loading" ? "Cargando..." : ArtistCtrl.artistName
                            color: "white"
                            font.pixelSize: 64
                            font.bold: true
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
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
                    
                    // Play Button
                    Rectangle {
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 56
                        radius: 28
                        color: playMouseArea.containsMouse ? "#a78bfa" : "#8b5cf6"
                        scale: playMouseArea.containsMouse ? 1.05 : 1.0
                        
                        Behavior on scale { NumberAnimation { duration: 150 } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "play_arrow"
                            font.family: root.iconFont
                            font.pixelSize: 32
                            color: "black"
                        }
                        
                        MouseArea {
                            id: playMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (ArtistCtrl.topTracks.length > 0) {
                                    ArtistCtrl.requestPlay(0)
                                }
                            }
                        }
                    }
                    
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

            // Top Tracks
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 40
                Layout.topMargin: 0
                spacing: 16
                
                Text {
                    text: "Populares"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                // Actual Tracks
                Repeater {
                    model: ArtistCtrl.topTracks
                    delegate: TrackRow {
                        trackId: modelData.id
                        trackTitle: modelData.title
                        trackArtist: modelData.artist
                        trackAlbum: modelData.album
                        trackThumbnail: modelData.thumbnail
                        trackDuration: modelData.duration
                        trackIndex: index + 1
                        mode: "artist"
                        isPlaying: false
                        
                        onPlayClicked: ArtistCtrl.requestPlay(index)
                    }
                }
                
                // Loading Skeletons
                Repeater {
                    model: 5
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: "transparent"
                        visible: ArtistCtrl.viewState === "loading"
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            spacing: 16
                            
                            Rectangle { Layout.preferredWidth: 40; Layout.preferredHeight: 40; color: "#2a2a2d"; radius: 4 }
                            ColumnLayout {
                                spacing: 8
                                Rectangle { Layout.preferredWidth: 200; Layout.preferredHeight: 14; color: "#2a2a2d"; radius: 4 }
                            }
                        }
                    }
                }
            }
            
            // Albums Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 40
                spacing: 16
                visible: ArtistCtrl.albums.length > 0
                
                Text {
                    text: "Discografía"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                GridView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.ceil(ArtistCtrl.albums.length / 5) * 280
                    cellWidth: 200
                    cellHeight: 280
                    interactive: false // Handled by outer ScrollView
                    model: ArtistCtrl.albums
                    
                    delegate: HomeCard {
                        itemId: modelData.id
                        itemType: "album"
                        title: modelData.title
                        subtitle: modelData.type
                        thumbnail: modelData.thumbnail
                        
                        onPlayClicked: ArtistCtrl.requestAlbum(modelData.id)
                    }
                }
            }

            Item { Layout.preferredHeight: 120 } // Bottom padding
        }
    }
}
