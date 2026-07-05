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

                // Dynamic background gradient based on playlist cover
                Rectangle {
                    anchors.fill: parent
                    opacity: 0.3
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: PlaylistCtrl.dominantColor !== "" ? PlaylistCtrl.dominantColor : "#1a1a1a" }
                        GradientStop { position: 1.0; color: "#0a0a0c" }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 40
                    spacing: 32

                    // Playlist Cover
                    Rectangle {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 240
                        Layout.alignment: Qt.AlignBottom
                        color: "#18181a"
                        radius: 12
                        
                        visible: PlaylistCtrl.viewState !== "loading"
                        
                        Image {
                            id: playlistImage
                            anchors.fill: parent
                            source: PlaylistCtrl.playlistCover !== "" ? PlaylistCtrl.playlistCover : ""
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
                        visible: PlaylistCtrl.viewState === "loading"
                    }

                    // Metadata
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        spacing: 8

                        Text {
                            text: "Playlist"
                            color: "white"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        Text {
                            text: PlaylistCtrl.viewState === "loading" ? "Cargando..." : PlaylistCtrl.playlistTitle
                            color: "white"
                            font.pixelSize: 48
                            font.bold: true
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Text {
                                text: PlaylistCtrl.playlistAuthor
                                color: "white"
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }
                            Text {
                                text: "•"
                                color: "#a0a0a0"
                                font.pixelSize: 14
                            }
                            Text {
                                text: PlaylistCtrl.playlistSongCount + " canciones"
                                color: "#a0a0a0"
                                font.pixelSize: 14
                            }
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
                            onClicked: PlaylistCtrl.requestPlayAll()
                        }
                    }
                    
                    // Shuffle Button
                    Button {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "shuffle"
                            font.family: root.iconFont
                            font.pixelSize: 32
                            color: parent.hovered ? "white" : "#a0a0a0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: PlaylistCtrl.requestShuffle()
                    }
                    
                    // Edit Button
                    Button {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "edit"
                            font.family: root.iconFont
                            font.pixelSize: 28
                            color: parent.hovered ? "white" : "#a0a0a0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: PlaylistCtrl.requestRename()
                    }

                    // Delete Button
                    Button {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "delete"
                            font.family: root.iconFont
                            font.pixelSize: 28
                            color: parent.hovered ? "#ff5252" : "#a0a0a0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: PlaylistCtrl.requestDelete()
                    }
                    
                    Item { Layout.fillWidth: true } // Spacer
                }
            }

            // Tracks List
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 40
                Layout.topMargin: 0
                spacing: 8
                
                // Track List Header
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 8
                    
                    Text {
                        text: "#"
                        color: "#a0a0a0"
                        font.pixelSize: 14
                        Layout.preferredWidth: 40
                        horizontalAlignment: Text.AlignHCenter
                    }
                    
                    Text {
                        text: "TÍTULO"
                        color: "#a0a0a0"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        Layout.fillWidth: true
                    }
                    
                    Text {
                        text: "ÁLBUM"
                        color: "#a0a0a0"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        Layout.preferredWidth: 200
                        Layout.fillWidth: true
                    }
                    
                    Text {
                        text: "schedule"
                        font.family: root.iconFont
                        color: "#a0a0a0"
                        font.pixelSize: 18
                        Layout.preferredWidth: 60
                        horizontalAlignment: Text.AlignRight
                    }
                }
                
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#2a2a2d"
                    Layout.bottomMargin: 16
                }
                
                // Loading Skeletons
                Repeater {
                    model: 10
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: "transparent"
                        visible: PlaylistCtrl.viewState === "loading"
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            spacing: 16
                            
                            Rectangle { Layout.preferredWidth: 40; Layout.preferredHeight: 40; color: "#2a2a2d"; radius: 4 }
                            ColumnLayout {
                                spacing: 8
                                Rectangle { Layout.preferredWidth: 200; Layout.preferredHeight: 14; color: "#2a2a2d"; radius: 4 }
                                Rectangle { Layout.preferredWidth: 100; Layout.preferredHeight: 10; color: "#2a2a2d"; radius: 4 }
                            }
                        }
                    }
                }
                
                // Actual Tracks
                Repeater {
                    model: PlaylistCtrl.tracks
                    delegate: TrackRow {
                        trackId: modelData.id
                        trackTitle: modelData.title
                        trackArtist: modelData.artist
                        trackAlbum: modelData.album
                        trackThumbnail: modelData.thumbnail
                        trackDuration: modelData.duration
                        trackIndex: index + 1
                        mode: "playlist"
                        isPlaying: false
                        
                        onPlayClicked: PlaylistCtrl.requestPlay(index)
                        onContextMenuClicked: menu.popup()

                        Menu {
                            id: menu
                            MenuItem {
                                text: "Eliminar de la playlist"
                                onClicked: PlaylistCtrl.requestRemoveTrack(index)
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 120 } // Bottom padding
        }
    }
}
