import QtQuick
import QtQuick.Layouts
import ".."

RowLayout {
    property alias text: label.text
    property alias iconPath: icon.path
    spacing: 8
    Icon { id: icon; size: 18; color: Theme.accent; visible: path !== "" }
    Text {
        id: label
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
}
