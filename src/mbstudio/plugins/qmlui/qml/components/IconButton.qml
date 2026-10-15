// Flat icon button with tooltip.
import QtQuick
import QtQuick.Controls
import ".."

ToolButton {
    id: root
    property string iconPath
    property string tip
    property color iconColor: Theme.textMuted
    implicitWidth: 28
    implicitHeight: 28
    padding: 4
    hoverEnabled: true
    contentItem: Icon {
        path: root.iconPath
        color: root.enabled ? (root.hovered ? Theme.text : root.iconColor) : Theme.textFaint
        size: 17
    }
    Accessible.name: tip
    ToolTip.visible: hovered && tip !== ""
    ToolTip.text: tip
    ToolTip.delay: 500
}
