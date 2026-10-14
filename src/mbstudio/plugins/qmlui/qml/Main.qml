// MB-Studio workbench.
//
// Presentation only: all state and behaviour come from the C++ view models
// exposed by the App singleton (MBStudio.Backend), which in turn use the
// services of the MB-Studio kernel plugins. Menus are built from the actions
// and the document tabs from the workspaces that plugins contribute to the
// kernel registry; the shell provides the regions:
//   title bar     identity, menus, window buttons (drawn by MB-Studio)
//   tool options  the active tool of the active document, or the processing chain
//   left          project: survey lines and products
//   centre        documents in tabs, split into two groups side by side
//   right         properties of the active document and the selection, or
//                 the active processing panel
//   bottom        background jobs and the log
//   status bar    task state, active tool, selection, cursor, data version
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
    minimumWidth: 960
    minimumHeight: 600
    // shown by WindowChrome.restoreGeometry (on a screen that still exists)
    visible: false
    title: (App.project.isOpen ? App.project.name + " — " : "") + "MB-Studio " + App.version
    color: Theme.background
    // desktop labels: no upper case buttons of the Material style
    font.capitalization: Font.MixedCase

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
    property bool closeConfirmed: false

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

    WindowChrome {
        id: chrome
        window: window
        titleBar: titleBar
    }

    Component.onCompleted: {
        chrome.restoreGeometry("main", 1440, 900)
        if (App.workspaces.count === 0)
            App.openWorkspace("start")
    }
    onClosing: (close) => {
        if (App.unsavedEdits && !window.closeConfirmed) {
            close.accepted = false
            unsavedDialog.open()
            return
        }
        chrome.saveGeometry("main")
    }

    function openPanel(kind) {
        if (kind === "import") {
            App.trigger("ui:addData")
            return
        }
        if (kind === "regenerate") {
            window.panel = ""
            window.rightVisible = true
            return
        }
        App.processing.kind = kind
        panel = kind
        rightVisible = true
    }

    // Workspace presets: which docks are shown and how large, which panel
    // and which document; the open documents stay as they are.
    readonly property var presets: ({
        overview: { left: true, right: true, bottom: true, panel: "", leftWidth: 280, rightWidth: 330, bottomHeight: 180, workspace: "map" },
        editing: { left: true, right: true, bottom: false, panel: "", leftWidth: 240, rightWidth: 300, bottomHeight: 160, workspace: "editor" },
        modeling: { left: true, right: true, bottom: true, panel: "grid", leftWidth: 260, rightWidth: 340, bottomHeight: 170, workspace: "map" },
        cartography: { left: false, right: true, bottom: false, panel: "figure", leftWidth: 260, rightWidth: 360, bottomHeight: 160, workspace: "map" },
        delivery: { left: true, right: true, bottom: true, panel: "export", leftWidth: 260, rightWidth: 340, bottomHeight: 200, workspace: "history" }
    })
    function applyLayout(p) {
        leftVisible = p.left
        rightVisible = p.right
        bottomVisible = p.bottom
        projectPanel.SplitView.preferredWidth = p.leftWidth
        rightDock.SplitView.preferredWidth = p.rightWidth
        bottomDock.SplitView.preferredHeight = p.bottomHeight
        if (p.panel !== "" && App.project.isOpen)
            openPanel(p.panel)
        else
            panel = ""
    }
    function applyPreset(name) {
        if (name === "save") {
            App.setUiValue("customLayout", JSON.stringify({
                left: leftVisible, right: rightVisible, bottom: bottomVisible, panel: panel,
                leftWidth: projectPanel.width, rightWidth: rightDock.width, bottomHeight: bottomDock.height, workspace: ""
            }))
            toast.show(qsTr("The panel layout is saved"))
            return
        }
        if (name === "reset") {
            App.setUiValue("customLayout", "")
            applyLayout(presets.overview)
            return
        }
        const p = presets[name]
        if (p === undefined)
            return
        applyLayout(p)
        if (p.workspace === "editor") {
            if (App.workspaces.indexOf("editor") >= 0) App.workspaces.currentIndex = App.workspaces.indexOf("editor")
            else App.trigger("ui:editor")
        } else if (p.workspace !== "") {
            App.openWorkspace(p.workspace)
        }
    }

    // Edit menu: the active document (the sounding editor) does the work.
    function editAction(name) {
        const doc = App.workspaces.currentDocument
        if (App.workspaces.currentWorkspace !== "editor" || doc === null) {
            if (name === "save" || name === "flag" || name === "unflag")
                toast.show(qsTr("Open the sounding editor to edit soundings"))
            return
        }
        if (name === "undo") doc.undo()
        else if (name === "redo") doc.redo()
        else if (name === "flag") doc.flagSelected()
        else if (name === "unflag") doc.unflagSelected()
        else if (name === "selectAll") doc.selectAll()
        else if (name === "clearSelection") doc.clearSelection()
        else if (name === "save") doc.save()
    }

    // the grid of the active document, the selected product or the newest grid
    function currentGridPath() {
        const ws = App.workspaces.currentWorkspace
        if (ws === "grid" || ws === "terrain")
            return App.workspaces.currentArgument
        if (selectedProduct !== "" && App.project.productInfo(selectedProduct).mappable === true)
            return App.project.productPath(selectedProduct)
        const products = App.project.products
        for (let i = products.count - 1; i >= 0; i--) {
            const p = products.get(i)
            if (p.type === "grid" && p.exists)
                return p.path
        }
        return ""
    }

    function handleAction(name) {
        if (name === "newProject") newProjectDialog.open()
        else if (name === "openProject") projectDialog.open()
        else if (name === "addData") {
            if (App.project.isOpen) dataDialog.open()
            else newProjectDialog.open()
        }
        else if (name === "addFolder") {
            if (App.project.isOpen) dataFolderDialog.open()
            else newProjectDialog.open()
        }
        else if (name === "openFile") openDialog.open()
        else if (name === "closeProject") App.project.closeProject()
        else if (name === "quit") window.close()
        else if (name === "toggleTheme") App.darkTheme = !App.darkTheme
        else if (name === "palette") palette.open()
        else if (name === "help") App.openDocumentation()
        else if (name === "website") App.openWebSite()
        else if (name === "about") aboutDialog.open()
        else if (name === "editor") App.openEditor([])
        else if (name === "terrain") {
            const grid = currentGridPath()
            if (grid !== "") App.openTerrain(grid, App.workspaces.currentWorkspace === "grid" ? 1 : -1)
            else toast.show(qsTr("Make or open a grid first"))
        }
        else if (name === "gridView") {
            const grid = currentGridPath()
            if (grid !== "") App.openGrid(grid)
            else toast.show(qsTr("Make or open a grid first"))
        }
        else if (name === "splitRight") {
            const i = App.workspaces.currentIndex
            if (i >= 0) App.workspaces.moveToGroup(i, App.workspaces.group(i) === 0 ? 1 : 0)
        }
        else if (name === "unsplit") App.workspaces.unsplit()
        else if (name === "inspectSelected") {
            if (App.project.selectedCount > 0) App.inspectLine(App.project.selectedIds[0], "")
            else toast.show(qsTr("Select a survey line first"))
        }
        else if (name === "selectAllLines") App.project.selectAll()
        else if (name === "removeSelectedLines") App.project.removeSelectedLines()
        else if (name.startsWith("edit:")) editAction(name.substring(5))
        else if (name.startsWith("preset:")) applyPreset(name.substring(7))
        else if (name.startsWith("workspace:")) App.openWorkspace(name.substring(10))
        else if (name.startsWith("panel:")) openPanel(name.substring(6))
    }

    function menuFor(id) {
        if (id === "file") return fileMenu
        if (id === "edit") return editMenu
        if (id === "view") return viewMenu
        if (id === "data") return dataMenu
        if (id === "process") return processMenu
        if (id === "analysis") return analysisMenu
        if (id === "help") return helpMenu
        return windowMenu
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
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: window.editAction("redo") }

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
    Dialog {
        id: unsavedDialog
        objectName: "unsavedDialog"
        title: qsTr("Unsaved sounding edits")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Discard | Dialog.Cancel
        Label {
            text: qsTr("The sounding editor has edits that are not saved. Close MB-Studio and discard them?")
            wrapMode: Text.WordWrap
            width: 360
        }
        onDiscarded: {
            window.closeConfirmed = true
            unsavedDialog.close()
            window.close()
        }
    }

    // ----------------------------------------------------------------- layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            id: titleBar
            objectName: "titleBar"
            Layout.fillWidth: true
            chrome: chrome
            title: (App.project.isOpen ? App.project.name + "  ·  " : "") + "MB-Studio"
            subtitle: App.project.isOpen ? "" : App.version
            unsaved: App.unsavedEdits

            MenuBar {
                id: menuBar
                objectName: "menuBar"
                Layout.fillHeight: true
                Layout.preferredWidth: implicitWidth
                font.pixelSize: Theme.fontBody
                background: Item {}
                Menu { id: fileMenu; title: qsTr("&File") }
                Menu { id: editMenu; title: qsTr("&Edit") }
                Menu {
                    id: viewMenu
                    title: qsTr("&View")
                    MenuItem { text: qsTr("Project panel"); checkable: true; checked: window.leftVisible; onTriggered: window.leftVisible = !window.leftVisible }
                    MenuItem { text: qsTr("Properties panel"); checkable: true; checked: window.rightVisible; onTriggered: window.rightVisible = !window.rightVisible }
                    MenuItem { text: qsTr("Jobs and log"); checkable: true; checked: window.bottomVisible; onTriggered: window.bottomVisible = !window.bottomVisible }
                    MenuSeparator {}
                }
                Menu { id: dataMenu; title: qsTr("&Data") }
                Menu { id: processMenu; title: qsTr("&Process") }
                Menu { id: analysisMenu; title: qsTr("&Analysis") }
                Menu { id: windowMenu; title: qsTr("&Window") }
                Menu { id: helpMenu; title: qsTr("&Help") }
            }

            tools: [
                IconButton { iconPath: Icons.search; tip: qsTr("Command palette (Ctrl+Shift+P)"); onClicked: palette.open() },
                IconButton {
                    iconPath: App.darkTheme ? Icons.sun : Icons.moon
                    tip: App.darkTheme ? qsTr("Light theme") : qsTr("Dark theme")
                    onClicked: App.darkTheme = !App.darkTheme
                },
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
                },
                IconButton { iconPath: Icons.help; tip: qsTr("Program help"); onClicked: App.openDocumentation() }
            ]
        }

        ToolOptionsBar {
            id: toolOptions
            objectName: "toolOptions"
            Layout.fillWidth: true
            panel: window.panel
            rightVisible: window.rightVisible
            onPanelRequested: (kind) => window.openPanel(kind)
        }

        SplitView {
            id: mainSplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ProjectPanel {
                id: projectPanel
                visible: window.leftVisible
                SplitView.preferredWidth: window.uiNumber("leftWidth", 270)
                SplitView.minimumWidth: 190
                onWidthChanged: if (visible && width > 0) App.setUiValue("leftWidth", width)
                selectedProduct: window.selectedProduct
                onProductSelected: (id) => {
                    window.selectedProduct = id
                    window.panel = ""
                    window.rightVisible = true
                }
                onAddFolderRequested: dataFolderDialog.open()
                onPanelRequested: (kind) => window.openPanel(kind)
            }

            SplitView {
                id: centerSplit
                SplitView.fillWidth: true
                orientation: Qt.Vertical

                DocumentArea {
                    id: documents
                    objectName: "documentArea"
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 200
                }

                // jobs and log
                Rectangle {
                    id: bottomDock
                    visible: window.bottomVisible
                    SplitView.preferredHeight: window.uiNumber("bottomHeight", 180)
                    SplitView.minimumHeight: 80
                    onHeightChanged: if (visible && height > 0) App.setUiValue("bottomHeight", height)
                    color: Theme.surface
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
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
                                    font.pixelSize: Theme.fontSmall + 1
                                    TabButton { text: App.jobs.activeCount > 0 ? qsTr("Jobs (%1 running)").arg(App.jobs.activeCount) : qsTr("Jobs"); width: implicitWidth + 20 }
                                    TabButton { text: qsTr("Log") + (App.log.warningCount > 0 ? "  ⚠ " + App.log.warningCount : ""); width: implicitWidth + 20 }
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    visible: window.bottomTab === 0 && App.jobs.count > App.jobs.activeCount
                                    text: qsTr("Clear finished")
                                    flat: true
                                    implicitHeight: 26
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

            // properties of the active document and the selection, or the active processing panel
            Item {
                id: rightDock
                visible: window.rightVisible
                SplitView.preferredWidth: window.uiNumber("rightWidth", 330)
                SplitView.minimumWidth: 250
                onWidthChanged: if (visible && width > 0) App.setUiValue("rightWidth", width)
                ProcessingPanel {
                    objectName: "processingPanel"
                    anchors.fill: parent
                    visible: window.panel !== ""
                    onCloseRequested: window.panel = ""
                }
                PropertiesPanel {
                    objectName: "propertiesPanel"
                    anchors.fill: parent
                    visible: window.panel === ""
                    product: window.selectedProduct
                    onProductClosed: window.selectedProduct = ""
                    onPanelRequested: (kind) => window.openPanel(kind)
                }
            }
        }

        StatusBar {
            Layout.fillWidth: true
            onJobsRequested: { window.bottomVisible = true; window.bottomTab = 0 }
            onLogRequested: { window.bottomVisible = true; window.bottomTab = 1 }
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
            radius: Theme.radius
            color: Qt.rgba(0.30, 0.61, 0.91, 0.12)
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

    // window frame: a border, and resizing where the window system does not hit test
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.border
        border.width: chrome.active && !chrome.maximized ? 1 : 0
        z: 999
    }
    ResizeHandles {
        anchors.fill: parent
        chrome: chrome
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
    }
}
