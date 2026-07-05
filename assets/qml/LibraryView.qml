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
        spacing: 0

        // Header / Tab Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                spacing: 24

                Text {
                    text: "Tu Biblioteca"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    Layout.rightMargin: 16
                }

                Repeater {
                    model: [
                        { key: "playlists", label: "Playlists" },
                        { key: "songs", label: "Canciones" },
                        { key: "albums", label: "Álbumes" },
                        { key: "artists", label: "Artistas" },
                        { key: "shows", label: "Podcasts" }
                    ]
                    
                    delegate: Rectangle {
                        Layout.preferredHeight: 32
                        Layout.preferredWidth: tabText.width + 32
                        radius: 16
                        color: LibraryCtrl.activeTab === modelData.key ? "white" : (tabMouseArea.containsMouse ? "#2a2a2d" : "transparent")
                        
                        Text {
                            id: tabText
                            anchors.centerIn: parent
                            text: modelData.label
                            color: LibraryCtrl.activeTab === modelData.key ? "black" : "white"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        MouseArea {
                            id: tabMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: LibraryCtrl.requestTabChange(modelData.key)
                        }
                    }
                }
                
                Item { Layout.fillWidth: true } // Spacer

                // Create Playlist Button
                Button {
                    visible: LibraryCtrl.activeTab === "playlists" && LibraryCtrl.authenticated
                    background: Rectangle {
                        color: parent.hovered ? "#3a3a3d" : "#2a2a2d"
                        radius: 20
                    }
                    contentItem: RowLayout {
                        spacing: 8
                        Text {
                            text: "add"
                            font.family: root.iconFont
                            font.pixelSize: 20
                            color: "white"
                        }
                        Text {
                            text: "Crear"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: "white"
                        }
                    }
                    Layout.preferredHeight: 40
                    padding: 16
                    onClicked: LibraryCtrl.requestCreatePlaylist()
                }
            }
        }

        // Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Unauthenticated State
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 16
                visible: !LibraryCtrl.authenticated
                
                Text {
                    text: "Inicia sesión para ver tu biblioteca"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 48
                    background: Rectangle {
                        color: "white"
                        radius: 24
                    }
                    contentItem: Text {
                        text: "INICIAR SESIÓN"
                        color: "black"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    leftPadding: 32
                    rightPadding: 32
                    onClicked: LibraryCtrl.requestLogin()
                }
            }

            // Loading / Empty States
            ColumnLayout {
                anchors.centerIn: parent
                visible: LibraryCtrl.authenticated && (LibraryCtrl.libraryState === "loading" || LibraryCtrl.libraryState === "empty")
                
                Text {
                    text: LibraryCtrl.libraryState === "loading" ? "Cargando..." : LibraryCtrl.stateMessage
                    color: "#a0a0a0"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Playlists Grid
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: LibraryCtrl.authenticated && LibraryCtrl.activeTab === "playlists" && LibraryCtrl.libraryState === "content"
                
                GridView {
                    anchors.fill: parent
                    cellWidth: 200
                    cellHeight: 280
                    model: LibraryCtrl.playlists
                    
                    delegate: HomeCard {
                        itemId: modelData.id
                        itemType: "playlist"
                        title: modelData.title
                        subtitle: "Playlist • " + modelData.author
                        thumbnail: modelData.thumbnail
                        
                        onPlayClicked: LibraryCtrl.requestPlaylist(modelData.id)
                    }
                }
            }

            // Albums Grid
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: LibraryCtrl.authenticated && LibraryCtrl.activeTab === "albums" && LibraryCtrl.libraryState === "content"
                
                GridView {
                    anchors.fill: parent
                    cellWidth: 200
                    cellHeight: 280
                    model: LibraryCtrl.albums
                    
                    delegate: HomeCard {
                        itemId: modelData.id
                        itemType: "album"
                        title: modelData.title
                        subtitle: "Álbum • " + modelData.artist
                        thumbnail: modelData.thumbnail
                        
                        onPlayClicked: LibraryCtrl.requestAlbum(modelData.id)
                    }
                }
            }

            // Artists Grid
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: LibraryCtrl.authenticated && LibraryCtrl.activeTab === "artists" && LibraryCtrl.libraryState === "content"
                
                GridView {
                    anchors.fill: parent
                    cellWidth: 200
                    cellHeight: 280
                    model: LibraryCtrl.artists
                    
                    delegate: HomeCard {
                        itemId: modelData.id
                        itemType: "artist"
                        title: modelData.name
                        subtitle: "Artista"
                        thumbnail: modelData.thumbnail
                        
                        onPlayClicked: LibraryCtrl.requestArtist(modelData.id)
                    }
                }
            }

            // Shows Grid
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: LibraryCtrl.authenticated && LibraryCtrl.activeTab === "shows" && LibraryCtrl.libraryState === "content"
                
                GridView {
                    anchors.fill: parent
                    cellWidth: 200
                    cellHeight: 280
                    model: LibraryCtrl.shows
                    
                    delegate: HomeCard {
                        itemId: modelData.id
                        itemType: "show"
                        title: modelData.title
                        subtitle: "Podcast • " + modelData.author
                        thumbnail: modelData.thumbnail
                        
                        onPlayClicked: LibraryCtrl.requestShow(modelData.id)
                    }
                }
            }

            // Songs List
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: LibraryCtrl.authenticated && LibraryCtrl.activeTab === "songs" && LibraryCtrl.libraryState === "content"
                
                ListView {
                    anchors.fill: parent
                    model: LibraryCtrl.songs
                    spacing: 8
                    
                    delegate: TrackRow {
                        trackId: modelData.id
                        trackTitle: modelData.title
                        trackArtist: modelData.artist
                        trackAlbum: modelData.album
                        trackThumbnail: modelData.thumbnail
                        trackDuration: modelData.duration
                        trackIndex: index + 1
                        mode: "playlist" // Use playlist mode for thumbnail
                        isPlaying: false
                        
                        onPlayClicked: LibraryCtrl.requestPlay(modelData.id)
                        
                        onContextMenuClicked: {
                            LibraryCtrl.requestRemoveFavorite(modelData.id)
                        }
                    }
                }
            }
        }
    }
}
