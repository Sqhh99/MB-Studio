// Properties of a line document: which file version is shown (original,
// preprocessed, processed) and the file.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ColumnLayout {
    id: root
    property var doc: null
    spacing: 6

    function versionName(v) {
        if (v === "original") return qsTr("Original (raw) file")
        if (v === "preprocessed") return qsTr("Preprocessed file")
        if (v === "processed") return qsTr("Processed file (mbprocess)")
        return v
    }

    Text {
        text: root.doc !== null ? root.doc.fileName : ""
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }
    SectionTitle { visible: root.doc !== null && root.doc.versions.length > 0; text: qsTr("Version"); iconPath: Icons.layers; Layout.topMargin: 6 }
    Repeater {
        model: root.doc !== null ? root.doc.versions : []
        delegate: RadioButton {
            objectName: "version_" + modelData.version
            text: root.versionName(modelData.version)
            enabled: modelData.exists
            checked: root.doc !== null && root.doc.version === modelData.version
            onToggled: if (checked) root.doc.version = modelData.version
        }
    }
    Text {
        visible: root.doc !== null && root.doc.versions.length > 0
        Layout.fillWidth: true
        text: qsTr("Statistics and depths are those of the version shown. Edits and corrections appear in the processed file after processing.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
    PropertyRow { Layout.fillWidth: true; label: qsTr("File"); value: root.doc !== null ? root.doc.path : ""; mono: true }
    Flow {
        Layout.fillWidth: true
        spacing: 4
        Button { text: qsTr("mbinfo"); flat: true; onClicked: App.runToolOn("mbinfo", root.doc.path) }
        Button {
            text: qsTr("Edit soundings")
            flat: true
            visible: root.doc !== null && root.doc.lineId !== ""
            onClicked: App.openEditor([root.doc.lineId])
        }
        Button { text: qsTr("Show in folder"); flat: true; onClicked: App.showInFolder(root.doc.path) }
    }
}
