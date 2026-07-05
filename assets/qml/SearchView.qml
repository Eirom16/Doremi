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

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                anchors.topMargin: 16
                spacing: 16

                Text {
                    text: SearchCtrl.currentQuery === "" ? "Buscar" : 'Resultados de "' + SearchCtrl.currentQuery + '"'
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                }

                // Filter Buttons
                RowLayout {
                    spacing: 8
                    visible: SearchCtrl.currentQuery !== ""

                    Repeater {
                        model: [
                            { key: "all", label: "Todo" },
                            { key: "songs", label: "Canciones" },
                            { key: "videos", label: "Videos" },
                            { key: "artists", label: "Artistas" },
                            { key: "albums", label: "Álbumes" },
                            { key: "playlists", label: "Playlists" },
                            { key: "shows", label: "Podcasts" }
                        ]

                        delegate: Button {
                            Layout.preferredHeight: 32
                            background: Rectangle {
                                color: SearchCtrl.activeFilter === modelData.key ? "white" : (parent.hovered ? "#3a3a3d" : "#2a2a2d")
                                radius: 16
                            }
                            contentItem: Text {
                                text: modelData.label
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: SearchCtrl.activeFilter === modelData.key ? "black" : "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            leftPadding: 16
                            rightPadding: 16
                            onClicked: SearchCtrl.requestFilterChange(modelData.key)
                        }
                    }
                }
            }
        }

        // Content Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 32
                
                Item { Layout.preferredHeight: 16 } // Top padding

                // Recent Searches
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 40
                    spacing: 16
                    visible: SearchCtrl.showingRecent && SearchCtrl.recentSearches.length > 0

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Búsquedas recientes"
                            color: "white"
                            font.pixelSize: 20
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        
                        Text {
                            text: "Borrar"
                            color: "#a0a0a0"
                            font.pixelSize: 14
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: SearchCtrl.requestClearSearchHistory()
                            }
                        }
                    }

                    Repeater {
                        model: SearchCtrl.recentSearches
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            color: hoverArea.containsMouse ? "#1a1a1a" : "transparent"
                            radius: 8
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 16
                                
                                Text {
                                    text: "history"
                                    font.family: root.iconFont
                                    font.pixelSize: 24
                                    color: "#a0a0a0"
                                }
                                
                                Text {
                                    text: modelData
                                    color: "white"
                                    font.pixelSize: 16
                                    Layout.fillWidth: true
                                }
                                
                                Text {
                                    text: "close"
                                    font.family: root.iconFont
                                    font.pixelSize: 20
                                    color: "#a0a0a0"
                                    visible: hoverArea.containsMouse
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -8
                                        onClicked: SearchCtrl.requestDeleteSearchHistory(modelData)
                                    }
                                }
                            }
                            
                            MouseArea {
                                id: hoverArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: SearchCtrl.requestSearch(modelData)
                            }
                        }
                    }
                }

                // Top Result & Top Tracks
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 40
                    spacing: 24
                    visible: SearchCtrl.hasTopResult && SearchCtrl.activeFilter === "all"
                    
                    // Top Result Card
                    ColumnLayout {
                        Layout.preferredWidth: 400
                        Layout.alignment: Qt.AlignTop
                        spacing: 16
                        
                        Text {
                            text: "Resultado principal"
                            color: "white"
                            font.pixelSize: 24
                            font.bold: true
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 240
                            color: topHover.containsMouse ? "#2a2a2d" : "#18181a"
                            radius: 8
                            
                            Behavior on color { ColorAnimation { duration: 150 } }
                            
                            MouseArea {
                                id: topHover
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: SearchCtrl.requestTopResult()
                            }
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 92
                                    radius: SearchCtrl.topResult.type === "artist" ? 46 : 8
                                    color: "#2a2a2d"
                                    
                                    Image {
                                        anchors.fill: parent
                                        source: SearchCtrl.topResult.thumbnail || ""
                                        fillMode: Image.PreserveAspectCrop
                                        layer.enabled: true
                                        layer.effect: OpacityMask {
                                            maskSource: Rectangle {
                                                width: 92
                                                height: 92
                                                radius: SearchCtrl.topResult.type === "artist" ? 46 : 8
                                            }
                                        }
                                    }
                                }
                                
                                Text {
                                    text: SearchCtrl.topResult.title || ""
                                    color: "white"
                                    font.pixelSize: 32
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                
                                RowLayout {
                                    spacing: 8
                                    Text {
                                        text: SearchCtrl.topResult.subtitle || ""
                                        color: "#a0a0a0"
                                        font.pixelSize: 14
                                    }
                                }
                            }
                        }
                    }
                    
                    // Top Tracks of Top Result removed because API doesn't provide tracks directly on TopResult
                }

                // General Songs Section
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: 40
                    spacing: 16
                    visible: SearchCtrl.songs.length > 0 && (SearchCtrl.activeFilter === "all" || SearchCtrl.activeFilter === "songs")
                    
                    Text {
                        text: "Canciones"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        visible: SearchCtrl.activeFilter === "all"
                    }
                    
                    Repeater {
                        model: SearchCtrl.activeFilter === "all" ? SearchCtrl.songs.slice(0, 4) : SearchCtrl.songs
                        delegate: TrackRow {
                            trackId: modelData.id
                            trackTitle: modelData.title
                            trackArtist: modelData.artist
                            trackAlbum: modelData.album
                            trackThumbnail: modelData.thumbnail
                            trackDuration: modelData.duration
                            trackIndex: index + 1
                            mode: "playlist" // show thumbnail
                            isPlaying: false
                            
                            onPlayClicked: SearchCtrl.requestPlayTrack(modelData.id)
                        }
                    }
                }

                // Generic Grid Section Component
                Component {
                    id: gridSection
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.margins: 40
                        spacing: 16
                        
                        property string titleText
                        property var sectionModel
                        property string cardType
                        property bool showSection: sectionModel.length > 0
                        visible: showSection
                        
                        Text {
                            text: titleText
                            color: "white"
                            font.pixelSize: 24
                            font.bold: true
                        }
                        
                        GridView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.ceil(sectionModel.length / 5) * 280
                            cellWidth: 200
                            cellHeight: 280
                            interactive: false
                            model: sectionModel
                            
                            delegate: HomeCard {
                                itemId: modelData.id
                                itemType: gridSection.cardType
                                title: modelData.title || modelData.name
                                subtitle: gridSection.cardType === "artist" ? "Artista" : (modelData.author || modelData.artist || "")
                                thumbnail: modelData.thumbnail
                                
                                onPlayClicked: {
                                    if (gridSection.cardType === "album") SearchCtrl.requestAlbum(modelData.id)
                                    else if (gridSection.cardType === "artist") SearchCtrl.requestArtist(modelData.id)
                                    else if (gridSection.cardType === "playlist") SearchCtrl.requestPlaylist(modelData.id)
                                    else if (gridSection.cardType === "show") SearchCtrl.requestShow(modelData.id)
                                }
                            }
                        }
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: gridSection
                    visible: SearchCtrl.activeFilter === "all" || SearchCtrl.activeFilter === "artists"
                    onLoaded: {
                        item.titleText = "Artistas"
                        item.sectionModel = SearchCtrl.artists
                        item.cardType = "artist"
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: gridSection
                    visible: SearchCtrl.activeFilter === "all" || SearchCtrl.activeFilter === "albums"
                    onLoaded: {
                        item.titleText = "Álbumes"
                        item.sectionModel = SearchCtrl.albums
                        item.cardType = "album"
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: gridSection
                    visible: SearchCtrl.activeFilter === "all" || SearchCtrl.activeFilter === "playlists"
                    onLoaded: {
                        item.titleText = "Playlists"
                        item.sectionModel = SearchCtrl.playlists
                        item.cardType = "playlist"
                    }
                }
                
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: gridSection
                    visible: SearchCtrl.activeFilter === "all" || SearchCtrl.activeFilter === "shows"
                    onLoaded: {
                        item.titleText = "Podcasts"
                        item.sectionModel = SearchCtrl.shows
                        item.cardType = "show"
                    }
                }

                Item { Layout.preferredHeight: 120 } // Bottom padding
            }
        }
    }
}
