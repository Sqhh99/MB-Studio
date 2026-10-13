// Title bar of a dock panel with optional tool buttons (default content).
import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    property string title
    property string iconPath
    property string subtitle
    default property alias tools: toolRow.data
    implicitHeight: 38
    color: Theme.panelHeader

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.panelPadding
        anchors.rightMargin: 4
        spacing: 8
        Icon { path: root.iconPath; color: Theme.accent; size: 16; visible: root.iconPath !== "" }
        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Text {
            text: root.subtitle
            visible: text !== ""
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Item { Layout.fillWidth: true; visible: root.subtitle === "" }
        Row { id: toolRow; spacing: 0 }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
}
