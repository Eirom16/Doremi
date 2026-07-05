import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 40
                anchors.rightMargin: 40
                
                Text {
                    text: "Historial de reproducción"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }

        // Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Empty State
            ColumnLayout {
                anchors.centerIn: parent
                visible: HistoryCtrl.history.length === 0
                spacing: 16
                
                Text {
                    text: "history"
                    font.family: root.iconFont
                    font.pixelSize: 64
                    color: "#a0a0a0"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "Aún no has reproducido nada"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "Tu historial de música aparecerá aquí."
                    color: "#a0a0a0"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // History List
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: HistoryCtrl.history.length > 0
                
                ListView {
                    anchors.fill: parent
                    model: HistoryCtrl.history
                    spacing: 8
                    
                    delegate: Item {
                        width: ListView.view.width
                        height: 56
                        
                        TrackRow {
                            anchors.fill: parent
                            trackId: modelData.id
                            trackTitle: modelData.title
                            trackArtist: modelData.artist
                            trackAlbum: modelData.album
                            trackThumbnail: modelData.thumbnail
                            trackDuration: modelData.duration
                            trackIndex: index + 1
                            mode: "playlist" // show thumbnail
                            isPlaying: false
                            
                            onPlayClicked: HistoryCtrl.requestPlay(modelData.id)
                        }
                        
                        // Overlay the 'playedAt' time at the right edge
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 100 // Leave space for duration/heart
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.playedAt
                            color: "#a0a0a0"
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
