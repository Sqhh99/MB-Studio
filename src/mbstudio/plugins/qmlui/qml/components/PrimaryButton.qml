// Filled call-to-action button. Drawn by MB-Studio itself (no style shader
// effects) so that it looks the same with every renderer, including the
// software rasterizer used on virtual machines and remote desktops.
import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: control
    property color fill: Theme.accent
    property color textColor: "#FFFFFF"
    hoverEnabled: true
    implicitHeight: 30
    leftPadding: 14
    rightPadding: 14
    font.weight: Font.DemiBold
    font.pixelSize: Theme.fontBody

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? control.textColor : Theme.textFaint
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        implicitWidth: 88
        radius: Theme.radius
        color: !control.enabled ? Theme.surfaceAlt
             : control.down ? Qt.darker(control.fill, 1.15)
             : control.hovered ? Qt.lighter(control.fill, 1.08) : control.fill
        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
