import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0a0a0c"

    property string iconFont: "Material Symbols Rounded"

    // Request stats from Rust via bridge.rs on_stats_requested
    // The days are requested by clicking the buttons. Default is 7.

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 24

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Estadísticas de Escucha"
                color: "white"
                font.pixelSize: 32
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "JSON"
                font.bold: true
                background: Rectangle {
                    color: parent.hovered ? "#33ffffff" : "#1a1a1a"
                    radius: 8
                    border.color: "#33ffffff"
                    border.width: 1
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: StatsCtrl.exportStatsAsJson()
            }

            Button {
                text: "CSV"
                font.bold: true
                background: Rectangle {
                    color: parent.hovered ? "#33ffffff" : "#1a1a1a"
                    radius: 8
                    border.color: "#33ffffff"
                    border.width: 1
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: StatsCtrl.exportStatsAsCsv()
            }
        }

        // Time Range Selector
        RowLayout {
            spacing: 8
            
            Repeater {
                model: [
                    { label: "Últimos 7 Días", days: 7 },
                    { label: "Últimos 30 Días", days: 30 },
                    { label: "Último Año", days: 365 },
                    { label: "Histórico", days: -1 }
                ]

                Button {
                    property bool isChecked: daysGroup.checkedDays === modelData.days
                    
                    background: Rectangle {
                        color: parent.isChecked ? "#8b5cf6" : (parent.hovered ? "#33ffffff" : "#1a1a1a")
                        radius: 16
                    }
                    contentItem: Text {
                        text: modelData.label
                        color: parent.isChecked ? "black" : "white"
                        font.bold: parent.isChecked
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    padding: 12
                    
                    onClicked: {
                        daysGroup.checkedDays = modelData.days
                        StatsCtrl.requestStats(modelData.days)
                    }
                }
            }

            Item { Layout.fillWidth: true } // spacer
        }

        QtObject {
            id: daysGroup
            property int checkedDays: 7
        }

        // Summary Cards
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            
            // Time Played
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#18181a"
                radius: 12
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text {
                        text: "schedule"
                        font.family: root.iconFont
                        color: "#8b5cf6"
                        font.pixelSize: 28
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: StatsCtrl.summary.totalPlayTime || "0m"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 24
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: "Tiempo Reproducido"
                        color: "#a0a0a0"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // Total Plays
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#18181a"
                radius: 12
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text {
                        text: "play_arrow"
                        font.family: root.iconFont
                        color: "#8b5cf6"
                        font.pixelSize: 28
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: StatsCtrl.summary.totalPlays !== undefined ? StatsCtrl.summary.totalPlays : "0"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 24
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: "Reproducciones Totales"
                        color: "#a0a0a0"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // Unique Artists
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#18181a"
                radius: 12
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Text {
                        text: "person"
                        font.family: root.iconFont
                        color: "#8b5cf6"
                        font.pixelSize: 28
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: StatsCtrl.summary.uniqueArtists !== undefined ? StatsCtrl.summary.uniqueArtists : "0"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 24
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: "Artistas Distintos"
                        color: "#a0a0a0"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // Top Tracks List
        Text {
            text: "Canciones Más Escuchadas"
            color: "white"
            font.pixelSize: 20
            font.bold: true
            Layout.topMargin: 16
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#18181a"
            radius: 12
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: 16
                model: StatsCtrl.topTracks
                spacing: 8
                
                delegate: TrackRow {
                    trackId: modelData.id
                    trackTitle: modelData.title
                    trackArtist: modelData.artist
                    trackAlbum: modelData.album
                    trackThumbnail: modelData.thumbnail
                    trackDuration: modelData.duration
                    trackIndex: index + 1
                    mode: "playlist" // to show thumbnail
                    isPlaying: false
                    
                    // Display plays count on the right
                    Item {
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        width: 80
                        height: parent.height
                        
                        Text {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.plays + " reprods"
                            color: "#a0a0a0"
                            font.pixelSize: 12
                        }
                    }

                    onPlayClicked: StatsCtrl.requestPlay(modelData.id)
                }

                Text {
                    visible: parent.count === 0
                    anchors.centerIn: parent
                    text: "No hay suficientes datos para este periodo."
                    color: "#a0a0a0"
                    font.pixelSize: 14
                }
            }
        }
    }
}
