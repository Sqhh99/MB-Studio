// Vector icon drawn from SVG path data (see Icons.qml), tinted with `color`.
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property string path
    property color color: "white"
    property real size: 20
    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.centerIn: parent
        width: root.size
        height: root.size
        layer.enabled: true
        layer.samples: 4
        ShapePath {
            fillColor: root.color
            strokeWidth: -1
            scale: Qt.size(root.size / 24, root.size / 24)
            PathSvg { path: root.path }
        }
    }
}
