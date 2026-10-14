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
    implicitHeight: 62
    implicitWidth: 180
    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.border

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        Rectangle {
            visible: root.iconPath !== ""
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: Theme.radius
            color: Qt.rgba(root.tint.r, root.tint.g, root.tint.b, 0.15)
            Icon { anchors.centerIn: parent; path: root.iconPath; color: root.tint; size: 16 }
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
                    font.pixelSize: 17
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
