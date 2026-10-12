// Small coloured label (state, category, format id).
import QtQuick
import ".."

Rectangle {
    id: root
    property alias text: label.text
    property color tint: Theme.accent
    implicitWidth: label.implicitWidth + 16
    implicitHeight: 22
    radius: 11
    color: Qt.rgba(tint.r, tint.g, tint.b, Theme.dark ? 0.18 : 0.14)
    border.color: Qt.rgba(tint.r, tint.g, tint.b, 0.45)
    Text {
        id: label
        anchors.centerIn: parent
        color: Theme.dark ? Qt.lighter(root.tint, 1.25) : Qt.darker(root.tint, 1.3)
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
    }
}
