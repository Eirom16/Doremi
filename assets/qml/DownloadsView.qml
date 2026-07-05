import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0a0a0c"

    property string iconFont: "Material Symbols Rounded"
    
    // Connect to progress signals
    Connections {
        target: DownloadsCtrl
        function onProgressUpdated(id, percent, status) {
            for (var i = 0; i < listView.contentItem.children.length; ++i) {
                var item = listView.contentItem.children[i];
                if (item && item.itemId === id) {
                    item.updateProgress(percent, status);
                }
            }
        }
        function onBatchProgressUpdated(id, total, completed, percent) {
            for (var i = 0; i < listView.contentItem.children.length; ++i) {
                var item = listView.contentItem.children[i];
                if (item && item.itemId === id) {
                    item.updateBatchProgress(total, completed, percent);
                }
            }
        }
    }

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
                    text: "Descargas"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                }

                // Filter Buttons
                RowLayout {
                    spacing: 8

                    Repeater {
                        model: [
                            { key: "all", label: "Todo" },
                            { key: "track", label: "Canciones" },
                            { key: "album", label: "Álbumes" },
                            { key: "playlist", label: "Playlists" }
                        ]

                        delegate: Button {
                            Layout.preferredHeight: 32
                            background: Rectangle {
                                color: DownloadsCtrl.activeTab === modelData.key ? "white" : (parent.hovered ? "#3a3a3d" : "#2a2a2d")
                                radius: 16
                            }
                            contentItem: Text {
                                text: modelData.label
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: DownloadsCtrl.activeTab === modelData.key ? "black" : "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            leftPadding: 16
                            rightPadding: 16
                            onClicked: DownloadsCtrl.requestTabChange(modelData.key)
                        }
                    }
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
                visible: DownloadsCtrl.downloads.length === 0
                spacing: 16
                
                Text {
                    text: "download"
                    font.family: root.iconFont
                    font.pixelSize: 64
                    color: "#a0a0a0"
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "No tienes descargas"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "Descarga música para escucharla sin conexión."
                    color: "#a0a0a0"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // List of Downloads
            ScrollView {
                anchors.fill: parent
                anchors.margins: 40
                anchors.topMargin: 16
                contentWidth: availableWidth
                clip: true
                visible: DownloadsCtrl.downloads.length > 0
                
                ListView {
                    id: listView
                    anchors.fill: parent
                    model: DownloadsCtrl.downloads
                    spacing: 16
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 72
                        color: hoverArea.containsMouse ? "#1a1a1a" : "transparent"
                        radius: 8
                        
                        visible: DownloadsCtrl.activeTab === "all" || DownloadsCtrl.activeTab === modelData.type
                        property string itemId: modelData.id
                        
                        function updateProgress(percent, status) {
                            progressText.text = status;
                            progressBar.value = percent;
                            statusIcon.text = percent >= 1.0 ? "check_circle" : "downloading";
                            statusIcon.color = percent >= 1.0 ? "#4CAF50" : "#2196F3";
                        }
                        
                        function updateBatchProgress(total, completed, percent) {
                            progressText.text = completed + " / " + total + " completados";
                            progressBar.value = percent;
                            statusIcon.text = percent >= 1.0 ? "check_circle" : "downloading";
                            statusIcon.color = percent >= 1.0 ? "#4CAF50" : "#2196F3";
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 16
                            
                            Rectangle {
                                Layout.preferredWidth: 56
                                Layout.preferredHeight: 56
                                radius: 4
                                color: "#2a2a2d"
                                
                                Image {
                                    anchors.fill: parent
                                    source: modelData.thumbnail || ""
                                    fillMode: Image.PreserveAspectCrop
                                    layer.enabled: true
                                    layer.effect: OpacityMask {
                                        maskSource: Rectangle { width: 56; height: 56; radius: 4 }
                                    }
                                }
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                
                                Text {
                                    text: modelData.title
                                    color: "white"
                                    font.pixelSize: 16
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                
                                Text {
                                    id: progressText
                                    text: modelData.status || modelData.author
                                    color: "#a0a0a0"
                                    font.pixelSize: 14
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                
                                ProgressBar {
                                    id: progressBar
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 4
                                    value: modelData.progress
                                    visible: value > 0 && value < 1.0
                                    
                                    background: Rectangle {
                                        color: "#2a2a2d"
                                        radius: 2
                                    }
                                    contentItem: Item {
                                        Rectangle {
                                            width: progressBar.visualPosition * parent.width
                                            height: parent.height
                                            color: "#2196F3"
                                            radius: 2
                                        }
                                    }
                                }
                            }
                            
                            Text {
                                id: statusIcon
                                text: modelData.progress >= 1.0 ? "check_circle" : (modelData.progress > 0 ? "downloading" : "pending")
                                color: modelData.progress >= 1.0 ? "#4CAF50" : (modelData.progress > 0 ? "#2196F3" : "#a0a0a0")
                                font.family: root.iconFont
                                font.pixelSize: 24
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                        
                        MouseArea {
                            id: hoverArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (modelData.type === "track") DownloadsCtrl.requestPlay(modelData.id)
                                else if (modelData.type === "album") DownloadsCtrl.requestAlbum(modelData.id)
                                else if (modelData.type === "playlist") DownloadsCtrl.requestPlaylist(modelData.id)
                                else if (modelData.type === "show") DownloadsCtrl.requestShow(modelData.id)
                            }
                        }
                    }
                }
            }
        }
    }
}
