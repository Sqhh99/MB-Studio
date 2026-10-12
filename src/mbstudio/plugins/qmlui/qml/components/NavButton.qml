// Entry of the navigation rail.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    property string iconPath
    property string label
    property bool selected: false
    signal clicked()
    implicitWidth: 76
    implicitHeight: 62

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 6
        width: 52
        height: 30
        radius: 15
        color: root.selected ? Theme.accentSoft : (mouse.containsMouse ? Theme.surfaceHover : "transparent")
        Behavior on color { ColorAnimation { duration: 120 } }
        Icon {
            anchors.centerIn: parent
            path: root.iconPath
            size: 20
            color: root.selected ? Theme.accent : Theme.textMuted
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 39
        text: root.label
        color: root.selected ? Theme.text : Theme.textMuted
        font.pixelSize: Theme.fontSmall
        font.weight: root.selected ? Font.DemiBold : Font.Normal
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
