// Flat icon button with tooltip.
import QtQuick
import QtQuick.Controls
import ".."

ToolButton {
    id: root
    property string iconPath
    property string tip
    property color iconColor: Theme.textMuted
    implicitWidth: 36
    implicitHeight: 36
    hoverEnabled: true
    contentItem: Icon {
        path: root.iconPath
        color: root.enabled ? (root.hovered ? Theme.text : root.iconColor) : Theme.textFaint
        size: 20
    }
    ToolTip.visible: hovered && tip !== ""
    ToolTip.text: tip
    ToolTip.delay: 500
}
