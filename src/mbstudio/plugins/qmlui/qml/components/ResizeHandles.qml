// Edges and corners that resize a window without a system frame, on
// platforms where the window system does not hit test for MB-Studio.
import QtQuick

Item {
    id: root
    property var chrome: null
    readonly property int b: chrome !== null ? chrome.resizeBorder : 6
    visible: chrome !== null && chrome.active && !chrome.nativeFrame && !chrome.maximized
    z: 1000

    component Edge: MouseArea {
        property int edges: 0
        acceptedButtons: Qt.LeftButton
        onPressed: root.chrome.startResize(edges)
    }

    Edge { x: 0; y: root.b * 2; width: root.b; height: root.height - root.b * 4; edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor }
    Edge { x: root.width - root.b; y: root.b * 2; width: root.b; height: root.height - root.b * 4; edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor }
    Edge { x: root.b * 2; y: 0; width: root.width - root.b * 4; height: root.b; edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor }
    Edge { x: root.b * 2; y: root.height - root.b; width: root.width - root.b * 4; height: root.b; edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor }
    Edge { x: 0; y: 0; width: root.b * 2; height: root.b * 2; edges: Qt.TopEdge | Qt.LeftEdge; cursorShape: Qt.SizeFDiagCursor }
    Edge { x: root.width - root.b * 2; y: 0; width: root.b * 2; height: root.b * 2; edges: Qt.TopEdge | Qt.RightEdge; cursorShape: Qt.SizeBDiagCursor }
    Edge { x: 0; y: root.height - root.b * 2; width: root.b * 2; height: root.b * 2; edges: Qt.BottomEdge | Qt.LeftEdge; cursorShape: Qt.SizeBDiagCursor }
    Edge { x: root.width - root.b * 2; y: root.height - root.b * 2; width: root.b * 2; height: root.b * 2; edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor }
}
