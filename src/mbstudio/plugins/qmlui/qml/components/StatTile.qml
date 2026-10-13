// A labelled value ("Records 1 024") for summary dashboards.
import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    property string label
    property string value
    property string unit
    property string iconPath
    property color tint: Theme.accent
    implicitHeight: 86
    implicitWidth: 180
    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.border

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12
        Rectangle {
            visible: root.iconPath !== ""
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: 20
            color: Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.15)
            Icon { anchors.centerIn: parent; path: root.iconPath; color: root.tint; size: 20 }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: root.label
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            RowLayout {
                spacing: 4
                Text {
                    text: root.value
                    color: Theme.text
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    Layout.maximumWidth: root.width - 90
                    elide: Text.ElideRight
                }
                Text {
                    text: root.unit
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                    Layout.alignment: Qt.AlignBaseline
                    visible: text !== ""
                }
            }
        }
    }
}
