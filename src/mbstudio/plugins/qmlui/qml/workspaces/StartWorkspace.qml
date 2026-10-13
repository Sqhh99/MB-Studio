// Start: create or open a project, recent projects and files, and the
// processing chain at a glance.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true
    readonly property var project: App.project

    ColumnLayout {
        width: page.availableWidth
        spacing: 20

        // hero
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
                    Text { text: "MB-Studio"; color: "white"; font.pixelSize: 32; font.weight: Font.Bold }
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
                            text: page.project.isOpen ? qsTr("Add survey data…") : qsTr("New project…")
                            fill: "white"
                            textColor: "#0F3554"
                            enabled: page.project.available
                            onClicked: App.trigger(page.project.isOpen ? "ui:addData" : "ui:newProject")
                        }
                        PrimaryButton {
                            text: qsTr("Open project…")
                            fill: Qt.rgba(1, 1, 1, 0.16)
                            textColor: "white"
                            enabled: page.project.available
                            onClicked: App.trigger("ui:openProject")
                        }
                        PrimaryButton {
                            text: qsTr("Open a file…")
                            fill: Qt.rgba(1, 1, 1, 0.16)
                            textColor: "white"
                            onClicked: App.trigger("ui:openFile")
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pagePadding
            Layout.rightMargin: Theme.pagePadding
            Layout.bottomMargin: Theme.pagePadding
            columns: page.availableWidth > 1000 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16

            // the processing chain
            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.rowSpan: 2
                implicitHeight: chain.implicitHeight + 36
                ColumnLayout {
                    id: chain
                    anchors.fill: parent
                    spacing: 6
                    SectionTitle { text: qsTr("From raw survey lines to a terrain model"); iconPath: Icons.route }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Every step runs the MB-System programs in the background, records the exact commands and adds the results to the project. Steps can be repeated with other settings at any time.")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall + 1
                        wrapMode: Text.WordWrap
                    }
                    WorkflowGuide {
                        Layout.fillWidth: true
                        enabled: page.project.isOpen
                        opacity: enabled ? 1 : 0.55
                        onStepClicked: (panel) => App.trigger(panel === "import" ? "ui:addData" : "ui:panel:" + panel)
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                implicitHeight: recentProjects.implicitHeight + 36
                ColumnLayout {
                    id: recentProjects
                    anchors.fill: parent
                    spacing: 4
                    SectionTitle { text: qsTr("Recent projects"); iconPath: Icons.folder }
                    Text {
                        visible: page.project.recentProjects.length === 0
                        text: qsTr("No projects yet.")
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontSmall + 1
                    }
                    Repeater {
                        model: page.project.recentProjects
                        delegate: ItemDelegate {
                            Layout.fillWidth: true
                            implicitHeight: 40
                            onClicked: if (page.project.openProject(modelData)) App.openWorkspace("map")
                            contentItem: ColumnLayout {
                                spacing: 0
                                Text { text: modelData.replace(/^.*[\\/]/, "").replace(/\.mbsproj$/, ""); color: Theme.text; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text { text: modelData; color: Theme.textFaint; font.pixelSize: Theme.fontSmall; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                implicitHeight: recentFiles.implicitHeight + 36
                ColumnLayout {
                    id: recentFiles
                    anchors.fill: parent
                    spacing: 4
                    RowLayout {
                        SectionTitle { text: qsTr("Recent files"); iconPath: Icons.file }
                        Button { text: qsTr("Clear"); flat: true; visible: App.recentFiles.length > 0; onClicked: App.clearRecentFiles() }
                    }
                    Text {
                        visible: App.recentFiles.length === 0
                        text: qsTr("Grids and swath files you open appear here.")
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontSmall + 1
                    }
                    Repeater {
                        model: App.recentFiles.slice(0, 8)
                        delegate: ItemDelegate {
                            Layout.fillWidth: true
                            implicitHeight: 34
                            onClicked: App.openFile(modelData)
                            contentItem: RowLayout {
                                spacing: 8
                                Icon { path: App.isGridFile(modelData) ? Icons.terrain : Icons.waves; size: 16; color: Theme.accent }
                                Text { text: modelData.replace(/^.*[\\/]/, ""); color: Theme.text; font.pixelSize: Theme.fontSmall + 1; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }
        }
    }
}
