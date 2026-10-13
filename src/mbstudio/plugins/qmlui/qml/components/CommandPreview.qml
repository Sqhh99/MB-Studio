// The exact program commands a job runs, with a copy button.
import QtQuick
import QtQuick.Layouts
import MBStudio.Backend
import ".."

Rectangle {
    id: root
    property string text
    implicitHeight: label.implicitHeight + 16
    radius: Theme.radiusSmall
    color: Theme.console
    Text {
        id: label
        anchors.left: parent.left
        anchors.right: copy.left
        anchors.top: parent.top
        anchors.margins: 8
        text: root.text
        color: Theme.consoleText
        font.family: Theme.monoFont
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WrapAnywhere
    }
    IconButton {
        id: copy
        anchors.right: parent.right
        anchors.top: parent.top
        implicitWidth: 28
        implicitHeight: 28
        iconPath: Icons.copy
        iconColor: "#8DA2B5"
        tip: qsTr("Copy the commands")
        onClicked: App.copyToClipboard(root.text.replace(/^\$ /gm, ""))
    }
}
