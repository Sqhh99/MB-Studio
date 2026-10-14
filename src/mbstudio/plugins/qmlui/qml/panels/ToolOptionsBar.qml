// The tool options bar below the title bar: the settings of the active
// tool of the active document (sounding editor: selection tools, mode,
// brush, colouring; 3D terrain: navigation, measuring, profiles, views),
// and otherwise the steps of the processing chain.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    property string panel: ""
    property bool rightVisible: true
    signal panelRequested(string kind)
    readonly property string workspace: App.workspaces.currentWorkspace
    readonly property var doc: App.workspaces.currentDocument
    implicitHeight: Theme.toolBarHeight
    color: Theme.sidebar
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    component ToolToggle: IconButton {
        id: toggle
        property bool active: false
        iconColor: active ? Theme.accent : Theme.textMuted
        background: Rectangle {
            radius: Theme.radiusSmall
            color: toggle.active ? Theme.accentSoft : toggle.hovered ? Theme.surfaceHover : "transparent"
        }
    }
    component Separator: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 18
        Layout.leftMargin: 6
        Layout.rightMargin: 6
        color: Theme.border
    }
    component BarLabel: Text {
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
    }

    // processing chain (no document tools)
    RowLayout {
        objectName: "processingToolbar"
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        spacing: 2
        visible: root.workspace !== "editor" && root.workspace !== "terrain"
        IconButton { iconPath: Icons.add; tip: qsTr("Add survey data… (Ctrl+I)"); enabled: App.project.isOpen; onClicked: App.trigger("ui:addData") }
        Separator {}
        Repeater {
            model: [
                { kind: "preprocess", label: qsTr("Preprocess") },
                { kind: "clean", label: qsTr("Clean") },
                { kind: "corrections", label: qsTr("Corrections") },
                { kind: "process", label: qsTr("Process") },
                { kind: "grid", label: qsTr("Grid") },
                { kind: "mosaic", label: qsTr("Mosaic") },
                { kind: "figure", label: qsTr("Figure") },
                { kind: "export", label: qsTr("Export") }
            ]
            delegate: ToolButton {
                objectName: "toolbar_" + modelData.kind
                text: modelData.label
                enabled: App.project.isOpen
                implicitHeight: 26
                font.pixelSize: Theme.fontSmall + 1
                highlighted: root.panel === modelData.kind && root.rightVisible
                onClicked: root.panelRequested(modelData.kind)
            }
        }
        Separator {}
        IconButton { iconPath: Icons.edit; tip: qsTr("Edit the soundings of the selected lines (Ctrl+Shift+E)"); enabled: App.project.lineCount > 0; onClicked: App.trigger("ui:editor") }
        IconButton { iconPath: Icons.cube; tip: qsTr("3D terrain view (Ctrl+T)"); enabled: App.project.isOpen; onClicked: App.trigger("ui:terrain") }
        Item { Layout.fillWidth: true }
        // the next step of the project
        Text {
            visible: App.project.isOpen && (App.project.nextAction.kind || "") !== ""
            Layout.maximumWidth: 420
            text: App.project.nextAction.reason || ""
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
        }
        ToolButton {
            objectName: "nextActionButton"
            visible: App.project.isOpen && (App.project.nextAction.kind || "") !== "" && App.project.nextAction.kind !== "regenerate"
            text: qsTr("Next step")
            implicitHeight: 26
            font.pixelSize: Theme.fontSmall + 1
            onClicked: {
                const kind = App.project.nextAction.kind
                if (kind === "import") App.trigger("ui:addData")
                else root.panelRequested(kind)
            }
        }
    }

    // sounding editor
    RowLayout {
        objectName: "editorToolbar"
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        spacing: 2
        visible: root.workspace === "editor" && root.doc !== null
        readonly property var ed: root.workspace === "editor" ? root.doc : null
        BarLabel { text: qsTr("Tool") }
        ToolToggle { iconPath: Icons.rotate; tip: qsTr("Navigate: orbit, pan, zoom; a click shows the ping"); active: parent.ed !== null && parent.ed.tool === 0; onClicked: parent.ed.tool = 0 }
        ToolToggle { objectName: "toolRectangle"; iconPath: Icons.select; tip: qsTr("Select with a rectangle"); active: parent.ed !== null && parent.ed.tool === 1; onClicked: parent.ed.tool = 1 }
        ToolToggle { iconPath: Icons.lasso; tip: qsTr("Select with a lasso"); active: parent.ed !== null && parent.ed.tool === 2; onClicked: parent.ed.tool = 2 }
        ToolToggle { iconPath: Icons.brush; tip: qsTr("Select with a brush"); active: parent.ed !== null && parent.ed.tool === 3; onClicked: parent.ed.tool = 3 }
        ToolToggle { iconPath: Icons.point; tip: qsTr("Select single soundings"); active: parent.ed !== null && parent.ed.tool === 4; onClicked: parent.ed.tool = 4 }
        Separator {}
        BarLabel { text: qsTr("Selection") }
        ComboBox {
            implicitHeight: 26
            implicitWidth: 120
            font.pixelSize: Theme.fontSmall
            model: [qsTr("Replace"), qsTr("Add (Shift)"), qsTr("Remove (Ctrl)")]
            currentIndex: parent.ed !== null ? parent.ed.selectionMode : 0
            onActivated: (index) => parent.ed.selectionMode = index
        }
        BarLabel { text: qsTr("Brush"); visible: parent.ed !== null && parent.ed.tool === 3 }
        Slider {
            visible: parent.ed !== null && parent.ed.tool === 3
            implicitWidth: 90
            from: 4; to: 120; stepSize: 1
            value: parent.ed !== null ? parent.ed.brushRadius : 18
            onMoved: parent.ed.brushRadius = value
        }
        Separator {}
        IconButton { iconPath: Icons.flag; tip: qsTr("Flag the selected soundings (Del)"); enabled: parent.ed !== null && parent.ed.selectedCount > 0; onClicked: parent.ed.flagSelected() }
        IconButton { iconPath: Icons.check; tip: qsTr("Unflag the selected soundings (Ctrl+U)"); enabled: parent.ed !== null && parent.ed.selectedCount > 0; onClicked: parent.ed.unflagSelected() }
        IconButton { iconPath: Icons.undo; tip: qsTr("Undo (Ctrl+Z)"); enabled: parent.ed !== null && parent.ed.canUndo; onClicked: parent.ed.undo() }
        IconButton { iconPath: Icons.redo; tip: qsTr("Redo (Ctrl+Y)"); enabled: parent.ed !== null && parent.ed.canRedo; onClicked: parent.ed.redo() }
        Separator {}
        BarLabel { text: qsTr("Colour") }
        ComboBox {
            implicitHeight: 26
            implicitWidth: 120
            font.pixelSize: Theme.fontSmall
            model: [qsTr("Depth"), qsTr("Survey line"), qsTr("Flag state"), qsTr("Beam number")]
            currentIndex: parent.ed !== null ? parent.ed.colorMode : 0
            onActivated: (index) => parent.ed.colorMode = index
        }
        CheckBox {
            text: qsTr("Flagged")
            font.pixelSize: Theme.fontSmall
            checked: parent.ed !== null && parent.ed.showFlagged
            onToggled: parent.ed.showFlagged = checked
        }
        Item { Layout.fillWidth: true }
        Text {
            visible: parent.ed !== null && parent.ed.editCount > 0
            text: parent.ed !== null ? qsTr("%1 unsaved edits").arg(parent.ed.editCount) : ""
            color: Theme.warning
            font.pixelSize: Theme.fontSmall
        }
        ToolButton {
            objectName: "saveEdits"
            text: parent.ed !== null && parent.ed.saving ? qsTr("Saving…") : qsTr("Save edits")
            enabled: parent.ed !== null && parent.ed.editCount > 0 && !parent.ed.saving
            implicitHeight: 26
            font.pixelSize: Theme.fontSmall + 1
            highlighted: enabled
            onClicked: parent.ed.save()
        }
    }

    // 3D terrain
    RowLayout {
        objectName: "terrainToolbar"
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        spacing: 2
        visible: root.workspace === "terrain" && root.doc !== null
        readonly property var t: root.workspace === "terrain" ? root.doc : null
        BarLabel { text: qsTr("Tool") }
        ToolToggle { iconPath: Icons.rotate; tip: qsTr("Navigate: orbit (left), pan (right), zoom (wheel)"); active: parent.t !== null && parent.t.tool === 0; onClicked: parent.t.tool = 0 }
        ToolToggle { iconPath: Icons.ruler; tip: qsTr("Measure the distance between two points"); active: parent.t !== null && parent.t.tool === 1; onClicked: parent.t.tool = 1 }
        ToolToggle { iconPath: Icons.profile; tip: qsTr("Profile between two points"); active: parent.t !== null && parent.t.tool === 2; onClicked: parent.t.tool = 2 }
        Separator {}
        BarLabel { text: qsTr("View") }
        ToolButton { text: qsTr("Top"); implicitHeight: 26; font.pixelSize: Theme.fontSmall; onClicked: parent.t.standardView(0) }
        ToolButton { text: qsTr("Front"); implicitHeight: 26; font.pixelSize: Theme.fontSmall; onClicked: parent.t.standardView(1) }
        ToolButton { text: qsTr("Side"); implicitHeight: 26; font.pixelSize: Theme.fontSmall; onClicked: parent.t.standardView(2) }
        IconButton { iconPath: Icons.fit; tip: qsTr("Fit the surface (F)"); onClicked: parent.t.fit() }
        CheckBox {
            text: qsTr("Perspective")
            font.pixelSize: Theme.fontSmall
            checked: parent.t !== null && parent.t.perspective
            onToggled: parent.t.perspective = checked
        }
        Separator {}
        BarLabel { text: qsTr("Vertical ×%1").arg(parent.t !== null ? Number(parent.t.exaggeration).toFixed(1) : "1") }
        Slider {
            implicitWidth: 110
            from: 1; to: 50; stepSize: 0.5
            value: parent.t !== null ? parent.t.exaggeration : 1
            onMoved: parent.t.exaggeration = value
        }
        Item { Layout.fillWidth: true }
    }
}
