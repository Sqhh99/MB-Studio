// Bathymetry grid viewer: colour shaded relief with interactive lighting.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: page
    readonly property var grid: App.grid
    readonly property var info: grid.info
    property var probe: ({})

    function fmt(v, digits) { return v === undefined ? "–" : Number(v).toFixed(digits) }
    function fitView() {
        view.zoom = 1
        view.panX = 0
        view.panY = 0
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !grid.loaded && !grid.busy
        iconPath: Icons.terrain
        title: grid.error !== "" ? qsTr("The grid could not be read") : qsTr("View a bathymetry grid")
        message: grid.error !== "" ? grid.error
                 : qsTr("Open a GMT grid (.grd, .nc) such as the output of mbgrid or mbmosaic to display it as colour shaded relief.")
        actionText: qsTr("Open grid…")
        onAction: App.trigger("ui:openFile")
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: 16
        visible: grid.loaded

        // map view
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.dark ? "#081018" : "#DFE6EC"
            border.color: Theme.border
            clip: true

            Item {
                id: view
                anchors.fill: parent
                anchors.margins: 1
                property real zoom: 1
                property real panX: 0
                property real panY: 0

                Image {
                    id: image
                    source: grid.imageSource
                    cache: false
                    smooth: view.zoom < 3
                    fillMode: Image.PreserveAspectFit
                    width: view.width * view.zoom
                    height: view.height * view.zoom
                    x: (view.width - width) / 2 + view.panX
                    y: (view.height - height) / 2 + view.panY
                    asynchronous: false
                }

                DragHandler {
                    id: drag
                    target: null
                    property real startX: 0
                    property real startY: 0
                    onActiveChanged: if (active) { startX = view.panX; startY = view.panY }
                    onTranslationChanged: {
                        view.panX = startX + translation.x
                        view.panY = startY + translation.y
                    }
                }
                WheelHandler {
                    onWheel: (event) => {
                        const factor = event.angleDelta.y > 0 ? 1.2 : 1 / 1.2
                        const newZoom = Math.max(0.5, Math.min(40, view.zoom * factor))
                        // keep the point under the cursor fixed
                        const cx = event.x - view.width / 2 - view.panX
                        const cy = event.y - view.height / 2 - view.panY
                        view.panX -= cx * (newZoom / view.zoom - 1)
                        view.panY -= cy * (newZoom / view.zoom - 1)
                        view.zoom = newZoom
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
                        page.probe = grid.probe((mouse.x - ox) / pw, (mouse.y - oy) / ph)
                    }
                    onExited: page.probe = ({})
                    onDoubleClicked: page.fitView()
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: grid.busy
                visible: grid.busy
            }

            // probe readout
            Rectangle {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 12
                visible: page.probe.x !== undefined
                radius: 6
                color: Qt.rgba(0, 0, 0, 0.6)
                implicitWidth: probeText.implicitWidth + 20
                implicitHeight: probeText.implicitHeight + 12
                Text {
                    id: probeText
                    anchors.centerIn: parent
                    color: "white"
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.fontSmall + 1
                    text: page.probe.x === undefined ? ""
                          : (info.geographic ? "lon " : "x ") + fmt(page.probe.x, 6) + "   "
                            + (info.geographic ? "lat " : "y ") + fmt(page.probe.y, 6) + "   z "
                            + (page.probe.valid ? fmt(page.probe.z, 2) + " " + (info.zUnits || "") : qsTr("no data"))
                            + (page.probe.count !== undefined ? "   " + qsTr("n %1").arg(page.probe.count) : "")
                            + (page.probe.stddev !== undefined ? "   " + qsTr("σ %1").arg(fmt(page.probe.stddev, 2)) : "")
                            + (page.probe.interpolated === true ? "   " + qsTr("(interpolated)") : "")
                }
            }

            Row {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 4
                IconButton { iconPath: Icons.fit; tip: qsTr("Fit to view (double click)"); iconColor: "#E3EAF1"; onClicked: page.fitView() }
            }
        }

        // side panel
        ScrollView {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 14

                Card {
                    Layout.fillWidth: true
                    implicitHeight: headerColumn.implicitHeight + 36
                    ColumnLayout {
                        id: headerColumn
                        anchors.fill: parent
                        spacing: 6
                        Text {
                            text: grid.fileName
                            color: Theme.text
                            font.pixelSize: Theme.fontTitle
                            font.weight: Font.DemiBold
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Text {
                            text: info.title || ""
                            visible: text !== ""
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        RowLayout {
                            Chip { text: (info.nx || 0) + " × " + (info.ny || 0) }
                            Chip { text: info.geographic ? qsTr("geographic") : qsTr("projected"); tint: Theme.accentStrong }
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    implicitHeight: shading.implicitHeight + 36
                    ColumnLayout {
                        id: shading
                        anchors.fill: parent
                        spacing: 4
                        SectionTitle { text: qsTr("Display"); iconPath: Icons.layers }
                        Text { text: qsTr("Colour table"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; Layout.topMargin: 6 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: grid.colorMaps
                            currentIndex: grid.colorMap
                            onActivated: (index) => grid.colorMap = index
                        }
                        // legend: the colour table drawn as gradient segments
                        Row {
                            id: legendBar
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            height: 14
                            readonly property var stops: grid.legend
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
                            Text { text: fmt(grid.autoRange ? info.zmin : grid.rangeMin, 1); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                            Item { Layout.fillWidth: true }
                            Text { text: fmt(grid.autoRange ? info.zmax : grid.rangeMax, 1) + " " + (info.zUnits || ""); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        }
                        Switch {
                            text: qsTr("Colour range from the data")
                            checked: grid.autoRange
                            onToggled: grid.autoRange = checked
                        }
                        RowLayout {
                            visible: !grid.autoRange
                            Layout.fillWidth: true
                            spacing: 6
                            TextField {
                                Layout.fillWidth: true
                                text: fmt(grid.rangeMin, 1)
                                selectByMouse: true
                                validator: RegularExpressionValidator { regularExpression: /-?\d{0,7}([.,]\d{0,4})?/ }
                                onEditingFinished: grid.rangeMin = Number(text.replace(",", "."))
                            }
                            Text { text: "…"; color: Theme.textMuted }
                            TextField {
                                Layout.fillWidth: true
                                text: fmt(grid.rangeMax, 1)
                                selectByMouse: true
                                validator: RegularExpressionValidator { regularExpression: /-?\d{0,7}([.,]\d{0,4})?/ }
                                onEditingFinished: grid.rangeMax = Number(text.replace(",", "."))
                            }
                        }
                        Switch {
                            text: qsTr("Hillshade")
                            checked: grid.hillshade
                            onToggled: grid.hillshade = checked
                        }
                        Text { text: qsTr("Light azimuth  %1°").arg(Math.round(grid.azimuth)); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; enabled: grid.hillshade }
                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 359; stepSize: 1
                            value: grid.azimuth
                            enabled: grid.hillshade
                            onMoved: grid.azimuth = value
                        }
                        Text { text: qsTr("Light elevation  %1°").arg(Math.round(grid.elevation)); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; enabled: grid.hillshade }
                        Slider {
                            Layout.fillWidth: true
                            from: 5; to: 90; stepSize: 1
                            value: grid.elevation
                            enabled: grid.hillshade
                            onMoved: grid.elevation = value
                        }
                        Text { text: qsTr("Vertical exaggeration  ×%1").arg(grid.exaggeration.toFixed(1)); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; enabled: grid.hillshade }
                        Slider {
                            Layout.fillWidth: true
                            from: 0.5; to: 20; stepSize: 0.5
                            value: grid.exaggeration
                            enabled: grid.hillshade
                            onMoved: grid.exaggeration = value
                        }
                    }
                }

                // coverage and quality: measured, interpolated and empty cells
                Card {
                    Layout.fillWidth: true
                    implicitHeight: quality.implicitHeight + 36
                    ColumnLayout {
                        id: quality
                        anchors.fill: parent
                        spacing: 6
                        SectionTitle { text: qsTr("Coverage and quality"); iconPath: Icons.check }
                        PropertyRow {
                            Layout.fillWidth: true
                            label: qsTr("Empty cells")
                            value: grid.statistics.emptyPercent !== undefined ? fmt(grid.statistics.emptyPercent, 1) + " %" : ""
                        }
                        PropertyRow {
                            Layout.fillWidth: true
                            visible: grid.hasQualityGrids
                            label: qsTr("Interpolated")
                            value: fmt(grid.interpolatedPercent, 1) + " %"
                        }
                        PropertyRow {
                            Layout.fillWidth: true
                            label: qsTr("Mean ± σ")
                            value: grid.statistics.mean !== undefined ? fmt(grid.statistics.mean, 2) + " ± " + fmt(grid.statistics.stddev, 2) : ""
                        }
                        Text {
                            visible: !grid.hasQualityGrids
                            Layout.fillWidth: true
                            text: qsTr("Grid with quality grids (mbgrid -M) to see data counts, standard deviations and interpolated cells.")
                            color: Theme.textFaint
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.WordWrap
                        }
                        Text { visible: grid.hasQualityGrids; text: qsTr("Show"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        ComboBox {
                            objectName: "gridLayer"
                            visible: grid.hasQualityGrids
                            Layout.fillWidth: true
                            model: [qsTr("Surface"), qsTr("Data count per cell"), qsTr("Standard deviation")]
                            currentIndex: grid.layer
                            onActivated: (index) => grid.layer = index
                        }
                        Switch {
                            visible: grid.hasQualityGrids && grid.layer === 0
                            text: qsTr("Hatch interpolated cells")
                            checked: grid.showInterpolated
                            onToggled: grid.showInterpolated = checked
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    implicitHeight: extent.implicitHeight + 36
                    ColumnLayout {
                        id: extent
                        anchors.fill: parent
                        spacing: 6
                        SectionTitle { text: qsTr("Extent"); iconPath: Icons.info }
                        Repeater {
                            model: [
                                [info.xName || "x", fmt(info.xmin, 6) + " … " + fmt(info.xmax, 6)],
                                [info.yName || "y", fmt(info.ymin, 6) + " … " + fmt(info.ymax, 6)],
                                ["z", fmt(info.zmin, 2) + " … " + fmt(info.zmax, 2)],
                                [qsTr("spacing"), fmt(info.dx, 6) + " / " + fmt(info.dy, 6)]
                            ]
                            delegate: ColumnLayout {
                                spacing: 0
                                Layout.fillWidth: true
                                Text { text: modelData[0]; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                                Text { text: modelData[1]; color: Theme.text; font.pixelSize: Theme.fontBody; font.family: Theme.monoFont; elide: Text.ElideRight; Layout.fillWidth: true }
                            }
                        }
                        RowLayout {
                            Layout.topMargin: 6
                            Button {
                                text: qsTr("Add to project")
                                flat: true
                                visible: App.project.isOpen
                                onClicked: {
                                    App.project.addGrid(grid.path)
                                    App.openWorkspace("map")
                                }
                            }
                            Button { text: qsTr("Open another…"); flat: true; onClicked: App.trigger("ui:openFile") }
                            Item { Layout.fillWidth: true }
                            IconButton { iconPath: Icons.folderOpen; tip: qsTr("Show in folder"); onClicked: App.showInFolder(grid.path) }
                        }
                    }
                }
            }
        }
    }
}
