// Transient notification at the bottom of the window.
import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    property alias text: label.text
    function show(message) {
        label.text = message
        opacity = 1
        timer.restart()
    }
    width: Math.min(label.implicitWidth + 48, parent ? parent.width - 40 : 600)
    height: 34
    radius: Theme.radius
    color: Theme.dark ? "#33363B" : "#2B2F35"
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 180 } }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 10
        Icon { path: Icons.info; color: Theme.accent; size: 18 }
        Text {
            id: label
            color: "#F2F6FA"
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
    Timer { id: timer; interval: 4500; onTriggered: root.opacity = 0 }
}
