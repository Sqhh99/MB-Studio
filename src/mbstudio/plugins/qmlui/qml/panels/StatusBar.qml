// Status bar: what runs in the background, the active tool, the selection,
// the cursor position with its units and the data version of the active
// document; buttons for the jobs and the log.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."

Rectangle {
    id: root
    signal jobsRequested()
    signal logRequested()
    readonly property string workspace: App.workspaces.currentWorkspace
    readonly property var doc: App.workspaces.currentDocument
    implicitHeight: 24
    color: Theme.sidebar
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }

    function toolName() {
        if (workspace === "editor" && doc !== null)
            return [qsTr("Navigate"), qsTr("Rectangle selection"), qsTr("Lasso selection"), qsTr("Brush selection"), qsTr("Point selection")][doc.tool]
        if (workspace === "terrain" && doc !== null)
            return [qsTr("Navigate"), qsTr("Measure"), qsTr("Profile")][doc.tool]
        return ""
    }
    function selectionText() {
        if (workspace === "editor" && doc !== null)
            return qsTr("%1 of %2 soundings selected").arg(doc.selectedCount).arg(doc.soundingCount)
        return App.project.isOpen ? App.project.selectionText : ""
    }
    function versionText() {
        if (doc === null)
            return ""
        if (workspace === "inspect")
            return doc.version !== "" ? qsTr("Version: %1").arg(doc.version) : doc.fileName
        if (workspace === "editor")
            return qsTr("Working files with saved edits")
        if (workspace === "grid" || workspace === "terrain")
            return (doc.info.projectionName || doc.info.projection || "") !== "" ? (doc.info.projectionName || doc.info.projection) : ""
        return ""
    }
    readonly property string cursorText: doc !== null && doc.statusText !== undefined && doc.statusText !== "" ? doc.statusText : App.cursorText

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 4
        spacing: 10
        BusyIndicator {
            implicitWidth: 14
            implicitHeight: 14
            running: App.jobs.activeCount > 0 || App.tools.running || App.project.mapBusy
            visible: running
        }
        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 80
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
            text: App.jobs.activity !== "" ? App.jobs.activity
                  : App.tools.running ? qsTr("Running %1…").arg(App.tools.current.name)
                  : qsTr("Ready  ·  %1 programs  ·  %2 formats").arg(App.tools.availableCount).arg(App.formats.totalCount)
        }
        Text {
            objectName: "statusTool"
            visible: text !== ""
            text: root.toolName()
            color: Theme.accent
            font.pixelSize: Theme.fontSmall
        }
        Text {
            visible: text !== ""
            text: root.selectionText()
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
        Text {
            objectName: "statusCursor"
            visible: text !== ""
            text: root.cursorText
            color: Theme.text
            font.pixelSize: Theme.fontSmall
            font.family: Theme.monoFont
            elide: Text.ElideMiddle
            Layout.maximumWidth: 460
        }
        Text {
            visible: text !== ""
            text: root.versionText()
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
        }
        ToolButton {
            implicitHeight: 22
            font.pixelSize: Theme.fontSmall
            text: App.jobs.failedCount > 0 ? qsTr("Jobs  ⚠ %1").arg(App.jobs.failedCount) : qsTr("Jobs")
            onClicked: root.jobsRequested()
        }
        ToolButton {
            implicitHeight: 22
            font.pixelSize: Theme.fontSmall
            text: qsTr("Log") + (App.log.warningCount > 0 ? "  ⚠ " + App.log.warningCount : "")
            onClicked: root.logRequested()
        }
    }
}
