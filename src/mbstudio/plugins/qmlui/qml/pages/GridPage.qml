// Grid document: colour shaded relief of one grid with its quality layers.
// The view takes the whole document area; display settings, statistics and
// the extent are in the properties panel (GridProperties.qml).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: page
    property var grid: null
    readonly property var info: grid !== null ? grid.info : ({})
    readonly property var probe: grid !== null ? grid.lastProbe : ({})
    color: Theme.mapBackground

    function fmt(v, digits) { return v === undefined ? "–" : Number(v).toFixed(digits) }
    function fitView() {
        grid.viewZoom = 1
        grid.viewPanX = 0
        grid.viewPanY = 0
    }

    EmptyState {
        anchors.centerIn: parent
        visible: page.grid === null || (!page.grid.loaded && !page.grid.busy)
        iconPath: Icons.terrain
        title: page.grid !== null && page.grid.error !== "" ? qsTr("The grid could not be read") : qsTr("View a bathymetry grid")
        message: page.grid !== null && page.grid.error !== "" ? page.grid.error
                 : qsTr("Open a GMT grid (.grd, .nc) such as the output of mbgrid or mbmosaic to display it as colour shaded relief.")
        actionText: qsTr("Open grid…")
        onAction: App.trigger("ui:openFile")
    }

    Item {
        id: view
        objectName: "gridView"
        anchors.fill: parent
        visible: page.grid !== null && page.grid.loaded
        clip: true

        Image {
            id: image
            source: page.grid !== null ? page.grid.imageSource : ""
            cache: false
            smooth: page.grid !== null && page.grid.viewZoom < 3
            fillMode: Image.PreserveAspectFit
            width: view.width * (page.grid !== null ? page.grid.viewZoom : 1)
            height: view.height * (page.grid !== null ? page.grid.viewZoom : 1)
            x: (view.width - width) / 2 + (page.grid !== null ? page.grid.viewPanX : 0)
            y: (view.height - height) / 2 + (page.grid !== null ? page.grid.viewPanY : 0)
            asynchronous: false
        }

        DragHandler {
            id: drag
            target: null
            property real startX: 0
            property real startY: 0
            onActiveChanged: if (active) { startX = page.grid.viewPanX; startY = page.grid.viewPanY }
            onTranslationChanged: {
                page.grid.viewPanX = startX + translation.x
                page.grid.viewPanY = startY + translation.y
            }
        }
        WheelHandler {
            onWheel: (event) => {
                const zoom = page.grid.viewZoom
                const factor = event.angleDelta.y > 0 ? 1.2 : 1 / 1.2
                const newZoom = Math.max(0.5, Math.min(40, zoom * factor))
                // keep the point under the cursor fixed
                const cx = event.x - view.width / 2 - page.grid.viewPanX
                const cy = event.y - view.height / 2 - page.grid.viewPanY
                page.grid.viewPanX -= cx * (newZoom / zoom - 1)
                page.grid.viewPanY -= cy * (newZoom / zoom - 1)
                page.grid.viewZoom = newZoom
            }
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            onPositionChanged: (mouse) => {
                // painted image rectangle inside the Image item (PreserveAspectFit)
                const pw = image.paintedWidth, ph = image.paintedHeight
                const ox = image.x + (image.width - pw) / 2
                const oy = image.y + (image.height - ph) / 2
                page.grid.probe((mouse.x - ox) / pw, (mouse.y - oy) / ph)
            }
            onExited: page.grid.clearProbe()
            onDoubleClicked: page.fitView()
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: page.grid !== null && page.grid.busy
        visible: running
    }

    // layer and tools
    Rectangle {
        visible: view.visible
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: tools.implicitWidth + 12
        implicitHeight: tools.implicitHeight + 6
        RowLayout {
            id: tools
            anchors.centerIn: parent
            spacing: 4
            ComboBox {
                objectName: "gridLayer"
                visible: page.grid !== null && page.grid.hasQualityGrids
                implicitHeight: 28
                implicitWidth: 170
                font.pixelSize: Theme.fontSmall
                model: [qsTr("Surface"), qsTr("Data count per cell"), qsTr("Standard deviation")]
                currentIndex: page.grid !== null ? page.grid.layer : 0
                onActivated: (index) => page.grid.layer = index
            }
            CheckBox {
                visible: page.grid !== null && page.grid.hasQualityGrids && page.grid.layer === 0
                text: qsTr("Hatch interpolated")
                font.pixelSize: Theme.fontSmall
                checked: page.grid !== null && page.grid.showInterpolated
                onToggled: page.grid.showInterpolated = checked
            }
            IconButton { iconPath: Icons.fit; tip: qsTr("Fit to view (double click)"); onClicked: page.fitView() }
            IconButton { iconPath: Icons.cube; tip: qsTr("Open in 3D (beside this view)"); onClicked: App.openTerrain(page.grid.path, 1) }
        }
    }

    // colour scale
    Rectangle {
        visible: view.visible
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: 190
        implicitHeight: scale.implicitHeight + 10
        ColumnLayout {
            id: scale
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2
            Row {
                id: legendBar
                Layout.fillWidth: true
                height: 9
                readonly property var stops: page.grid !== null ? page.grid.legend : []
                Repeater {
                    model: Math.max(0, legendBar.stops.length - 1)
                    Rectangle {
                        width: legendBar.width / (legendBar.stops.length - 1)
                        height: legendBar.height
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: legendBar.stops[index].color }
                            GradientStop { position: 1; color: legendBar.stops[index + 1].color }
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: page.fmt(page.grid !== null && !page.grid.autoRange ? page.grid.rangeMin : page.info.zmin, 1); color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
                Item { Layout.fillWidth: true }
                Text { text: page.fmt(page.grid !== null && !page.grid.autoRange ? page.grid.rangeMax : page.info.zmax, 1) + " " + (page.info.zUnits || ""); color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
            }
        }
    }

    // probe readout
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        visible: page.probe.x !== undefined
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: probeText.implicitWidth + 16
        implicitHeight: probeText.implicitHeight + 10
        Text {
            id: probeText
            anchors.centerIn: parent
            color: Theme.overlayText
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSmall
            text: page.probe.x === undefined ? ""
                  : (page.info.geographic ? "lon " : "x ") + page.fmt(page.probe.x, page.info.geographic ? 6 : 1) + "   "
                    + (page.info.geographic ? "lat " : "y ") + page.fmt(page.probe.y, page.info.geographic ? 6 : 1)
                    + (!page.info.geographic && page.probe.lon !== undefined ? "   (" + page.fmt(page.probe.lon, 5) + ", " + page.fmt(page.probe.lat, 5) + ")" : "")
                    + "   z " + (page.probe.valid ? page.fmt(page.probe.z, 2) + " " + (page.info.zUnits || "") : qsTr("no data"))
                    + (page.probe.count !== undefined ? "   " + qsTr("n %1").arg(page.probe.count) : "")
                    + (page.probe.stddev !== undefined ? "   " + qsTr("σ %1").arg(page.fmt(page.probe.stddev, 2)) : "")
                    + (page.probe.interpolated === true ? "   " + qsTr("(interpolated)") : "")
        }
    }
}
