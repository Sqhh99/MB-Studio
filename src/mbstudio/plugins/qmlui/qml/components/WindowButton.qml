// Minimise, maximise/restore or close button of the title bar.
import QtQuick
import QtQuick.Controls
import ".."

AbstractButton {
    id: root
    property string iconPath
    property string accessibleName
    property bool danger: false
    implicitWidth: 44
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.role: Accessible.Button
    Accessible.name: accessibleName
    ToolTip.visible: hovered && accessibleName !== ""
    ToolTip.text: accessibleName
    ToolTip.delay: 800

    background: Rectangle {
        color: root.down ? (root.danger ? "#A8322E" : Theme.border)
             : root.hovered ? (root.danger ? Theme.danger : Theme.surfaceHover) : "transparent"
        border.color: root.visualFocus ? Theme.accent : "transparent"
        border.width: root.visualFocus ? 1 : 0
    }
    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            path: root.iconPath
            size: 15
            color: root.hovered && root.danger ? "#FFFFFF" : Theme.textMuted
        }
    }
}
