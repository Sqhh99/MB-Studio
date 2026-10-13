// MB-Studio front end for the MB-System command line programs.
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: page
    readonly property var tools: App.tools
    readonly property var current: tools.current

    FolderDialog {
        id: folderDialog
        title: qsTr("Working directory")
        currentFolder: App.toUrl(tools.workingDirectory)
        onAccepted: tools.workingDirectory = App.toLocalFile(selectedFolder)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: 16

        // program list
        Card {
            Layout.preferredWidth: 330
            Layout.fillHeight: true
            padding: 12
            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                TextField {
                    id: search
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search %1 programs").arg(tools.count)
                    text: tools.filter
                    onTextChanged: tools.filter = text
                    leftPadding: 34
                    Icon { x: 8; anchors.verticalCenter: parent.verticalCenter; path: Icons.search; color: Theme.textFaint; size: 18 }
                }
                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: tools
                    currentIndex: -1
                    section.property: "category"
                    section.delegate: Text {
                        width: ListView.view.width
                        topPadding: 12
                        bottomPadding: 4
                        leftPadding: 6
                        text: section
                        color: Theme.accent
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 0.8
                    }
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 54
                        highlighted: current.name === name
                        opacity: available ? 1 : 0.5
                        onClicked: tools.currentIndex = index
                        contentItem: ColumnLayout {
                            spacing: 2
                            Text {
                                text: name
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                font.family: Theme.monoFont
                            }
                            Text {
                                text: summary
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }

        // selected program
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            EmptyState {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: current.name === undefined
                iconPath: Icons.code
                title: qsTr("MB-System programs")
                message: qsTr("Choose a program on the left. MB-Studio runs the same command line programs that are installed in its bin directory, so every step can also be scripted from a console.")
            }

            Card {
                Layout.fillWidth: true
                visible: current.name !== undefined
                implicitHeight: form.implicitHeight + 36
                ColumnLayout {
                    id: form
                    anchors.fill: parent
                    spacing: 10
                    RowLayout {
                        spacing: 10
                        Text {
                            text: current.name || ""
                            color: Theme.text
                            font.pixelSize: Theme.fontHeadline
                            font.weight: Font.DemiBold
                            font.family: Theme.monoFont
                        }
                        Chip { text: current.category || "" }
                        Chip { visible: current.available === false; text: qsTr("not installed"); tint: Theme.danger }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Usage")
                            flat: true
                            enabled: current.available === true && !tools.running
                            onClicked: tools.showHelp()
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Run the program with -h and show its usage message")
                            ToolTip.delay: 600
                        }
                    }
                    Text {
                        text: current.summary || ""
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Text { text: qsTr("Arguments"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; Layout.topMargin: 4 }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: argsField
                            Layout.fillWidth: true
                            text: tools.arguments
                            onTextEdited: tools.arguments = text
                            placeholderText: current.example ? qsTr("e.g. %1").arg(current.example) : ""
                            font.family: Theme.monoFont
                            selectByMouse: true
                            Keys.onReturnPressed: if (!tools.running) tools.run()
                        }
                        Button { text: qsTr("Example"); flat: true; enabled: (current.example || "") !== ""; onClicked: tools.useExample() }
                    }
                    Text { text: qsTr("Working directory"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            Layout.fillWidth: true
                            text: tools.workingDirectory
                            onEditingFinished: tools.workingDirectory = text
                            selectByMouse: true
                        }
                        Button { text: qsTr("Browse…"); flat: true; onClicked: folderDialog.open() }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        implicitHeight: preview.implicitHeight + 16
                        radius: Theme.radiusSmall
                        color: Theme.surfaceAlt
                        Text {
                            id: preview
                            anchors.fill: parent
                            anchors.margins: 8
                            text: "$ " + tools.commandPreview
                            color: Theme.textMuted
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                    RowLayout {
                        spacing: 8
                        PrimaryButton {
                            text: tools.running ? qsTr("Running…") : qsTr("Run")
                            enabled: current.available === true && !tools.running
                            onClicked: tools.run()
                        }
                        Button { text: qsTr("Stop"); enabled: tools.running; onClicked: tools.stop() }
                        BusyIndicator { running: tools.running; visible: running; implicitWidth: 32; implicitHeight: 32 }
                        Item { Layout.fillWidth: true }
                        Chip {
                            visible: !tools.running && tools.output !== ""
                            text: tools.lastError !== "" ? qsTr("failed") : qsTr("exit %1").arg(tools.exitCode)
                            tint: tools.exitCode === 0 && tools.lastError === "" ? Theme.success : Theme.danger
                        }
                    }
                }
            }

            TabBar {
                id: tabs
                objectName: "toolTabs"
                Layout.fillWidth: true
                visible: current.name !== undefined
                background: null
                TabButton { text: qsTr("Help"); width: implicitWidth + 24 }
                TabButton { text: qsTr("Output"); width: implicitWidth + 24 }
            }
            Connections {
                target: tools
                function onRunningChanged() { if (tools.running) tabs.currentIndex = 1 }
                function onCurrentChanged() { if (!tools.running && tools.output === "") tabs.currentIndex = 0 }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: current.name !== undefined
                currentIndex: tabs.currentIndex

                // help page of the program in the interface language
                Rectangle {
                    radius: Theme.radius
                    color: Theme.surface
                    border.color: Theme.border
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 4
                            Text {
                                Layout.fillWidth: true
                                text: tools.helpText === "" ? ""
                                      : tools.helpLanguage !== App.effectiveLanguage
                                        ? qsTr("This page is not available in your language yet; the English page is shown.")
                                        : qsTr("Help for %1").arg(current.name || "")
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                                elide: Text.ElideRight
                            }
                            IconButton {
                                iconPath: Icons.openInNew
                                tip: qsTr("Open the help page in the web browser")
                                visible: tools.helpText !== ""
                                onClicked: tools.openHelpPage()
                            }
                        }
                        ScrollView {
                            id: helpScroll
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            contentWidth: availableWidth
                            TextArea {
                                id: helpView
                                objectName: "helpView"
                                text: tools.helpText !== "" ? tools.helpText
                                      : qsTr("No help page is available for this program. Use the Usage button to show the program's own usage message.")
                                textFormat: tools.helpText !== "" ? TextEdit.MarkdownText : TextEdit.PlainText
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                leftPadding: 16
                                rightPadding: 16
                                topPadding: 4
                                bottomPadding: 16
                                background: null
                                onTextChanged: helpScroll.ScrollBar.vertical.position = 0
                            }
                        }
                    }
                }

                // output console
                Rectangle {
                    radius: Theme.radius
                    color: Theme.dark ? "#0A1016" : "#1D2630"
                    border.color: Theme.border
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 4
                            Text { text: qsTr("Output"); color: "#8DA2B5"; font.pixelSize: Theme.fontSmall; Layout.fillWidth: true }
                            IconButton { iconPath: Icons.copy; tip: qsTr("Copy output"); iconColor: "#8DA2B5"; onClicked: App.copyToClipboard(tools.output) }
                            IconButton { iconPath: Icons.sweep; tip: qsTr("Clear"); iconColor: "#8DA2B5"; onClicked: tools.clearOutput() }
                        }
                        ScrollView {
                            id: outputScroll
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            TextArea {
                                id: output
                                text: tools.output
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.NoWrap
                                font.family: Theme.monoFont
                                font.pixelSize: Theme.fontBody - 1
                                color: "#D5E2EE"
                                background: null
                                onTextChanged: cursorPosition = length  // follow the output
                            }
                        }
                    }
                }
            }
        }
    }
}
