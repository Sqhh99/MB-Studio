// Placeholder shown when a page has no content yet.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

ColumnLayout {
    id: root
    property string iconPath
    property string title
    property string message
    property string actionText
    signal action()
    spacing: 14

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        width: 84; height: 84; radius: 42
        color: Theme.accentSoft
        Icon { anchors.centerIn: parent; path: root.iconPath; size: 40; color: Theme.accent }
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.title
        color: Theme.text
        font.pixelSize: Theme.fontTitle + 2
        font.weight: Font.DemiBold
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 460
        text: root.message
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
    PrimaryButton {
        Layout.alignment: Qt.AlignHCenter
        visible: root.actionText !== ""
        text: root.actionText
        onClicked: root.action()
    }
}
