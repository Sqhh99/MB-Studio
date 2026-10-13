// "Label  value" line of a property sheet.
import QtQuick
import QtQuick.Layouts
import ".."

RowLayout {
    id: root
    property string label
    property string value
    property bool mono: false
    spacing: 8
    Text {
        text: root.label
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall + 1
        Layout.preferredWidth: 110
        Layout.alignment: Qt.AlignTop
        elide: Text.ElideRight
    }
    Text {
        text: root.value === "" ? "–" : root.value
        color: Theme.text
        font.pixelSize: Theme.fontSmall + 1
        font.family: root.mono ? Theme.monoFont : font.family
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }
}
