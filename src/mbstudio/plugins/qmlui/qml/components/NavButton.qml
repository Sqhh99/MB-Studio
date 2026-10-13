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

    // The hover and selection highlights are separate layers whose opacity
    // is animated. Animating "color" from "transparent" (transparent black)
    // passes through dark intermediate colours, which made the background
    // flicker when the pointer entered or left the button.
    Item {
        id: pill
        anchors.horizontalCenter: parent.horizontalCenter
        y: 6
        width: 52
        height: 30
        Rectangle {
            objectName: "navHover"
            anchors.fill: parent
            radius: 15
            color: Theme.surfaceHover
            opacity: mouse.containsMouse && !root.selected ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
        }
        Rectangle {
            objectName: "navSelected"
            anchors.fill: parent
            radius: 15
            color: Theme.accentSoft
            opacity: root.selected ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
        }
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
