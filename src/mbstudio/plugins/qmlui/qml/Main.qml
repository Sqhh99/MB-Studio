// MB-Studio workbench.
//
// Presentation only: all state and behaviour come from the C++ view models
// exposed by the App singleton (MBStudio.Backend), which in turn use the
// services of the MB-Studio kernel plugins. Menus are built from the actions
// and the document tabs from the workspaces that plugins contribute to the
// kernel registry; the shell only provides the docks:
//   left    project: survey lines and products
//   centre  document tabs (map, grids, line inspector, history, programs, ...)
//   right   properties of the selection, or the active processing panel
//   bottom  background jobs and the log
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import MBStudio.Backend
import "."
import "components"
import "pages"
import "panels"

ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1000
    minimumHeight: 640
    visible: true
    title: (App.project.isOpen ? App.project.name + " — " : "") + "MB-Studio " + App.version
    color: Theme.background

    Material.theme: App.darkTheme ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.primary: Theme.accentStrong
    Material.background: Theme.background

    Binding { target: Theme; property: "dark"; value: App.darkTheme }

    // processing panel shown in the right dock ("" for the properties)
    property string panel: ""
    property string selectedProduct: ""
    property bool leftVisible: uiBool("leftVisible", true)
    property bool rightVisible: uiBool("rightVisible", true)
    property bool bottomVisible: uiBool("bottomVisible", true)
    property int bottomTab: 0

    onLeftVisibleChanged: App.setUiValue("leftVisible", leftVisible)
    onRightVisibleChanged: App.setUiValue("rightVisible", rightVisible)
    onBottomVisibleChanged: App.setUiValue("bottomVisible", bottomVisible)

    // QSettings returns the text "false" for a saved boolean
    function uiBool(key, fallback) {
        const v = App.uiValue(key, fallback)
        return v === true || v === "true"
    }
    function uiNumber(key, fallback) {
        const v = Number(App.uiValue(key, fallback))
        return isNaN(v) ? fallback : v
    }

    function openPanel(kind) {
        if (kind === "import") {
            App.trigger("ui:addData")
            return
        }
        App.processing.kind = kind
        panel = kind
        rightVisible = true
    }

    function handleAction(name) {
        if (name === "newProject") newProjectDialog.open()
        else if (name === "openProject") projectDialog.open()
        else if (name === "addData") {
            if (App.project.isOpen) dataDialog.open()
            else newProjectDialog.open()
        }
        else if (name === "openFile") openDialog.open()
        else if (name === "closeProject") App.project.closeProject()
        else if (name === "quit") Qt.quit()
        else if (name === "toggleTheme") App.darkTheme = !App.darkTheme
        else if (name === "palette") palette.open()
        else if (name === "help") App.openDocumentation()
        else if (name === "website") App.openWebSite()
        else if (name === "about") aboutDialog.open()
        else if (name.startsWith("workspace:")) App.openWorkspace(name.substring(10))
        else if (name.startsWith("panel:")) openPanel(name.substring(6))
    }

    function menuFor(id) {
        if (id === "file") return fileMenu
        if (id === "view") return viewMenu
        if (id === "process") return processMenu
        if (id === "help") return helpMenu
        return toolsMenu
    }

    Connections {
        target: App
        function onUiAction(name) { window.handleAction(name) }
        function onNotify(message) { toast.show(message) }
    }
    Connections {
        target: App.processing
        function onStarted(jobId, title) {
            window.bottomVisible = true
            window.bottomTab = 0
        }
    }

    Component.onCompleted: {
        if (App.workspaces.count === 0)
            App.openWorkspace("start")
    }

    // ------------------------------------------------------------------ menus
    menuBar: MenuBar {
        Menu { id: fileMenu; title: qsTr("&File") }
        Menu {
            id: viewMenu
            title: qsTr("&View")
            MenuItem { text: qsTr("Project panel"); checkable: true; checked: window.leftVisible; onTriggered: window.leftVisible = !window.leftVisible }
            MenuItem { text: qsTr("Properties panel"); checkable: true; checked: window.rightVisible; onTriggered: window.rightVisible = !window.rightVisible }
            MenuItem { text: qsTr("Jobs and log"); checkable: true; checked: window.bottomVisible; onTriggered: window.bottomVisible = !window.bottomVisible }
            MenuSeparator {}
        }
        Menu { id: processMenu; title: qsTr("&Process") }
        Menu { id: toolsMenu; title: qsTr("&Tools") }
        Menu { id: helpMenu; title: qsTr("&Help") }
    }
    // actions contributed by plugins (menu, shortcut, command)
    Instantiator {
        model: App.actions
        delegate: Action {
            property var entry: modelData
            text: modelData.title
            shortcut: modelData.shortcut === "Ctrl+O" ? StandardKey.Open : modelData.shortcut
            onTriggered: App.trigger(entry.command)
        }
        onObjectAdded: (index, object) => window.menuFor(object.entry.menu).addAction(object)
        onObjectRemoved: (index, object) => window.menuFor(object.entry.menu).removeAction(object)
    }
    Shortcut { sequence: "Ctrl+L"; onActivated: { window.bottomVisible = true; window.bottomTab = 1 } }

    // ---------------------------------------------------------------- dialogs
    FileDialog {
        id: openDialog
        title: qsTr("Open a project, swath file or grid")
        nameFilters: [
            qsTr("Projects, swath data and grids (*.mbsproj *.mb* *.all *.kmall *.s7k *.gsf *.xtf *.jsf *.grd *.nc)"),
            qsTr("GMT grids (*.grd *.nc)"),
            qsTr("All files (*)")
        ]
        onAccepted: App.openUrl(selectedFile)
    }
    FileDialog {
        id: projectDialog
        title: qsTr("Open project")
        nameFilters: [qsTr("MB-Studio projects (*.mbsproj)")]
        onAccepted: if (App.project.openProject(App.toLocalFile(selectedFile))) App.openWorkspace("map")
    }
    FileDialog {
        id: dataDialog
        title: qsTr("Add survey files")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("Swath data (*.all *.kmall *.s7k *.gsf *.xtf *.jsf *.mb* *.fbt *.wcd *.xse *.hsx *.sdf *.raw)"),
            qsTr("All files (*)")
        ]
        onAccepted: App.project.addData(selectedFiles)
    }
    FolderDialog {
        id: dataFolderDialog
        title: qsTr("Add a folder of survey files")
        onAccepted: App.project.addData([selectedFolder])
    }
    NewProjectDialog { id: newProjectDialog }
    LogDialog { id: logDialog }
    AboutDialog { id: aboutDialog }
    CommandPalette { id: palette }

    // ----------------------------------------------------------------- layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // toolbar: the processing chain
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.sidebar
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 4
                Image {
                    source: "qrc:/mbstudio/images/mbstudio.png"
                    sourceSize: Qt.size(64, 64)
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    smooth: true
                }
                Text {
                    Layout.leftMargin: 6
                    Layout.rightMargin: 10
                    text: App.project.isOpen ? App.project.name : qsTr("No project")
                    color: App.project.isOpen ? Theme.text : Theme.textMuted
                    font.pixelSize: Theme.fontBody + 1
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.maximumWidth: 220
                }
                IconButton { iconPath: Icons.add; tip: qsTr("Add survey data… (Ctrl+I)"); enabled: App.project.isOpen; onClicked: App.trigger("ui:addData") }
                Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 22; color: Theme.border; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                Repeater {
                    model: [
                        { kind: "preprocess", label: qsTr("Preprocess") },
                        { kind: "clean", label: qsTr("Clean") },
                        { kind: "corrections", label: qsTr("Corrections") },
                        { kind: "process", label: qsTr("Process") },
                        { kind: "grid", label: qsTr("Grid") },
                        { kind: "mosaic", label: qsTr("Mosaic") },
                        { kind: "export", label: qsTr("Export") }
                    ]
                    delegate: ToolButton {
                        objectName: "toolbar_" + modelData.kind
                        text: modelData.label
                        enabled: App.project.isOpen
                        font.pixelSize: Theme.fontSmall + 1
                        highlighted: window.panel === modelData.kind && window.rightVisible
                        onClicked: window.openPanel(modelData.kind)
                    }
                }
                Item { Layout.fillWidth: true }
                IconButton { iconPath: Icons.search; tip: qsTr("Command palette (Ctrl+Shift+P)"); onClicked: palette.open() }
                IconButton {
                    iconPath: App.darkTheme ? Icons.sun : Icons.moon
                    tip: App.darkTheme ? qsTr("Light theme") : qsTr("Dark theme")
                    onClicked: App.darkTheme = !App.darkTheme
                }
                IconButton {
                    id: languageButton
                    objectName: "languageButton"
                    iconPath: Icons.translate
                    tip: qsTr("Language")
                    onClicked: languageMenu.popup(languageButton, 0, languageButton.height)
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
                            onObjectAdded: (index, object) => languageMenu.insertItem(index + 2, object)
                            onObjectRemoved: (index, object) => languageMenu.removeItem(object)
                        }
                    }
                }
                IconButton { iconPath: Icons.help; tip: qsTr("Program help"); onClicked: App.openDocumentation() }
            }
        }

        SplitView {
            id: mainSplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ProjectPanel {
                id: projectPanel
                visible: window.leftVisible
                SplitView.preferredWidth: window.uiNumber("leftWidth", 290)
                SplitView.minimumWidth: 200
                onWidthChanged: if (visible && width > 0) App.setUiValue("leftWidth", width)
                selectedProduct: window.selectedProduct
                onProductSelected: (id) => {
                    window.selectedProduct = id
                    window.panel = ""
                    window.rightVisible = true
                }
                onAddFolderRequested: dataFolderDialog.open()
            }

            SplitView {
                id: centerSplit
                SplitView.fillWidth: true
                orientation: Qt.Vertical

                // documents
                Item {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 200
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            color: Theme.panelHeader
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
                            ListView {
                                id: tabs
                                objectName: "documentTabs"
                                anchors.fill: parent
                                orientation: ListView.Horizontal
                                clip: true
                                model: App.workspaces
                                boundsBehavior: Flickable.StopAtBounds
                                delegate: Rectangle {
                                    id: tab
                                    readonly property bool current: index === App.workspaces.currentIndex
                                    width: tabRow.implicitWidth + 24
                                    height: ListView.view.height
                                    color: current ? Theme.tabActive : tabMouse.containsMouse ? Theme.surfaceHover : "transparent"
                                    Rectangle { anchors.top: parent.top; width: parent.width; height: 2; color: Theme.accent; visible: parent.current }
                                    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }
                                    MouseArea {
                                        id: tabMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                                        onClicked: (mouse) => {
                                            if (mouse.button === Qt.MiddleButton) App.workspaces.close(index)
                                            else App.workspaces.currentIndex = index
                                        }
                                    }
                                    RowLayout {
                                        id: tabRow
                                        anchors.centerIn: parent
                                        spacing: 6
                                        Icon { path: Icons.byName(model.icon); size: 15; color: tab.current ? Theme.accent : Theme.textMuted }
                                        Text {
                                            text: model.title
                                            color: tab.current ? Theme.text : Theme.textMuted
                                            font.pixelSize: Theme.fontSmall + 1
                                            elide: Text.ElideMiddle
                                            Layout.maximumWidth: 220
                                        }
                                        IconButton {
                                            visible: model.closable
                                            implicitWidth: 22
                                            implicitHeight: 22
                                            iconPath: Icons.close
                                            tip: qsTr("Close")
                                            onClicked: App.workspaces.close(index)
                                        }
                                    }
                                }
                            }
                        }
                        Item {
                            id: documents
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Repeater {
                                model: App.workspaces
                                delegate: Loader {
                                    anchors.fill: parent
                                    source: model.url
                                    visible: index === App.workspaces.currentIndex
                                }
                            }
                        }
                    }
                }

                // jobs and log
                Rectangle {
                    id: bottomDock
                    visible: window.bottomVisible
                    SplitView.preferredHeight: window.uiNumber("bottomHeight", 210)
                    SplitView.minimumHeight: 90
                    onHeightChanged: if (visible && height > 0) App.setUiValue("bottomHeight", height)
                    color: Theme.surface
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            color: Theme.panelHeader
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
                            RowLayout {
                                anchors.fill: parent
                                anchors.rightMargin: 4
                                spacing: 0
                                TabBar {
                                    id: bottomTabs
                                    objectName: "bottomTabs"
                                    currentIndex: window.bottomTab
                                    onCurrentIndexChanged: window.bottomTab = currentIndex
                                    background: null
                                    TabButton { text: App.jobs.activeCount > 0 ? qsTr("Jobs (%1 running)").arg(App.jobs.activeCount) : qsTr("Jobs"); width: implicitWidth + 24 }
                                    TabButton { text: qsTr("Log") + (App.log.warningCount > 0 ? "  ⚠ " + App.log.warningCount : ""); width: implicitWidth + 24 }
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    visible: window.bottomTab === 0 && App.jobs.count > App.jobs.activeCount
                                    text: qsTr("Clear finished")
                                    flat: true
                                    font.pixelSize: Theme.fontSmall
                                    onClicked: App.jobs.clearFinished()
                                }
                                IconButton { iconPath: Icons.copy; tip: qsTr("Copy log"); visible: window.bottomTab === 1; onClicked: App.copyToClipboard(App.log.text()) }
                                IconButton { iconPath: Icons.sweep; tip: qsTr("Clear log"); visible: window.bottomTab === 1; onClicked: App.log.clear() }
                                IconButton { iconPath: Icons.close; tip: qsTr("Hide"); onClicked: window.bottomVisible = false }
                            }
                        }
                        StackLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: window.bottomTab
                            JobsPanel { onShowLog: (job) => logDialog.showJob(job) }
                            ConsolePanel { showHeader: false }
                        }
                    }
                }
            }

            // properties or the active processing panel
            Item {
                id: rightDock
                visible: window.rightVisible
                SplitView.preferredWidth: window.uiNumber("rightWidth", 360)
                SplitView.minimumWidth: 260
                onWidthChanged: if (visible && width > 0) App.setUiValue("rightWidth", width)
                ProcessingPanel {
                    objectName: "processingPanel"
                    anchors.fill: parent
                    visible: window.panel !== ""
                    onCloseRequested: window.panel = ""
                }
                PropertiesPanel {
                    anchors.fill: parent
                    visible: window.panel === ""
                    product: window.selectedProduct
                    onProductClosed: window.selectedProduct = ""
                    onPanelRequested: (kind) => window.openPanel(kind)
                }
            }
        }

        // status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.sidebar
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 6
                spacing: 12
                BusyIndicator {
                    implicitWidth: 18
                    implicitHeight: 18
                    running: App.jobs.activeCount > 0 || App.swath.busy || App.grid.busy || App.tools.running || App.project.mapBusy
                    visible: running
                }
                Text {
                    Layout.fillWidth: true
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                    text: App.jobs.activity !== "" ? App.jobs.activity
                          : App.tools.running ? qsTr("Running %1…").arg(App.tools.current.name)
                          : App.swath.busy ? qsTr("Reading %1…").arg(App.swath.fileName)
                          : App.grid.busy ? qsTr("Rendering %1…").arg(App.grid.fileName)
                          : qsTr("Ready  ·  %1 programs  ·  %2 formats").arg(App.tools.availableCount).arg(App.formats.totalCount)
                }
                Text {
                    visible: App.project.isOpen
                    text: App.project.selectionText
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                ToolButton {
                    implicitHeight: 26
                    font.pixelSize: Theme.fontSmall
                    text: App.jobs.failedCount > 0 ? qsTr("Jobs  ⚠ %1").arg(App.jobs.failedCount) : qsTr("Jobs")
                    onClicked: { window.bottomVisible = true; window.bottomTab = 0 }
                }
                ToolButton {
                    implicitHeight: 26
                    font.pixelSize: Theme.fontSmall
                    text: qsTr("Log") + (App.log.warningCount > 0 ? "  ⚠ " + App.log.warningCount : "")
                    onClicked: { window.bottomVisible = true; window.bottomTab = 1 }
                }
            }
        }
    }

    // drag and drop of files and folders from Explorer
    DropArea {
        id: drop
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (event) => {
            const data = []
            for (let i = 0; i < event.urls.length; i++) {
                const path = App.toLocalFile(event.urls[i])
                if (App.project.isOpen && !App.isGridFile(path) && !App.isProjectFile(path))
                    data.push(event.urls[i])
                else
                    App.openUrl(event.urls[i])
            }
            if (data.length > 0)
                App.project.addData(data)
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
                text: App.project.isOpen ? qsTr("Drop swath files or folders to add them to the project") : qsTr("Drop a project, swath files or grids to open them")
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
}
