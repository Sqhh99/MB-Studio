// The processing chain of a survey with the state of each step; a step
// opens its processing panel.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."

ColumnLayout {
    id: root
    readonly property var p: App.project.progress
    signal stepClicked(string panel)
    spacing: 2

    Repeater {
        model: [
            { panel: "import", title: qsTr("Add survey lines"), done: root.p.lines > 0, detail: root.p.lines > 0 ? qsTr("%1 lines, %2 read").arg(root.p.lines).arg(root.p.inspected) : qsTr("Files or folders, any MB-System format") },
            { panel: "preprocess", title: qsTr("Preprocess"), done: root.p.preprocessed > 0, detail: root.p.preprocessed > 0 ? qsTr("%1 lines").arg(root.p.preprocessed) : qsTr("Recommended for raw vendor formats") },
            { panel: "clean", title: qsTr("Clean"), done: root.p.cleaned === true, detail: qsTr("Flag outliers automatically") },
            { panel: "corrections", title: qsTr("Corrections"), done: root.p.corrected === true, detail: qsTr("Sound velocity, biases, draft, tide") },
            { panel: "process", title: qsTr("Process"), done: root.p.processed > 0, detail: root.p.processed > 0 ? qsTr("%1 lines").arg(root.p.processed) : qsTr("Apply edits and corrections") },
            { panel: "grid", title: qsTr("Grid"), done: root.p.grids > 0, detail: root.p.grids > 0 ? qsTr("%1 grids").arg(root.p.grids) : qsTr("Terrain model with quality grids") },
            { panel: "mosaic", title: qsTr("Mosaic"), done: root.p.mosaics > 0, detail: qsTr("Amplitude or sidescan") },
            { panel: "export", title: qsTr("Export"), done: root.p.exports > 0, detail: qsTr("GeoTIFF, LAS, XYZ, glTF") }
        ]
        delegate: ItemDelegate {
            Layout.fillWidth: true
            implicitHeight: 36
            onClicked: root.stepClicked(modelData.panel)
            contentItem: RowLayout {
                spacing: 10
                Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    radius: 11
                    color: modelData.done ? Theme.success : "transparent"
                    border.color: modelData.done ? Theme.success : Theme.border
                    border.width: 2
                    Text {
                        anchors.centerIn: parent
                        visible: !modelData.done
                        text: index + 1
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                    }
                    Icon { anchors.centerIn: parent; visible: modelData.done; path: Icons.check; size: 14; color: "white" }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text { text: modelData.title; color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                    Text { text: modelData.detail; color: Theme.textMuted; font.pixelSize: Theme.fontSmall; elide: Text.ElideRight; Layout.fillWidth: true }
                }
                Icon { path: Icons.chevronRight; size: 16; color: Theme.textFaint }
            }
        }
    }
}
