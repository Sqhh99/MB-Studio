// One field of a processing parameter schema (see ProcessingViewModel):
// a number with its unit, an integer, a choice, a switch, a file or text.
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import MBStudio.Backend
import ".."

ColumnLayout {
    id: root
    property var field: ({})
    property var vm: App.processing
    readonly property string key: field.key || ""
    readonly property string type: field.type || ""
    // re-evaluated whenever a parameter changes
    readonly property var current: vm.revision >= 0 ? vm.value(key) : undefined
    readonly property var choices: vm.revision >= 0 ? (field.options !== undefined ? field.options : vm.options(key)) : []
    visible: vm.revision >= 0 && vm.fieldVisible(key)
    spacing: 3

    function choiceIndex() {
        for (let i = 0; i < choices.length; i++)
            if (String(choices[i].value) === String(current))
                return i
        return -1
    }

    // section heading
    Text {
        visible: root.type === "section"
        Layout.topMargin: 10
        text: root.field.label || ""
        color: Theme.accent
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
    }

    Text {
        visible: root.type !== "section" && root.type !== "bool"
        text: root.field.label || ""
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall + 1
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    // numbers: free text in the C locale with the unit next to it
    RowLayout {
        visible: root.type === "number" || root.type === "integer"
        Layout.fillWidth: true
        spacing: 6
        TextField {
            id: numberField
            objectName: "param_" + root.key
            Layout.fillWidth: true
            text: root.current === undefined ? "" : String(root.current)
            selectByMouse: true
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: RegularExpressionValidator { regularExpression: root.type === "integer" ? /-?\d{0,9}/ : /-?\d{0,9}([.,]\d{0,9})?/ }
            onEditingFinished: {
                let v = Number(text.replace(",", "."))
                if (isNaN(v)) { text = String(root.current); return }
                if (root.field.min !== undefined) v = Math.max(root.field.min, v)
                if (root.field.max !== undefined) v = Math.min(root.field.max, v)
                if (root.type === "integer") v = Math.round(v)
                root.vm.setValue(root.key, v)
                text = String(v)
            }
        }
        Text {
            visible: (root.field.unit || "") !== ""
            text: root.field.unit || ""
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall + 1
        }
    }

    ComboBox {
        visible: root.type === "enum"
        objectName: "param_" + root.key
        Layout.fillWidth: true
        model: root.choices
        textRole: "label"
        currentIndex: root.choiceIndex()
        onActivated: (index) => root.vm.setValue(root.key, root.choices[index].value)
    }

    Switch {
        visible: root.type === "bool"
        objectName: "param_" + root.key
        Layout.fillWidth: true
        text: root.field.label || ""
        checked: root.current === true
        onToggled: root.vm.setValue(root.key, checked)
    }

    TextField {
        visible: root.type === "text"
        objectName: "param_" + root.key
        Layout.fillWidth: true
        text: root.current === undefined ? "" : String(root.current)
        selectByMouse: true
        onEditingFinished: root.vm.setValue(root.key, text)
    }

    RowLayout {
        visible: root.type === "file"
        Layout.fillWidth: true
        spacing: 4
        TextField {
            Layout.fillWidth: true
            text: root.current === undefined ? "" : String(root.current)
            placeholderText: qsTr("none")
            selectByMouse: true
            onEditingFinished: root.vm.setValue(root.key, text)
        }
        Button {
            text: qsTr("Browse…")
            flat: true
            onClicked: fileDialog.open()
        }
        IconButton {
            iconPath: Icons.close
            tip: qsTr("Clear")
            visible: String(root.current || "") !== ""
            onClicked: root.vm.setValue(root.key, "")
        }
        FileDialog {
            id: fileDialog
            title: root.field.label || ""
            onAccepted: root.vm.setValue(root.key, App.toLocalFile(selectedFile))
        }
    }

    Text {
        visible: (root.field.help || "") !== "" && root.type !== "section"
        text: root.field.help || ""
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
