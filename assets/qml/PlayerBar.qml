import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: parent ? parent.width : 1200
    height: 90
    color: "#0a0a0c"
    
    // Top border line
    Rectangle {
        width: parent.width
        height: 1
        anchors.top: parent.top
        color: "#1a1a1a"
    }

    property string iconFont: "Material Symbols Rounded"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 1. LEFT SECTION (Track Info)
        MouseArea {
            id: leftSectionArea
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: PlayerCtrl.clickLeftSection()
            
            RowLayout {
                anchors.fill: parent
                spacing: 16
                
                // Artwork
                Rectangle {
                    width: 56
                    height: 56
                    radius: 8
                    color: "#1a1a1a"
                    clip: true
                    
                    Image {
                        anchors.fill: parent
                        source: PlayerCtrl.thumbnail !== "" ? PlayerCtrl.thumbnail : ""
                        fillMode: Image.PreserveAspectCrop
                        visible: PlayerCtrl.thumbnail !== ""
                    }
                    
                    // Fallback Icon
                    Text {
                        anchors.centerIn: parent
                        text: "music_note"
                        font.family: root.iconFont
                        font.pixelSize: 24
                        color: "#4d4d4d"
                        visible: PlayerCtrl.thumbnail === ""
                    }
                }
                
                // Text
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text: PlayerCtrl.title !== "" ? PlayerCtrl.title : "No Track"
                        color: leftSectionArea.containsMouse ? "#8b5cf6" : "white"
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: PlayerCtrl.artist !== "" ? PlayerCtrl.artist : "Unknown Artist"
                        color: "#b3b3b3"
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // 2. CENTER SECTION (Controls + Progress)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            
            // Buttons
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 24
                
                Button { 
                    text: "shuffle"
                    onClicked: PlayerCtrl.toggleShuffle()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 20; color: PlayerCtrl.shuffleOn ? "#8b5cf6" : (parent.hovered ? "white" : "#b3b3b3") } 
                }
                Button { 
                    text: "skip_previous"
                    onClicked: PlayerCtrl.previous()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 28; color: parent.hovered ? "white" : "#b3b3b3" } 
                }
                Button {
                    id: playBtn
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    text: PlayerCtrl.isPlaying ? "pause_circle" : "play_circle"
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 40; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Item {}
                    onClicked: PlayerCtrl.togglePlayPause()
                }
                Button { 
                    text: "skip_next"
                    onClicked: PlayerCtrl.next()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 28; color: parent.hovered ? "white" : "#b3b3b3" } 
                }
                Button { 
                    text: "repeat"
                    onClicked: PlayerCtrl.cycleRepeat()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 20; color: PlayerCtrl.repeatMode > 0 ? "#8b5cf6" : (parent.hovered ? "white" : "#b3b3b3") } 
                    // To do: Show 'repeat_one' if mode == 2
                }
            }
            
            // Progress Bar
            RowLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: 600
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                
                Text { text: formatTime(PlayerCtrl.positionMs); color: "#b3b3b3"; font.pixelSize: 12; Layout.minimumWidth: 40; horizontalAlignment: Text.AlignRight }
                
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: PlayerCtrl.durationMs > 0 ? PlayerCtrl.durationMs : 100
                    value: PlayerCtrl.positionMs
                    onMoved: PlayerCtrl.seek(value)
                    
                    background: Rectangle {
                        x: parent.leftPadding
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: parent.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#4d4d4d"

                        Rectangle {
                            width: parent.parent.visualPosition * parent.width
                            height: parent.height
                            color: parent.parent.hovered ? "#8b5cf6" : "white"
                            radius: 2
                        }
                    }
                    handle: Rectangle {
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width) - width/2
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 12
                        implicitHeight: 12
                        radius: 6
                        color: "white"
                        visible: parent.hovered || parent.pressed
                    }
                }

                Text { text: formatTime(PlayerCtrl.durationMs); color: "#b3b3b3"; font.pixelSize: 12; Layout.minimumWidth: 40 }
            }
        }
        
        // 3. RIGHT SECTION (Volume)
        RowLayout {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: 12
            
            Item { Layout.fillWidth: true } // spacer
            
            Text {
                text: PlayerCtrl.volumeValue === 0 ? "volume_off" : (PlayerCtrl.volumeValue < 50 ? "volume_down" : "volume_up")
                font.family: root.iconFont
                font.pixelSize: 20
                color: "#b3b3b3"
            }
            
            Slider {
                Layout.preferredWidth: 100
                from: 0
                to: 100
                value: PlayerCtrl.volumeValue
                onMoved: PlayerCtrl.setVolume(value)
                
                background: Rectangle {
                    x: parent.leftPadding
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 100
                    implicitHeight: 4
                    width: parent.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#4d4d4d"

                    Rectangle {
                        width: parent.parent.visualPosition * parent.width
                        height: parent.height
                        color: parent.parent.hovered ? "#8b5cf6" : "white"
                        radius: 2
                    }
                }
                handle: Rectangle {
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width) - width/2
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 12
                    implicitHeight: 12
                    radius: 6
                    color: "white"
                    visible: parent.hovered || parent.pressed
                }
            }
        }
    }

    function formatTime(ms) {
        if (ms <= 0) return "0:00";
        let totalSeconds = Math.floor(ms / 1000);
        let minutes = Math.floor(totalSeconds / 60);
        let seconds = totalSeconds % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }
}
