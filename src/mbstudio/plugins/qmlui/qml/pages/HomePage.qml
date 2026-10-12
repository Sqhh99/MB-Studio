// Welcome page: quick actions, recent files and installation summary.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ScrollView {
    id: page
    signal openRequested()
    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: page.availableWidth
        spacing: 20

        // Hero
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pagePadding
            Layout.rightMargin: Theme.pagePadding
            Layout.topMargin: Theme.pagePadding
            implicitHeight: 176
            radius: Theme.radius + 4
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.dark ? "#0F3554" : "#1E88E5" }
                GradientStop { position: 1.0; color: Theme.dark ? "#0B5563" : "#1FB6C9" }
            }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 24
                Image {
                    source: "qrc:/mbstudio/images/mbstudio.png"
                    Layout.preferredWidth: 112
                    Layout.preferredHeight: 112
                    sourceSize: Qt.size(224, 224)
                    smooth: true
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "MB-Studio"
                        color: "white"
                        font.pixelSize: 32
                        font.weight: Font.Bold
                    }
                    Text {
                        text: qsTr("Processing and visualisation of swath sonar and seafloor mapping data")
                        color: "#DDEFFB"
                        font.pixelSize: 15
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Text {
                        text: qsTr("Version %1  ·  MB-System %2").arg(App.version).arg(App.mbsystemVersion)
                        color: "#B9DAF0"
                        font.pixelSize: Theme.fontBody
                    }
                    RowLayout {
                        Layout.topMargin: 8
                        spacing: 10
                        PrimaryButton {
                            text: qsTr("Open data…")
                            fill: "white"
                            textColor: "#0F3554"
                            onClicked: page.openRequested()
                        }
                        PrimaryButton {
                            text: qsTr("Browse programs")
                            fill: Qt.rgba(1, 1, 1, 0.16)
                            textColor: "white"
                            onClicked: App.currentPage = AppController.ToolsPage
                        }
                    }
                }
            }
        }

        // Quick actions
        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pagePadding
            Layout.rightMargin: Theme.pagePadding
            columns: page.availableWidth > 1100 ? 4 : 2
            columnSpacing: 14
            rowSpacing: 14
            Repeater {
                model: [
                    { icon: Icons.waves, title: qsTr("Inspect swath data"), text: qsTr("Statistics, navigation track and depth profile of a sonar file."), page: AppController.InspectPage },
                    { icon: Icons.terrain, title: qsTr("View bathymetry grids"), text: qsTr("Colour shaded relief of GMT grids made with mbgrid or mbmosaic."), page: AppController.GridPage },
                    { icon: Icons.code, title: qsTr("Run MB-System programs"), text: qsTr("All MB-System command line programs with live output."), page: AppController.ToolsPage },
                    { icon: Icons.list, title: qsTr("Supported formats"), text: qsTr("Browse the swath data formats MB-System can read and write."), page: AppController.FormatsPage }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 128
                    radius: Theme.radius
                    color: hover.containsMouse ? Theme.surfaceHover : Theme.surface
                    border.color: hover.containsMouse ? Theme.accent : Theme.border
                    Behavior on color { ColorAnimation { duration: 120 } }
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        Rectangle {
                            width: 38; height: 38; radius: 10
                            color: Theme.accentSoft
                            Icon { anchors.centerIn: parent; path: modelData.icon; color: Theme.accent; size: 22 }
                        }
                        Text { text: modelData.title; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
                        Text {
                            text: modelData.text
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: App.currentPage = modelData.page
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pagePadding
            Layout.rightMargin: Theme.pagePadding
            Layout.bottomMargin: Theme.pagePadding
            spacing: 14

            // Recent files
            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                Layout.preferredHeight: 320
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    RowLayout {
                        SectionTitle { text: qsTr("Recent files"); iconPath: Icons.file; Layout.fillWidth: true }
                        IconButton {
                            iconPath: Icons.sweep
                            tip: qsTr("Clear the list")
                            visible: App.recentFiles.length > 0
                            onClicked: App.clearRecentFiles()
                        }
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: App.recentFiles
                        visible: count > 0
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            height: 46
                            contentItem: RowLayout {
                                spacing: 12
                                Icon {
                                    path: App.isGridFile(modelData) ? Icons.terrain : Icons.waves
                                    color: Theme.accent
                                    size: 18
                                }
                                ColumnLayout {
                                    spacing: 0
                                    Layout.fillWidth: true
                                    Text {
                                        text: modelData.replace(/^.*[\\\/]/, "")
                                        color: Theme.text
                                        font.pixelSize: Theme.fontBody
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData
                                        color: Theme.textFaint
                                        font.pixelSize: Theme.fontSmall
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            onClicked: App.openFile(modelData)
                        }
                    }
                    Text {
                        visible: App.recentFiles.length === 0
                        text: qsTr("Files you open appear here. You can also drop files onto the window.")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // Installation summary
            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.preferredHeight: 320
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    SectionTitle { text: qsTr("Installation"); iconPath: Icons.info }
                    GridLayout {
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 8
                        Layout.fillWidth: true
                        Repeater {
                            model: [
                                [qsTr("MB-Studio"), App.version],
                                [qsTr("MB-System"), App.mbsystemVersion],
                                [qsTr("Platform"), App.platformName],
                                [qsTr("Plugins"), App.plugins.filter(p => p.state === "active").length + " / " + App.plugins.length + qsTr(" active")],
                                [qsTr("Programs"), App.tools.availableCount + " / " + App.tools.count + qsTr(" installed")],
                                [qsTr("Data formats"), App.formats.totalCount]
                            ]
                            delegate: Item {
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                implicitHeight: 22
                                Text { text: modelData[0]; color: Theme.textMuted; font.pixelSize: Theme.fontBody }
                                Text {
                                    anchors.right: parent.right
                                    width: parent.width - 110
                                    horizontalAlignment: Text.AlignRight
                                    text: modelData[1]
                                    color: Theme.text
                                    font.pixelSize: Theme.fontBody
                                    elide: Text.ElideMiddle
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Button {
                        text: qsTr("MB-System documentation")
                        flat: true
                        onClicked: App.openDocumentation()
                    }
                }
            }
        }
    }
}
