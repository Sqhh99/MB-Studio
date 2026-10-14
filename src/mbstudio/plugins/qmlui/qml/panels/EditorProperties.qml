// Properties of a sounding editor document: the survey lines being edited,
// the counts, the sounding under the cursor (with its identity in the
// file) and saving or discarding the edits.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ColumnLayout {
    id: root
    property var doc: null
    readonly property var probe: doc !== null ? doc.probe : ({})
    spacing: 6

    Text {
        text: qsTr("Sounding editor")
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
    }
    Text {
        Layout.fillWidth: true
        text: qsTr("Edits are kept in the edit save files of the lines (as with mbedit and mbeditviz) and applied when the lines are processed; the data files are not changed.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
    Text {
        visible: root.doc !== null && root.doc.warning !== ""
        Layout.fillWidth: true
        text: root.doc !== null ? root.doc.warning : ""
        color: Theme.warning
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }

    SectionTitle { text: qsTr("Survey lines"); iconPath: Icons.route; Layout.topMargin: 6 }
    Repeater {
        model: root.doc !== null ? root.doc.lines : []
        delegate: RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Rectangle { Layout.preferredWidth: 10; Layout.preferredHeight: 10; radius: 2; color: modelData.color }
            Text {
                Layout.fillWidth: true
                text: modelData.name
                color: Theme.text
                font.pixelSize: Theme.fontSmall + 1
                elide: Text.ElideMiddle
            }
            Text {
                text: qsTr("%1 pings").arg(modelData.pings)
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    SectionTitle { text: qsTr("Soundings"); iconPath: Icons.layers; Layout.topMargin: 6 }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Soundings"); value: root.doc !== null ? Number(root.doc.soundingCount).toLocaleString(Qt.locale(), 'f', 0) : "" }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Flagged"); value: root.doc !== null ? Number(root.doc.flaggedCount).toLocaleString(Qt.locale(), 'f', 0) : "" }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Selected"); value: root.doc !== null ? Number(root.doc.selectedCount).toLocaleString(Qt.locale(), 'f', 0) : "" }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Unsaved edits"); value: root.doc !== null ? String(root.doc.editCount) : "" }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Depth"); value: root.doc !== null ? Number(root.doc.depthMin).toFixed(1) + " … " + Number(root.doc.depthMax).toFixed(1) + " m" : "" }
    RowLayout {
        Layout.fillWidth: true
        Text { text: qsTr("Vertical ×%1").arg(root.doc !== null ? Number(root.doc.exaggeration).toFixed(1) : "1"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        Slider {
            Layout.fillWidth: true
            from: 1; to: 50; stepSize: 0.5
            value: root.doc !== null ? root.doc.exaggeration : 1
            onMoved: root.doc.exaggeration = value
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Text { text: qsTr("Point size"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        SpinBox {
            from: 1; to: 8
            value: root.doc !== null ? root.doc.pointSize : 2
            onValueModified: root.doc.pointSize = value
        }
    }

    // the sounding under the cursor and what an edit of it refers to
    SectionTitle { text: qsTr("Sounding under the cursor"); iconPath: Icons.point; Layout.topMargin: 6 }
    Text {
        visible: root.probe.index === undefined
        text: qsTr("Point at a sounding to see its line, ping and beam.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Survey line"); value: root.probe.line || "" }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Ping"); value: root.probe.ping !== undefined ? String(root.probe.ping) + (root.probe.multiplicity > 0 ? " (" + qsTr("multiplicity %1").arg(root.probe.multiplicity) + ")" : "") : "" }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Time"); value: root.probe.time || ""; mono: true }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Beam"); value: root.probe.beam !== undefined ? String(root.probe.beam) : "" }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Depth"); value: root.probe.depth !== undefined ? Number(root.probe.depth).toFixed(2) + " m" : "" }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("Position"); value: root.probe.lon !== undefined ? Number(root.probe.lon).toFixed(6) + "  " + Number(root.probe.lat).toFixed(6) : ""; mono: true }
    PropertyRow { visible: root.probe.index !== undefined; Layout.fillWidth: true; label: qsTr("State"); value: (root.probe.flag || "") + (root.probe.edited === true ? " · " + qsTr("edited") : "") }

    Flow {
        Layout.fillWidth: true
        Layout.topMargin: 8
        spacing: 4
        PrimaryButton {
            text: root.doc !== null && root.doc.saving ? qsTr("Saving…") : qsTr("Save edits")
            enabled: root.doc !== null && root.doc.editCount > 0 && !root.doc.saving
            onClicked: root.doc.save()
        }
        Button {
            text: qsTr("Discard")
            flat: true
            enabled: root.doc !== null && root.doc.editCount > 0
            onClicked: root.doc.revert()
        }
        Button {
            text: qsTr("Process these lines…")
            flat: true
            enabled: root.doc !== null && root.doc.editCount === 0
            onClicked: {
                App.project.selectIds(root.doc.lineIds)
                App.trigger("ui:panel:process")
            }
        }
        Button {
            text: qsTr("Reload")
            flat: true
            enabled: root.doc !== null && !root.doc.busy
            onClicked: root.doc.reload()
        }
    }
}
