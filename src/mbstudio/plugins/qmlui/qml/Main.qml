// MB-Studio main window.
//
// Presentation only: all state and behaviour come from the C++ view models
// exposed by the App singleton (MBStudio.Backend), which in turn use the
// services of the MB-Studio kernel plugins.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import MBStudio.Backend
import "."
import "components"
import "pages"

ApplicationWindow {
    id: window
    width: 1360
    height: 860
    minimumWidth: 1000
    minimumHeight: 640
    visible: true
    title: (App.currentPage === AppController.InspectPage && App.swath.fileName !== "" ? App.swath.fileName + " — "
           : App.currentPage === AppController.GridPage && App.grid.fileName !== "" ? App.grid.fileName + " — " : "")
           + "MB-Studio " + App.version
    color: Theme.background

    Material.theme: App.darkTheme ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.primary: Theme.accentStrong
    Material.background: Theme.background

    Binding { target: Theme; property: "dark"; value: App.darkTheme }

    readonly property var pages: [
        { icon: Icons.home, label: qsTr("Home") },
        { icon: Icons.waves, label: qsTr("Inspect") },
        { icon: Icons.terrain, label: qsTr("Grids") },
        { icon: Icons.code, label: qsTr("Programs") },
        { icon: Icons.list, label: qsTr("Formats") },
        { icon: Icons.extension, label: qsTr("Plugins") }
    ]

    function openData() { openDialog.open() }

    FileDialog {
        id: openDialog
        title: qsTr("Open swath data or grid")
        nameFilters: [
            qsTr("Swath data and grids (*.mb* *.all *.kmall *.s7k *.gsf *.xtf *.jsf *.grd *.nc *.fbt *.fnv)"),
            qsTr("GMT grids (*.grd *.nc)"),
            qsTr("All files (*)")
        ]
        onAccepted: App.openUrl(selectedFile)
    }

    // sequences: the platform may map StandardKey.Open to several key bindings
    Shortcut { sequences: [StandardKey.Open]; onActivated: window.openData() }
    Shortcut { sequence: "Ctrl+L"; onActivated: consolePanel.expanded = !consolePanel.expanded }
    Repeater {
        model: window.pages.length
        Item {
            Shortcut { sequence: "Ctrl+" + (index + 1); onActivated: App.currentPage = index }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // navigation rail
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 84
            color: Theme.sidebar
            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 14
                anchors.bottomMargin: 10
                spacing: 4
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 14
                    source: "qrc:/mbstudio/images/mbstudio.png"
                    sourceSize: Qt.size(88, 88)
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    smooth: true
                }
                Repeater {
                    model: window.pages
                    NavButton {
                        Layout.alignment: Qt.AlignHCenter
                        iconPath: modelData.icon
                        label: modelData.label
                        selected: App.currentPage === index
                        onClicked: App.currentPage = index
                    }
                }
                Item { Layout.fillHeight: true }
                IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    iconPath: App.darkTheme ? Icons.sun : Icons.moon
                    tip: App.darkTheme ? qsTr("Light theme") : qsTr("Dark theme")
                    onClicked: App.darkTheme = !App.darkTheme
                }
                IconButton {
                    id: languageButton
                    objectName: "languageButton"
                    Layout.alignment: Qt.AlignHCenter
                    iconPath: Icons.translate
                    tip: qsTr("Language")
                    onClicked: languageMenu.popup(languageButton, languageButton.width + 4, 0)
                    Menu {
                        id: languageMenu
                        MenuItem {
                            text: qsTr("System default")
                            checkable: true
                            checked: App.language === ""
                            onTriggered: App.language = ""
                        }
                        MenuSeparator {}
                        Instantiator {
                            model: App.languages
                            delegate: MenuItem {
                                required property var modelData
                                text: modelData.name
                                checkable: true
                                checked: App.language === modelData.code
                                onTriggered: App.language = modelData.code
                            }
                            // after "System default" and the separator
                            onObjectAdded: (index, object) => languageMenu.insertItem(index + 2, object)
                            onObjectRemoved: (index, object) => languageMenu.removeItem(object)
                        }
                    }
                }
                IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    iconPath: Icons.help
                    tip: qsTr("Program help")
                    onClicked: App.openDocumentation()
                }
            }
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // top bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.background
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.pagePadding
                    anchors.rightMargin: 16
                    spacing: 10
                    Text {
                        text: window.pages[App.currentPage].label
                        color: Theme.text
                        font.pixelSize: Theme.fontTitle + 1
                        font.weight: Font.DemiBold
                    }
                    Item { Layout.fillWidth: true }
                    PrimaryButton {
                        text: qsTr("Open…")
                        onClicked: window.openData()
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Open a swath data file or a grid (Ctrl+O)")
                        ToolTip.delay: 600
                    }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: App.currentPage
                HomePage { onOpenRequested: window.openData() }
                InspectPage { onOpenRequested: window.openData() }
                GridPage { onOpenRequested: window.openData() }
                ToolsPage {}
                FormatsPage {}
                PluginsPage {}
            }

            ConsolePanel {
                id: consolePanel
                property bool expanded: false
                Layout.fillWidth: true
                Layout.preferredHeight: expanded ? 210 : 0
                visible: Layout.preferredHeight > 0
                Behavior on Layout.preferredHeight { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            }

            // status bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.sidebar
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 8
                    spacing: 14
                    BusyIndicator {
                        implicitWidth: 18
                        implicitHeight: 18
                        running: App.swath.busy || App.grid.busy || App.tools.running
                        visible: running
                    }
                    Text {
                        Layout.fillWidth: true
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                        text: App.tools.running ? qsTr("Running %1…").arg(App.tools.current.name)
                              : App.swath.busy ? qsTr("Reading %1…").arg(App.swath.fileName)
                              : App.grid.busy ? qsTr("Rendering %1…").arg(App.grid.fileName)
                              : qsTr("Ready  ·  %1 programs  ·  %2 formats").arg(App.tools.availableCount).arg(App.formats.totalCount)
                    }
                    ToolButton {
                        implicitHeight: 28
                        font.pixelSize: Theme.fontSmall
                        text: qsTr("Log") + (App.log.warningCount > 0 ? "  ⚠ " + App.log.warningCount : "")
                        onClicked: consolePanel.expanded = !consolePanel.expanded
                    }
                }
            }
        }
    }

    // drag and drop of files from Explorer
    DropArea {
        id: drop
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (event) => {
            for (let i = 0; i < event.urls.length; i++)
                App.openUrl(event.urls[i])
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            visible: drop.containsDrag
            radius: Theme.radius + 4
            color: Qt.rgba(0.12, 0.71, 0.79, 0.12)
            border.color: Theme.accent
            border.width: 2
            Text {
                anchors.centerIn: parent
                text: qsTr("Drop swath files or grids to open them")
                color: Theme.accent
                font.pixelSize: Theme.fontTitle + 2
                font.weight: Font.DemiBold
            }
        }
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 46
    }
    Connections {
        target: App
        function onNotify(message) { toast.show(message) }
    }
}
