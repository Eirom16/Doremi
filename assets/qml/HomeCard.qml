import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    width: itemType === "artist" ? 180 : 180
    height: itemType === "artist" ? 220 : 250
    color: "#18181a"
    radius: 12
    clip: true

    property string itemId: ""
    property string title: ""
    property string subtitle: ""
    property string thumbnail: ""
    property string itemType: "album" // song, album, artist, playlist, show, episode

    property string iconFont: "Material Symbols Rounded"

    signal playClicked()
    signal cardClicked()

    // Hover effect
    Behavior on color { ColorAnimation { duration: 200 } }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.cardClicked()
        onEntered: root.color = "#2a2a2d"
        onExited: root.color = "#18181a"
        cursorShape: Qt.PointingHandCursor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Thumbnail Container
        Item {
            Layout.preferredWidth: root.width - 32
            Layout.preferredHeight: Layout.preferredWidth
            Layout.alignment: Qt.AlignHCenter
            
            // Image mask for rounded corners or circle (if artist)
            Rectangle {
                id: imageMask
                anchors.fill: parent
                radius: root.itemType === "artist" ? width / 2 : 8
                color: "#2a2a2d"
                visible: false
            }

            Image {
                id: img
                anchors.fill: parent
                source: root.thumbnail.startsWith("/") ? "file://" + root.thumbnail : root.thumbnail
                fillMode: Image.PreserveAspectCrop
                visible: false
            }

            OpacityMask {
                anchors.fill: parent
                source: img
                maskSource: imageMask
            }

            // Play button overlay
            Rectangle {
                id: playOverlay
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: root.itemType === "artist" ? 0 : 8
                width: 48
                height: 48
                radius: 24
                color: "#8b5cf6"
                opacity: (mouseArea.containsMouse || playMouseArea.containsMouse) ? 1.0 : 0.0
                scale: opacity > 0 ? 1.0 : 0.8
                visible: root.itemType !== "artist" // Typically artists don't have play button directly on card in this design, but if they do, remove this.
                
                Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }

                Text {
                    anchors.centerIn: parent
                    text: "play_arrow"
                    font.family: root.iconFont
                    font.pixelSize: 28
                    color: "black"
                }

                MouseArea {
                    id: playMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.playClicked()
                    }
                }
            }
        }

        // Text Info
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            
            Text {
                text: root.title
                color: "white"
                font.bold: true
                font.pixelSize: 16
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: root.itemType === "artist" ? Text.AlignHCenter : Text.AlignLeft
            }
            
            Text {
                text: root.itemType === "song" ? "Canción • " + root.subtitle :
                      root.itemType === "album" ? "Álbum • " + root.subtitle :
                      root.itemType === "playlist" ? "Playlist • " + root.subtitle :
                      root.subtitle
                color: "#a0a0a0"
                font.pixelSize: 14
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: root.itemType === "artist" ? Text.AlignHCenter : Text.AlignLeft
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer to push text up
    }
}
