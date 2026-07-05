import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: parent ? parent.width : 1300
    height: parent ? parent.height : 820
    color: "#0a0a0c"

    // Material Icons Font helper
    property string iconFont: "Material Symbols Rounded"

    // 1) Opaque background to hide the underlying app UI
    Rectangle {
        anchors.fill: parent
        color: "#0a0a0c"
    }

    // 2) Dynamic gradient based on dominant colors
    Rectangle {
        anchors.fill: parent
        opacity: 0.45
        gradient: Gradient {
            GradientStop { 
                position: 0.0
                color: (NowPlayingCtrl.dominantColors && NowPlayingCtrl.dominantColors.length > 0) ? NowPlayingCtrl.dominantColors[0] : "#1a1a1a"
            }
            GradientStop { 
                position: 1.0
                color: "#0a0a0c" 
            }
        }
    }

    // Top Left Close button
    Button {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 24
        z: 10
        width: 48
        height: 48
        background: Rectangle {
            color: parent.hovered ? "#33ffffff" : "transparent"
            radius: 24
        }
        contentItem: Text {
            text: "expand_more"
            font.family: root.iconFont
            font.pixelSize: 32
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: NowPlayingCtrl.closeView()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 60

        // LEFT COLUMN (Artwork & Controls)
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.45
            spacing: 24

            Item { Layout.fillHeight: true } // Spacer

            // Album Art
            Rectangle {
                id: albumArtContainer
                Layout.preferredWidth: Math.min(parent.width, parent.height) * 0.7
                Layout.preferredHeight: Layout.preferredWidth
                Layout.alignment: Qt.AlignHCenter
                radius: 16
                color: "#1a1a1a"
                clip: true
                
                Image {
                    anchors.fill: parent
                    source: NowPlayingCtrl.artworkUrl
                    fillMode: Image.PreserveAspectCrop
                }

                SequentialAnimation on y {
                    loops: Animation.Infinite
                    running: NowPlayingCtrl.isPlaying
                    NumberAnimation { to: albumArtContainer.y - 8; duration: 3000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: albumArtContainer.y; duration: 3000; easing.type: Easing.InOutSine }
                }
            }

            // Metadata & Actions (Like, Download)
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width * 0.9
                spacing: 16
                
                Item { Layout.fillWidth: true } // Spacer
                
                Button {
                    text: "favorite"
                    font.family: root.iconFont
                    font.pixelSize: 28
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; color: parent.pressed ? "#8b5cf6" : "white"; font.pixelSize: 28; verticalAlignment: Text.AlignVCenter }
                    onClicked: NowPlayingCtrl.toggleLike()
                }

                ColumnLayout {
                    spacing: 8
                    Text {
                        text: NowPlayingCtrl.title !== "" ? NowPlayingCtrl.title : "No Track"
                        color: "white"
                        font.pixelSize: 32
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                        elide: Text.ElideRight
                        Layout.maximumWidth: root.width * 0.35
                    }
                    Text {
                        text: NowPlayingCtrl.artist !== "" ? NowPlayingCtrl.artist : "Unknown Artist"
                        color: "#b3b3b3"
                        font.pixelSize: 18
                        Layout.alignment: Qt.AlignHCenter
                        elide: Text.ElideRight
                        Layout.maximumWidth: root.width * 0.35
                    }
                }

                Button {
                    text: "download"
                    font.family: root.iconFont
                    font.pixelSize: 28
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; color: parent.pressed ? "#8b5cf6" : "white"; font.pixelSize: 28; verticalAlignment: Text.AlignVCenter }
                    onClicked: NowPlayingCtrl.downloadCurrent()
                }
                
                Item { Layout.fillWidth: true } // Spacer
            }

            // Progress Bar
            RowLayout {
                Layout.preferredWidth: parent.width * 0.9
                Layout.alignment: Qt.AlignHCenter
                spacing: 16

                Text { text: formatTime(NowPlayingCtrl.positionMs); color: "#b3b3b3"; font.pixelSize: 14 }
                
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: NowPlayingCtrl.durationMs > 0 ? NowPlayingCtrl.durationMs : 100
                    value: NowPlayingCtrl.positionMs
                    onMoved: NowPlayingCtrl.seek(value)
                    
                    background: Rectangle {
                        x: parent.leftPadding
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 6
                        width: parent.availableWidth
                        height: implicitHeight
                        radius: 3
                        color: "#4d4d4d"

                        Rectangle {
                            width: parent.parent.visualPosition * parent.width
                            height: parent.height
                            color: parent.parent.hovered ? "#8b5cf6" : "white"
                            radius: 3
                        }
                    }
                    handle: Rectangle {
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 14
                        implicitHeight: 14
                        radius: 7
                        color: parent.pressed ? "#8b5cf6" : "white"
                        visible: parent.hovered || parent.pressed
                    }
                }

                Text { text: formatTime(NowPlayingCtrl.durationMs); color: "#b3b3b3"; font.pixelSize: 14 }
            }

            // Controls
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 32
                Layout.topMargin: 8

                Button { 
                    text: "shuffle"
                    onClicked: NowPlayingCtrl.toggleShuffle()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 28; color: NowPlayingCtrl.shuffleOn ? "#8b5cf6" : "white" } 
                }
                Button { 
                    text: "skip_previous"
                    onClicked: NowPlayingCtrl.previous()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 36; color: "white" } 
                }
                Button { 
                    text: NowPlayingCtrl.isPlaying ? "pause" : "play_arrow"
                    onClicked: NowPlayingCtrl.togglePlayPause()
                    background: Rectangle { color: parent.pressed ? "#e6e6e6" : "white"; radius: 32; implicitWidth: 64; implicitHeight: 64 }
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 36; color: "black"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button { 
                    text: "skip_next"
                    onClicked: NowPlayingCtrl.next()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 36; color: "white" } 
                }
                Button { 
                    text: "repeat"
                    onClicked: NowPlayingCtrl.cycleRepeat()
                    background: Item{}
                    contentItem: Text { text: parent.text; font.family: root.iconFont; font.pixelSize: 28; color: NowPlayingCtrl.repeatMode > 0 ? "#8b5cf6" : "white" } 
                }
            }

            Item { Layout.fillHeight: true } // Spacer
        }

        // RIGHT COLUMN (Tabs)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.55
            color: "#1a2a2a35"
            radius: 24
            border.color: "#33ffffff"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 20

                TabBar {
                    id: rightTabs
                    Layout.fillWidth: true
                    background: Rectangle { color: "transparent" }
                    
                    TabButton { 
                        text: "Cola"
                        contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#80ffffff"; font.bold: parent.checked; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "transparent"; Rectangle { width: parent.width; height: 2; anchors.bottom: parent.bottom; color: parent.parent.checked ? "white" : "transparent" } }
                    }
                    TabButton { 
                        text: "Letras"
                        contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#80ffffff"; font.bold: parent.checked; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "transparent"; Rectangle { width: parent.width; height: 2; anchors.bottom: parent.bottom; color: parent.parent.checked ? "white" : "transparent" } }
                    }
                    TabButton { 
                        text: "Similares"
                        contentItem: Text { text: parent.text; color: parent.checked ? "white" : "#80ffffff"; font.bold: parent.checked; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: "transparent"; Rectangle { width: parent.width; height: 2; anchors.bottom: parent.bottom; color: parent.parent.checked ? "white" : "transparent" } }
                    }
                }

                StackLayout {
                    currentIndex: rightTabs.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Cola
                    ListView {
                        id: queueList
                        model: NowPlayingCtrl.queue
                        clip: true
                        spacing: 8
                        ScrollBar.vertical: ScrollBar { active: true }
                        
                        delegate: Rectangle {
                            width: ListView.view.width - 16
                            height: 64
                            color: queueMouse.containsMouse ? "#2affffff" : (index === NowPlayingCtrl.currentIndex ? "#1a1db954" : "transparent")
                            radius: 8
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 16
                                Image {
                                    source: modelData.thumbnail
                                    Layout.preferredWidth: 48
                                    Layout.preferredHeight: 48
                                    fillMode: Image.PreserveAspectCrop
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.title; color: index === NowPlayingCtrl.currentIndex ? "#8b5cf6" : "white"; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: modelData.artist; color: "#b3b3b3"; elide: Text.ElideRight; Layout.fillWidth: true }
                                }
                                
                                // Action buttons for queue
                                RowLayout {
                                    visible: queueMouse.containsMouse
                                    spacing: 8
                                    Layout.rightMargin: 16
                                    
                                    Button {
                                        text: "delete"
                                        font.family: root.iconFont
                                        font.pixelSize: 20
                                        background: Rectangle { color: "transparent"; radius: 16; implicitWidth: 32; implicitHeight: 32 }
                                        contentItem: Text { text: parent.text; font.family: root.iconFont; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: NowPlayingCtrl.removeQueueItem(index)
                                    }
                                }

                                Text {
                                    visible: !queueMouse.containsMouse
                                    text: formatTime(modelData.duration_ms)
                                    color: "#b3b3b3"
                                    Layout.rightMargin: 16
                                }
                            }
                            MouseArea {
                                id: queueMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: NowPlayingCtrl.playQueueItem(index)
                            }
                        }
                    }

                    // Letras
                    ScrollView {
                        clip: true
                        ScrollBar.vertical: ScrollBar { active: true }
                        
                        TextArea {
                            readOnly: true
                            text: NowPlayingCtrl.plainLyrics !== "" ? NowPlayingCtrl.plainLyrics : "Letras no disponibles"
                            color: "white"
                            font.pixelSize: 26
                            font.bold: true
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            background: Item {} // Transparent background
                            
                            // Center align vertically somewhat if short
                            topPadding: 40
                            bottomPadding: 40
                        }
                    }

                    // Similares
                    ListView {
                        id: relatedList
                        model: NowPlayingCtrl.relatedTracks
                        clip: true
                        spacing: 8
                        ScrollBar.vertical: ScrollBar { active: true }

                        delegate: Rectangle {
                            width: ListView.view.width - 16
                            height: 64
                            color: relatedMouse.containsMouse ? "#2affffff" : "transparent"
                            radius: 8
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 16
                                Image {
                                    source: modelData.thumbnail
                                    Layout.preferredWidth: 48
                                    Layout.preferredHeight: 48
                                    fillMode: Image.PreserveAspectCrop
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.title; color: "white"; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                    Text { text: modelData.artist; color: "#b3b3b3"; elide: Text.ElideRight; Layout.fillWidth: true }
                                }
                                
                                // Buttons for related actions
                                RowLayout {
                                    visible: relatedMouse.containsMouse
                                    spacing: 8
                                    Layout.rightMargin: 16
                                    
                                    Button {
                                        text: "play_arrow"
                                        font.family: root.iconFont
                                        font.pixelSize: 20
                                        background: Rectangle { color: "#33ffffff"; radius: 16; implicitWidth: 32; implicitHeight: 32 }
                                        contentItem: Text { text: parent.text; font.family: root.iconFont; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: NowPlayingCtrl.playRelated(index)
                                    }
                                    Button {
                                        text: "add"
                                        font.family: root.iconFont
                                        font.pixelSize: 24
                                        background: Rectangle { color: "transparent"; radius: 16; implicitWidth: 32; implicitHeight: 32; border.color: "#80ffffff" }
                                        contentItem: Text { text: parent.text; font.family: root.iconFont; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: NowPlayingCtrl.queueRelated(index)
                                    }
                                }
                            }
                            MouseArea {
                                id: relatedMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                // Play on click is good UX too
                                onClicked: NowPlayingCtrl.playRelated(index)
                            }
                        }
                    }
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
