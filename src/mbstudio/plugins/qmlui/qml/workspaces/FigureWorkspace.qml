// A figure document: the PNG of a GMT figure of the project (its recipe and
// actions are in the properties panel).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: page
    // the asset id of the figure (the argument of the tab)
    property string argument: ""
    readonly property var info: argument !== "" && App.project.mapRevision >= 0 ? App.project.productInfo(argument) : ({})
    readonly property string path: info.path || ""
    readonly property bool isRaster: path.toLowerCase().endsWith(".png")
    property real zoom: 1
    color: Theme.mapBackground

    Flickable {
        id: flick
        anchors.fill: parent
        visible: page.isRaster
        contentWidth: Math.max(width, image.width * page.zoom)
        contentHeight: Math.max(height, image.height * page.zoom)
        clip: true
        ScrollBar.vertical: ScrollBar {}
        ScrollBar.horizontal: ScrollBar {}
        Image {
            id: image
            objectName: "figureImage"
            source: page.isRaster ? App.toUrl(page.path) : ""
            cache: false
            fillMode: Image.PreserveAspectFit
            width: flick.width
            height: flick.height
            scale: page.zoom
            transformOrigin: Item.TopLeft
            smooth: true
            mipmap: true
        }
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: (event) => page.zoom = Math.max(0.25, Math.min(8, page.zoom * (event.angleDelta.y > 0 ? 1.2 : 1 / 1.2)))
        }
    }
    EmptyState {
        anchors.centerIn: parent
        visible: !page.isRaster
        iconPath: Icons.image
        title: page.path === "" ? qsTr("The figure is not in the project") : qsTr("No preview")
        message: page.path === "" ? qsTr("It may have been removed.")
                 : qsTr("The figure was written as %1; open it with another program.").arg(page.path.replace(/^.*[\\/]/, ""))
        actionText: page.path !== "" ? qsTr("Open the file") : ""
        onAction: App.openPath(page.path)
    }
    Row {
        visible: page.isRaster
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 2
        IconButton { iconPath: Icons.zoomIn; tip: qsTr("Zoom in (Ctrl + wheel)"); onClicked: page.zoom = Math.min(8, page.zoom * 1.25) }
        IconButton { iconPath: Icons.zoomOut; tip: qsTr("Zoom out"); onClicked: page.zoom = Math.max(0.25, page.zoom / 1.25) }
        IconButton { iconPath: Icons.fit; tip: qsTr("Fit"); onClicked: page.zoom = 1 }
    }
}
