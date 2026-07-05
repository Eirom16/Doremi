import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: windowRoot
    anchors.fill: parent

    Rectangle {
        id: root
        anchors.fill: parent
        color: "#18181a"
        radius: 16

        property string iconFont: "Material Symbols Rounded"
        property string currentAccent: SettingsCtrl.config.accent !== undefined ? SettingsCtrl.config.accent : "#8b5cf6"

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                width: root.width
                height: root.height
                radius: 16
            }
        }

        // Close button
        Rectangle {
            z: 99
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 16
            width: 36
            height: 36
            radius: 18
            color: closeHover.hovered ? "#33ffffff" : "transparent"

            HoverHandler { id: closeHover }
            TapHandler { onTapped: SettingsCtrl.closeDialog() }

            Text {
                anchors.centerIn: parent
                text: "close"
                font.family: root.iconFont
                font.pixelSize: 24
                color: closeHover.hovered ? "white" : "#a0a0a0"
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Sidebar
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 240
                color: "#1e1e20" // slightly lighter than main bg
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    anchors.topMargin: 24
                    spacing: 8

                    Text {
                        text: "Configuración"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        Layout.fillWidth: true
                        Layout.bottomMargin: 24
                        Layout.leftMargin: 8
                    }

                    Repeater {
                        model: [
                            { name: "Apariencia", icon: "palette" },
                            { name: "Reproducción", icon: "headphones" },
                            { name: "Subtítulos", icon: "subtitles" },
                            { name: "Descargas", icon: "download" },
                            { name: "Integraciones", icon: "hub" },
                            { name: "Almacenamiento", icon: "storage" }
                        ]
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: 8
                            color: stack.currentIndex === index ? "#33ffffff" : (hoverHandler.hovered ? "#1affffff" : "transparent")

                            HoverHandler { id: hoverHandler }
                            TapHandler {
                                onTapped: stack.currentIndex = index
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 16
                                
                                Text {
                                    text: modelData.icon
                                    font.family: root.iconFont
                                    font.pixelSize: 20
                                    color: stack.currentIndex === index ? root.currentAccent : "#a0a0a0"
                                }
                                
                                Text {
                                    text: modelData.name
                                    color: stack.currentIndex === index ? "white" : "#d0d0d0"
                                    font.pixelSize: 15
                                    font.bold: stack.currentIndex === index
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                }
            }

            // Main Content
            StackLayout {
                id: stack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: 0

                // 0: Apariencia
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Apariencia"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Tema"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            ComboBox {
                                model: ["light", "dark"]
                                currentIndex: SettingsCtrl.config.theme === "light" ? 0 : 1
                                onActivated: SettingsCtrl.updateSetting("theme", model[currentIndex])
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Color de Acento"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            ComboBox {
                                model: ["#8b5cf6", "#1db954", "#ff0000", "#0088ff", "#ff8800"]
                                currentIndex: model.indexOf(SettingsCtrl.config.accent) !== -1 ? model.indexOf(SettingsCtrl.config.accent) : 0
                                onActivated: SettingsCtrl.updateSetting("accent", model[currentIndex])
                            }
                        }
                        
                        Item { Layout.fillHeight: true }
                    }
                }

                // 1: Reproducción
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Reproducción"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Normalizar Volumen"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.normalize === true
                                onClicked: SettingsCtrl.updateSetting("normalize", checked)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Crossfade (Transición Suave)"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.crossfade === true
                                onClicked: SettingsCtrl.updateSetting("crossfade", checked)
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // 2: Subtítulos
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Subtítulos"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Alineación"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            ComboBox {
                                model: ["left", "center", "right"]
                                currentIndex: model.indexOf(SettingsCtrl.config.subtitleAlignment) !== -1 ? model.indexOf(SettingsCtrl.config.subtitleAlignment) : 1
                                onActivated: SettingsCtrl.updateSetting("subtitleAlignment", model[currentIndex])
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Scroll Automático"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.subtitleAutoScroll !== false
                                onClicked: SettingsCtrl.updateSetting("subtitleAutoScroll", checked)
                            }
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Efecto de Brillo"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.subtitleGlowEffect !== false
                                onClicked: SettingsCtrl.updateSetting("subtitleGlowEffect", checked)
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // 3: Descargas
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Descargas"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Formato Preferido"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            ComboBox {
                                model: ["mp3", "m4a", "original"]
                                currentIndex: model.indexOf(SettingsCtrl.config.downloadFormat) !== -1 ? model.indexOf(SettingsCtrl.config.downloadFormat) : 0
                                onActivated: SettingsCtrl.updateSetting("downloadFormat", model[currentIndex])
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Calidad"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            ComboBox {
                                model: ["best", "320k", "256k", "192k", "128k"]
                                currentIndex: model.indexOf(SettingsCtrl.config.downloadQuality) !== -1 ? model.indexOf(SettingsCtrl.config.downloadQuality) : 0
                                onActivated: SettingsCtrl.updateSetting("downloadQuality", model[currentIndex])
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // 4: Integraciones
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Integraciones"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Discord Rich Presence"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.discordRpc === true
                                onClicked: SettingsCtrl.updateSetting("discordRpc", checked)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Controles de Sistema (MPRIS)"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.mprisEnabled === true
                                onClicked: SettingsCtrl.updateSetting("mprisEnabled", checked)
                            }
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Last.fm Scrobbling"; color: "white"; font.pixelSize: 15; Layout.fillWidth: true }
                            Switch {
                                checked: SettingsCtrl.config.lastfmEnabled === true
                                onClicked: SettingsCtrl.updateSetting("lastfmEnabled", checked)
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // 5: Almacenamiento
                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: stack.width - 64
                        spacing: 24
                        anchors.margins: 32
                        anchors.topMargin: 32

                        Text { text: "Almacenamiento"; color: "white"; font.pixelSize: 24; font.bold: true; Layout.bottomMargin: 16 }
                        
                        Text { text: "Gestión de caché e historial local."; color: "#a0a0a0"; font.pixelSize: 14 }
                        
                        Button {
                            text: "Actualizar Tamaños"
                            onClicked: SettingsCtrl.refresh_storage_sizes()
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
