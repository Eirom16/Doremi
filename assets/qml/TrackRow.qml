import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 56
    color: "transparent"
    radius: 6

    property string trackId: ""
    property string trackTitle: ""
    property string trackArtist: ""
    property string trackAlbum: ""
    property string trackDuration: ""
    property string trackThumbnail: ""
    property int trackIndex: -1
    property string mode: "playlist" // "album", "playlist", "artist", "history"
    
    // For now playing highlight
    property bool isPlaying: false

    property string iconFont: "Material Symbols Rounded"

    signal playClicked()
    signal contextMenuClicked()
    signal favoriteClicked()

    HoverHandler {
        id: hoverHandler
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                root.contextMenuClicked();
            } else {
                root.playClicked();
            }
        }
        onDoubleClicked: {
            root.playClicked();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: hoverHandler.hovered ? "#1a1a1a" : (isPlaying ? "#1a1db954" : "transparent")
        radius: 6
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 16

            // Left Section: Index or Thumbnail
            Item {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40

                // Album mode: Show number or Play icon on hover
                Text {
                    anchors.centerIn: parent
                    text: hoverHandler.hovered ? "play_arrow" : (root.trackIndex > 0 ? root.trackIndex.toString() : "")
                    font.family: root.iconFont
                    font.pixelSize: hoverHandler.hovered ? 24 : 16
                    color: hoverHandler.hovered ? "white" : (isPlaying ? "#8b5cf6" : "#a0a0a0")
                    visible: root.mode === "album"
                }

                // Playlist/Artist/History mode: Show thumbnail with play overlay
                Rectangle {
                    anchors.fill: parent
                    radius: 4
                    color: "#2a2a2d"
                    visible: root.mode !== "album"
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: root.trackThumbnail !== "" ? root.trackThumbnail : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "black"
                        opacity: hoverHandler.hovered ? 0.5 : 0.0
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "play_arrow"
                        font.family: root.iconFont
                        font.pixelSize: 24
                        color: "white"
                        visible: hoverHandler.hovered
                    }
                }
            }

            // Title & Artist Column
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: root.trackTitle
                    color: isPlaying ? "#8b5cf6" : "white"
                    font.pixelSize: 15
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: root.mode === "artist" ? root.trackAlbum : root.trackArtist
                    color: "#a0a0a0"
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            // Album (Only for playlist mode)
            Text {
                text: root.trackAlbum
                color: "#a0a0a0"
                font.pixelSize: 14
                elide: Text.ElideRight
                Layout.preferredWidth: 200
                Layout.fillWidth: true
                visible: root.mode === "playlist"
            }

            // Favorite Button
            Button {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                visible: hoverHandler.hovered || isPlaying
                background: Item {}
                contentItem: Text {
                    text: "favorite"
                    font.family: root.iconFont
                    font.pixelSize: 20
                    color: parent.hovered ? "white" : "#a0a0a0"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.favoriteClicked()
            }

            // Duration
            Text {
                text: root.trackDuration
                color: "#a0a0a0"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
